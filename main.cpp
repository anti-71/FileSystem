#include "DiskManager.h"
#include "UserManager.h"
#include "DirectoryManager.h"
#include "FileManager.h"
#include "Shell.h"
#include "FileSystem.h"

int main()
{
    SetConsoleOutputCP(65001); // 强制更改终端编码

    DiskManager dm(VDISK_PATH);
    UserManager um;
    DirectoryManager dirm(&dm);
    FileManager fm(&dm, &dirm);
    SystemContext ctx;
    Shell shell;

    // 读入用户列表
    um.LoadUsers(ctx);
    ctx.currentUser.userId = 0;
    // 初始化系统
    if (!dm.FileExists(VDISK_PATH))
    {
        std::cout << "检测到镜像不存在，正在进行首次初始化..." << std::endl;
        dm.InitializeDisk(VDISK_PATH);
        dirm.InitializeRoot();
    }
    else
        dm.Mount();
    // 启动 Shell
    shell.Run(dm, um, dirm, fm, ctx);

    return 0;
}