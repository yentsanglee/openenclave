// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "malloc.h"
#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>
#include "abort.h"
#include "sbrk.h"
#include "string.h"

#if defined(BUILD_STATIC)
#define USE_DLMALLOC
#endif

#if defined(USE_DLMALLOC)

static void* memset(void* s, int c, size_t n)
{
    return ve_memset(s, c, n);
}

static void* memcpy(void* dest, const void* src, size_t n)
{
    return ve_memcpy(dest, src, n);
}

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
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wparentheses-equality"
#endif
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

#endif /* defined(USE_DLMALLOC) */

void* malloc(size_t size);
void free(void* ptr);
void* calloc(size_t nmemb, size_t size);
void* realloc(void* ptr, size_t size);
void* memalign(size_t alignment, size_t size);

void* ve_malloc(size_t size)
{
    return malloc(size);
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
