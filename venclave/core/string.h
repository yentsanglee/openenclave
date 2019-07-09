// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_ENCLAVE_STRING_H
#define _VE_ENCLAVE_STRING_H

#include <openenclave/corelibc/string.h>
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

const char* ve_ostr(ve_ostr_buf* buf, uint64_t x, size_t* size);

const char* ve_ustr(ve_ustr_buf* buf, uint64_t x, size_t* size);

const char* ve_xstr(ve_xstr_buf* buf, uint64_t x, size_t* size);

const char* ve_dstr(ve_dstr_buf* buf, int64_t x, size_t* size);

#endif /* _VE_ENCLAVE_STRING_H */
