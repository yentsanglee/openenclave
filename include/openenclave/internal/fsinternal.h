#ifndef _OE_FSINTERNAL_H
#define _OE_FSINTERNAL_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>

OE_EXTERNC_BEGIN

typedef struct _oe_file oe_file_t;
typedef struct _oe_dir oe_dir_t;

struct _oe_file
{
    int32_t (*f_fclose)(oe_file_t* file);

    size_t (*f_fread)(void* ptr, size_t size, size_t nmemb, oe_file_t* file);

    size_t (
        *f_fwrite)(const void* ptr, size_t size, size_t nmemb, oe_file_t* file);

    int64_t (*f_ftell)(oe_file_t* file);

    int32_t (*f_fseek)(oe_file_t* file, int64_t offset, int whence);

    int32_t (*f_fflush)(oe_file_t* file);

    int32_t (*f_ferror)(oe_file_t* file);

    int32_t (*f_feof)(oe_file_t* file);

    int32_t (*f_clearerr)(oe_file_t* file);
};

struct _oe_dir
{
    int32_t (*d_readdir)(oe_dir_t* dir, oe_dirent_t* entry, oe_dirent_t** result);

    int32_t (*d_closedir)(oe_dir_t* dir);
};

OE_EXTERNC_END

#endif /* _OE_FSINTERNAL_H */
