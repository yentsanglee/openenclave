#ifndef OE_WINPORT
#define OE_WINPORT 1
#endif

#if defined(OE_WINPORT) && defined(__NEED_va_list) && !defined(__DEFINED_va_list)
typedef char* va_list;
#define __DEFINED_va_list
#endif

#define __builtin_va_start(ap, x) __va_start(&ap, x)

#define __builtin_va_arg(ap, type) \
    *((type*)((ap += sizeof(__int64)) - sizeof(__int64)))

#define __builtin_va_end(ap) (ap = (va_list)0)

#if defined(OE_WINPORT) && defined(__NEED___isoc_va_list) && !defined(__DEFINED___isoc_va_list)
typedef char* __isoc_va_list;
#define __DEFINED___isoc_va_list
#endif

/* MSVC defines size_t, so suppress redefinition in alltypes.inc. */
#define __DEFINED_size_t
#define __DEFINED_int64_t
#define __DEFINED_uint64_t
#define __DEFINED_u_int64_t
#define __DEFINED_uintptr_t
#define __DEFINED_ptrdiff_t
#define __DEFINED_ssize_t
#define __DEFINED_intptr_t
#define __DEFINED_regoff_t
#define __DEFINED_intmax_t
#define __DEFINED_uintmax_t
#define __DEFINED_off_t
#define __DEFINED_ino_t
#define __DEFINED_dev_t
#define __DEFINED_blkcnt_t
#define __DEFINED_fsblkcnt_t
#define __DEFINED_fsfilcnt_t

#define __DEFINED_locale_t
#if defined(OE_WINPORT) 
typedef void* locale_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;
typedef unsigned long long u_int64_t;
typedef size_t off_t;
typedef size_t ssize_t;
typedef size_t uintptr_t;
typedef long long int_fast64_t;
typedef unsigned long long uint_fast64_t;
#endif

/* Include the alltypes.h header that MUSL generates during Linux x86_64 builds. */
#include "alltypes.inc"

