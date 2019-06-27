// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_STRING_H
#define _VE_STRING_H

#include "common.h"

/* Buffer for octal uint64_t strings. */
typedef struct _ve_ostr_buf
{
    char data[23];
} ve_ostr_buf;

/* Buffer for int64_t strings. */
typedef struct _ve_dstr_buf
{
    char data[21];
} ve_dstr_buf;

/* Buffer for uint64_t strings. */
typedef struct _ve_ustr_buf
{
    char data[21];
} ve_ustr_buf;

/* Buffer for hex uint64_t strings. */
typedef struct _ve_xstr_buf
{
    char data[17];
} ve_xstr_buf;

size_t ve_strlen(const char* s);

int ve_strcmp(const char* s1, const char* s2);

size_t ve_strlcpy(char* dest, const char* src, size_t size);

size_t ve_strlcat(char* dest, const char* src, size_t size);

const char* ve_ostr(ve_ostr_buf* buf, uint64_t x, size_t* size);

const char* ve_ustr(ve_ustr_buf* buf, uint64_t x, size_t* size);

const char* ve_xstr(ve_xstr_buf* buf, uint64_t x, size_t* size);

const char* ve_dstr(ve_dstr_buf* buf, int64_t x, size_t* size);

void* ve_memset(void* s, int c, size_t n);

void* ve_memcpy(void* dest, const void* src, size_t n);

int ve_memcmp(const void* s1, const void* s2, size_t n);

#endif /* _VE_STRING_H */
