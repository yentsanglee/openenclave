#define _GNU_SOURCE
#include "filesys.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ENTRIES 64

typedef struct _entry
{
    /* Path on which the file system is mounted. */
    char path[PATH_MAX];

    /* The file system object */
    oe_filesys_t* filesys;
} entry_t;

static entry_t _entries[MAX_ENTRIES];
static size_t _num_entries;
static pthread_mutex_t _lock = PTHREAD_MUTEX_INITIALIZER;

/* Check that the path is a canonical path (see notes in function). */
static bool _is_canonical_path(const char* path)
{
    bool ret = false;
    char buf[PATH_MAX];
    char* p;
    char* save;

    if (!path || strlen(path) >= PATH_MAX)
        goto done;

    strlcpy(buf, path, PATH_MAX);

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

int oe_filesys_mount(oe_filesys_t* filesys, const char* path)
{
    int rc = 1;
    bool locked = false;

    if (!path || !_is_canonical_path(path) || !filesys)
        goto done;

    pthread_mutex_lock(&_lock);
    locked = true;

    if (_num_entries == MAX_ENTRIES)
        goto done;

    /* Reject if path is already used. */
    for (size_t i = 0; i < _num_entries; i++)
    {
        if (strcmp(_entries[i].path, path) == 0)
            goto done;
    }

    /* Inject the entry. */
    {
        entry_t entry;
        strlcpy(entry.path, path, PATH_MAX);
        entry.filesys = filesys;
        _entries[_num_entries++] = entry;
    }

    rc = 0;

done:

    if (locked)
        pthread_mutex_unlock(&_lock);

    return rc;
}

oe_filesys_t* oe_filesys_lookup(const char* path, char path_out[PATH_MAX])
{
    oe_filesys_t* ret = NULL;
    size_t match_len = 0;
    bool locked = false;

    if (!path || !path_out || !_is_canonical_path(path))
        goto done;

    pthread_mutex_lock(&_lock);
    locked = true;

    /* Find the longest mount point that contains this path. */
    for (size_t i = 0; i < _num_entries; i++)
    {
        size_t path_len = strlen(_entries[i].path);

        if (strncmp(_entries[i].path, path, path_len) == 0 &&
            (path[path_len] == '/' || path[path_len] == '\0'))
        {
            if (path_len > match_len)
            {
                strlcpy(path_out, path + path_len, PATH_MAX);
                match_len = path_len;
                ret = _entries[i].filesys;
            }
        }
    }

done:

    if (locked)
        pthread_mutex_unlock(&_lock);

    return ret;
}
