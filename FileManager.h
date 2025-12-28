#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include "DiskManager.h"
#include "FileSystem.h"
#include "DirectoryManager.h"

class FileManager
{
private:
    DiskManager *disk;       // 引用底层的磁盘管理器
    DirectoryManager *dir;   // 引用底层的目录管理器
    uint32_t currentInodeId; // 记录当前所在目录的 Inode 编号

public:
    FileManager(DiskManager *dm, DirectoryManager *dirm);

    bool CreateFile(const std::string &name);
    bool DeleteFile(const std::string &name);
};

#endif