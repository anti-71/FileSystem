#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <iostream>
#include <vector>
#include <string>
#include <ctime>
#include <windows.h>

// --- 基础常量定义 ---
const int BLOCK_SIZE = 1024;       // 数据块大小 1KB
const int MAX_INODES = 1024;       // 最大 Inode 数量
const int MAX_DATA_BLOCKS = 4096;  // 最大数据块数量
const int DIRECT_INDEX_COUNT = 10; // 直接索引数量
const std::string DISK_PATH = "virtual_disk.img";
const std::string USER_DATA_PATH = "user_data.dat";

// 权限位定义 (类 Linux)
const int S_IRUSR = 0400; // 所有者读
const int S_IWUSR = 0200; // 所有者写
const int S_IXUSR = 0100; // 所有者执行
const int S_IRGRP = 0040; // 组读
const int S_IWGRP = 0020; // 组写
const int S_IXGRP = 0010; // 组执行
const int S_IROTH = 0004; // 其他人读
const int S_IWOTH = 0002; // 其他人写
const int S_IXOTH = 0001; // 其他人执行

// ================= 用户相关结构体 =================

enum UserType { ADMIN, NORMAL_USER };

struct User {
    int userId;
    std::string username;
    std::string password;      // 存储简单加密后的密文
    UserType type;
    std::string userGroup;
};

struct UserList {
    std::vector<User> users;   // 使用 vector 方便增删查
    // 后续在此添加：addUser(), deleteUser(), findUser()
};

// ================= 文件系统元数据结构体 =================

struct SuperBlock {
    long totalDiskSize;        // 虚拟磁盘总大小
    int blockSize;             // 块大小
    int inodeTotal;            // inode 总数
    int inodeFree;             // 空闲 inode 数
    int dataBlockTotal;        // 数据块总数
    int dataBlockFree;         // 空闲数据块总数
    int rootInodeId;           // 根目录 inode 编号
    
    bool inodeBitmap[MAX_INODES];       // inode 位图
    bool dataBlockBitmap[MAX_DATA_BLOCKS]; // 数据块位图
};

enum FileType { F_FILE, F_DIRECTORY };

struct Inode {
    int inodeId;               // 唯一标识
    int ownerId;               // 所有者 ID
    int groupId;               // 所属组 ID
    int permissions;           // 权限位，如 0755
    FileType type;             // 文件或目录
    long fileSize;             // 文件大小
    time_t createdAt;          // 创建时间
    time_t modifiedAt;         // 修改时间
    time_t accessedAt;         // 访问时间
    
    int directIndices[DIRECT_INDEX_COUNT]; // 直接索引
    int firstIndirectIndex;                // 一级间接索引
    int secondIndirectIndex;               // 二级间接索引
    int linkCount;                         // 引用计数
};

// ================= 树状目录支撑结构体 =================

struct DirEntry {
    std::string fileName;      // 文件名
    int inodeId;               // 对应 inode 编号
    int parentInodeId;         // 父目录 inode 编号
    DirEntry* next;            // 链表形式维护同一目录下的项
};

struct CurrentDir {
    std::string path;          // 工作目录路径，如 "/home/user"
    int currentInodeId;        // 当前目录的 inode 编号
};

// ================= 文件操作与保护结构体 =================

enum AccessMode { READ, WRITE, READ_WRITE };

struct FileDescriptor {
    int fdId;                  // 文件描述符 ID
    int inodeId;               // 指向的 inode 编号
    AccessMode mode;           // 访问模式
    long offset;               // 当前读写偏移量
    int ownerUserId;           // 打开该文件的用户 ID
};

enum LockType { READ_LOCK, WRITE_LOCK };

struct FileLock {
    int inodeId;
    LockType lockType;
    std::vector<int> holderUserIds; // 持有锁的用户列表
    int lockCount;                  // 锁计数
};

// ================= 存储相关结构体 =================

struct DataBlock {
    char data[BLOCK_SIZE];     // 实际存储内容的字节数组
};

struct VirtualDisk {
    SuperBlock sBlock;
    Inode inodes[MAX_INODES];
    DataBlock blocks[MAX_DATA_BLOCKS];
    
    // 该结构体可以整体持久化到本地二进制文件
};

struct SystemContext {
    UserList uList;
    CurrentDir currentDir;
    std::vector<FileDescriptor> openFiles;
    User currentUser;
};

// 每个目录项固定 64 字节，一个 1KB 的块可以存 16 个记录
struct DirRecord {
    char name[56];      // 文件名
    int inodeId;        // Inode 编号
    int isUsed;         // 0: 空闲，1: 已使用
};

#endif