#ifndef DIRECTORY_MANAGER_H
#define DIRECTORY_MANAGER_H

#include "DiskManager.h"
#include "FileSystem.h"
#include <vector>
#include <string>

class DirectoryManager
{
public:
    // 核心命令
    bool mkdir(DiskManager &dm, SystemContext &ctx, std::string dirName);
    void ls(DiskManager &dm, SystemContext &ctx);
    bool cd(DiskManager &dm, SystemContext &ctx, std::string path);
    std::string pwd(SystemContext &ctx);

    // 路径解析：将 "/home/user" 转为 Inode ID
    int resolvePath(DiskManager &dm, int startInodeId, std::string path);
    std::string normalizePath(std::string currentPath, std::string inputPath);
    int findEntryInDirectory(DiskManager &dm, int dirInodeId, const std::string &name);

    // 用户数据持久化
    bool saveUsers(const UserList &uList);
    bool loadUsers(UserList &uList);

    // 日志记录
    void logAction(const std::string &username, const std::string &command, const std::string &result);

private:
    // 辅助：向目录添加一条记录
    bool addEntryToDir(DiskManager &dm, int parentInodeId, int childInodeId, std::string name);
};

#endif