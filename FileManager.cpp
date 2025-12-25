#include "FileManager.h"

FileManager::FileManager(DiskManager *dm,DirectoryManager *dirm) : disk(dm), dir(dirm) {}

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
    if (!disk->WriteInode(inodeNum, &newNode))
        return false;
    // 4. 写入 inode 到磁盘
    if (!disk->WriteInode(inodeNum, &newNode))
        return false;
    // 5. 写入文件名
    if(!dir->AddDirEntry(currentInodeId, name, inodeNum))
        return false;

    return true;
}

// 删除文件
bool DeleteFile(const std::string &name)
{
    // 1. 找到文件对应的 inode
    uint32_t inodeNum = dir->FindInodeId(currentInodeId, name);
}