#include "FileManager.h"
#include <iostream>
#include <algorithm>
#include <cstring>

bool FileManager::create(DiskManager &dm, DirectoryManager &dirM, SystemContext &ctx, std::string fileName)
{
    // 1. 分配新 Inode
    int newInodeId = dm.allocateInode();
    if (newInodeId == -1)
        return false;

    Inode *inode = dm.getInode(newInodeId);
    inode->type = F_FILE;
    inode->permissions = 0644; // rw-r--r--
    inode->fileSize = 0;
    inode->ownerId = 0; // 简化处理，设为 root

    // 2. 在当前目录下添加记录 (复用 DirectoryManager 的 addEntry 逻辑)
    // 注意：这里建议在 DirectoryManager 里把 addEntryToDir 设为 public
    if (!dirM.mkdir(dm, ctx, fileName))
    { // 暂时借用 mkdir，后续可优化为通用的 addEntry
        dm.freeInode(newInodeId);
        return false;
    }

    // 修正：mkdir 会把类型设为 DIR，我们需要手动改回 FILE
    dm.getInode(newInodeId)->type = F_FILE;
    dm.saveToDisk();
    return true;
}

int FileManager::write(DiskManager &dm, SystemContext &ctx, int fd, const std::string &content)
{
    FileDescriptor *fdesc = getDescriptor(ctx, fd);
    if (!fdesc || fdesc->mode == READ)
        return -1;

    Inode *inode = dm.getInode(fdesc->inodeId);

    // 简化版逻辑：将内容写入第一个块（支持追加）
    if (inode->directIndices[0] == -1)
    {
        inode->directIndices[0] = dm.allocateDataBlock();
    }

    DataBlock *db = dm.getDataBlock(inode->directIndices[0]);
    int writeLen = std::min((int)content.length(), BLOCK_SIZE - (int)fdesc->offset);

    memcpy(db->data + fdesc->offset, content.c_str(), writeLen);

    fdesc->offset += writeLen;
    inode->fileSize = std::max(inode->fileSize, fdesc->offset);
    inode->modifiedAt = time(nullptr);

    dm.saveToDisk();
    return writeLen;
}

std::string FileManager::read(DiskManager &dm, SystemContext &ctx, int fd, int length)
{
    FileDescriptor *fdesc = getDescriptor(ctx, fd);
    if (!fdesc)
        return "";

    Inode *inode = dm.getInode(fdesc->inodeId);
    int realLen = std::min(length, (int)(inode->fileSize - fdesc->offset));
    if (realLen <= 0)
        return "";

    DataBlock *db = dm.getDataBlock(inode->directIndices[0]); // 简化：只读第一块
    std::string result(db->data + fdesc->offset, realLen);

    fdesc->offset += realLen;
    return result;
}

FileDescriptor *FileManager::getDescriptor(SystemContext &ctx, int fd)
{
    for (auto &fdesc : ctx.openFiles)
    {
        if (fdesc.fdId == fd)
        {
            return &fdesc;
        }
    }
    std::cerr << "错误：未找到文件描述符 ID: " << fd << std::endl;
    return nullptr;
}

int FileManager::open(DiskManager &dm, DirectoryManager &dirM, SystemContext &ctx, std::string path, AccessMode mode)
{
    // 1. 调用路径解析找到 Inode (如果 path 是相对路径，需结合 ctx.currentDir)
    int inodeId = dirM.resolvePath(dm, ctx.currentDir.currentInodeId, path);
    if (inodeId == -1)
    {
        std::cerr << "错误：找不到文件 " << path << std::endl;
        return -1;
    }

    // 2. 检查是否真的是文件（不是目录）
    if (dm.getInode(inodeId)->type != F_FILE)
    {
        std::cerr << "错误：" << path << " 是一个目录" << std::endl;
        return -1;
    }

    // 3. 分配文件描述符 (FD)
    FileDescriptor newFD;
    newFD.fdId = ctx.openFiles.size() + 1; // 简单分配 ID
    newFD.inodeId = inodeId;
    newFD.mode = mode;
    newFD.offset = 0;

    ctx.openFiles.push_back(newFD);
    return newFD.fdId;
}

// 实现 close 函数
void FileManager::close(SystemContext &ctx, int fd)
{
    for (auto it = ctx.openFiles.begin(); it != ctx.openFiles.end(); ++it)
    {
        if (it->fdId == fd)
        {
            ctx.openFiles.erase(it);
            return;
        }
    }
}