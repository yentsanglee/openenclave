#ifndef _OE_FSINTERNAL_H
#define _OE_FSINTERNAL_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>

OE_EXTERNC_BEGIN

typedef struct _IO_FILE FILE;
typedef struct __dirstream oe_dir_t;

struct _IO_FILE
{
    int32_t (*f_fclose)(FILE* file);

    size_t (*f_fread)(void* ptr, size_t size, size_t nmemb, FILE* file);

    size_t (
        *f_fwrite)(const void* ptr, size_t size, size_t nmemb, FILE* file);

    int64_t (*f_ftell)(FILE* file);

    int32_t (*f_fseek)(FILE* file, int64_t offset, int whence);

    int32_t (*f_fflush)(FILE* file);

    int32_t (*f_ferror)(FILE* file);

    int32_t (*f_feof)(FILE* file);

    int32_t (*f_clearerr)(FILE* file);
};

struct __dirstream
{
    int32_t (*d_readdir)(oe_dir_t* dir, oe_dirent_t* entry, oe_dirent_t** result);

    int32_t (*d_closedir)(oe_dir_t* dir);
};

OE_EXTERNC_END

#endif /* _OE_FSINTERNAL_H */
