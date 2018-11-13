#include "mount.h"
#include "fs.h"

int fs_mount_hostfs(const char* target)
{
    int ret = -1;
    fs_t* fs = NULL;
    
    if (!target)
        goto done;

    if (fs_hostfs_new(&fs) != 0)
        goto done;

    if (fs_mount(fs, target) != 0)
        goto done;

    ret = 0;

done:

    if (fs)
        fs->fs_release(fs);

    return ret;
}

int fs_mount_ramfs(const char* target, uint32_t flags, size_t nblks)
{
    int ret = -1;
    fs_t* fs = NULL;
    
    if (!target)
        goto done;

    if (fs_ramfs_new(&fs, flags, nblks) != 0)
        goto done;

    if (fs_mount(fs, target) != 0)
        goto done;

    ret = 0;

done:

    if (fs)
        fs->fs_release(fs);

    return ret;
}

int fs_mount_oefs(
    const char* source,
    const char* target,
    uint32_t flags,
    size_t nblks,
    const uint8_t key[FS_KEY_SIZE])
{
    int ret = -1;
    fs_t* fs = NULL;
    
    if (!target)
        goto done;

    if (fs_oefs_new(&fs, source, flags, nblks, key) != 0)
        goto done;

    if (fs_mount(fs, target) != 0)
        goto done;

    ret = 0;

done:

    if (fs)
        fs->fs_release(fs);

    return ret;
}
