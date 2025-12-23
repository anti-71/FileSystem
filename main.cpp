#include "DiskManager.h"
#include "UserManager.h"
#include "DirectoryManager.h"
#include "FileManager.h"
#include "Shell.h"

int main()
{
    SetConsoleOutputCP(65001); // 强制更改终端编码

    DiskManager dm;
    UserManager um;
    DirectoryManager dirM;
    FileManager fileM;
    SystemContext ctx;
    Shell shell;

    ctx.currentUser.userId = 0;
    ctx.currentUser.username = "root";
    ctx.currentDir.path = "/";

    // 1. 初始化系统
    if (!dm.loadFromDisk())
    {
        dm.formatDisk();
    }

    // 2. 初始化上下文 (从根目录开始)
    ctx.currentDir.currentInodeId = 0;
    ctx.currentDir.path = "/";

    // 3. 启动 Shell
    shell.run(dm, um, dirM, fileM, ctx);

    return 0;
}