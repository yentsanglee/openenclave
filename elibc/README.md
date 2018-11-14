elibc
=====

**elibc** is a minimal C library for **Linux** and **Windows**, suitable for 
building building **mbed TLS** and **oeenlave**. It may also suffice for other 
applications with minimal dependencies on the C library.

**Elibc** coexists with other C libraries by renaming all C library symbol 
references through the use of inlines. For example, the **string.h** header
defines **strcpy** as follows.

```
char* elibc_strcpy(char* dest, const char* src);

ELIBC_INLINE
char* strcpy(char* dest, const char* src)
{
    return elibc_strcpy(dest, src);
}
```

Since **strcpy** is an inline function, the caller's object file contains a 
reference to **oelibc_strcpy** rather than **strcpy**. This avoids conflicts 
with other C libraries, but requires that the calling sources be recompiled
with the **elibc** headers.

**Elibc** implements functions from following headers. A quick inspection
of these headers will reveal the subset of functions supported.

- **assert.h**
- **ctype.h**
- **errno.h**
- **limits.h**
- **pthread.h**
- **sched.h**
- **setjmp.h**
- **stdarg.h**
- **stddef.h**
- **stdint.h**
- **stdio.h**
- **stdlib.h**
- **string.h**
- **time.h**
- **unistd.h**

**Elibc** supports building of the **Open Enclave** core libraries, which 
may be linked in the following order.

- **oeenclave**
- **mbedx509**
- **mbedcrypto**
- **oeelibc**
- **oecore**

Note that the **elibc** library (**oelibc**) may be replaced with the **MUSL**
library (**oelibc**).
