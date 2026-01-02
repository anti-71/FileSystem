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
    SystemContext *ctx;      // 系统上下文
    uint32_t currentInodeId; // 记录当前所在目录的 Inode 编号

public:
    FileManager(DiskManager *dm, DirectoryManager *dirm, SystemContext *ctx);

    bool CreateFile(const std::string &name, uint32_t customPerm);
    bool DeleteFile(const std::string &name);
    uint32_t GetCurrentInodeId();
    bool MakeDirectory(const std::string &name, uint32_t customPerm = 0);
    bool ChangeDirectory(const std::string &path);
    std::string GetAbsolutePath();
    bool TouchFile(const std::string &name, uint32_t customPerm = 0);
    bool WriteFile(const std::string &name, const std::string &content);
    std::string ReadFile(const std::string &name);
    bool HasPermission(uint32_t inodeId, int requiredPerm, const User &user);
};

#endif