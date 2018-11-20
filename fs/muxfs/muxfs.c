#include <openenclave/internal/fsinternal.h>
#include <openenclave/internal/fs.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#define MAGIC 0x114c4d21

typedef struct _entry entry_t;

struct _entry
{
    entry_t* next;
    char path[PATH_MAX];
    oe_fs_t* fs;
};

typedef struct _impl
{
    uint64_t magic;
    pthread_spinlock_t lock;
    entry_t* entries;
}
impl_t;

static entry_t* _find_entry(impl_t* impl, const char* path, entry_t** prev)
{
    if (prev)
        *prev = NULL;

    for (entry_t* p = impl->entries; p; p = p->next)
    {
        if (strcmp(p->path, path) == 0)
            return p;

        if (prev)
            *prev = p;
    }

    if (prev)
        *prev = NULL;

    return NULL;
}

static FILE* _fs_fopen(
    oe_fs_t* fs,
    const char* path,
    const char* mode,
    const void* args)
{
    FILE* ret = NULL;

    if (!path || !mode)
        goto done;

done:

    return ret;
}

static DIR* _fs_opendir(oe_fs_t* fs, const char* name, const void* args)
{
    DIR* ret = NULL;

    if (!fs || !name)
        goto done;

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

static int _fs_stat(oe_fs_t* fs, const char* path, struct stat* stat)
{
    int ret = -1;

    if (!fs || !path || !stat)
        goto done;

done:

    return ret;
}

static int _fs_rename(
    oe_fs_t* fs, const char* old_path, const char* new_path)
{
    int ret = -1;

    if (!fs || !old_path || !new_path)
        goto done;

done:

    return ret;
}

static int _fs_remove(oe_fs_t* fs, const char* path)
{
    int ret = -1;

    if (!fs || !path)
        goto done;

done:

    return ret;
}

static int _fs_mkdir(oe_fs_t* fs, const char* path, unsigned int mode)
{
    int ret = -1;

    if (!fs || !path)
        goto done;

done:

    return ret;
}

static int _fs_rmdir(oe_fs_t* fs, const char* path)
{
    int ret = -1;

    if (!fs || !path)
        goto done;

done:

    return ret;
}

oe_fs_t oe_muxfs = {
    .__impl = { MAGIC },
    .fs_release = _fs_release,
    .fs_fopen = _fs_fopen,
    .fs_opendir = _fs_opendir,
    .fs_stat = _fs_stat,
    .fs_remove = _fs_remove,
    .fs_rename = _fs_rename,
    .fs_mkdir = _fs_mkdir,
    .fs_rmdir = _fs_rmdir,
};

int oe_muxfs_register_fs(oe_fs_t* muxfs, const char* path, oe_fs_t* fs)
{
    impl_t* impl = (impl_t*)&muxfs->__impl;
    int ret = -1;
    bool locked = false;
    entry_t* entry = NULL;

    if (!muxfs || !path || !fs || impl->magic != MAGIC)
        goto done;

    pthread_spin_lock(&impl->lock);
    locked = true;

    /* Check for duplicate. */
    if (_find_entry(impl, path, NULL))
        goto done;

    /* Insert a new entry. */
    {
        if (!(entry = calloc(1, sizeof(entry_t))))
            goto done;

        if (strlcpy(
            entry->path, path, sizeof(entry->path)) >= sizeof(entry->path))
        {
            goto done;
        }

        entry->next = impl->entries;
        impl->entries = entry;
        entry = NULL;
    }

    ret = 0;

done:

    if (entry)
        free(entry);

    if (locked)
        pthread_spin_unlock(&impl->lock);

    return ret;
}

int oe_muxfs_unregister_fs(oe_fs_t* muxfs, const char* path)
{
    impl_t* impl = (impl_t*)&muxfs->__impl;
    int ret = -1;
    bool locked = false;
    entry_t* entry = NULL;

    if (!muxfs || path || impl->magic != MAGIC)
        goto done;

    pthread_spin_lock(&impl->lock);
    locked = true;

    /* Remove the entry. */
    {
        entry_t* prev = NULL;

        if (!(entry = _find_entry(impl, path, &prev)))
            goto done;

        if (prev)
            prev->next = entry->next;
        else
            impl->entries = entry->next;
    }

    ret = 0;

done:

    if (entry)
        free(entry);

    if (locked)
        pthread_spin_unlock(&impl->lock);

    return ret;
}
