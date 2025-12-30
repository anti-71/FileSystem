#ifndef SHELL_H
#define SHELL_H

#include "DiskManager.h"
#include "DirectoryManager.h"
#include "FileManager.h"
#include "UserManager.h"
#include "FileSystem.h"

class Shell
{
public:
    void Run(DiskManager &dm, UserManager &um, DirectoryManager &dirm, FileManager &fm, SystemContext &ctx);

private:
    std::vector<std::string> ParseInput(const std::string &input);
    void ExecuteCommand(const std::vector<std::string> &args, DiskManager &dm, UserManager &um, DirectoryManager &dirm, FileManager &fm, SystemContext &ctx);
    void ShowHelp();
    void PrintPrompt(SystemContext &ctx, FileManager &fm);
    void ShowList(uint32_t currentInodeId, DirectoryManager &dir_mgr, DiskManager *disk);
    void ExecuteCD(const std::string &path, FileManager &fm);
    void ExecuteRM(const std::string &filename, DirectoryManager &dirm, FileManager &fm, DiskManager *disk);
};

#endif