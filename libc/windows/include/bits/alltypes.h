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

/* Include the alltypes.h header that MUSL generates during Linux x86_64 builds. */
#include "alltypes.inc"

