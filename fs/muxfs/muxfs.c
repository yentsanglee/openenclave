#include <errno.h>
#include <limits.h>
#include <openenclave/bits/properties.h>
#include <openenclave/bits/fs.h>
#include <openenclave/internal/hostfs.h>
#include <openenclave/internal/defs.h>
#include <openenclave/internal/fs.h>
#include <openenclave/internal/muxfs.h>
#include <openenclave/internal/sgxfs.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#define MAGIC 0x114c4d216581d07c

#define MAX_ENTRIES 16

typedef struct _entry
{
    char path[PATH_MAX];
    oe_fs_t* fs;
} entry_t;

/* Global spinlock across all instance of muxfs. */
static pthread_spinlock_t _lock;

/* This implementation overlays oe_fs_t. */
typedef struct _fs_impl
{
    oe_fs_base_t base;
    uint64_t magic;
    size_t num_entries;
    size_t max_entries;
    entry_t* entries;
    uint64_t padding[10];
} fs_impl_t;

OE_STATIC_ASSERT(sizeof(fs_impl_t) == sizeof(oe_fs_t));

OE_INLINE fs_impl_t* _get_impl(oe_fs_t* fs)
{
    fs_impl_t* impl = (fs_impl_t*)fs->__impl;

    if (impl->magic != MAGIC)
        return NULL;

    return impl;
}

static oe_fs_t* _find_fs(oe_fs_t* fs, const char* path, char suffix[PATH_MAX])
{
    oe_fs_t* ret = NULL;
    fs_impl_t* impl = _get_impl(fs);
    size_t match_len = 0;
    bool locked = false;

    if (!impl || !path || !suffix)
        goto done;

    pthread_spin_lock(&_lock);
    locked = true;

    /* Find the longest target that contains this path. */
    for (size_t i = 0; i < impl->num_entries; i++)
    {
        entry_t* entry = &impl->entries[i];
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

    ret = oe_fs_ft(fs)->fs_fopen(fs, suffix, mode, ap);

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

    ret = oe_fs_ft(fs)->fs_opendir(fs, suffix);

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

    ret = oe_fs_ft(fs)->fs_stat(fs, suffix, stat);

done:

    return ret;
}

static int _fs_rename(
    oe_fs_t* muxfs,
    const char* old_path,
    const char* new_path)
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

    ret = oe_fs_ft(old_fs)->fs_rename(old_fs, old_suffix, new_suffix);

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

    ret = oe_fs_ft(fs)->fs_remove(fs, suffix);

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

    ret = oe_fs_ft(fs)->fs_mkdir(fs, suffix, mode);

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

    ret = oe_fs_ft(fs)->fs_rmdir(fs, suffix);

done:

    return ret;
}

static entry_t _entries[MAX_ENTRIES] = {
    {
        "/hostfs",
        &oe_hostfs,
    },
    {
        "/sgxfs",
        &oe_sgxfs,
    },
};

static const size_t _num_entries = 2;

static oe_fs_ft_t _ft = {
    .fs_release = _fs_release,
    .fs_fopen = _fs_fopen,
    .fs_opendir = _fs_opendir,
    .fs_stat = _fs_stat,
    .fs_remove = _fs_remove,
    .fs_rename = _fs_rename,
    .fs_mkdir = _fs_mkdir,
    .fs_rmdir = _fs_rmdir,
};

oe_fs_t oe_muxfs = {
    (uint64_t)OE_FS_MAGIC,
    (uint64_t)&_ft,
    MAGIC,
    _num_entries,
    OE_COUNTOF(_entries),
    (uint64_t)_entries,
};

int oe_muxfs_register_fs(oe_fs_t* muxfs, const char* path, oe_fs_t* fs)
{
    int ret = -1;
    fs_impl_t* impl = _get_impl(muxfs);
    bool locked = false;
    entry_t* entry;

    if (!impl || !path || !fs)
        goto done;

    pthread_spin_lock(&_lock);
    locked = true;

    if (impl->num_entries == impl->max_entries)
        goto done;

    /* Is this path already registered? */
    for (size_t i = 0; i < impl->num_entries; i++)
    {
        if (strcmp(impl->entries[i].path, path) == 0)
            goto done;
    }

    entry = &impl->entries[impl->num_entries];

    if (strlcpy(entry->path, path, sizeof(entry->path)) >= sizeof(entry->path))
        goto done;

    entry->fs = fs;
    impl->num_entries++;

    ret = 0;

done:

    if (locked)
        pthread_spin_unlock(&_lock);

    return ret;
}

int oe_muxfs_unregister_fs(oe_fs_t* muxfs, const char* path)
{
    int ret = -1;
    fs_impl_t* impl = _get_impl(muxfs);
    bool locked = false;
    bool found = false;

    if (!impl || !path)
        goto done;

    pthread_spin_lock(&_lock);
    locked = true;

    for (size_t i = 0; i < impl->num_entries; i++)
    {
        if (strcmp(impl->entries[i].path, path) == 0)
        {
            /* Remove this entry by exchanging it with the last entry. */
            impl->entries[i] = impl->entries[impl->num_entries - 1];
            impl->num_entries--;
            found = true;
            break;
        }
    }

    if (!found)
        goto done;

    ret = 0;

done:

    if (locked)
        pthread_spin_unlock(&_lock);

    return ret;
}
