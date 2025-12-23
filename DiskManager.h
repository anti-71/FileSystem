#ifndef DISK_MANAGER_H
#define DISK_MANAGER_H

#include "FileSystem.h"
#include <string>

class DiskManager {
private:
    VirtualDisk* vDisk;

public:
    DiskManager();
    ~DiskManager();

    // 磁盘操作
    void formatDisk();      // 格式化磁盘，初始化超级块和根目录
    bool saveToDisk();      // 将内存数据持久化到二进制文件
    bool loadFromDisk();    // 从二进制文件加载数据到内存

    // 获取磁盘对象指针
    VirtualDisk* getDiskPtr() const;
    
    // 辅助工具
    void showDiskUsage() const;
    
    // --- Inode 管理 ---
    int allocateInode();                // 分配一个空闲 Inode，返回编号，-1 表示满
    void freeInode(int inodeId);        // 释放指定 Inode
    Inode* getInode(int inodeId);       // 根据编号获取 Inode 指针

    // --- 数据块管理 ---
    int allocateDataBlock();            // 分配一个空闲数据块，返回编号
    void freeDataBlock(int blockId);    // 释放指定数据块
    DataBlock* getDataBlock(int blockId); // 获取数据块指针

    // --- 索引管理 ---
    // 为指定 Inode 增加一个数据块（处理直接索引和一级间接索引逻辑）
    bool addBlockToInode(int inodeId, int blockId);

    bool saveUsers(const UserList& uList);
    void logAction(const std::string& username, const std::string& command, const std::string& result);

    // 建议把加载也加上
    bool loadUsers(UserList& uList);
};

#endif