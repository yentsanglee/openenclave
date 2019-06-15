// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _FREESTANDING_STRING_H
#define _FREESTANDING_STRING_H

#include "../freestanding/types.h"

size_t fs_strlen(const char* s);

int fs_strcmp(const char* s1, const char* s2);

size_t fs_strlcpy(char* dest, const char* src, size_t size);

size_t fs_strlcat(char* dest, const char* src, size_t size);

typedef struct _fs_intstr_buf
{
    char data[32];
} fs_intstr_buf_t;

const char* fs_uint64_octstr(fs_intstr_buf_t* buf, uint64_t x, size_t* size);

const char* fs_uint64_decstr(fs_intstr_buf_t* buf, uint64_t x, size_t* size);

const char* fs_uint64_hexstr(fs_intstr_buf_t* buf, uint64_t x, size_t* size);

const char* fs_int64_decstr(fs_intstr_buf_t* buf, int64_t x, size_t* size);

#endif /* _FREESTANDING_STRING_H */
