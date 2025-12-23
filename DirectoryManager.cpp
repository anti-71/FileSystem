#include "DirectoryManager.h"
#include <sstream>
#include <cstring>

bool DirectoryManager::mkdir(DiskManager &dm, SystemContext &ctx, std::string dirName)
{
    // 1. 分配新 Inode
    int newInodeId = dm.allocateInode();
    if (newInodeId == -1)
    {
        std::cout << "Error: No available Inode." << std::endl;
        return false;
    }

    Inode *newInode = dm.getInode(newInodeId);
    newInode->type = F_DIRECTORY;
    newInode->permissions = 0755;
    newInode->ownerId = ctx.currentUser.userId; // 使用当前切换后的用户ID
    newInode->fileSize = 0;

    // 2. 关键步骤：初始化目录项，添加 "." 和 ".."
    // "." 指向自己
    if (!addEntryToDir(dm, newInodeId, newInodeId, "."))
    {
        dm.freeInode(newInodeId);
        return false;
    }
    // ".." 指向父目录 (即当前工作目录的 Inode ID)
    if (!addEntryToDir(dm, newInodeId, ctx.currentDir.currentInodeId, ".."))
    {
        // 注意：这里如果失败，需要清理刚才添加的 "." 逻辑（实际实现中 addEntry 会处理块分配）
        dm.freeInode(newInodeId);
        return false;
    }

    // 3. 将新目录挂载到当前工作目录的目录项中
    if (!addEntryToDir(dm, ctx.currentDir.currentInodeId, newInodeId, dirName))
    {
        dm.freeInode(newInodeId);
        return false;
    }

    dm.saveToDisk();
    return true;
}

bool DirectoryManager::addEntryToDir(DiskManager &dm, int parentInodeId, int childInodeId, std::string name)
{
    Inode *parent = dm.getInode(parentInodeId);

    // 简化逻辑：只在第一个数据块找空位
    if (parent->directIndices[0] == -1)
    {
        parent->directIndices[0] = dm.allocateDataBlock();
    }

    DataBlock *db = dm.getDataBlock(parent->directIndices[0]);
    DirRecord *records = reinterpret_cast<DirRecord *>(db->data);

    for (int i = 0; i < BLOCK_SIZE / sizeof(DirRecord); i++)
    {
        if (records[i].isUsed == 0)
        {
            records[i].isUsed = 1;
            records[i].inodeId = childInodeId;
            strncpy(records[i].name, name.c_str(), 55);
            return true;
        }
    }
    return false; // 目录满了
}

void DirectoryManager::ls(DiskManager &dm, SystemContext &ctx)
{
    Inode *curInode = dm.getInode(ctx.currentDir.currentInodeId);

    // 遍历直接索引块
    for (int i = 0; i < 10; i++)
    {
        int blockId = curInode->directIndices[i];
        if (blockId == -1)
            continue;

        DataBlock *db = dm.getDataBlock(blockId);
        DirRecord *records = reinterpret_cast<DirRecord *>(db->data);

        for (int j = 0; j < BLOCK_SIZE / sizeof(DirRecord); j++)
        {
            if (records[j].isUsed)
            {
                Inode *item = dm.getInode(records[j].inodeId);
                std::cout << (item->type == F_DIRECTORY ? "[DIR] " : "[FILE] ")
                          << records[j].name << "\tID: " << records[j].inodeId << std::endl;
            }
        }
    }
}

int DirectoryManager::resolvePath(DiskManager &dm, int startInodeId, std::string path)
{
    if (path == "/")
        return 0; // 快速返回根目录

    int currentInodeId = startInodeId;
    if (path[0] == '/')
    {
        currentInodeId = 0; // 绝对路径从根开始
        path = path.substr(1);
    }

    std::stringstream ss(path);
    std::string token;

    // 按照 '/' 分割路径：如 "a/b/../c" 分解为 "a", "b", "..", "c"
    while (std::getline(ss, token, '/'))
    {
        if (token == "" || token == ".")
            continue; // 忽略空字符和当前目录

        // 在当前目录下查找名为 token 的目录项
        int nextInodeId = findEntryInDirectory(dm, currentInodeId, token);

        if (nextInodeId == -1)
        {
            return -1; // 路径中的某一环不存在
        }
        currentInodeId = nextInodeId;
    }

    return currentInodeId;
}

std::string normalizePath(std::string currentPath, std::string inputPath)
{
    // 1. 确定基准路径：如果是以 / 开头则是绝对路径，否则叠加在当前路径上
    std::string fullPath = (inputPath[0] == '/') ? inputPath : (currentPath + "/" + inputPath);

    std::vector<std::string> stack;
    std::stringstream ss(fullPath);
    std::string part;

    // 2. 使用 '/' 作为分隔符拆分路径
    while (std::getline(ss, part, '/'))
    {
        if (part == "" || part == ".")
        {
            // 忽略空字符串（由多余的/产生）和当前目录标识
            continue;
        }
        if (part == "..")
        {
            // 遇到 ..，弹出栈顶（回退一级），前提是栈不为空
            if (!stack.empty())
            {
                stack.pop_back();
            }
        }
        else
        {
            // 普通目录名，入栈
            stack.push_back(part);
        }
    }

    // 3. 将栈中的组件重新拼接成路径
    if (stack.empty())
    {
        return "/";
    }

    std::string result = "";
    for (const std::string &dir : stack)
    {
        result += "/" + dir;
    }

    return result;
}

#include <cstring>

// DirectoryManager.cpp
#include <sstream>

int DirectoryManager::findEntryInDirectory(DiskManager &dm, int dirInodeId, const std::string &name)
{
    // 1. 找到对应的 Inode
    Inode *dirInode = dm.getInode(dirInodeId);
    if (!dirInode || dirInode->type != F_DIRECTORY)
        return -1;
    DirEntry *current = dirMap[dirInodeId]; // 获取该目录的链表头

    while (current != nullptr)
    {
        if (current->fileName == name)
        {
            return current->inodeId;
        }
        current = current->next;
    }

    return -1;
}