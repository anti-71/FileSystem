#include "Shell.h"

void Shell::run(DiskManager &dm, UserManager &um, SystemContext &ctx)
{
    std::string input;
    std::cout << "欢迎使用FS！ (输入'help'获取指令列表)" << std::endl;

    while (true)
    {
        printPrompt(ctx);
        if (!std::getline(std::cin, input))
            break; // 处理 Ctrl+C 等异常退出
        if (input.empty())
            continue; // 忽略空输入
        std::vector<std::string> args = parseInput(input);
        std::string cmd = args[0];

        if (cmd == "exit" || cmd == "logout")
        {
            dm.UnMount();
            um.SaveUsersToFile(ctx);
            // dm.logAction(uname, "exit", "Success");
            std::cout << "再见！" << std::endl;
            break;
        }
        executeCommand(args, um, ctx);
    }
}

// 解析用户输入的命令行参数
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

// 实时输出 context 里的信息
void Shell::printPrompt(SystemContext &ctx)
{
    std::cout << "[";
    if (ctx.currentUser.userId == 0)
    {
        std::cout << "管理员";
    }
    else
    {
        std::cout << "用户" << ctx.currentUser.userId;
    }
    std::cout << "@FS" << "]$ ";
}

// 执行命令
void Shell::executeCommand(const std::vector<std::string> &args, UserManager &um, SystemContext &ctx)
{
    std::string cmd = args[0];

    if (cmd == "help")
    {
        showHelp();
    }
    else if (cmd == "su")
    {
        um.SwitchUser(ctx, args);
    }
    // else if (cmd == "ls")
    // {
    //     dirM.ls(dm, ctx);
    // }
    // else if (cmd == "cd")
    // {
    //     std::string target = (args.size() < 2) ? "/" : args[1];
    //     int targetInode = dirM.resolvePath(dm, ctx.currentDir.currentInodeId, target);

    //     if (targetInode != -1 && dm.getInode(targetInode)->type == F_DIR)
    //     {
    //         // 更新 ID
    //         ctx.currentDir.currentInodeId = targetInode;
    //         // 更新规范化后的路径
    //         ctx.currentDir.path = normalizePath(ctx.currentDir.path, target);
    //     }
    //     else
    //     {
    //         std::cout << "cd: " << target << ": No such directory" << std::endl;
    //     }
    // }
    // else if (cmd == "mkdir")
    // {
    //     if (args.size() < 2)
    //         std::cout << "Usage: mkdir <dirname>" << std::endl;
    //     else
    //         dirM.mkdir(dm, ctx, args[1]);
    // }
    // else if (cmd == "touch")
    // {
    //     if (args.size() < 2)
    //         std::cout << "Usage: touch <filename>" << std::endl;
    //     else
    //         fileM.create(dm, dirM, ctx, args[1]);
    // }
    // else if (cmd == "cat")
    // {
    //     if (args.size() < 2)
    //         std::cout << "Usage: cat <filename>" << std::endl;
    //     else
    //     {
    //         int fd = fileM.open(dm, dirM, ctx, args[1], READ);
    //         if (fd != -1)
    //         {
    //             std::cout << fileM.read(dm, ctx, fd, 1024) << std::endl;
    //             fileM.close(ctx, fd);
    //         }
    //     }
    // }
    // else if (cmd == "write")
    // {
    //     if (args.size() < 3)
    //         std::cout << "Usage: write <filename> <content>" << std::endl;
    //     else
    //     {
    //         int fd = fileM.open(dm, dirM, ctx, args[1], WRITE);
    //         if (fd != -1)
    //         {
    //             fileM.write(dm, ctx, fd, args[2]);
    //             fileM.close(ctx, fd);
    //         }
    //     }
    // }
    // else if (cmd == "pwd")
    // {
    //     std::cout << ctx.currentDir.path << std::endl;
    // }
    // else
    // {
    //     std::cout << "Invalid command: " << cmd << ". Type 'help' for list." << std::endl;
    // }
}

// 显示指令列表
void Shell::showHelp()
{
    std::cout << "支持的指令:\n"
              << "  ls                  列出目录内容\n"
              << "  mkdir <名称>        创建目录\n"
              << "  touch <名称>        创建空文件\n"
              << "  cat   <名称>        显示文件内容\n"
              << "  write <名称> <内容> 向文件写入信息\n"
              << "  pwd                 显示当前路径\n"
              << "  su    <用户ID>      切换用户（不存在则自动创建）\n"
              << "  exit/logout         保存并退出系统" << std::endl;
}