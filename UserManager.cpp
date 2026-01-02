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
        outFile << user.userId << " " << user.groupId << "\n";
    outFile.close();
}

// 加载用户数据
void UserManager::LoadUsers(SystemContext &ctx)
{
    std::ifstream inFile(filename);
    if (!inFile)
        return;
    ctx.uList.clear(); // 先清空当前列表，防止重复
    int uid, gid;
    while (inFile >> uid >> gid)
    {
        User tempUser;
        tempUser.userId = uid;
        tempUser.groupId = gid;
        ctx.uList.push_back(tempUser);
    }
    inFile.close();
}

// 切换用户
void UserManager::SwitchUser(SystemContext &ctx, const std::vector<std::string> &args)
{
    int targetId, targetGroupId;
    try
    {
        targetId = std::stoi(args[1]);
        targetGroupId = std::stoi(args[2]);
    }
    catch (...)
    {
        std::cout << "错误：请输入有效的数字 ID！" << std::endl;
        return;
    }
    // 1. 在 uList 中查找用户是否存在
    auto it = std::find_if(ctx.uList.begin(), ctx.uList.end(), [targetId](const User &u)
                           { return u.userId == targetId; });
    // 2. 找到用户，比对 GID，若没找到，增加用户
    if (it != ctx.uList.end())
    {
        if (it->groupId != targetGroupId && ctx.currentUser.groupId == GID_ROOT && targetGroupId != GID_ROOT)
        {
            std::cout << "修改 ID 为" << targetId << " 的组身份从 " << it->groupId << " 变更为 " << targetGroupId << std::endl;
            it->groupId = targetGroupId; // 更新列表中的组信息
        }
        else if (it->groupId != targetGroupId && ctx.currentUser.groupId == GID_ROOT)
        {
            std::cout << "错误：无法将用户修改为管理员！" << std::endl;
            return;
        }
        else if (it->groupId != targetGroupId)
        {
            std::cout << "错误：当前用户组身份无权限修改组身份！" << std::endl;
            return;
        }
        ctx.currentUser = *it; // 赋值给当前会话
    }
    else
        AddUser(ctx, targetId, targetGroupId);
    // 3. 执行切换逻辑
    std::cout << "身份已切换至: ";
    if (ctx.currentUser.groupId == GID_ROOT)
        std::cout << "管理员";
    else if (ctx.currentUser.groupId == GID_USERS)
        std::cout << "用户" << ctx.currentUser.userId;
    else if (ctx.currentUser.groupId == GID_GUEST)
        std::cout << "访客" << ctx.currentUser.userId;
    std::cout << std::endl;
}

// 添加用户
void UserManager::AddUser(SystemContext &ctx, int targetId, int targetGroupId)
{
    User newUser;
    newUser.userId = targetId;
    newUser.groupId = targetGroupId;
    ctx.uList.push_back(newUser);
    std::cout << "检测到新用户，已自动创建 UID: " << targetId << " GID: " << targetGroupId << std::endl;
}