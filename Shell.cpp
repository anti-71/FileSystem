#include "Shell.h"

void Shell::Run(DiskManager &dm, UserManager &um, DirectoryManager &dirm, FileManager &fm, LockManager &lm, SystemContext &ctx)
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
            std::cout << "再见！" << std::endl;
            break;
        }
        ExecuteCommand(args, dm, um, dirm, fm, lm, ctx);
    }
}

// 解析用户输入的命令行参数
std::vector<std::string> Shell::ParseInput(const std::string &input)
{
    std::vector<std::string> args;
    std::stringstream ss(input);
    std::string arg;
    while (ss >> arg)
        args.push_back(arg);
    return args;
}

// 实时输出 context 里的信息
void Shell::PrintPrompt(SystemContext &ctx, FileManager &fm)
{
    std::cout << "[";
    if (ctx.currentUser.groupId == GID_ROOT)
        std::cout << "管理员";
    else if (ctx.currentUser.groupId == GID_USERS)
        std::cout << "用户" << ctx.currentUser.userId;
    else if (ctx.currentUser.groupId == GID_GUEST)
        std::cout << "访客" << ctx.currentUser.userId;
    std::string path = fm.GetAbsolutePath();
    std::cout << "@FS" << path << "]$ ";
}

// 执行命令
void Shell::ExecuteCommand(const std::vector<std::string> &args, DiskManager &dm, UserManager &um, DirectoryManager &dirm, FileManager &fm, LockManager &lm, SystemContext &ctx)
{
    std::string cmd = args[0];
    if (cmd == "help")
        ShowHelp();
    else if (cmd == "su")
    {
        if (args.size() < 3)
        {
            std::cout << "用法: su <userID> <groupID>" << std::endl;
            return;
        }
        else
            um.SwitchUser(ctx, args);
    }
    else if (cmd == "ls")
        ShowList(fm.GetCurrentInodeId(), dirm, &dm);
    else if (cmd == "cd")
    {
        if (args.size() < 2)
            std::cout << "用法: cd <dirname>" << std::endl;
        else
            ExecuteCD(args[1], fm);
    }
    else if (cmd == "mkdir")
    {
        if (args.size() < 2)
            std::cout << "用法: mkdir <dirname> [perm]" << std::endl;
        else if (args.size() == 2)
            fm.MakeDirectory(args[1]);
        else
            fm.MakeDirectory(args[1], std::stoul(args[2], nullptr, 8));
    }
    else if (cmd == "touch")
    {
        if (args.size() < 2)
            std::cout << "用法: touch <filename> [perm]" << std::endl;
        else if (args.size() == 2)
            fm.TouchFile(args[1]);
        else
            fm.TouchFile(args[1], std::stoul(args[2], nullptr, 8));
    }
    else if (cmd == "rm")
    {
        if (args.size() < 2)
            std::cout << "用法: rm <filename>" << std::endl;
        else
            ExecuteRM(args[1], dirm, fm, &dm, lm, ctx);
    }
    else if (cmd == "cat")
    {
        if (args.size() < 2)
            std::cout << "用法: cat <filename>" << std::endl;
        else
            ExecuteCat(args[1], dm, dirm, lm, fm, ctx);
    }
    else if (cmd == "write")
    {
        if (args.size() < 3)
            std::cout << "用法: write <filename> <content>" << std::endl;
        else
        {
            std::string full_content = "";
            for (size_t i = 2; i < args.size(); ++i)
            {
                full_content += args[i];
                if (i != args.size() - 1)
                {
                    full_content += " ";
                }
            }
            ExecuteWrite(args[1], full_content, dm, dirm, lm, fm, ctx);
        }
    }
    else
        std::cout << "无效指令: " << cmd << "！输入'help'获取指令列表" << std::endl;
}

// 显示指令列表
void Shell::ShowHelp()
{
    std::cout << "支持的指令:\n"
              << "    ls                      列出目录内容\n"
              << "    cd    <目录名>          切换当前工作目录\n"
              << "    mkdir <名称> [权限]     创建目录\n"
              << "    touch <名称> [权限]     创建空文件\n"
              << "    rm    <名称>            删除文件或目录\n"
              << "    cat   <名称>            显示文件内容\n"
              << "    write <名称> <内容>     向文件覆盖式写入信息\n"
              << "    su    <用户ID> <组ID>   切换用户（不存在则自动创建）\n"
              << "    exit/logout             保存并退出系统" << std::endl;
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
            // 1. 提取类型和权限
            uint32_t fileType = node.mode >> 9;
            std::string typeTag = (fileType == 2 ? "[DIR]" : "[FILE]");
            uint32_t permissions = node.mode & 0777;
            // 2. 构造权限字符串
            std::string permStr = GetPermString(permissions);
            // 3. 格式化输出
            std::cout << std::left
                      << std::setw(8) << typeTag                 // 1. 类型简写 (如 [DIR])
                      << std::setw(20) << entry.name             // 2. 文件名 (留宽一点)
                      << "UID:" << std::setw(6) << node.owner_id // 3. 所有者
                      << "GID:" << std::setw(6) << node.group_id // 4. 所属组
                      << "  " << permStr                         // 5. 权限位
                      << std::endl;
        }
    }
}

// 格式化权限位为可读字符串
std::string Shell::GetPermString(uint32_t permissions)
{
    std::string permStr = "";
    // 属主权限
    permStr += (permissions & 0400) ? "r" : "-";
    permStr += (permissions & 0200) ? "w" : "-";
    permStr += (permissions & 0100) ? "x" : "-";
    permStr += " ";
    // 属组权限
    permStr += (permissions & 0040) ? "r" : "-";
    permStr += (permissions & 0020) ? "w" : "-";
    permStr += (permissions & 0010) ? "x" : "-";
    permStr += " ";
    // 其他人权限
    permStr += (permissions & 0004) ? "r" : "-";
    permStr += (permissions & 0002) ? "w" : "-";
    permStr += (permissions & 0001) ? "x" : "-";
    return permStr;
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
void Shell::ExecuteRM(const std::string &filename, DirectoryManager &dirm, FileManager &fm, DiskManager *disk, LockManager &lm, SystemContext &ctx)
{
    // 1. 权限预检：检查父目录权限
    // 访客拦截逻辑
    if (ctx.currentUser.groupId == GID_GUEST)
    {
        std::cerr << "权限拒绝：访客账户无法执行 rm 操作！" << std::endl;
        return;
    }
    // 删除文件本质是修改父目录的内容（移除目录项），所以必须有父目录的 W 权限
    uint32_t parentInodeId = fm.GetCurrentInodeId();
    if (!fm.HasPermission(parentInodeId, PERM_W, ctx.currentUser))
    {
        std::cerr << "权限拒绝：您没有父目录的写权限！" << std::endl;
        return;
    }
    // 2. 安全检查：获取该文件的 Inode
    uint32_t inodeId = dirm.FindInodeId(filename, parentInodeId);
    if (inodeId == (uint32_t)-1)
    {
        std::cerr << "错误: 无法删除 '" << filename << "': 没有那个文件或目录！" << std::endl;
        return;
    }
    // 3. 申请写权限
    if (!lm.RequestAccess(inodeId, true))
    {
        std::cerr << "文件保护：无法删除 '" << filename << "'正在被其他用户访问，请稍后再试!" << std::endl;
        return;
    }
    // 4. 执行删除
    if (fm.DeleteFile(filename))
    { // 删除成功，模拟 Linux 默认不输出
    }
    else
        std::cerr << "rm: 删除失败" << std::endl;
    // 无论删除成功与否，操作结束必须释放锁
    lm.ReleaseAccess(inodeId, true);
}

// 执行显示文件内容逻辑
void Shell::ExecuteCat(const std::string &filename, DiskManager &dm, DirectoryManager &dirm, LockManager &lm, FileManager &fm, SystemContext &ctx)
{
    // 获取文件的 Inode ID (必须先知道 ID 才能锁住对应的资源)
    uint32_t inodeId = dirm.FindInodeId(filename, fm.GetCurrentInodeId());
    if (inodeId == (uint32_t)-1)
    {
        std::cout << "错误：文件 '" << filename << "' 不存在!" << std::endl;
        return;
    }
    Inode targetNode;
    dm.ReadInode(inodeId, targetNode);
    // 检查是否为目录：cat 不能查看目录内容
    if ((targetNode.mode >> 9) == 2)
    {
        std::cout << "错误: " << filename << ":是一个目录！" << std::endl;
        return;
    }
    // 权限校验：必须拥有读权限 (PERM_R)
    if (!fm.HasPermission(inodeId, PERM_R, ctx.currentUser))
    {
        std::cout << "权限拒绝：您没有当前文件的读权限！" << std::endl;
        return;
    }
    // 申请“读权限” (isWrite = false)
    if (lm.RequestAccess(inodeId, false))
    {
        // std::this_thread::sleep_for(std::chrono::seconds(10)); // 模拟读大文件耗时
        // 执行读取操作
        std::cout << fm.ReadFile(filename) << std::endl;
        // 读取完成后，必须释放读权限
        lm.ReleaseAccess(inodeId, false);
    }
    else
        // 如果返回 false，说明此时有人正在“写”或者“删除”该文件
        std::cout << "文件保护：文件 '" << filename << "' 正在被其他用户访问，请稍后再试!" << std::endl;
}

// 执行写入文件逻辑
void Shell::ExecuteWrite(const std::string &filename, const std::string &content, DiskManager &dm, DirectoryManager &dirm, LockManager &lm, FileManager &fm, SystemContext &ctx)
{
    // 1. 查找 Inode ID
    uint32_t inodeId = dirm.FindInodeId(filename, fm.GetCurrentInodeId());
    if (inodeId == (uint32_t)-1)
    {
        std::cout << "错误：文件 '" << filename << "' 不存在!" << std::endl;
        return;
    }
    Inode targetNode;
    dm.ReadInode(inodeId, targetNode);
    // 2. 检查是否为目录：write 不能写入目录
    if ((targetNode.mode >> 9) == 2)
    {
        std::cout << "错误: " << filename << ":是一个目录！" << std::endl;
        return;
    }
    // 3. 权限校验：必须拥有写权限 (PERM_W)
    if (!fm.HasPermission(inodeId, PERM_W, ctx.currentUser))
    {
        std::cout << "权限拒绝：您没有当前文件的写权限！" << std::endl;
        return;
    }
    // 4. 申请写权限
    if (lm.RequestAccess(inodeId, true))
    {
        // 5. 执行实际的写入操作
        fm.WriteFile(filename, content);
        // 6. 操作完成后必须释放权限
        lm.ReleaseAccess(inodeId, true);
    }
    else
        // 如果 RequestAccess 返回 false，说明有人正在 cat (读) 或正在 write (写)
        std::cout << "文件保护：文件 '" << filename << "' 正在被其他用户访问，请稍后再试!" << std::endl;
}