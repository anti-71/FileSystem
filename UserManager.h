#ifndef USER_MANAGER_H
#define USER_MANAGER_H

#include "FileSystem.h"
#include <string>

class UserManager {
private:
    User* currentUser;
    std::string encryptDecrypt(std::string data);

public:
    UserManager();
    
    // 用户操作
    bool registerUser(SystemContext& ctx, std::string name, std::string pwd, UserType type, std::string group);
    bool login(SystemContext& ctx, std::string name, std::string pwd);
    void logout(SystemContext& ctx);

    // 权限检查核心
    bool checkPermission(const Inode& inode, int opPermission);

    // 获取当前状态
    User* getCurrentUser() const;
    bool isLoggedIn() const;
};

#endif