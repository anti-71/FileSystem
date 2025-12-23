#include "Shell.h"
#include <iostream>
#include <sstream>

void Shell::run(DiskManager &dm, UserManager &um, DirectoryManager &dirM, FileManager &fileM, SystemContext &ctx)
{
    std::string input;
    std::cout << "Welcome to MiniFS (Type 'help' for commands)" << std::endl;

    while (true)
    {
        printPrompt(ctx);
        if (!std::getline(std::cin, input))
            break; // 处理 Ctrl+C 等异常退出
        if (input.empty())
            continue;
        std::vector<std::string> args = parseInput(input);

        std::string cmd = args[0];

        if (cmd == "exit" || cmd == "logout")
        {
            dm.saveToDisk();
            dm.saveUsers(ctx.uList);
            // 如果 SystemContext 里没存 currentUser，我们从 UserManager 获取
            std::string uname = um.getCurrentUser() ? um.getCurrentUser()->username : "Guest";
            dm.logAction(uname, "exit", "Success");

            std::cout << "Saving data and exiting... Goodbye!" << std::endl;
            break;
        }
        executeCommand(args, dm, um, dirM, fileM, ctx);
    }
}

std::vector<std::string> Shell::parseInput(const std::string &input)
{
    std::vector<std::string> args;
    std::stringstream ss(input);
    std::string arg;
    while (ss >> arg)
    {
        args.push_back(arg);
    }
    return args;
}

void Shell::printPrompt(SystemContext &ctx)
{
    // 实时读取 context 里的信息
    std::cout << "[" << ctx.currentUser.username << "@MiniFS " << ctx.currentDir.path << "]$ ";
}

void Shell::executeCommand(const std::vector<std::string> &args, DiskManager &dm, UserManager &um, DirectoryManager &dirM, FileManager &fileM, SystemContext &ctx)
{
    std::string cmd = args[0];

    if (cmd == "help")
    {
        showHelp();
    }
    else if (cmd == "su")
    {
        if (args.size() < 2)
        {
            std::cout << "用法: su <用户ID>" << std::endl;
        }
        else
        {
            // 将输入的字符串转为整数 ID
            int newId = std::stoi(args[1]);
            ctx.currentUser.userId = newId;
            // 简单的名称映射逻辑
            if (newId == 0)
            {
                ctx.currentUser.username = "root";
            }
            else
            {
                std::string tempName = "User" + std::to_string(newId);
                ctx.currentUser.username = tempName;
            }
            std::cout << "身份已切换至UID: " << ctx.currentUser.userId
                      << " (" << ctx.currentUser.username << ")" << std::endl;
        }
    }
    else if (cmd == "ls")
    {
        dirM.ls(dm, ctx);
    }
    else if (cmd == "cd")
    {
        std::string target = (args.size() < 2) ? "/" : args[1];
        int targetInode = dirM.resolvePath(dm, ctx.currentDir.currentInodeId, target);

        if (targetInode != -1 && dm.getInode(targetInode)->type == F_DIR)
        {
            // 更新 ID
            ctx.currentDir.currentInodeId = targetInode;
            // 更新规范化后的路径
            ctx.currentDir.path = normalizePath(ctx.currentDir.path, target);
        }
        else
        {
            std::cout << "cd: " << target << ": No such directory" << std::endl;
        }
    }
    else if (cmd == "mkdir")
    {
        if (args.size() < 2)
            std::cout << "Usage: mkdir <dirname>" << std::endl;
        else
            dirM.mkdir(dm, ctx, args[1]);
    }
    else if (cmd == "touch")
    {
        if (args.size() < 2)
            std::cout << "Usage: touch <filename>" << std::endl;
        else
            fileM.create(dm, dirM, ctx, args[1]);
    }
    else if (cmd == "cat")
    {
        if (args.size() < 2)
            std::cout << "Usage: cat <filename>" << std::endl;
        else
        {
            int fd = fileM.open(dm, dirM, ctx, args[1], READ);
            if (fd != -1)
            {
                std::cout << fileM.read(dm, ctx, fd, 1024) << std::endl;
                fileM.close(ctx, fd);
            }
        }
    }
    else if (cmd == "write")
    {
        if (args.size() < 3)
            std::cout << "Usage: write <filename> <content>" << std::endl;
        else
        {
            int fd = fileM.open(dm, dirM, ctx, args[1], WRITE);
            if (fd != -1)
            {
                fileM.write(dm, ctx, fd, args[2]);
                fileM.close(ctx, fd);
            }
        }
    }
    else if (cmd == "pwd")
    {
        std::cout << ctx.currentDir.path << std::endl;
    }
    else
    {
        std::cout << "Invalid command: " << cmd << ". Type 'help' for list." << std::endl;
    }
}

void Shell::showHelp()
{
    std::cout << "Supported commands:\n"
              << "  ls                List directory contents\n"
              << "  mkdir <name>      Create a directory\n"
              << "  touch <name>      Create an empty file\n"
              << "  cat <name>        Display file content\n"
              << "  write <name> <msg> Write message to file\n"
              << "  pwd               Show current path\n"
              << "  exit/logout       Save and exit system" << std::endl;
}