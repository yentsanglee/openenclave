// Copyright (c) Microsoft Corporation. All rights reserved._ops
// Licensed under the MIT License.

#ifndef _OE_STDIOEX_H
#define _OE_STDIOEX_H

#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct oe_file OE_FILE;

OE_FILE *oe_fopen_dev(int device_id, const char *path, const char *mode);

size_t oe_fread(void *ptr, size_t size, size_t nmemb, OE_FILE *stream);

size_t oe_fwrite(const void *ptr, size_t size, size_t nmemb, OE_FILE *stream);

int oe_fseek(OE_FILE *stream, long offset, int whence);

int64_t oe_ftell(OE_FILE *stream);

int oe_fputs(const char *s, OE_FILE *stream);

char *oe_fgets(char *s, int size, OE_FILE *stream);

int oe_feof(OE_FILE* stream);

int oe_ferror(OE_FILE *stream);

int oe_fflush(OE_FILE *stream);

int oe_fclose(OE_FILE* stream);

#ifdef __cplusplus
}
#endif

#endif // _OE_STDIOEX_H
