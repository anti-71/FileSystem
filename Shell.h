#ifndef SHELL_H
#define SHELL_H

#include "DiskManager.h"
#include "DirectoryManager.h"
#include "FileManager.h"
#include "UserManager.h"
#include "FileSystem.h"
#include "LockManager.h"

class Shell
{
public:
    void Run(DiskManager &dm, UserManager &um, DirectoryManager &dirm, FileManager &fm,LockManager &lm, SystemContext &ctx);

private:
    std::vector<std::string> ParseInput(const std::string &input);
    void ExecuteCommand(const std::vector<std::string> &args, DiskManager &dm, UserManager &um, DirectoryManager &dirm, FileManager &fm, LockManager &lm, SystemContext &ctx);
    void ShowHelp();
    void PrintPrompt(SystemContext &ctx, FileManager &fm);
    void ShowList(uint32_t currentInodeId, DirectoryManager &dir_mgr, DiskManager *disk);
    void ExecuteCD(const std::string &path, FileManager &fm);
    void ExecuteRM(const std::string &filename, DirectoryManager &dirm, FileManager &fm, DiskManager *disk,LockManager &lm);
    void ExecuteCat(const std::string &filename, DirectoryManager &dirm, LockManager &lm, FileManager &fm);
    void ExecuteWrite(const std::string &filename, const std::string &content, DirectoryManager &dirm, LockManager &lm, FileManager &fm);
};

#endif