#ifndef _OE_MUSL_LIBC_H
#define _OE_MUSL_LIBC_H

#ifndef OE_WINPORT
#define OE_WINPORT 1
#endif

#include "../../../3rdparty/musl/musl/src/internal/libc.h"

#undef weak_alias
#define weak_alias(old, new) __pragma(comment(linker, "/alternatename:" #new "=" #old))

#endif /* _OE_MUSL_LIBC_H */
