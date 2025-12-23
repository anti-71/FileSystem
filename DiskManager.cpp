#include "DiskManager.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <ctime>

DiskManager::DiskManager()
{
    vDisk = new VirtualDisk();
}

DiskManager::~DiskManager() {
    if (vDisk != nullptr) {
        saveToDisk(); // 最后一次自动强制保存
        delete vDisk; // 释放 4MB+ 的堆内存
        vDisk = nullptr;
    }
}

void DiskManager::formatDisk()
{
    std::cout << "[DiskManager] 正在格式化虚拟磁盘..." << std::endl;

    // 1. 初始化超级块 (SuperBlock)
    vDisk->sBlock.totalDiskSize = sizeof(VirtualDisk);
    vDisk->sBlock.blockSize = BLOCK_SIZE;
    vDisk->sBlock.inodeTotal = MAX_INODES;
    vDisk->sBlock.inodeFree = MAX_INODES - 1;
    vDisk->sBlock.dataBlockTotal = MAX_DATA_BLOCKS;
    vDisk->sBlock.dataBlockFree = MAX_DATA_BLOCKS;
    vDisk->sBlock.rootInodeId = 0;

    // 初始化位图
    memset(vDisk->sBlock.inodeBitmap, 0, sizeof(vDisk->sBlock.inodeBitmap));
    memset(vDisk->sBlock.dataBlockBitmap, 0, sizeof(vDisk->sBlock.dataBlockBitmap));

    // 2. 初始化根目录 Inode
    vDisk->sBlock.inodeBitmap[0] = true;
    Inode &root = vDisk->inodes[0];

    // 清空整个结构体，防止脏数据
    memset(&root, 0, sizeof(Inode));

    root.inodeId = 0;
    root.ownerId = 0;
    root.permissions = 0755;
    root.type = F_DIRECTORY;
    root.fileSize = 0;
    root.createdAt = std::time(nullptr);
    root.linkCount = 1;

    // 初始化所有索引为 -1
    for (int i = 0; i < DIRECT_INDEX_COUNT; ++i)
    {
        root.directIndices[i] = -1;
    }
    root.firstIndirectIndex = -1;  // 新增：初始化间接索引
    root.secondIndirectIndex = -1; // 新增：初始化二级间接索引

    // 3. 立即持久化到文件
    if (saveToDisk())
    {
        std::cout << "[DiskManager] 磁盘格式化完成并已持久化。" << std::endl;
    }
}

bool DiskManager::saveToDisk()
{
    std::ofstream outFile(DISK_PATH, std::ios::binary | std::ios::out);
    if (!outFile)
    {
        std::cerr << "[Error] 无法打开文件进行写入: " << DISK_PATH << std::endl;
        return false;
    }
    outFile.write(reinterpret_cast<char *>(vDisk), sizeof(VirtualDisk));
    outFile.close();
    return true;
}

bool DiskManager::loadFromDisk()
{
    std::ifstream inFile(DISK_PATH, std::ios::binary | std::ios::in);
    if (!inFile)
    {
        return false; // 文件不存在
    }
    inFile.read(reinterpret_cast<char *>(vDisk), sizeof(VirtualDisk));
    inFile.close();
    return true;
}

VirtualDisk *DiskManager::getDiskPtr() const
{
    return vDisk;
}

void DiskManager::showDiskUsage() const
{
    std::cout << "\n--- 磁盘状态报告 ---" << std::endl;
    std::cout << "空闲 Inodes: " << vDisk->sBlock.inodeFree << " / " << MAX_INODES << std::endl;
    std::cout << "空闲数据块: " << vDisk->sBlock.dataBlockFree << " / " << MAX_DATA_BLOCKS << std::endl;
    std::cout << "超级块大小: " << sizeof(SuperBlock) << " bytes" << std::endl;
    std::cout << "--------------------\n"
              << std::endl;
}

// DiskManager.cpp 实现部分

// --- Inode 分配 ---
int DiskManager::allocateInode()
{
    SuperBlock &sb = vDisk->sBlock;
    for (int i = 0; i < MAX_INODES; i++)
    {
        if (!sb.inodeBitmap[i])
        {
            sb.inodeBitmap[i] = true;
            sb.inodeFree--;

            Inode &inode = vDisk->inodes[i];
            // 关键修改：不要只用 memset 0
            memset(&inode, 0, sizeof(Inode));
            inode.inodeId = i;

            // 必须显式初始化索引为 -1
            for (int j = 0; j < DIRECT_INDEX_COUNT; j++)
            {
                inode.directIndices[j] = -1;
            }
            inode.firstIndirectIndex = -1;
            inode.secondIndirectIndex = -1;

            return i;
        }
    }
    return -1;
}

void DiskManager::freeInode(int inodeId)
{
    if (inodeId < 0 || inodeId >= MAX_INODES)
        return;
    vDisk->sBlock.inodeBitmap[inodeId] = false;
    vDisk->sBlock.inodeFree++;
}

Inode *DiskManager::getInode(int inodeId)
{
    if (inodeId < 0 || inodeId >= MAX_INODES)
        return nullptr;
    return &vDisk->inodes[inodeId];
}

// --- 数据块分配 ---
int DiskManager::allocateDataBlock()
{
    SuperBlock &sb = vDisk->sBlock;
    for (int i = 0; i < MAX_DATA_BLOCKS; i++)
    {
        if (!sb.dataBlockBitmap[i])
        {
            sb.dataBlockBitmap[i] = true;
            sb.dataBlockFree--;
            // 清空块内容（模拟格式化）
            memset(vDisk->blocks[i].data, 0, BLOCK_SIZE);
            return i;
        }
    }
    return -1;
}

void DiskManager::freeDataBlock(int blockId)
{
    if (blockId < 0 || blockId >= MAX_DATA_BLOCKS)
        return;
    vDisk->sBlock.dataBlockBitmap[blockId] = false;
    vDisk->sBlock.dataBlockFree++;
}

DataBlock *DiskManager::getDataBlock(int blockId)
{
    if (blockId < 0 || blockId >= MAX_DATA_BLOCKS)
        return nullptr;
    return &vDisk->blocks[blockId];
}

// --- 索引模拟 (核心：直接索引 + 一级间接索引) ---
bool DiskManager::addBlockToInode(int inodeId, int blockId)
{
    Inode *inode = getInode(inodeId);
    if (!inode)
        return false;

    // 1. 尝试放入直接索引 (0-9)
    for (int i = 0; i < DIRECT_INDEX_COUNT; i++)
    {
        if (inode->directIndices[i] == -1)
        {
            inode->directIndices[i] = blockId;
            return true;
        }
    }

    // 2. 如果直接索引满了，使用一级间接索引
    if (inode->firstIndirectIndex == -1)
    {
        // 需要分配一个额外的数据块来存储索引表
        int indexBlockId = allocateDataBlock();
        if (indexBlockId == -1)
            return false;
        inode->firstIndirectIndex = indexBlockId;

        // 初始化索引块，全部设为 -1
        int *indexTable = reinterpret_cast<int *>(getDataBlock(indexBlockId)->data);
        for (int i = 0; i < BLOCK_SIZE / sizeof(int); i++)
            indexTable[i] = -1;
    }

    // 在一级间接索引块中查找空位
    int *indexTable = reinterpret_cast<int *>(getDataBlock(inode->firstIndirectIndex)->data);
    for (int i = 0; i < BLOCK_SIZE / sizeof(int); i++)
    {
        if (indexTable[i] == -1)
        {
            indexTable[i] = blockId;
            return true;
        }
    }

    return false; // 文件太大，超出支撑范围
}

bool DiskManager::saveUsers(const UserList &uList)
{
    std::ofstream ofs("user_data.dat", std::ios::binary);
    if (!ofs)
        return false;
    // 写入用户数量
    int count = uList.users.size();
    ofs.write(reinterpret_cast<const char *>(&count), sizeof(int));
    // 循环写入每个用户结构
    for (const auto &user : uList.users)
    {
        ofs.write(reinterpret_cast<const char *>(&user), sizeof(User));
    }
    ofs.close();
    return true;
}

void DiskManager::logAction(const std::string &username, const std::string &command, const std::string &result)
{
    std::ofstream logFile("fs_log.txt", std::ios::app); // 追加模式
    time_t now = time(0);
    char *dt = ctime(&now);
    dt[strlen(dt) - 1] = '\0'; // 去掉换行符
    logFile << "[" << dt << "] User: " << username << " | Cmd: " << command << " | Result: " << result << std::endl;
}