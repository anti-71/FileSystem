#ifndef LOCK_MANAGER_H
#define LOCK_MANAGER_H

#include "FileSystem.h"
#include "DiskManager.h"

class LockManager
{
private:
    DiskManager *disk;

public:
    LockManager(DiskManager *dm);
    bool RequestAccess(uint32_t inodeId, bool isWrite);
    void ReleaseAccess(uint32_t inodeId, bool isWrite);
};

#endif