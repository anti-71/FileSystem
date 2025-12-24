#ifndef SHELL_H
#define SHELL_H

#include "DiskManager.h"
// #include "DirectoryManager.h"
// #include "FileManager.h"
#include "UserManager.h"
#include <string>
#include <vector>

class Shell
{
public:
    void run(DiskManager &dm, UserManager &um, SystemContext &ctx);

private:
    std::vector<std::string> parseInput(const std::string &input);
    void executeCommand(const std::vector<std::string> &args, UserManager &um, SystemContext &ctx);
    void showHelp();
    void printPrompt(SystemContext &ctx);
};

#endif