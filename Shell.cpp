#include "Shell.h"

void Shell::Run(DiskManager &dm, UserManager &um, DirectoryManager &dirm, FileManager &fm, SystemContext &ctx)
{
    std::string input;
    std::cout << "欢迎使用FS！ (输入'help'获取指令列表)" << std::endl;

    while (true)
    {
        PrintPrompt(ctx, fm);
        if (!std::getline(std::cin, input))
            break; // 处理 Ctrl+C 等异常退出
        if (input.empty())
            continue; // 忽略空输入
        std::vector<std::string> args = ParseInput(input);
        std::string cmd = args[0];

        if (cmd == "exit" || cmd == "logout")
        {
            dm.UnMount();
            um.SaveUsersToFile(ctx);
            // dm.logAction(uname, "exit", "Success");
            std::cout << "再见！" << std::endl;
            break;
        }
        ExecuteCommand(args, dm, um, dirm, fm, ctx);
    }
}

// 解析用户输入的命令行参数
std::vector<std::string> Shell::ParseInput(const std::string &input)
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
void Shell::PrintPrompt(SystemContext &ctx, FileManager &fm)
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
    std::string path = fm.GetAbsolutePath();
    std::cout << "@FS" << path << "]$ ";
}

// 执行命令
void Shell::ExecuteCommand(const std::vector<std::string> &args, DiskManager &dm, UserManager &um, DirectoryManager &dirm, FileManager &fm, SystemContext &ctx)
{
    std::string cmd = args[0];

    if (cmd == "help")
    {
        ShowHelp();
    }
    else if (cmd == "su")
    {
        um.SwitchUser(ctx, args);
    }
    else if (cmd == "ls")
    {
        ShowList(fm.GetCurrentInodeId(), dirm, &dm);
    }
    else if (cmd == "cd")
    {
        ExecuteCD(args[1], fm);
    }
    else if (cmd == "mkdir")
    {
        if (args.size() < 2)
            std::cout << "用法: mkdir <dirname>" << std::endl;
        else
            fm.MakeDirectory(args[1]);
    }
    else if (cmd == "touch")
    {
        if (args.size() < 2)
            std::cout << "用法: touch <filename>" << std::endl;
        else
            fm.TouchFile(args[1]);
    }
    else if (cmd == "rm")
    {
        if (args.size() < 2)
            std::cout << "用法: rm <filename>" << std::endl;
        else
            ExecuteRM(args[1], dirm, fm, &dm);
    }
    else if (cmd == "cat")
    {
        if (args.size() < 2)
            std::cout << "用法: cat <filename>" << std::endl;
        else
            std::cout << fm.ReadFile(args[1]) << std::endl;
    }
    else if (cmd == "write")
    {
        if (args.size() < 3)
            std::cout << "用法: write <filename> <content>" << std::endl;
        else
            fm.WriteFile(args[1], args[2]);
    }
    else
    {
        std::cout << "无效指令: " << cmd << "！输入'help'获取指令列表" << std::endl;
    }
}

// 显示指令列表
void Shell::ShowHelp()
{
    std::cout << "支持的指令:\n"
              << "  ls                  列出目录内容\n"
              << "  mkdir <名称>        创建目录\n"
              << "  touch <名称>        创建空文件\n"
              << "  rm    <名称>        删除文件或目录\n"
              << "  cat   <名称>        显示文件内容\n"
              << "  write <名称> <内容> 向文件覆盖式写入信息\n"
              << "  su    <用户ID>      切换用户（不存在则自动创建）\n"
              << "  exit/logout         保存并退出系统" << std::endl;
}

// 显示目录内容的详细信息
void Shell::ShowList(uint32_t currentInodeId, DirectoryManager &dir_mgr, DiskManager *disk)
{
    std::vector<DirEntry> entries = dir_mgr.ListDirectory(currentInodeId);
    for (const auto &entry : entries)
    {
        std::string name(entry.name);
        if (name == "." || name == "..")
            continue;
        Inode node;
        // 需要通过 DiskManager 读取该条目对应的 Inode 获取类型
        if (disk->ReadInode(entry.inode_id, node))
        {
            std::string type = (node.mode == 2) ? "[DIR]" : "[FILE]";
            std::cout << std::left << std::setw(10) << type
                      << std::setw(20) << entry.name << std::endl;
        }
    }
}

// 执行 cd 命令
void Shell::ExecuteCD(const std::string &path, FileManager &fm)
{
    if (path.empty() || path == ".")
        return; // 原地踏步
    if (fm.ChangeDirectory(path))
        return;
}

// 执行删除文件逻辑
void Shell::ExecuteRM(const std::string &filename, DirectoryManager &dirm, FileManager &fm, DiskManager *disk)
{
    // 1. 安全检查：获取该文件的 Inode
    uint32_t inodeId = dirm.FindInodeId(filename, fm.GetCurrentInodeId());
    if (inodeId == (uint32_t)-1)
    {
        std::cerr << "错误: 无法删除 '" << filename << "': 没有那个文件或目录！" << std::endl;
        return;
    }
    Inode node;
    if (disk->ReadInode(inodeId, node))
        // 2. 检查是否为目录 (mode == 2)
        if (node.mode == 2)
        {
            std::cerr << "rm: 无法删除 '" << filename << "': 是一个目录" << std::endl;
            return;
        }
    // 3. 执行删除
    if (fm.DeleteFile(filename))
    { // 删除成功，模拟 Linux 默认不输出
    }
    else
    {
        std::cerr << "rm: 删除失败" << std::endl;
        return;
    }
}