#ifndef DIRECTORY_MANAGER_H
#define DIRECTORY_MANAGER_H

#include "FileSystem.h"

class DirectoryManager
{
private:
    DiskManager *disk; // 引用底层的磁盘管理器

public:
    DirectoryManager(DiskManager *dm);
    bool InitializeRoot();
    bool AddDirEntry(uint32_t currentInodeId, const std::string &fileName, uint32_t newInodeId);
    uint32_t FindInodeId(const std::string &name, uint32_t currentDirInodeId);
};

#endif