#include "DiskManager.h"

DiskManager::DiskManager(const std::string &vdisk_path) : path(vdisk_path)
{
    uint32_t bitmapTotalBytes = 8 * BLOCK_SIZE;
    bitmap.resize(bitmapTotalBytes);
}

DiskManager::~DiskManager()
{
    if (disk.is_open())
        disk.close();
}

// 检查文件是否存在
bool DiskManager::FileExists(const std::string &path)
{
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

// 初始化磁盘
bool DiskManager::InitializeDisk(const std::string &path)
{
    // 1. 以读写+二进制模式创建/打开文件
    // ios::trunc 确保每次运行都会清空旧文件，重新格式化
    std::fstream fs(path, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!fs)
    {
        std::cerr << "错误：无法创建镜像文件！" << std::endl;
        return false;
    }
    // 2. 预分配空间（16MB）
    // 跳到末尾写一个字节，操作系统会自动分配空洞文件
    fs.seekp(TOTAL_BLOCKS * BLOCK_SIZE - 1);
    char null_byte = 0;
    fs.write(&null_byte, 1);
    // 3. 准备超级块 (Block 0)
    memset(&sb, 0, sizeof(SuperBlock)); // 先全部清零填充 padding
    sb.total_blocks = TOTAL_BLOCKS;
    sb.bitmap_start = 1;
    sb.inode_start = 9;
    sb.data_start = 1033;
    // 初始空闲块 = 总块数 - 系统占用块 (0号到1032号)
    sb.free_blocks = TOTAL_BLOCKS - 1033;
    // 写入第 0 块
    fs.seekp(0);
    fs.write(reinterpret_cast<char *>(&sb), sizeof(SuperBlock));
    // 4. 准备并写入位图 (Block 1 - 8)
    // 我们需要标记前 8 个位为 1
    std::vector<uint8_t> bitmap(BLOCK_SIZE * 8, 0);
    for (uint32_t i = 0; i < 8; ++i)
    {
        // 计算在第几个字节，第几个位
        uint32_t byte_idx = i / 8;
        uint32_t bit_idx = i % 8;
        // 使用 0x80 >> bit_idx 是为了让位图在字节内从高位向低位排列，方便观察
        bitmap[byte_idx] |= (0x80 >> bit_idx);
    }
    // 寻址到 Block 1 的起始位置写入
    fs.seekp(BLOCK_SIZE * 1);
    fs.write(reinterpret_cast<char *>(bitmap.data()), bitmap.size());
    // 5. 准备并写入空的 Inode 区 (Block 9 - 1032)
    // 这里暂时只填充全 0
    std::vector<char> empty_block(BLOCK_SIZE, 0);
    fs.seekp(BLOCK_SIZE * 9);
    for (int i = 0; i < 1024; ++i)
        fs.write(empty_block.data(), BLOCK_SIZE);
    fs.close();
    this->Mount();
    return true;
}

// 挂载磁盘
bool DiskManager::Mount()
{
    // 1. 打开文件流
    disk.open(path, std::ios::in | std::ios::out | std::ios::binary);
    if (!disk.is_open())
    {
        std::cerr << "错误：无法打开虚拟磁盘文件! " << std::endl;
        return false;
    }
    // 2. 读取超级块到内存 (Block 0)
    char buffer[BLOCK_SIZE];
    if (!ReadBlock(0, buffer))
        return false;
    memcpy(&sb, buffer, sizeof(SuperBlock));
    // 3. 根据超级块信息，加载位图到内存 (Block 1 - 8)
    uint32_t bitmap_total_size = BITMAP_SIZE * BLOCK_SIZE;
    bitmap.resize(bitmap_total_size);
    // 定位到位图区起始点
    disk.seekg(sb.bitmap_start * BLOCK_SIZE, std::ios::beg);
    disk.read(reinterpret_cast<char *>(bitmap.data()), bitmap_total_size);
    if (!disk.good())
    {
        std::cerr << "错误：加载位图失败!" << std::endl;
        return false;
    }
    // std::cout << "磁盘已挂载:总块数: " << sb.total_blocks
    //           << ", 空闲块: " << sb.free_blocks << std::endl;
    return true;
}

// 卸载磁盘
void DiskManager::UnMount()
{
    if (disk.is_open())
    {
        // 1. 强制同步超级块到 Block 0
        if (!WriteBlock(0, reinterpret_cast<char *>(&sb)))
            std::cerr << "错误：同步超级块到磁盘失败!" << std::endl;
        // 2. 强制同步完整的位图区 (Block 1 到 8)
        // 即使 AllocateBlock 里有单块同步，卸载时全量覆盖可防止内存与磁盘长期的微小偏差
        disk.seekp(sb.bitmap_start * BLOCK_SIZE, std::ios::beg);
        disk.write(reinterpret_cast<const char *>(bitmap.data()), bitmap.size());
        // 3. 刷新缓冲区并关闭文件
        disk.flush();
        disk.close();
    }
}

// 读取指定块
bool DiskManager::ReadBlock(uint32_t block_id, char *buffer)
{
    disk.seekg(block_id * BLOCK_SIZE, std::ios::beg);
    disk.read(buffer, BLOCK_SIZE);
    return disk.good();
}

// 写入指定块
bool DiskManager::WriteBlock(uint32_t block_id, char *buffer)
{
    disk.seekp(block_id * BLOCK_SIZE, std::ios::beg);
    disk.write(buffer, BLOCK_SIZE);
    disk.flush();
    return disk.good();
}

// 申请一个物理空闲块，返回物理块号，失败返回 -1
int DiskManager::AllocateBlock()
{
    // 1. 扫描内存中的位图 (从数据区起始位置开始扫描)
    for (uint32_t i = sb.data_start; i < sb.total_blocks; ++i)
    {
        uint32_t byte_idx = i / 8;
        uint32_t bit_idx = i % 8;
        // 检查该位是否为 0
        if (!(bitmap[byte_idx] & (0x80 >> bit_idx)))
        {
            // 2. 找到空闲块，在内存位图中将其置为 1
            bitmap[byte_idx] |= (0x80 >> bit_idx);
            // 3. 计算该位所在的物理块号并写回磁盘
            // byte_idx / BLOCK_SIZE 得到该字节在位图区的第几个块 (0-7)
            uint32_t bitmap_block_id = sb.bitmap_start + (byte_idx / BLOCK_SIZE);
            // 获取该 512B 块在内存 vector 中的起始指针
            uint32_t offset_in_bitmap = (byte_idx / BLOCK_SIZE) * BLOCK_SIZE;
            char *block_ptr = reinterpret_cast<char *>(&bitmap[offset_in_bitmap]);
            // 同步位图块到磁盘
            if (!WriteBlock(bitmap_block_id, block_ptr))
            {
                std::cerr << "错误：同步位图块到磁盘失败!" << std::endl;
                return -1;
            }
            // 4. 更新内存中的超级块信息
            sb.free_blocks--;
            // 5. 同步超级块到磁盘 (Block 0)
            if (!WriteBlock(0, reinterpret_cast<char *>(&sb)))
            {
                std::cerr << "错误：同步超级块到磁盘失败!" << std::endl;
                return -1;
            }
            // 6. 返回成功分配的物理块号
            return i;
        }
    }
    // 如果遍历完所有块都没有找到 0
    std::cerr << "错误：没有可用的物理块!" << std::endl;
    return -1;
}

// 释放指定的物理块
bool DiskManager::FreeBlock(uint32_t block_id)
{
    // 1. 安全检查
    if (block_id < sb.data_start || block_id >= sb.total_blocks)
    {
        std::cerr << "错误：不能释放保留区块! " << block_id << std::endl;
        return false;
    }
    // 2. 定位位图中的位置
    uint32_t byte_idx = block_id / 8;
    uint32_t bit_idx = block_id % 8;
    // 3. 将位图对应位置改为 0
    bitmap[byte_idx] &= ~(0x80 >> bit_idx);
    // 4. 同步该位图块到磁盘
    uint32_t bitmap_block_id = sb.bitmap_start + (byte_idx / BLOCK_SIZE);
    uint32_t offset_in_bitmap = (byte_idx / BLOCK_SIZE) * BLOCK_SIZE;
    char *block_ptr = reinterpret_cast<char *>(&bitmap[offset_in_bitmap]);
    if (!WriteBlock(bitmap_block_id, block_ptr))
        return false;
    // 5. 更新超级块空闲统计
    sb.free_blocks++;
    // 6. 同步超级块
    if (!WriteBlock(0, reinterpret_cast<char *>(&sb)))
        return false;
    return true;
}

// 读取 Inode
bool DiskManager::ReadInode(uint32_t inode_id, Inode &node)
{
    // 1. 计算物理位置
    uint32_t block_id = sb.inode_start + (inode_id / INODES_PER_BLOCK);
    uint32_t offset = (inode_id % INODES_PER_BLOCK) * sizeof(Inode);
    // 2. 读取整个块
    char buffer[BLOCK_SIZE];
    if (!ReadBlock(block_id, buffer))
        return false;
    // 3. 从块中拷贝出对应的 Inode 部分
    memcpy(&node, buffer + offset, sizeof(Inode));
    return true;
}

// 写入 Inode
bool DiskManager::WriteInode(uint32_t inode_id, const Inode &node)
{
    // 1. 定位
    uint32_t target_block = sb.inode_start + (inode_id / 4);
    uint32_t offset_in_block = (inode_id % 4) * sizeof(Inode);
    // 2. 读出原有的块数据 (读-改-写的第一步)
    char buffer[BLOCK_SIZE];
    if (!ReadBlock(target_block, buffer))
        return false;
    // 3. 将新的 Inode 数据覆盖到缓冲区的正确位置 (修改)
    memcpy(buffer + offset_in_block, &node, sizeof(Inode));
    // 4. 写回整个块 (写)
    if (!WriteBlock(target_block, buffer))
        return false;
    return true;
}

// 申请一个 Inode，返回 Inode 编号，失败返回 -1
int DiskManager::AllocateInode()
{
    int foundId = -1;
    // 1. 扫描查找
    for (uint32_t i = 0; i < INODE_BITMAP_BYTES; ++i)
    {
        uint32_t currentByteIdx = INODE_BITMAP_START_BYTE + i;
        if (bitmap[currentByteIdx] != 0xFF)
            for (int bit = 0; bit < 8; ++bit)
                if (!(bitmap[currentByteIdx] & (0x80 >> bit)))
                {
                    foundId = i * 8 + bit;
                    break;
                }
        if (foundId != -1)
            break;
    }
    if (foundId == -1)
        return -1;
    // 2. 更新内存位图 (必须使用和查找时完全一样的偏移逻辑)
    uint32_t targetByteIdx = INODE_BITMAP_START_BYTE + (foundId / 8);
    bitmap[targetByteIdx] |= (0x80 >> (foundId % 8));
    // 3. 计算磁盘同步位置
    // byteOffset 是该字节在整个位图区（从 bitmap[0] 开始算）的偏移
    uint32_t byteOffset = targetByteIdx;
    uint32_t blockOffset = byteOffset / BLOCK_SIZE;
    uint32_t targetPhysBlock = sb.bitmap_start + blockOffset;
    // 4. 写回受影响的“对齐”块
    // 必须从这个块的起始地址开始写，即 memStartIdx 必须是 BLOCK_SIZE 的倍数
    uint32_t memStartIdx = blockOffset * BLOCK_SIZE;
    if (!WriteBlock(targetPhysBlock, reinterpret_cast<char *>(&bitmap[memStartIdx])))
    {
        // 回滚
        bitmap[targetByteIdx] &= ~(0x80 >> (foundId % 8));
        return -1;
    }
    return foundId;
}

// 初始化 Inode
bool DiskManager::InitInode(uint32_t inode_id, uint32_t mode, uint32_t block_id)
{
    Inode newNode;
    // 1. 清空内存，确保 padding 和未使用的指针为 0
    memset(&newNode, 0, sizeof(Inode));
    // 2. 设置基础元数据
    newNode.inode_id = inode_id;
    newNode.mode = mode;
    newNode.size = 0;
    newNode.block_count = 1; // 初始占用 1 个块
    newNode.direct_ptr[0] = block_id;
    newNode.reader_count = 0; // 初始没有读者
    newNode.is_writing = 0;   // 初始没有写者
    // 3. 将 Inode 写入磁盘
    if (!WriteInode(inode_id, newNode))
        return false;
    return true;
}

// 释放 Inode
bool DiskManager::FreeInode(uint32_t inodeId)
{
    // 1. 计算在内存 bitmap 向量中的位置
    uint32_t byteOffset = INODE_BITMAP_START_BYTE + (inodeId / 8);
    uint32_t bitOffset = inodeId % 8;
    // 2. 修改内存位图 (清零)
    // 检查是否已经是 0，防止重复释放导致 sb.free_inodes 计数错误
    if (!(bitmap[byteOffset] & (0x80 >> bitOffset)))
    {
        std::cerr << "错误：Inode " << inodeId << " 已经是空闲状态！" << std::endl;
        return true;
    }
    bitmap[byteOffset] &= ~(0x80 >> bitOffset);
    // 3. 同步位图到磁盘 (只写回受影响的那个块)
    // 确定该字节属于位图的第几个块
    uint32_t blockOffset = byteOffset / BLOCK_SIZE;
    uint32_t targetBlockId = sb.bitmap_start + blockOffset;
    // 获取指向该块起始位置的指针
    uint8_t *blockPtr = &bitmap[blockOffset * BLOCK_SIZE];
    if (!WriteBlock(targetBlockId, reinterpret_cast<char *>(blockPtr)))
        return false;
    // 4. 更新内存中的超级块计数并写回磁盘
    if (!WriteBlock(targetBlockId, reinterpret_cast<char *>(&bitmap[blockOffset * BLOCK_SIZE])))
        return -1;
    // 5. 清理磁盘上的 Inode 结构体区域 (防止残留数据)
    Inode emptyInode;
    memset(&emptyInode, 0, sizeof(Inode));
    emptyInode.inode_id = inodeId; // 保持 ID 一致
    if (!WriteInode(inodeId, emptyInode))
        return false;
    return true;
}

void DiskManager::DumpBitmapOccupiedPart()
{
    // 假设系统占用块是 1032 (8个位图块 + 1024个Inode块)
    const uint32_t SYSTEM_BLOCKS = 1032;
    // 对应的字节数 = 1032 / 8 = 129 字节
    uint32_t bytes_to_read = (SYSTEM_BLOCKS + 7) / 8;
    std::vector<uint8_t> buffer(bytes_to_read);
    // 从磁盘起始位置（Block 1）读取位图内容
    std::ifstream ifs(this->path, std::ios::binary);
    if (!ifs)
    {
        std::cerr << "错误：无法打开磁盘文件进行调试！" << std::endl;
        return;
    }
    ifs.seekg(512);
    ifs.read(reinterpret_cast<char *>(buffer.data()), bytes_to_read);
    std::cout << "--- 位图占用区（系统块）状态 ---" << std::endl;
    std::cout << "每行显示 8 个字节 (代表 64 个块的状态)" << std::endl;
    for (uint32_t i = 0; i < buffer.size(); ++i)
    {
        // 打印块号范围索引
        if (i % 8 == 0)
        {
            std::cout << "\nBlock " << std::setw(4) << i * 8 << " - "
                      << std::setw(4) << (i * 8) + 63 << ": ";
        }
        // 将字节转换为二进制字符串显示
        // std::bitset<8> 会把字节转成 01 字符串
        std::cout << std::bitset<8>(buffer[i]) << " ";
    }
    std::cout << "\n\n--- 输出结束 ---" << std::endl;
    ifs.close();
}