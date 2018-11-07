#ifndef OE_WINPORT
#define OE_WINPORT 1
#endif

/* ATTN: implement va_list */
#if defined(OE_WINPORT) && defined(__NEED_va_list) && !defined(__DEFINED_va_list)
typedef int va_list;
#define __DEFINED_va_list
#endif

/* Define __isoc_va_list to va_list. */
#if defined(OE_WINPORT) && defined(__NEED___isoc_va_list) && !defined(__DEFINED___isoc_va_list)
#define __isoc_va_list va_list
#define __DEFINED___isoc_va_list
#endif

/* MSVC defines size_t, so suppress redefinition in alltypes.inc. */
#define __DEFINED_size_t

/* Include the alltypes.h header that MUSL generates during Linux x86_64 builds. */
#include "alltypes.inc"
