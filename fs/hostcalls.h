// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _FS_HOSTCALLS_H
#define _FS_HOSTCALLS_H

#include "common.h"
#include "guid.h"

typedef struct _fs_host_calls fs_host_calls_t;

struct _fs_host_calls
{
    void* (*malloc)(fs_host_calls_t* host_calls, size_t size);

    void* (*calloc)(fs_host_calls_t* host_calls, size_t nmemb, size_t size);

    void (*free)(fs_host_calls_t* host_calls, void* ptr);

    int (*call)(
        fs_host_calls_t* host_calls, 
        const fs_guid_t* guid,
        void* args,
        size_t size);
};

extern fs_host_calls_t fs_host_calls;

FS_INLINE void* fs_host_malloc(size_t size)
{
    return fs_host_calls.malloc(&fs_host_calls, size);
}

FS_INLINE void* fs_host_calloc(size_t nmemb, size_t size)
{
    return fs_host_calls.calloc(&fs_host_calls, nmemb, size);
}

FS_INLINE void fs_host_free(void* ptr)
{
    return fs_host_calls.free(&fs_host_calls, ptr);
}

FS_INLINE int fs_host_call(const fs_guid_t* guid, void* args, size_t size)
{
    return fs_host_calls.call(&fs_host_calls, guid, args, size);
}

#endif /* _FS_HOSTCALLS_H */
