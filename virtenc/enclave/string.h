// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_STRING_H
#define _VE_STRING_H

#include <openenclave/bits/types.h>

typedef struct _fs_intstr_buf
{
    char data[32];
} ve_intstr_buf_t;

size_t ve_strlen(const char* s);

int ve_strcmp(const char* s1, const char* s2);

size_t ve_strlcpy(char* dest, const char* src, size_t size);

size_t ve_strlcat(char* dest, const char* src, size_t size);

const char* ve_uint64_octstr(ve_intstr_buf_t* buf, uint64_t x, size_t* size);

const char* ve_uint64_decstr(ve_intstr_buf_t* buf, uint64_t x, size_t* size);

const char* ve_uint64_hexstr(ve_intstr_buf_t* buf, uint64_t x, size_t* size);

const char* ve_int64_decstr(ve_intstr_buf_t* buf, int64_t x, size_t* size);

void* ve_memset(void* s, int c, size_t n);

void* ve_memcpy(void* dest, const void* src, size_t n);

int ve_memcmp(const void* s1, const void* s2, size_t n);

#endif /* _VE_STRING_H */
