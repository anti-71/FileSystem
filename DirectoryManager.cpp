#include "DirectoryManager.h"

DirectoryManager::DirectoryManager(DiskManager *dm) : disk(dm) {}

// 初始化根目录
bool DirectoryManager::InitializeRoot()
{
    // 1. 分配根目录的 Inode 编号
    uint32_t rootInodeId = disk->AllocateInode();
    if (rootInodeId != 0)
    {
        std::cerr << "错误：根目录 Inode 编号不是 0，请检查磁盘初始化状态!" << std::endl;
        return false;
    }
    // 2. 分配根目录的第一个物理数据块
    uint32_t rootBlockId = disk->AllocateBlock();
    if (rootBlockId == -1)
        return false;
    // 3. 初始化 Inode 元数据
    if (!disk->InitInode(rootInodeId, 2, rootBlockId))
        return false;
    // 4. 构造目录项数据块 (包含 . 和 ..)
    char buffer[BLOCK_SIZE];
    memset(buffer, 0, BLOCK_SIZE);
    DirEntry *entries = reinterpret_cast<DirEntry *>(buffer);
    // 设置 "."
    strncpy(entries[0].fileName, ".", 27);
    entries[0].inodeId = rootInodeId;
    // 设置 ".."
    strncpy(entries[1].fileName, "..", 27);
    entries[1].inodeId = rootInodeId; // 根目录的父目录是它自己
    // 5. 写入磁盘
    if (!disk->WriteBlock(rootBlockId, buffer))
        return false;

    // 6. 更新 Inode 的 size (2个条目 * 32字节 = 64字节)
    Inode rootNode;
    disk->ReadInode(rootInodeId, rootNode);
    rootNode.size = 2 * sizeof(DirEntry);
    disk->WriteInode(rootInodeId, rootNode);

    return true;
}

// 添加目录项
bool DirectoryManager::AddDirEntry(uint32_t currentInodeId, const std::string &fileName, uint32_t newInodeId)
{
    // 1. 读取当前目录的 Inode 信息
    Inode currentNode;
    if (!disk.ReadInode(currentInodeId, currentNode))
        return false;
    // 2. 计算当前目录项应该存放的位置
    uint32_t currentSize = currentNode.size;
    uint32_t ptrIndex = currentSize / BLOCK_SIZE;      // 使用哪一个 direct_ptr
    uint32_t offsetInBlock = currentSize % BLOCK_SIZE; // 块内的字节偏移
    // 3. 边界检查：防止超过 10 个直接索引块的上限
    if (ptrIndex >= 10)
    {
        std::cerr << "错误：目录已达到最大容量限制!" << std::endl;
        return false;
    }
    // 4. 检查是否需要分配新块
    // 如果 offset 为 0 且 size > 0，说明上一个块刚好填满，需要为当前 ptrIndex 分配新块
    if (offsetInBlock == 0 && currentSize > 0)
    {
        uint32_t newBlock = disk.AllocateBlock();
        if (newBlock == -1)
            return false;
        currentNode.direct_ptr[ptrIndex] = newBlock;
        parentNode.block_count++;
        // 此时需要将新块清零，防止读到旧数据
        char zeroBuf[BLOCK_SIZE] = {0};
        disk->WriteBlock(newBlock, zeroBuf);
    }
    // 5. 获取物理块号并读取内容
    uint32_t physBlockId = currentNode.direct_ptr[ptrIndex];
    char buffer[BLOCK_SIZE];
    disk->ReadBlock(physBlockId, buffer);
    // 6. 在正确的位置写入新的 DirEntry
    DirEntry newEntry;
    memset(&newEntry, 0, sizeof(DirEntry));
    strncpy(newEntry.fileName, fileName.c_str(), 27);
    newEntry.inodeId = newInodeId;
    // 将 newEntry 拷贝到 buffer 的偏移位置
    memcpy(buffer + offsetInBlock, &newEntry, sizeof(DirEntry));
    // 7. 写回磁盘块
    if (!disk->WriteBlock(physBlockId, buffer))
        return false;
    // 8. 更新父目录 Inode 的 size 并写回
    currentNode.size += sizeof(DirEntry);
    if (!disk->WriteInode(currentInodeId, currentNode))
        return false;

    return true;
}

// 查找 Inode 编号
uint32_t FileManager::FindInodeId(const std::string &name, uint32_t currentDirInodeId)
{
    Inode currentNode;
    if (!disk->ReadInode(currentDirInodeId, currentNode))
        return -1;
    // 1. 计算目录中总共有多少个条目
    uint32_t entryCount = currentNode.size / sizeof(DirEntry);
    char buffer[BLOCK_SIZE];
    uint32_t lastBlockIdx = 0xFFFFFFFF; // 缓存块索引，避免重复读取同一个块
    // 2. 遍历所有条目
    for (uint32_t i = 0; i < entryCount; ++i)
    {
        // 计算当前条目在哪个 direct_ptr 索引下，以及块内偏移
        uint32_t totalOffset = i * sizeof(DirEntry);
        uint32_t ptrIdx = totalOffset / BLOCK_SIZE;
        uint32_t offsetInBlock = totalOffset % BLOCK_SIZE;
        // 如果超出了直接索引范围（10个块），则停止
        if (ptrIdx >= 10)
            break;
        // 3. 读取数据块（带有简单的缓存逻辑：如果还在同一个块内，就不重读磁盘）
        if (ptrIdx != lastBlockIdx)
        {
            uint32_t physBlockId = currentNode.direct_ptr[ptrIdx];
            if (!disk->ReadBlock(physBlockId, buffer))
                continue;
            lastBlockIdx = ptrIdx;
        }
        // 5. 获取目录项并比对名字
        DirEntry *entry = reinterpret_cast<DirEntry *>(buffer + offsetInBlock);
        // 注意：由于是 char 数组，直接比对 string
        if (name == entry->fileName)
            return entry->inodeId;
    }

    return -1; // 未找到
}
