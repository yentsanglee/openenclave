// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _GNU_SOURCE
#include "fs.h"
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include "oefs.h"

#define MAX_MOUNTS 64

typedef struct _binding
{
    fs_t* fs;
    char path[FS_PATH_MAX];
} binding_t;

static binding_t _bindings[MAX_MOUNTS];
static size_t _num_bindings;
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

    /* If this is the root directory. */
    if (buf[1] == '\0')
    {
        ret = true;
        goto done;
    }

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

int fs_bind(fs_t* fs, const char* path)
{
    int ret = -1;
    binding_t binding;

    if (!fs || !path || !_is_path_normalized(path))
        goto done;

    pthread_spin_lock(&_lock);
    {
        if (_num_bindings == MAX_MOUNTS)
            goto done;

        /* Check whether path is already in use. */
        for (size_t i = 0; i < _num_bindings; i++)
        {
            if (strcmp(_bindings[i].path, path) == 0)
            {
                pthread_spin_unlock(&_lock);
                goto done;
            }
        }

        /* Add the new binding. */
        strlcpy(binding.path, path, FS_PATH_MAX);
        binding.fs = fs;
        _bindings[_num_bindings++] = binding;
    }
    pthread_spin_unlock(&_lock);

    ret = 0;

done:

    return ret;
}

int fs_unbind(const char* path)
{
    int ret = -1;
    fs_t* fs = NULL;

    if (!path)
        goto done;

    pthread_spin_lock(&_lock);
    {
        /* Find the binding for this path. */
        for (size_t i = 0; i < _num_bindings; i++)
        {
            if (strcmp(_bindings[i].path, path) == 0)
            {
                fs = _bindings[i].fs;
                _bindings[i] = _bindings[_num_bindings - 1];
                _num_bindings--;
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

    if (!path)
        goto done;

    pthread_spin_lock(&_lock);
    {
        /* Find the longest binding point that contains this path. */
        for (size_t i = 0; i < _num_bindings; i++)
        {
            size_t len = strlen(_bindings[i].path);

            if (strncmp(_bindings[i].path, path, len) == 0 &&
                (path[len] == '/' || path[len] == '\0'))
            {
                if (len > match_len)
                {
                    if (suffix)
                        strlcpy(suffix, path + len, FS_PATH_MAX);

                    match_len = len;
                    ret = _bindings[i].fs;
                }
            }
        }
    }
    pthread_spin_unlock(&_lock);

done:

    return ret;
}
