// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _ENCLIBC_STDLIB_H
#define _ENCLIBC_STDLIB_H

#include "bits/common.h"

ENCLIBC_EXTERNC_BEGIN

#define ENCLIBC_RAND_MAX (0x7fffffff)

int enclibc_rand(void);

void* enclibc_malloc(size_t size);

void enclibc_free(void* ptr);

void* enclibc_calloc(size_t nmemb, size_t size);

void* enclibc_realloc(void* ptr, size_t size);

void* enclibc_memalign(size_t alignment, size_t size);

int enclibc_posix_memalign(void** memptr, size_t alignment, size_t size);

unsigned long int enclibc_strtoul(const char* nptr, char** endptr, int base);

int enclibc_atexit(void (*function)(void));

#if defined(ENCLIBC_NEED_STDC_NAMES)

#define RAND_MAX ENCLIBC_RAND_MAX

ENCLIBC_INLINE
int rand(void)
{
    return enclibc_rand();
}

ENCLIBC_INLINE
void* malloc(size_t size)
{
    return enclibc_malloc(size);
}

ENCLIBC_INLINE
void free(void* ptr)
{
    enclibc_free(ptr);
}

ENCLIBC_INLINE
void* calloc(size_t nmemb, size_t size)
{
    return enclibc_calloc(nmemb, size);
}

ENCLIBC_INLINE
void* realloc(void* ptr, size_t size)
{
    return enclibc_realloc(ptr, size);
}

ENCLIBC_INLINE
void* memalign(size_t alignment, size_t size)
{
    return enclibc_memalign(alignment, size);
}

ENCLIBC_INLINE
int posix_memalign(void** memptr, size_t alignment, size_t size)
{
    return enclibc_posix_memalign(memptr, alignment, size);
}

ENCLIBC_INLINE
unsigned long int strtoul(const char* nptr, char** endptr, int base)
{
    return enclibc_strtoul(nptr, endptr, base);
}

ENCLIBC_INLINE
int atexit(void (*function)(void))
{
    return enclibc_atexit(function);
}

#endif /* defined(ENCLIBC_NEED_STDC_NAMES) */

ENCLIBC_EXTERNC_END

#endif /* _ENCLIBC_STDLIB_H */
