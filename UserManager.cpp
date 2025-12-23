#include "UserManager.h"
#include <iostream>

UserManager::UserManager() : currentUser(nullptr) {}

std::string UserManager::encryptDecrypt(std::string data) {
    char key = 'K'; 
    std::string output = data;
    for (int i = 0; i < data.length(); i++)
        output[i] = data[i] ^ key;
    return output;
}

bool UserManager::registerUser(SystemContext& ctx, std::string name, std::string pwd, UserType type, std::string group) {
    for (const auto& u : ctx.uList.users) {
        if (u.username == name) {
            std::cout << "Error: Username already exists." << std::endl;
            return false;
        }
    }
    User newUser;
    newUser.userId = ctx.uList.users.size();
    newUser.username = name;
    newUser.password = encryptDecrypt(pwd);
    newUser.type = type;
    newUser.userGroup = group;
    ctx.uList.users.push_back(newUser);
    return true;
}

bool UserManager::login(SystemContext& ctx, std::string name, std::string pwd) {
    std::string encryptedPwd = encryptDecrypt(pwd);
    for (auto& u : ctx.uList.users) {
        if (u.username == name && u.password == encryptedPwd) {
            currentUser = &u;
            ctx.currentDir.path = "/";
            ctx.currentDir.currentInodeId = 0; 
            std::cout << "Successfully logged in as " << name << "." << std::endl;
            return true;
        }
    }
    std::cout << "Login failed: Incorrect username or password." << std::endl;
    return false;
}

void UserManager::logout(SystemContext& ctx) {
    currentUser = nullptr;
    ctx.openFiles.clear();
    std::cout << "User logged out." << std::endl;
}

bool UserManager::checkPermission(const Inode& inode, int opPermission) {
    if (currentUser == nullptr) return false;
    if (currentUser->type == ADMIN) return true;

    if (inode.ownerId == currentUser->userId) {
        return (inode.permissions & (opPermission)) != 0;
    } else if (inode.groupId == 0) { // 这里可以根据需求细化组逻辑
        return (inode.permissions & (opPermission >> 3)) != 0;
    } else {
        return (inode.permissions & (opPermission >> 6)) != 0;
    }
}

User* UserManager::getCurrentUser() const { return currentUser; }
bool UserManager::isLoggedIn() const { return currentUser != nullptr; }