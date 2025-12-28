#include "FileManager.h"

FileManager::FileManager(DiskManager *dm, DirectoryManager *dirm) : currentInodeId(0)
{
    this->disk = dm;
    this->dir = dirm;
}

// 创建文件
bool FileManager::CreateFile(const std::string &name)
{
    // 1. 分配空闲的 inode
    uint32_t inodeNum = disk->AllocateInode();
    if (inodeNum == -1)
        return false;
    // 2. 分配空闲的 block
    uint32_t blockNum = disk->AllocateBlock();
    if (blockNum == -1)
        return false;
    // 3. 初始化 inode
    if (!disk->InitInode(inodeNum, 1, blockNum))
        return false;
    // 4. 写入文件名
    if (!dir->AddDirEntry(currentInodeId, name, inodeNum))
        return false;

    return true;
}

bool FileManager::DeleteFile(const std::string &name)
{
    // 1. 在当前目录查找文件
    Inode parentNode;
    if (!disk->ReadInode(currentInodeId, parentNode))
        return false;
    uint32_t entryCount = parentNode.size / DIR_ENTRY_SIZE;        // 目录项数量
    int targetEntryIdx = -1;                                       // 目标目录项的索引
    uint32_t fileInodeId = dir->FindInodeId(name, currentInodeId); // 文件对应的 inode 编号
    if (fileInodeId == (uint32_t)-1)
    {
        std::cerr << "错误：文件 " << name << " 不存在！" << std::endl;
        return false;
    }
    for (uint32_t i = 0; i < entryCount; ++i)
    {
        uint32_t ptrIdx = (i * DIR_ENTRY_SIZE) / BLOCK_SIZE;
        uint32_t offset = (i * DIR_ENTRY_SIZE) % BLOCK_SIZE;
        char buffer[BLOCK_SIZE];
        disk->ReadBlock(parentNode.direct_ptr[ptrIdx], buffer);
        DirEntry *de = reinterpret_cast<DirEntry *>(buffer + offset);
        if (name == de->name)
        {
            targetEntryIdx = i;
            break;
        }
    }
    // 2. 释放文件占用的磁盘资源
    Inode fileNode;
    if (disk->ReadInode(fileInodeId, fileNode))
    {
        // 释放该文件占用的所有物理块 (直接索引部分)
        for (int i = 0; i < (int)fileNode.block_count; ++i)
        {
            if (fileNode.direct_ptr[i] != 0)
                disk->FreeBlock(fileNode.direct_ptr[i]);
        }
        // 释放 Inode 编号
        disk->FreeInode(fileInodeId);
    }
    // 3. 清理目录项（覆盖法维持目录紧凑）
    uint32_t lastIdx = entryCount - 1; // 最后一个目录项的索引
    // 如果删除的不是最后一个，需要用最后一个来填补坑位
    if (targetEntryIdx != (int)lastIdx)
    {
        // 读取最后一个目录项
        char lastBlockBuf[BLOCK_SIZE];
        uint32_t lastPtrIdx = (lastIdx * DIR_ENTRY_SIZE) / BLOCK_SIZE;
        uint32_t lastOffset = (lastIdx * DIR_ENTRY_SIZE) % BLOCK_SIZE;
        disk->ReadBlock(parentNode.direct_ptr[lastPtrIdx], lastBlockBuf);
        DirEntry *lastEntry = reinterpret_cast<DirEntry *>(lastBlockBuf + lastOffset);
        // 复制最后一个条目到目标位置（被删条目位置）
        char targetBlockBuf[BLOCK_SIZE];
        uint32_t targetPtrIdx = (targetEntryIdx * DIR_ENTRY_SIZE) / BLOCK_SIZE;
        uint32_t targetOffset = (targetEntryIdx * DIR_ENTRY_SIZE) % BLOCK_SIZE;
        disk->ReadBlock(parentNode.direct_ptr[targetPtrIdx], targetBlockBuf);
        DirEntry *targetEntry = reinterpret_cast<DirEntry *>(targetBlockBuf + targetOffset);
        memcpy(targetEntry, lastEntry, sizeof(DirEntry));
        // 写回目标块
        disk->WriteBlock(parentNode.direct_ptr[targetPtrIdx], targetBlockBuf);
    }

    // 4. 更新父目录元数据
    parentNode.size -= DIR_ENTRY_SIZE;
    // 如果 size 刚好退回到块边界，可以考虑减少 block_count
    if (parentNode.size > 0 && parentNode.size % BLOCK_SIZE == 0 && parentNode.block_count > 1)
    {
        uint32_t emptyBlock = parentNode.direct_ptr[parentNode.block_count - 1];
        disk->FreeBlock(emptyBlock);
        parentNode.block_count--;
    }
    // 写回父目录元数据
    disk->WriteInode(currentInodeId, parentNode);
    return true;
}