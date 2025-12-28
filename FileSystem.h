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
#include <bitset>
#include <iomanip>

// --- 基础常量定义 ---
#define BLOCK_SIZE 512
#define TOTAL_BLOCKS 32768
#define BITMAP_SIZE 8
#define INODES_PER_BLOCK 4
#define DIR_ENTRY_SIZE 32

const uint32_t INODE_BITMAP_BYTES = 512;
const uint32_t INODE_BITMAP_START_BYTE = 1;
const std::string VDISK_PATH = "vdisk.img";

// 用户结构体
struct User
{
    int userId;
};

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

// Inode 结构：占用 1 个块 (128B)，实际只用了前面一部分
struct Inode
{
    uint32_t inode_id;       // Inode 编号
    uint32_t mode;           // 模式：0 代表空闲，1 代表文件，2 代表目录
    uint32_t size;           // 文件大小（字节）
    uint32_t block_count;    // 已占用的数据块数量
    uint32_t direct_ptr[10]; // 直接索引：记录该文件占用的物理块号
    char padding[68];        // 填充至 128 字节
};

// 目录项结构：正好 32 字节，一块 (512B) 可存 16 个
struct DirEntry
{
    char name[28];     // 文件名
    uint32_t inode_id; // 对应 Inode 编号
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