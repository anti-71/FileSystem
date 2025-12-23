#ifndef SHELL_H
#define SHELL_H

#include "DiskManager.h"
#include "DirectoryManager.h"
#include "FileManager.h"
#include "UserManager.h"
#include <string>
#include <vector>

class Shell {
public:
    // 启动 Shell 循环
    void run(DiskManager& dm, UserManager& um, DirectoryManager& dirM, FileManager& fileM, SystemContext& ctx);

private:
    // 将输入的字符串拆分为参数列表 (e.g., "mkdir /home/docs" -> ["mkdir", "/home/docs"])
    std::vector<std::string> parseInput(const std::string& input);

    // 命令分发
    void executeCommand(const std::vector<std::string>& args, DiskManager& dm, UserManager& um, DirectoryManager& dirM, FileManager& fileM, SystemContext& ctx);

    // 显示帮助信息
    void showHelp();

    // 格式化输出提示符
    void printPrompt(SystemContext& ctx);
};

#endif