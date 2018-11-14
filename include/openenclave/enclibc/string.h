// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _ENCLIBC_STRING_H
#define _ENCLIBC_STRING_H

#include "bits/common.h"

ENCLIBC_EXTERNC_BEGIN

size_t enclibc_strlen(const char* s);

size_t enclibc_strnlen(const char* s, size_t n);

int enclibc_strcmp(const char* s1, const char* s2);

int enclibc_strncmp(const char* s1, const char* s2, size_t n);

int enclibc_strcasecmp(const char* s1, const char* s2);

int enclibc_strncasecmp(const char* s1, const char* s2, size_t n);

char* enclibc_strcpy(char* dest, const char* src);

char* enclibc_strncpy(char* dest, const char* src, size_t n);

char* enclibc_strcat(char* dest, const char* src);

char* enclibc_strncat(char* dest, const char* src, size_t n);

char* enclibc_strchr(const char* s, int c);

char* enclibc_strrchr(const char* s, int c);

char* enclibc_index(const char* s, int c);

char* enclibc_rindex(const char* s, int c);

char* enclibc_strstr(const char* haystack, const char* needle);

void* enclibc_memset(void* s, int c, size_t n);

void* enclibc_memcpy(void* dest, const void* src, size_t n);

int enclibc_memcmp(const void* s1, const void* s2, size_t n);

void* enclibc_memmove(void* dest, const void* src, size_t n);

size_t enclibc_strlcpy(char* dest, const char* src, size_t size);

size_t enclibc_strlcat(char* dest, const char* src, size_t size);

char* enclibc_strdup(const char* s);

char* enclibc_strndup(const char* s, size_t n);

char* enclibc_strerror(int errnum);

int enclibc_strerror_r(int errnum, char* buf, size_t buflen);

#if defined(ENCLIBC_NEED_STDC_NAMES)

ENCLIBC_INLINE
size_t strlen(const char* s)
{
    return enclibc_strlen(s);
}

ENCLIBC_INLINE
size_t strnlen(const char* s, size_t n)
{
    return enclibc_strnlen(s, n);
}

ENCLIBC_INLINE
int strcmp(const char* s1, const char* s2)
{
    return enclibc_strcmp(s1, s2);
}

ENCLIBC_INLINE
int strcasecmp(const char* s1, const char* s2)
{
    return enclibc_strcasecmp(s1, s2);
}

ENCLIBC_INLINE
int strncasecmp(const char* s1, const char* s2, size_t n)
{
    return enclibc_strncasecmp(s1, s2, n);
}

ENCLIBC_INLINE
int strncmp(const char* s1, const char* s2, size_t n)
{
    return enclibc_strncmp(s1, s2, n);
}

ENCLIBC_INLINE
char* strcpy(char* dest, const char* src)
{
    return enclibc_strcpy(dest, src);
}

ENCLIBC_INLINE
char* strncpy(char* dest, const char* src, size_t n)
{
    return enclibc_strncpy(dest, src, n);
}

ENCLIBC_INLINE
char* strcat(char* dest, const char* src)
{
    return enclibc_strcat(dest, src);
}

ENCLIBC_INLINE
char* strncat(char* dest, const char* src, size_t n)
{
    return enclibc_strncat(dest, src, n);
}

ENCLIBC_INLINE
char* strchr(const char* s, int c)
{
    return enclibc_strchr(s, c);
}

ENCLIBC_INLINE
char* strrchr(const char* s, int c)
{
    return enclibc_strrchr(s, c);
}

ENCLIBC_INLINE
char* index(const char* s, int c)
{
    return enclibc_index(s, c);
}

ENCLIBC_INLINE
char* rindex(const char* s, int c)
{
    return enclibc_rindex(s, c);
}

ENCLIBC_INLINE
char* strstr(const char* haystack, const char* needle)
{
    return enclibc_strstr(haystack, needle);
}

ENCLIBC_INLINE
void* memset(void* s, int c, size_t n)
{
    return enclibc_memset(s, c, n);
}

ENCLIBC_INLINE
void* memcpy(void* dest, const void* src, size_t n)
{
    return enclibc_memcpy(dest, src, n);
}

ENCLIBC_INLINE
int memcmp(const void* s1, const void* s2, size_t n)
{
    return enclibc_memcmp(s1, s2, n);
}

ENCLIBC_INLINE
void* memmove(void* dest, const void* src, size_t n)
{
    return enclibc_memmove(dest, src, n);
}

ENCLIBC_INLINE
size_t strlcpy(char* dest, const char* src, size_t size)
{
    return enclibc_strlcpy(dest, src, size);
}

ENCLIBC_INLINE
size_t strlcat(char* dest, const char* src, size_t size)
{
    return enclibc_strlcat(dest, src, size);
}

ENCLIBC_INLINE
char* strdup(const char* s)
{
    return enclibc_strdup(s);
}

ENCLIBC_INLINE
char* strndup(const char* s, size_t n)
{
    return enclibc_strndup(s, n);
}

ENCLIBC_INLINE
char* strerror(int errnum)
{
    return enclibc_strerror(errnum);
}

ENCLIBC_INLINE
int strerror_r(int errnum, char* buf, size_t buflen)
{
    return enclibc_strerror_r(errnum, buf, buflen);
}

#endif /* defined(ENCLIBC_NEED_STDC_NAMES) */

ENCLIBC_EXTERNC_END

#endif /* _ENCLIBC_STRING_H */
