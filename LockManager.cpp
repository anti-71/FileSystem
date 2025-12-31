#include "LockManager.h"

LockManager::LockManager(DiskManager *dm)
{
    this->disk = dm;
}

// 请求访问权限
bool LockManager::RequestAccess(uint32_t inodeId, bool isWrite)
{
    Inode node;
    disk->ReadInode(inodeId, node); // 1. 从磁盘拿最新的状态
    if (isWrite)
    {
        if (node.is_writing != 0 || node.reader_count > 0)
            return false;
        node.is_writing = 1;
    }
    else
    {
        if (node.is_writing != 0)
            return false;
        node.reader_count++;
    }
    disk->WriteInode(inodeId, node); // 2. 立即同步回磁盘，实现进程间可见
    return true;
}

// 释放访问权限
void LockManager::ReleaseAccess(uint32_t inodeId, bool isWrite)
{
    // 1. 从磁盘读取该 Inode
    Inode node;
    disk->ReadInode(inodeId, node);
    // 2. 更新状态位
    if (isWrite)
        node.is_writing = 0; // 释放写锁
    else
    {
        if (node.reader_count > 0)
            node.reader_count--; // 读者减一
    }
    // 3. 写回磁盘并同步
    disk->WriteInode(inodeId, node);
}