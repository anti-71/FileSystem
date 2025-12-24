#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <ctime>
#include <windows.h>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <cstdint>
#include <sys/stat.h>

// --- 基础常量定义 ---
#define BLOCK_SIZE 512
#define TOTAL_BLOCKS 32768
#define BITMAP_SIZE 8
#define INODES_PER_BLOCK 4

const std::string VDISK_PATH = "vdisk.img";

// ================= 用户相关结构体 =================

struct User
{
    int userId;
};

// ================= 文件系统元数据结构体 =================

// 超级块结构：占用 1 个块 (512B)，实际只用了前面一部分
struct SuperBlock
{
    uint32_t total_blocks; // 总块数
    uint32_t free_blocks;  // 空闲块数

    uint32_t bitmap_start; // 位图区起始块号
    uint32_t inode_start;  // Inode区起始块号
    uint32_t data_start;   // 数据区起始块号

    char padding[488]; // 填充至 512 字节
};

struct Inode
{
    uint32_t inode_id;       // Inode 编号
    uint32_t mode;           // 模式：0 代表空闲，1 代表文件，2 代表目录
    uint32_t size;           // 文件大小（字节）
    uint32_t block_count;    // 已占用的数据块数量
    uint32_t direct_ptr[10]; // 直接索引：记录该文件占用的物理块号
    char padding[68];        // 填充至 128 字节
};

// ================= 树状目录支撑结构体 =================

struct DirEntry
{
    std::string fileName; // 文件名
    int inodeId;          // 对应 inode 编号
    int parentInodeId;    // 父目录 inode 编号
    DirEntry *next;       // 链表形式维护同一目录下的项
};

struct CurrentDir
{
    std::string path;   // 工作目录路径，如 "/home/user"
    int currentInodeId; // 当前目录的 inode 编号
};

// ================= 文件操作与保护结构体 =================

enum AccessMode
{
    READ,
    WRITE,
    READ_WRITE
};

struct FileDescriptor
{
    int fdId;        // 文件描述符 ID
    int inodeId;     // 指向的 inode 编号
    AccessMode mode; // 访问模式
    long offset;     // 当前读写偏移量
    int ownerUserId; // 打开该文件的用户 ID
};

enum LockType
{
    READ_LOCK,
    WRITE_LOCK
};

struct FileLock
{
    int inodeId;
    LockType lockType;
    std::vector<int> holderUserIds; // 持有锁的用户列表
    int lockCount;                  // 锁计数
};

// ================= 存储相关结构体 =================

struct DataBlock
{
    char data[BLOCK_SIZE]; // 实际存储内容的字节数组
};

// 每个目录项固定 64 字节，一个 1KB 的块可以存 16 个记录
struct DirRecord
{
    char name[56]; // 文件名
    int inodeId;   // Inode 编号
    int isUsed;    // 0: 空闲，1: 已使用
};

// 系统内容结构体
struct SystemContext
{
    std::vector<User> uList; // 用户列表
    User currentUser;        // 当前用户
    // CurrentDir currentDir;                 // 当前目录
    // std::vector<FileDescriptor> openFiles; // 打开的文件列表
};

#endif