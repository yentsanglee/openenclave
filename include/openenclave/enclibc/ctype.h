// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _ENCLIBC_CTYPE_H
#define _ENCLIBC_CTYPE_H

#include "bits/common.h"

#define __ENCLIBC_ISALNUM_BIT (1 << 0)
#define __ENCLIBC_ISALPHA_BIT (1 << 1)
#define __ENCLIBC_ISCNTRL_BIT (1 << 2)
#define __ENCLIBC_ISDIGIT_BIT (1 << 3)
#define __ENCLIBC_ISGRAPH_BIT (1 << 4)
#define __ENCLIBC_ISLOWER_BIT (1 << 5)
#define __ENCLIBC_ISPRINT_BIT (1 << 6)
#define __ENCLIBC_ISPUNCT_BIT (1 << 7)
#define __ENCLIBC_ISSPACE_BIT (1 << 8)
#define __ENCLIBC_ISUPPER_BIT (1 << 9)
#define __ENCLIBC_ISXDIGIT_BIT (1 << 10)

extern const unsigned short* __enclibc_ctype_b_loc;

extern const unsigned int* __enclibc_ctype_tolower_loc;

extern const unsigned int* __enclibc_ctype_toupper_loc;

ENCLIBC_INLINE
int enclibc_isalnum(int c)
{
    return __enclibc_ctype_b_loc[c] & __ENCLIBC_ISALNUM_BIT;
}

ENCLIBC_INLINE
int enclibc_isalpha(int c)
{
    return __enclibc_ctype_b_loc[c] & __ENCLIBC_ISALPHA_BIT;
}

ENCLIBC_INLINE
int enclibc_iscntrl(int c)
{
    return __enclibc_ctype_b_loc[c] & __ENCLIBC_ISCNTRL_BIT;
}

ENCLIBC_INLINE
int enclibc_isdigit(int c)
{
    return __enclibc_ctype_b_loc[c] & __ENCLIBC_ISDIGIT_BIT;
}

ENCLIBC_INLINE
int enclibc_isgraph(int c)
{
    return __enclibc_ctype_b_loc[c] & __ENCLIBC_ISGRAPH_BIT;
}

ENCLIBC_INLINE
int enclibc_islower(int c)
{
    return __enclibc_ctype_b_loc[c] & __ENCLIBC_ISLOWER_BIT;
}

ENCLIBC_INLINE
int enclibc_isprint(int c)
{
    return __enclibc_ctype_b_loc[c] & __ENCLIBC_ISPRINT_BIT;
}

ENCLIBC_INLINE
int enclibc_ispunct(int c)
{
    return __enclibc_ctype_b_loc[c] & __ENCLIBC_ISPUNCT_BIT;
}

ENCLIBC_INLINE
int enclibc_isspace(int c)
{
    return __enclibc_ctype_b_loc[c] & __ENCLIBC_ISSPACE_BIT;
}

ENCLIBC_INLINE
int enclibc_isupper(int c)
{
    return __enclibc_ctype_b_loc[c] & __ENCLIBC_ISUPPER_BIT;
}

ENCLIBC_INLINE
int enclibc_isxdigit(int c)
{
    return __enclibc_ctype_b_loc[c] & __ENCLIBC_ISXDIGIT_BIT;
}

ENCLIBC_INLINE
int enclibc_toupper(int c)
{
    return __enclibc_ctype_toupper_loc[c];
}

ENCLIBC_INLINE
int enclibc_tolower(int c)
{
    return __enclibc_ctype_tolower_loc[c];
}

#if defined(ENCLIBC_NEED_STDC_NAMES)

ENCLIBC_INLINE
int isalnum(int c)
{
    return enclibc_isalnum(c);
}

ENCLIBC_INLINE
int isalpha(int c)
{
    return enclibc_isalpha(c);
}

ENCLIBC_INLINE
int iscntrl(int c)
{
    return enclibc_iscntrl(c);
}

ENCLIBC_INLINE
int isdigit(int c)
{
    return enclibc_isdigit(c);
}

ENCLIBC_INLINE
int isgraph(int c)
{
    return enclibc_isgraph(c);
}

ENCLIBC_INLINE
int islower(int c)
{
    return enclibc_islower(c);
}

ENCLIBC_INLINE
int isprint(int c)
{
    return enclibc_isprint(c);
}

ENCLIBC_INLINE
int ispunct(int c)
{
    return enclibc_ispunct(c);
}

ENCLIBC_INLINE
int isspace(int c)
{
    return enclibc_isspace(c);
}

ENCLIBC_INLINE
int isupper(int c)
{
    return enclibc_isupper(c);
}

ENCLIBC_INLINE
int isxdigit(int c)
{
    return enclibc_isxdigit(c);
}

ENCLIBC_INLINE
int toupper(int c)
{
    return enclibc_toupper(c);
}

ENCLIBC_INLINE
int tolower(int c)
{
    return enclibc_tolower(c);
}

#endif /* defined(ENCLIBC_NEED_STDC_NAMES) */

#endif /* _ENCLIBC_CTYPE_H */
