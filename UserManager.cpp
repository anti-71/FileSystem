#include "UserManager.h"

UserManager::UserManager() {}

// 保存用户数据到文件
void UserManager::SaveUsersToFile(SystemContext &ctx)
{
    // 使用 std::ofstream 打开文件
    // std::ios::out 表示写入，std::ios::trunc 表示覆盖旧文件
    std::ofstream outFile(filename, std::ios::out | std::ios::trunc);
    if (!outFile.is_open())
    {
        std::cerr << "错误：无法打开文件进行写入！" << std::endl;
        return;
    }
    // 遍历 UserList 中的每一个用户
    for (const auto &user : ctx.uList)
    {
        outFile << user.userId << "\n";
    }
    outFile.close();
}

// 加载用户数据
void UserManager::LoadUsers(SystemContext &ctx)
{
    std::ifstream inFile(filename);
    if (!inFile)
    {
        return;
    }
    ctx.uList.clear(); // 先清空当前列表，防止重复
    int id;
    while (inFile >> id)
    {
        User tempUser;
        tempUser.userId = id;
        ctx.uList.push_back(tempUser);
    }
    // while (inFile >> id >> age) { ... }
    inFile.close();
}

// 切换用户
void UserManager::SwitchUser(SystemContext &ctx, const std::vector<std::string> &args)
{
    if (args.size() < 2)
    {
        std::cout << "用法: su <用户ID>" << std::endl;
        return;
    }
    int targetId;
    try
    {
        targetId = std::stoi(args[1]);
    }
    catch (...)
    {
        std::cout << "错误：请输入有效的数字 ID！" << std::endl;
        return;
    }
    // 1. 在 uList 中查找用户是否存在
    auto it = std::find_if(ctx.uList.begin(), ctx.uList.end(),
                           [targetId](const User &u)
                           {
                               return u.userId == targetId;
                           });
    // 2. 如果没找到，则添加到列表中
    if (it == ctx.uList.end())
    {
        AddUser(ctx, targetId);
    }
    // 3. 执行切换逻辑
    ctx.currentUser.userId = targetId;
    std::cout << "身份已切换至: ";
    if (ctx.currentUser.userId == 0)
    {
        std::cout << "管理员";
    }
    else
    {
        std::cout << "用户" << ctx.currentUser.userId;
    }
    std::cout << std::endl;
}

// 添加用户
void UserManager::AddUser(SystemContext &ctx, int targetId)
{
    User newUser;
    newUser.userId = targetId;
    ctx.uList.push_back(newUser);
    std::cout << "检测到新用户，已自动创建 UID: " << targetId << std::endl;
}