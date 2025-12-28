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
    void PrintPrompt(SystemContext &ctx);
    void ShowList(uint32_t currentInodeId, DirectoryManager &dir_mgr, DiskManager *disk);
};

#endif