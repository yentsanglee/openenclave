#include <openenclave/internal/fsinternal.h>
#include <openenclave/internal/defs.h>
#include <openenclave/bits/properties.h>
#include <openenclave/internal/fs.h>
#include <openenclave/internal/muxfs.h>
#include <openenclave/internal/hostfs.h>
#include <openenclave/internal/sgxfs.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#define MAGIC 0x114c4d216581d07c

typedef struct _entry
{
    char path[PATH_MAX];
    oe_fs_t* fs;
}
entry_t;

/* Global spinlock across all instance of muxfs. */
static pthread_spinlock_t _lock;

#define FS_MAGIC(fs)    fs->__impl[0]
#define FS_NENTRIES(fs) fs->__impl[1]
#define FS_ENTRIES(fs)  ((entry_t*)fs->__impl[2])

OE_INLINE bool _valid_muxfs(const oe_fs_t* fs)
{
    return fs && (FS_MAGIC(fs) == MAGIC);
}

static oe_fs_t* _find_fs(oe_fs_t* fs, const char* path, char suffix[PATH_MAX])
{
    oe_fs_t* ret = NULL;
    size_t match_len = 0;
    bool locked = false;

    if (!_valid_muxfs(fs) || !path || !suffix)
        goto done;

    pthread_spin_lock(&_lock);
    locked = true;

    /* Find the longest target that contains this path. */
    for (size_t i = 0; i < FS_NENTRIES(fs); i++)
    {
        entry_t* entry = &FS_ENTRIES(fs)[i];
        size_t len = strlen(entry->path);

        if (strncmp(entry->path, path, len) == 0 &&
            (path[len] == '/' || path[len] == '\0'))
        {
            if (len > match_len)
            {
                if (suffix)
                    strlcpy(suffix, path + len, PATH_MAX);

                match_len = len;
                ret = entry->fs;
            }
        }
    }

done:

    if (locked)
        pthread_spin_unlock(&_lock);

    return ret;
}

static FILE* _fs_fopen(
    oe_fs_t* muxfs,
    const char* path,
    const char* mode,
    va_list ap)
{
    FILE* ret = NULL;
    oe_fs_t* fs;
    char suffix[PATH_MAX];

    if (!(fs = _find_fs(muxfs, path, suffix)))
    {
        errno = EINVAL;
        goto done;
    }

    ret = fs->fs_fopen(fs, suffix, mode, ap);

done:

    return ret;
}

static DIR* _fs_opendir(oe_fs_t* muxfs, const char* name)
{
    DIR* ret = NULL;
    oe_fs_t* fs;
    char suffix[PATH_MAX];

    if (!(fs = _find_fs(muxfs, name, suffix)))
    {
        errno = EINVAL;
        goto done;
    }

    ret = fs->fs_opendir(fs, suffix);

done:

    return ret;
}

static int _fs_release(oe_fs_t* fs)
{
    uint32_t ret = -1;

    if (!fs)
        goto done;

    ret = 0;

done:
    return ret;
}

static int _fs_stat(oe_fs_t* muxfs, const char* path, struct stat* stat)
{
    int ret = -1;
    oe_fs_t* fs;
    char suffix[PATH_MAX];

    if (!(fs = _find_fs(muxfs, path, suffix)))
    {
        errno = EINVAL;
        goto done;
    }

    ret = fs->fs_stat(fs, suffix, stat);

done:

    return ret;
}

static int _fs_rename(
    oe_fs_t* muxfs, const char* old_path, const char* new_path)
{
    int ret = -1;
    oe_fs_t* old_fs;
    oe_fs_t* new_fs;
    char old_suffix[PATH_MAX];
    char new_suffix[PATH_MAX];

    if (!(old_fs = _find_fs(muxfs, old_path, old_suffix)))
    {
        errno = EINVAL;
        goto done;
    }

    if (!(new_fs = _find_fs(muxfs, new_path, new_suffix)))
    {
        errno = EINVAL;
        goto done;
    }

    if (old_fs != new_fs)
    {
        errno = EINVAL;
        goto done;
    }

    ret = old_fs->fs_rename(old_fs, old_suffix, new_suffix);

done:

    return ret;
}

static int _fs_remove(oe_fs_t* muxfs, const char* path)
{
    int ret = -1;
    oe_fs_t* fs;
    char suffix[PATH_MAX];

    if (!(fs = _find_fs(muxfs, path, suffix)))
    {
        errno = EINVAL;
        goto done;
    }

    ret = fs->fs_remove(fs, suffix);

done:

    return ret;
}

static int _fs_mkdir(oe_fs_t* muxfs, const char* path, unsigned int mode)
{
    int ret = -1;
    oe_fs_t* fs;
    char suffix[PATH_MAX];

    if (!(fs = _find_fs(muxfs, path, suffix)))
    {
        errno = EINVAL;
        goto done;
    }

    ret = fs->fs_mkdir(fs, suffix, mode);

done:

    return ret;
}

static int _fs_rmdir(oe_fs_t* muxfs, const char* path)
{
    int ret = -1;
    oe_fs_t* fs;
    char suffix[PATH_MAX];

    if (!(fs = _find_fs(muxfs, path, suffix)))
    {
        errno = EINVAL;
        goto done;
    }

    ret = fs->fs_rmdir(fs, suffix);

done:

    return ret;
}

static entry_t _fs_entries[] =
{
    {
        "/hostfs",
        &oe_hostfs,
    },
    {
        "/sgxfs",
        &oe_sgxfs,
    },
};

// clang-format off
oe_fs_t oe_muxfs = {
    .__impl = 
    { 
        MAGIC,
        OE_COUNTOF(_fs_entries),
        (uint64_t)_fs_entries,
    },
    .fs_magic = OE_FS_MAGIC,
    .fs_release = _fs_release,
    .fs_fopen = _fs_fopen,
    .fs_opendir = _fs_opendir,
    .fs_stat = _fs_stat,
    .fs_remove = _fs_remove,
    .fs_rename = _fs_rename,
    .fs_mkdir = _fs_mkdir,
    .fs_rmdir = _fs_rmdir,
};
// clang-format on
