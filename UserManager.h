#ifndef USER_MANAGER_H
#define USER_MANAGER_H

#include "FileSystem.h"
#include <string>

class UserManager
{
private:
    const std::string filename = "user_data.dat";

public:
    UserManager();

    void SaveUsersToFile(SystemContext &ctx);
    void LoadUsers(SystemContext &ctx);
    void SwitchUser(SystemContext &ctx, const std::vector<std::string> &args);
    void AddUser(SystemContext &ctx, int targetId);
};

#endif