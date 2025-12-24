#include "DiskManager.h"
#include "UserManager.h"
// #include "DirectoryManager.h"
// #include "FileManager.h"
#include "Shell.h"

int main()
{
    SetConsoleOutputCP(65001); // 强制更改终端编码

    DiskManager dm(VDISK_PATH);
    UserManager um;
    // DirectoryManager dirM;
    // FileManager fileM;
    SystemContext ctx;
    Shell shell;

    // 读入用户列表
    um.LoadUsers(ctx);
    ctx.currentUser.userId = 0;
    // ctx.currentDir.path = "/";

    // 1. 初始化系统
    if (!dm.FileExists(VDISK_PATH))
    {
        std::cout << "检测到镜像不存在，正在进行首次初始化..." << std::endl;
        dm.InitializeDisk(VDISK_PATH);
    }

    // // 2. 初始化上下文 (从根目录开始)
    // ctx.currentDir.currentInodeId = 0;
    // ctx.currentDir.path = "/";

    // 3. 启动 Shell
    shell.run(dm, um, ctx);

    return 0;
}