// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OE_MUSL_PATCHES_STDIO_H
#define _OE_MUSL_PATCHES_STDIO_H

/* Include the original MUSL stdio.h header. */
#include "__stdio.h"

/* Redefine certain path-oriented functions as needed. */
#if defined(OE_DEFAULT_FS)
#define fopen(...) oe_fopen(OE_DEFAULT_FS, __VA_ARGS__)
#define opendir(...) oe_opendir(OE_DEFAULT_FS, __VA_ARGS__)
#define stat(...) oe_stat(OE_DEFAULT_FS, __VA_ARGS__)
#define remove(...) oe_remove(OE_DEFAULT_FS, __VA_ARGS__)
#define rename(...) oe_rename(OE_DEFAULT_FS, __VA_ARGS__)
#define mkdir(...) oe_mkdir(OE_DEFAULT_FS, __VA_ARGS__)
#define rmdir(...) oe_rmdir(OE_DEFAULT_FS, __VA_ARGS__)
#endif

#endif /* _OE_MUSL_PATCHES_STDIO_H */
