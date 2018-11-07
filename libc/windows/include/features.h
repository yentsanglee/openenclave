#ifndef _OE_LIBC_FEATURES_H
#define _OE_LIBC_FEATURES_H

#ifndef OE_WINPORT
#define OE_WINPORT 1
#endif

#ifdef OE_WINPORT
#define __STDC_VERSION__ 199901L
#endif

/* Include the original MUSL features.h header. */
#include "features.inc"

#ifdef OE_WINPORT
# undef __restrict
# define __restrict
# define restrict
#endif

#define __OE_LIBC_PASTE(X, Y) X##Y
#define OE_LIBC_PASTE(X, Y) __OE_LIBC_PASTE(X, Y)

#endif /* _OE_LIBC_FEATURES_H */
