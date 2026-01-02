#include "FileManager.h"

FileManager::FileManager(DiskManager *dm, DirectoryManager *dirm, SystemContext *ctx) : currentInodeId(0)
{
    this->disk = dm;
    this->dir = dirm;
    this->ctx = ctx;
}

// 创建文件
bool FileManager::CreateFile(const std::string &name, uint32_t customPerm)
{
    // 1. 分配空闲的 inode
    uint32_t inodeNum = disk->AllocateInode();
    if (inodeNum == -1)
        return false;
    // 2. 分配空闲的 block
    uint32_t blockNum = disk->AllocateBlock();
    if (blockNum == -1)
        return false;
    // 3. 初始化 inode
    // 如果用户没传权限，设置文件默认权限
    uint32_t perm = (customPerm == 0) ? ROOT_FILE_MODE : (customPerm & PERM_MASK);
    uint32_t mode = (TYPE_FILE << 9) | perm;
    if (!disk->InitInode(inodeNum, mode, blockNum, (uint32_t)ctx->currentUser.userId, (uint32_t)ctx->currentUser.groupId))
        return false;
    // 4. 写入文件名
    if (!dir->AddDirEntry(currentInodeId, name, inodeNum))
        return false;
    return true;
}

bool FileManager::DeleteFile(const std::string &name)
{
    // 1. 在当前目录查找文件
    Inode parentNode;
    if (!disk->ReadInode(currentInodeId, parentNode))
        return false;
    uint32_t entryCount = parentNode.size / DIR_ENTRY_SIZE;        // 目录项数量
    int targetEntryIdx = -1;                                       // 目标目录项的索引
    uint32_t fileInodeId = dir->FindInodeId(name, currentInodeId); // 文件对应的 inode 编号
    if (fileInodeId == (uint32_t)-1)
    {
        std::cerr << "错误：文件 " << name << " 不存在！" << std::endl;
        return false;
    }
    for (uint32_t i = 0; i < entryCount; ++i)
    {
        uint32_t ptrIdx = (i * DIR_ENTRY_SIZE) / BLOCK_SIZE;
        uint32_t offset = (i * DIR_ENTRY_SIZE) % BLOCK_SIZE;
        char buffer[BLOCK_SIZE];
        disk->ReadBlock(parentNode.direct_ptr[ptrIdx], buffer);
        DirEntry *de = reinterpret_cast<DirEntry *>(buffer + offset);
        if (name == de->name)
        {
            targetEntryIdx = i;
            break;
        }
    }
    // 2. 释放文件占用的磁盘资源
    Inode fileNode;
    if (disk->ReadInode(fileInodeId, fileNode))
    {
        // 释放该文件占用的所有物理块 (直接索引部分)
        for (int i = 0; i < (int)fileNode.block_count; ++i)
        {
            if (fileNode.direct_ptr[i] != 0)
                disk->FreeBlock(fileNode.direct_ptr[i]);
        }
        // 释放 Inode 编号
        disk->FreeInode(fileInodeId);
    }
    // 3. 清理目录项（覆盖法维持目录紧凑）
    uint32_t lastIdx = entryCount - 1; // 最后一个目录项的索引
    // 如果删除的不是最后一个，需要用最后一个来填补坑位
    if (targetEntryIdx != (int)lastIdx)
    {
        // 读取最后一个目录项
        char lastBlockBuf[BLOCK_SIZE];
        uint32_t lastPtrIdx = (lastIdx * DIR_ENTRY_SIZE) / BLOCK_SIZE;
        uint32_t lastOffset = (lastIdx * DIR_ENTRY_SIZE) % BLOCK_SIZE;
        disk->ReadBlock(parentNode.direct_ptr[lastPtrIdx], lastBlockBuf);
        DirEntry *lastEntry = reinterpret_cast<DirEntry *>(lastBlockBuf + lastOffset);
        // 复制最后一个条目到目标位置（被删条目位置）
        char targetBlockBuf[BLOCK_SIZE];
        uint32_t targetPtrIdx = (targetEntryIdx * DIR_ENTRY_SIZE) / BLOCK_SIZE;
        uint32_t targetOffset = (targetEntryIdx * DIR_ENTRY_SIZE) % BLOCK_SIZE;
        disk->ReadBlock(parentNode.direct_ptr[targetPtrIdx], targetBlockBuf);
        DirEntry *targetEntry = reinterpret_cast<DirEntry *>(targetBlockBuf + targetOffset);
        memcpy(targetEntry, lastEntry, sizeof(DirEntry));
        // 写回目标块
        disk->WriteBlock(parentNode.direct_ptr[targetPtrIdx], targetBlockBuf);
    }
    // 4. 更新父目录元数据
    parentNode.size -= DIR_ENTRY_SIZE;
    // 如果 size 刚好退回到块边界，可以考虑减少 block_count
    if (parentNode.size > 0 && parentNode.size % BLOCK_SIZE == 0 && parentNode.block_count > 1)
    {
        uint32_t emptyBlock = parentNode.direct_ptr[parentNode.block_count - 1];
        disk->FreeBlock(emptyBlock);
        parentNode.block_count--;
    }
    // 写回父目录元数据
    disk->WriteInode(currentInodeId, parentNode);
    return true;
}

// 获取当前所在目录的 Inode 编号
uint32_t FileManager::GetCurrentInodeId()
{
    return currentInodeId;
}

// 创建目录
bool FileManager::MakeDirectory(const std::string &name, uint32_t customPerm)
{
    // 1. 权限校验
    // 硬拦截访客:如果当前组是 GID_GUEST，直接拒绝
    if (ctx->currentUser.groupId == GID_GUEST)
    {
        std::cout << "权限拒绝：访客组用户禁止创建目录!" << std::endl;
        return false;
    }
    // 目录权限校验:在父目录创建子目录，需要对父目录有写权限(W=2)
    if (!HasPermission(currentInodeId, PERM_W, ctx->currentUser))
    {
        std::cout << "权限拒绝：你没有在当前目录下创建条目的权限!" << std::endl;
        return false;
    }
    // 2. 分配 Inode
    uint32_t newDirInodeId = disk->AllocateInode();
    if (newDirInodeId == (uint32_t)-1)
        return false;
    // 3. 分配第一个数据块
    uint32_t newDirBlockId = disk->AllocateBlock();
    if (newDirBlockId == (uint32_t)-1)
        return false;
    // 4. 初始化 Inode
    uint32_t perm = (customPerm == 0) ? ROOT_DIR_MODE : (customPerm & PERM_MASK);
    uint32_t mode = (TYPE_DIR << 9) | perm;
    if (!disk->InitInode(newDirInodeId, mode, newDirBlockId, (uint32_t)ctx->currentUser.userId, (uint32_t)ctx->currentUser.groupId))
        return false;
    // 5. 初始化目录项 (. 和 ..)
    char buffer[BLOCK_SIZE] = {0};
    DirEntry *entries = reinterpret_cast<DirEntry *>(buffer);
    strncpy(entries[0].name, ".", 27);
    entries[0].inode_id = newDirInodeId;
    strncpy(entries[1].name, "..", 27);
    entries[1].inode_id = currentInodeId; // 指向当前父目录
    disk->WriteBlock(newDirBlockId, buffer);
    // 6. 设置 Inode 大小并写回
    Inode node;
    disk->ReadInode(newDirInodeId, node);
    node.size = 2 * sizeof(DirEntry);
    disk->WriteInode(newDirInodeId, node);
    // 7. 在父目录中添加该目录项
    return dir->AddDirEntry(currentInodeId, name, newDirInodeId);
}

// 切换当前工作目录
bool FileManager::ChangeDirectory(const std::string &path)
{
    // 1. 在当前目录下查找目标路径对应的 Inode ID
    uint32_t targetInodeId = dir->FindInodeId(path, currentInodeId);
    if (targetInodeId == (uint32_t)-1)
    {
        std::cerr << "错误：路径 '" << path << "' 不存在！" << std::endl;
        return false;
    }
    // 2. 获取该 Inode 的元数据，校验是否为目录
    Inode targetNode;
    if (!disk->ReadInode(targetInodeId, targetNode))
        return false;
    // fileType 为 2 代表目录，1 代表普通文件
    uint32_t fileType = targetNode.mode >> 9;
    // 提取权限位:使用掩码
    uint32_t filePerm = targetNode.mode & PERM_MASK;
    if (fileType != 2)
    {
        std::cerr << "错误：'" << path << "' 不是一个目录！" << std::endl;
        return false;
    }
    // 需要校验进入目录的权限
    if (!HasPermission(targetInodeId, PERM_X, ctx->currentUser))
    {
        std::cerr << "错误：权限不足，无法进入目录 '" << path << "'!" << std::endl;
        return false;
    }
    // 3. 更新当前路径指针
    currentInodeId = targetInodeId;
    return true;
}

// 获取当前工作目录的绝对路径字符串
std::string FileManager::GetAbsolutePath()
{
    // 如果已经在根目录，直接返回 "/"
    if (currentInodeId == 0)
        return "/";
    std::vector<std::string> pathComponents;
    uint32_t tempInodeId = currentInodeId;
    // 向上追溯直到根目录 (Inode 0)
    while (tempInodeId != 0)
    {
        // 1. 获取当前目录的父目录 ID (通过读取当前目录下的 ".." 条目)
        uint32_t parentInodeId = dir->FindInodeId("..", tempInodeId);
        if (parentInodeId == (uint32_t)-1)
            break;
        // 2. 在父目录中查找 tempInodeId 对应的名字
        // 我们需要遍历父目录的所有目录项
        std::vector<DirEntry> parentEntries = dir->ListDirectory(parentInodeId);
        bool found = false;
        for (const auto &entry : parentEntries)
            if (entry.inode_id == tempInodeId)
            {
                pathComponents.push_back(entry.name);
                found = true;
                break;
            }
        if (!found)
            break;
        // 3. 移动到父目录，继续向上找
        tempInodeId = parentInodeId;
    }
    // 4. 将路径组件反向拼接
    std::string fullPath = "";
    for (int i = pathComponents.size() - 1; i >= 0; --i)
        fullPath += "/" + pathComponents[i];
    return fullPath;
}

// 创建文件
bool FileManager::TouchFile(const std::string &name, uint32_t customPerm)
{
    // 1. 权限预检
    // 访客拦截
    if (ctx->currentUser.groupId == GID_GUEST)
    {
        std::cout << "权限拒绝：访客账户无法执行 touch 操作!" << std::endl;
        return false;
    }
    // 检查对当前目录的写权限
    if (!HasPermission(currentInodeId, PERM_W, ctx->currentUser))
    {
        std::cout << "权限拒绝：您没有当前目录的写权限!" << std::endl;
        return false;
    }
    // 2. 在当前目录下查找该文件是否已存在
    uint32_t existingInodeId = dir->FindInodeId(name, currentInodeId);
    if (existingInodeId != (uint32_t)-1)
        return false; 
    else
        return CreateFile(name, customPerm); // 3. 如果文件不存在：直接创建新文件
}

// 向文件内写入内容
bool FileManager::WriteFile(const std::string &name, const std::string &content)
{
    // 1. 查找文件 Inode
    uint32_t inodeId = dir->FindInodeId(name, currentInodeId);
    if (inodeId == (uint32_t)-1)
    {
        std::cerr << "错误：文件不存在！" << std::endl;
        return false;
    }
    Inode node;
    disk->ReadInode(inodeId, node);
    if (node.mode != 1)
    { // 确认是文件而非目录
        std::cerr << "错误：不能向目录写入内容！" << std::endl;
        return false;
    }
    // 2. 清理旧块 (假设 write 是覆盖式写入)
    for (int i = 0; i < 10; ++i)
    {
        if (node.direct_ptr[i] != 0)
        {
            disk->FreeBlock(node.direct_ptr[i]);
            node.direct_ptr[i] = 0;
        }
    }
    // 3. 计算所需块数并分配
    size_t contentLen = content.length();
    uint32_t numBlocks = (contentLen + BLOCK_SIZE - 1) / BLOCK_SIZE;
    if (numBlocks > 10)
    {
        std::cerr << "错误：内容过大，超出直接索引限制！" << std::endl;
        return false;
    }
    // 4. 写入数据
    for (uint32_t i = 0; i < numBlocks; ++i)
    {
        uint32_t newBlockId = disk->AllocateBlock();
        if (newBlockId == (uint32_t)-1)
        {
            std::cerr << "错误：磁盘空间不足！" << std::endl;
            return false;
        }
        node.direct_ptr[i] = newBlockId;
        // 填充缓冲区并写入
        char buffer[BLOCK_SIZE] = {0};
        size_t offset = i * BLOCK_SIZE;
        size_t toWrite = std::min((size_t)BLOCK_SIZE, contentLen - offset);
        memcpy(buffer, content.c_str() + offset, toWrite);
        disk->WriteBlock(newBlockId, buffer);
    }
    // 5. 更新 Inode 元数据
    node.size = contentLen;
    return disk->WriteInode(inodeId, node);
}

// 读取文件内容并返回字符串
std::string FileManager::ReadFile(const std::string &name)
{
    // 1. 在当前目录下查找文件 Inode ID
    uint32_t inodeId = dir->FindInodeId(name, currentInodeId);
    if (inodeId == (uint32_t)-1)
        return "错误：文件不存在!";
    // 2. 读取 Inode 元数据
    Inode node;
    if (!disk->ReadInode(inodeId, node))
        return "错误：无法读取 Inode!";
    // 3. 根据 size 循环读取物理块
    std::string result = "";
    uint32_t remainingSize = node.size;
    char buffer[BLOCK_SIZE];
    for (int i = 0; i < 10 && remainingSize > 0; ++i)
    {
        uint32_t physBlockId = node.direct_ptr[i];
        if (physBlockId == 0)
            break; // 安全保护：不应该出现的空指针
        disk->ReadBlock(physBlockId, buffer);
        // 计算当前块中实际有效的数据长度
        uint32_t bytesToRead = std::min((uint32_t)BLOCK_SIZE, remainingSize);
        result.append(buffer, bytesToRead);
        remainingSize -= bytesToRead;
    }
    return result;
}

// 判断用户是否具有指定权限
bool FileManager::HasPermission(uint32_t inodeId, int requiredPerm, const User &user)
{
    // 1. Root 用户 (UID 0) 拥有绝对权限
    if (user.groupId == GID_ROOT)
        return true;
    Inode node;
    if (!disk->ReadInode(inodeId, node))
        return false;
    // 2. 提取权限位
    uint32_t mode = node.mode & PERM_MASK;
    uint32_t ownerPerm = (mode >> 6) & 07;
    uint32_t groupPerm = (mode >> 3) & 07;
    uint32_t otherPerm = mode & 07;
    // 3. 判定身份并校验
    if (node.owner_id == user.userId)
        return (ownerPerm & requiredPerm) == requiredPerm;
    else if (node.group_id == user.groupId)
        return (groupPerm & requiredPerm) == requiredPerm;
    else
        return (otherPerm & requiredPerm) == requiredPerm;
}