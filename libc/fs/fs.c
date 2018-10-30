#define _GNU_SOURCE
#include "fs.h"
#include "oefs.h"
#include <pthread.h>
#include <stdbool.h>
#include <string.h>

#define MAX_MOUNTS 64

typedef struct _mount
{
    fs_t* fs;
    char path[FS_PATH_MAX];
} mount_t;

static mount_t _mounts[MAX_MOUNTS];
static size_t _num_mounts;
static pthread_spinlock_t _lock;

/* Check that the path is normalized (see notes in function). */
static bool _is_path_normalized(const char* path)
{
    bool ret = false;
    char buf[FS_PATH_MAX];
    char* p;
    char* save;

    if (!path || strlen(path) >= FS_PATH_MAX)
        goto done;

    strlcpy(buf, path, FS_PATH_MAX);

    /* The path must begin with a slash. */
    if (buf[0] != '/')
        goto done;

    for (const char* p = buf; *p; p++)
    {
        /* The last character must not be a slash. */
        if (p[0] == '/' && p[1] == '\0')
            goto done;

        /* The path may not have consecutive slashes. */
        if (p[0] == '/' && p[1] == '/')
            goto done;
    }

    /* The path may not have "." and ".." components. */
    for (p = strtok_r(buf, "/", &save); p; p = strtok_r(NULL, "/", &save))
    {
        if (strcmp(p, ".") == 0 || strcmp(p, "..") == 0)
            goto done;
    }

    ret = true;

done:
    return ret;
}

int fs_mount(fs_t* fs, const char* path)
{
    int ret = -1;
    bool locked = false;
    mount_t mount;

    if (!fs || !path || !_is_path_normalized(path))
        goto done;

    pthread_spin_lock(&_lock);
    locked = true;

    if (_num_mounts == MAX_MOUNTS)
        goto done;

    /* Check whether path is already in use. */
    for (size_t i = 0; i < _num_mounts; i++)
    {
        if (strcmp(_mounts[i].path, path) == 0)
            goto done;
    }

    /* Add the new mount. */
    strlcpy(mount.path, path, FS_PATH_MAX);
    mount.fs = fs;
    _mounts[_num_mounts++] = mount;

    ret = 0;

done:

    if (locked)
        pthread_spin_unlock(&_lock);

    return ret;
}

int fs_unmount(const char* path)
{
    int ret = -1;
    fs_t* fs = NULL;

    if (!path)
        goto done;

    pthread_spin_lock(&_lock);
    {
        /* Find the mount for this path. */
        for (size_t i = 0; i < _num_mounts; i++)
        {
            if (strcmp(_mounts[i].path, path) == 0)
            {
                fs = _mounts[i].fs;
                _mounts[i] = _mounts[_num_mounts-1];
                _num_mounts--;
                break;
            }
        }
    }
    pthread_spin_unlock(&_lock);

    if (!fs)
        goto done;

    fs->fs_release(fs);

    ret = 0;

done:

    return ret;
}

fs_t* fs_lookup(const char* path, char suffix[FS_PATH_MAX])
{
    fs_t* ret = NULL;
    size_t match_len = 0;

    if (!path || !suffix)
        goto done;

    pthread_spin_lock(&_lock);
    {
        /* Find the longest mount point that contains this path. */
        for (size_t i = 0; i < _num_mounts; i++)
        {
            size_t len = strlen(_mounts[i].path);

            if (strncmp(_mounts[i].path, path, len) == 0 &&
                (path[len] == '/' || path[len] == '\0'))
            {
                if (len > match_len)
                {
                    strlcpy(suffix, path + len, FS_PATH_MAX);
                    match_len = len;
                    ret = _mounts[i].fs;
                }
            }
        }
    }
    pthread_spin_unlock(&_lock);

done:


    return ret;
}
