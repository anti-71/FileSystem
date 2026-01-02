#ifndef DISK_MANAGER_H
#define DISK_MANAGER_H

#include "FileSystem.h"

class DiskManager
{
private:
    std::fstream disk;
    SuperBlock sb;               // 常驻内存的超级块
    std::vector<uint8_t> bitmap; // 常驻内存的位图 (4096 字节)
    std::string path;            // 虚拟磁盘的路径

public:
    DiskManager(const std::string &vdisk_path);
    ~DiskManager();

    bool FileExists(const std::string &path);
    bool InitializeDisk(const std::string &path);

    bool Mount();
    void UnMount();
    bool ReadBlock(uint32_t block_id, char *buffer);
    bool WriteBlock(uint32_t block_id, char *buffer);

    int AllocateBlock();
    bool FreeBlock(uint32_t block_id);

    bool ReadInode(uint32_t inode_id, Inode &node);
    bool WriteInode(uint32_t inode_id, const Inode &node);
    int AllocateInode();
    bool InitInode(uint32_t inode_id, uint32_t mode, uint32_t block_id, uint32_t uid, uint32_t gid);
    bool FreeInode(uint32_t inode_id);

    void DumpBitmapOccupiedPart();
};
#endif