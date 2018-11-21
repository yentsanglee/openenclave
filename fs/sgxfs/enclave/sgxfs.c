// clang-format off
#include "linux-sgx/common/inc/sgx_tprotected_fs.h"
#undef FILENAME_MAX
#undef FOPEN_MAX
// clang-format on

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <openenclave/internal/sgxfs.h>
#include <openenclave/internal/fsinternal.h>

extern oe_fs_t oe_hostfs;

typedef struct _file
{
    FILE base;
    SGX_FILE* sgx_file;
} file_t;

OE_INLINE bool _valid_file(file_t* file)
{
    return file && file->base.magic == OE_FILE_MAGIC;
}

static int _f_fclose(FILE* base)
{
    int ret = -1;
    file_t* file = (file_t*)base;

    if (!_valid_file(file))
        goto done;

    if (sgx_fclose(file->sgx_file) != 0)
        goto done;

    free(file);

    ret = 0;

done:
    return ret;
}

static size_t _f_fread(void* ptr, size_t size, size_t nmemb, FILE* base)
{
    size_t ret = 0;
    file_t* file = (file_t*)base;

    if (!ptr || !_valid_file(file))
        goto done;

    ret = sgx_fread(ptr, size, nmemb, file->sgx_file);

done:

    return ret;
}

static size_t _f_fwrite(const void* ptr, size_t size, size_t nmemb, FILE* base)
{
    size_t ret = 0;
    file_t* file = (file_t*)base;

    if (!ptr || !_valid_file(file))
        goto done;

    ret = sgx_fwrite(ptr, size, nmemb, file->sgx_file);

done:

    return ret;
}

static int64_t _f_ftell(FILE* base)
{
    int64_t ret = -1;
    file_t* file = (file_t*)base;

    if (!_valid_file(file))
        goto done;

    ret = sgx_ftell(file->sgx_file);

done:
    return ret;
}

static int _f_fseek(FILE* base, int64_t offset, int whence)
{
    int ret = -1;
    file_t* file = (file_t*)base;

    if (!_valid_file(file))
        goto done;

    ret = sgx_fseek(file->sgx_file, offset, whence);

done:
    return ret;
}

static int _f_fflush(FILE* base)
{
    int ret = -1;
    file_t* file = (file_t*)base;

    if (!_valid_file(file))
        goto done;

    ret = sgx_fflush(file->sgx_file);

done:
    return ret;
}

static int _f_ferror(FILE* base)
{
    int ret = -1;
    file_t* file = (file_t*)base;

    if (!_valid_file(file))
        goto done;

    ret = sgx_ferror(file->sgx_file);

done:
    return ret;
}

static int _f_feof(FILE* base)
{
    int ret = -1;
    file_t* file = (file_t*)base;

    if (!_valid_file(file))
        goto done;

    ret = sgx_feof(file->sgx_file);

done:
    return ret;
}

static void _f_clearerr(FILE* base)
{
    file_t* file = (file_t*)base;

    if (!_valid_file(file))
        goto done;

    sgx_clearerr(file->sgx_file);

done:
    return;
}

static FILE* _fs_fopen(
    oe_fs_t* fs,
    const char* path,
    const char* mode,
    va_list ap)
{
    FILE* ret = NULL;
    file_t* file = NULL;

    if (!path || !mode)
        goto done;

    if (!(file = calloc(1, sizeof(file_t))))
        goto done;

    if (!(file->sgx_file = sgx_fopen_auto_key(path, mode)))
        return NULL;

    file->base.magic = OE_FILE_MAGIC;
    file->base.f_fclose = _f_fclose;
    file->base.f_fread = _f_fread;
    file->base.f_fwrite = _f_fwrite;
    file->base.f_ftell = _f_ftell;
    file->base.f_fseek = _f_fseek;
    file->base.f_fflush = _f_fflush;
    file->base.f_ferror = _f_ferror;
    file->base.f_feof = _f_feof;
    file->base.f_clearerr = _f_clearerr;

    ret = &file->base;
    file = NULL;

done:

    if (file)
        free(file);

    return ret;
}

static int _fs_release(oe_fs_t* fs)
{
    uint32_t ret = -1;

    if (!fs)
        goto done;

    ret = 0;

done:
    return ret;
}

static DIR* _fs_opendir(oe_fs_t* fs, const char* name)
{
    return oe_opendir(&oe_hostfs, name);
}

static int _fs_stat(oe_fs_t* fs, const char* path, struct stat* stat)
{
    return oe_stat(&oe_hostfs, path, stat);
}

static int _fs_rename(
    oe_fs_t* fs, const char* old_path, const char* new_path)
{
    return oe_rename(&oe_hostfs, old_path, new_path);
}

static int _fs_remove(oe_fs_t* fs, const char* path)
{
    return oe_remove(&oe_hostfs, path);
}

static int _fs_mkdir(oe_fs_t* fs, const char* path, unsigned int mode)
{
    return oe_mkdir(&oe_hostfs, path, mode);
}

static int _fs_rmdir(oe_fs_t* fs, const char* path)
{
    return oe_rmdir(&oe_hostfs, path);
}

oe_fs_t oe_sgxfs = {
    .fs_magic = OE_FS_MAGIC,
    .fs_release = _fs_release,
    .fs_fopen = _fs_fopen,
    .fs_opendir = _fs_opendir,
    .fs_stat = _fs_stat,
    .fs_remove = _fs_remove,
    .fs_rename = _fs_rename,
    .fs_mkdir = _fs_mkdir,
    .fs_rmdir = _fs_rmdir,
};
