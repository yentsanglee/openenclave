#ifndef _OE_HOSTFSARGS_H
#define _OE_HOSTFSARGS_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>
#include <openenclave/internal/fs.h>

OE_EXTERNC_BEGIN

#define OE_HOSTFS_MODE_MAX 8

typedef enum _oe_hostfs_op {
    OE_HOSTFS_OP_NONE,
    OE_HOSTFS_OP_FOPEN,
    OE_HOSTFS_OP_FCLOSE,
    OE_HOSTFS_OP_FREAD,
    OE_HOSTFS_OP_FWRITE,
    OE_HOSTFS_OP_FTELL,
    OE_HOSTFS_OP_FSEEK,
    OE_HOSTFS_OP_FFLUSH,
    OE_HOSTFS_OP_FERROR,
    OE_HOSTFS_OP_FEOF,
    OE_HOSTFS_OP_CLEARERR,
    OE_HOSTFS_OP_OPENDIR,
    OE_HOSTFS_OP_READDIR,
    OE_HOSTFS_OP_CLOSEDIR,
    OE_HOSTFS_OP_STAT,
    OE_HOSTFS_OP_UNLINK,
    OE_HOSTFS_OP_RENAME,
    OE_HOSTFS_OP_MKDIR,
    OE_HOSTFS_OP_RMDIR,
} oe_hostfs_op_t;

typedef struct _oe_hostfs_args
{
    oe_hostfs_op_t op;
    int32_t err;
    union {
        struct
        {
            void* ret;
            char path[OE_FS_PATH_MAX];
            char mode[OE_HOSTFS_MODE_MAX];
        } fopen;
        struct
        {
            int32_t ret;
            void* file;
        } fclose;
        struct
        {
            size_t ret;
            void* ptr;
            size_t size;
            size_t nmemb;
            void* file;
        } fread;
        struct
        {
            size_t ret;
            const void* ptr;
            size_t size;
            size_t nmemb;
            void* file;
        } fwrite;
        struct
        {
            int64_t ret;
            void* file;
        } ftell;
        struct
        {
            int32_t ret;
            void* file;
            int64_t offset;
            int whence;
        } fseek;
        struct
        {
            int32_t ret;
            void* file;
        } fflush;
        struct
        {
            int32_t ret;
            void* file;
        } ferror;
        struct
        {
            int32_t ret;
            void* file;
        } feof;
        struct
        {
            int32_t ret;
            void* file;
        } clearerr;
        struct
        {
            void* ret;
            char name[OE_FS_PATH_MAX];
        } opendir;
        struct
        {
            int32_t ret;
            void* dir;
            struct
            {
                uint64_t d_ino;
                int64_t d_off;
                unsigned short d_reclen;
                uint8_t d_type;
                char d_name[OE_FS_PATH_MAX];
            } entry;
            void* result;
        } readdir;
        struct
        {
            int32_t ret;
            void* dir;
        } closedir;
        struct
        {
            int32_t ret;
            char path[OE_FS_PATH_MAX];
            struct
            {
                uint32_t st_dev;
                uint32_t st_ino;
                uint16_t st_mode;
                uint32_t st_nlink;
                uint16_t st_uid;
                uint16_t st_gid;
                uint32_t st_rdev;
                uint32_t st_size;
                uint32_t st_blksize;
                uint32_t st_blocks;
            } buf;
        } stat;
        struct
        {
            int32_t ret;
            char path[OE_FS_PATH_MAX];
        } unlink;
        struct
        {
            int32_t ret;
            char old_path[OE_FS_PATH_MAX];
            char new_path[OE_FS_PATH_MAX];
        } rename;
        struct
        {
            int32_t ret;
            char path[OE_FS_PATH_MAX];
            unsigned int mode;
        } mkdir;
        struct
        {
            int32_t ret;
            char path[OE_FS_PATH_MAX];
        } rmdir;
    } u;
    uint8_t buf[];
} oe_hostfs_args_t;

OE_EXTERNC_END

#endif /* _OE_HOSTFSARGS_H */
