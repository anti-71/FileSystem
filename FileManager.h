#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include "DiskManager.h"
#include "DirectoryManager.h"
#include <string>
#include <vector>

class FileManager {
public:
    // --- 核心操作 ---
    bool create(DiskManager& dm, DirectoryManager& dirM, SystemContext& ctx, std::string fileName);
    int open(DiskManager& dm, DirectoryManager& dirM, SystemContext& ctx, std::string path, AccessMode mode);
    void close(SystemContext& ctx, int fd);
    
    // --- 读写操作 ---
    int write(DiskManager& dm, SystemContext& ctx, int fd, const std::string& content);
    std::string read(DiskManager& dm, SystemContext& ctx, int fd, int length);
    
    // --- 辅助命令 ---
    void cat(DiskManager& dm, SystemContext& ctx, int fd);
    bool rm(DiskManager& dm, DirectoryManager& dirM, SystemContext& ctx, std::string fileName);

private:
    // 内部辅助：通过 FD 获取描述符对象
    FileDescriptor* getDescriptor(SystemContext& ctx, int fd);
};

#endif