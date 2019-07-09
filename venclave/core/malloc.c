// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "malloc.h"
#include "common.h"
#include "print.h"
#include "process.h"
#include "sbrk.h"
#include "string.h"

static void* sbrk(intptr_t increment)
{
    return ve_sbrk(increment);
}

static void abort(void)
{
    ve_abort();
}

static const int EINVAL = 22;
static const int ENOMEM = 12;

#define HAVE_MMAP 0
#define LACKS_UNISTD_H
#define LACKS_SYS_PARAM_H
#define LACKS_SYS_TYPES_H
#define LACKS_TIME_H
#define LACKS_STDLIB_H
#define LACKS_STRING_H
#define LACKS_ERRNO_H 1
#define LACKS_SCHED_H 1
#define NO_MALLOC_STATS 1
#define MALLOC_FAILURE_ACTION
#define USE_LOCKS 1
#define USE_DL_PREFIX

#pragma GCC diagnostic push

#if defined(__clang__)
#pragma GCC diagnostic ignored "-Wparentheses-equality"
#pragma GCC diagnostic ignored "-Wnull-pointer-arithmetic"
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#endif

#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"

#include "../../3rdparty/dlmalloc/dlmalloc/malloc.c"

#pragma GCC diagnostic pop

void* malloc(size_t size)
{
    return dlmalloc(size);
}

void free(void* ptr)
{
    return dlfree(ptr);
}

void* calloc(size_t nmemb, size_t size)
{
    return dlcalloc(nmemb, size);
}

void* realloc(void* ptr, size_t size)
{
    return dlrealloc(ptr, size);
}

void* memalign(size_t alignment, size_t size)
{
    return dlmemalign(alignment, size);
}

int posix_memalign(void** memptr, size_t alignment, size_t size)
{
    return dlposix_memalign(memptr, alignment, size);
}

void* ve_malloc(size_t size)
{
#if defined(ZERO_FILL_MALLOCS)
    void* ptr;

    if ((ptr = malloc(size)))
        memset(ptr, 0, size);

    return ptr;
#else
    return malloc(size);
#endif
}

void ve_free(void* ptr)
{
    return free(ptr);
}

void* ve_calloc(size_t nmemb, size_t size)
{
    return calloc(nmemb, size);
}

void* ve_realloc(void* ptr, size_t size)
{
    return realloc(ptr, size);
}

void* ve_memalign(size_t alignment, size_t size)
{
    return memalign(alignment, size);
}

int ve_posix_memalign(void** memptr, size_t alignment, size_t size)
{
    return posix_memalign(memptr, alignment, size);
}
