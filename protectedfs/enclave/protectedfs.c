#include "../common/protectedfs.h"
#include "../../fs/fs.h"
#include "../../fs/raise.h"
#include "../../fs/atomic.h"
#include <errno.h>

#ifdef FILENAME_MAX
#undef FILENAME_MAX
#endif

#ifdef FOPEN_MAX
#undef FOPEN_MAX
#endif

#include "linux-sgx/common/inc/sgx_tprotected_fs.h"

#define PFS_MAGIC 0x5042e4e1
#define FILE_MAGIC 0x82c485d3

#if 0
#define D(X) X
#else
#define D(X)
#endif

typedef struct _pfs pfs_t;

struct _pfs
{
    fs_t base;
    uint32_t magic;
    volatile uint64_t ref_count;
};

struct _fs_file
{
    uint32_t magic;
    SGX_FILE* sgx_file;
};

FS_INLINE bool _valid_pfs(fs_t* fs)
{
    return fs && ((pfs_t*)fs)->magic == PFS_MAGIC;
}

static bool _valid_file(fs_file_t* file)
{
    return file && file->magic == FILE_MAGIC && file->sgx_file;
}

static fs_errno_t _fs_read(
    fs_file_t* file,
    void* data,
    size_t size,
    ssize_t* nread)
{
    fs_errno_t err = FS_EOK;
    ssize_t n;

    D( printf("ENTER: protectedfs.read()\n"); )

    if (!file || !data || !nread)
        FS_RAISE(FS_EINVAL);

    errno = 0;
    n = sgx_fread(data, 1, size, file->sgx_file);

    if (n < 0)
    {
        if (errno)
            FS_RAISE(errno);

        FS_RAISE(FS_EBADF);
    }

    *nread = n;

done:
    return err;
}

static fs_errno_t _fs_write(
    fs_file_t* file,
    const void* data,
    size_t size,
    ssize_t* nwritten)
{
    fs_errno_t err = FS_EOK;
    ssize_t n;

    D( printf("ENTER: protectedfs.write()\n"); )

    if (!file || !data || !nwritten)
        FS_RAISE(FS_EINVAL);

    errno = 0;
    n = sgx_fwrite(data, 1, size, file->sgx_file);

    if (n != size)
    {
        if (errno)
            FS_RAISE(errno);

        FS_RAISE(FS_EBADF);
    }

    *nwritten = n;

done:
    return err;
}

static fs_errno_t _fs_close(fs_file_t* file)
{
    fs_errno_t err = FS_EOK;

    D( printf("ENTER: protectedfs.close()\n"); )

    if (!_valid_file(file))
        FS_RAISE(FS_EINVAL);

    errno = 0;
    int r = sgx_fclose(file->sgx_file);

    if (r != 0)
    {
        if (errno)
            FS_RAISE(errno);

        FS_RAISE(FS_EBADF);
    }

    free(file);

done:

    return err;
}

static fs_errno_t _fs_release(fs_t* fs)
{
    fs_errno_t err = FS_EOK;
    pfs_t* pfs = (pfs_t*)fs;

    D( printf("ENTER: protectedfs.release()\n"); )

    if (!_valid_pfs(fs))
        FS_RAISE(FS_EINVAL);


    if (fs_atomic_decrement(&pfs->ref_count) == 0)
    {
        memset(pfs, 0xdd, sizeof(pfs_t));
        free(pfs);
    }

done:

    return err;
}

static fs_errno_t _fs_add_ref(fs_t* fs)
{
    fs_errno_t err = FS_EOK;
    pfs_t* pfs = (pfs_t*)fs;

    if (!_valid_pfs(fs))
        FS_RAISE(FS_EINVAL);

    fs_atomic_increment(&pfs->ref_count);

done:

    return err;
}

static fs_errno_t _fs_opendir(fs_t* fs, const char* path, fs_dir_t** dir)
{
    fs_errno_t err = FS_EOK;

    D( printf("ENTER: protectedfs.opendir()\n"); )

    FS_RAISE(FS_EINVAL);

    if (!_valid_pfs(fs))
        FS_RAISE(FS_EINVAL);

done:

    return err;
}

static fs_errno_t _fs_readdir(fs_dir_t* dir, fs_dirent_t** ent)
{
    fs_errno_t err = FS_EOK;

    D( printf("ENTER: protectedfs.readdir()\n"); )

    FS_RAISE(FS_EINVAL);


done:

    return err;
}

static fs_errno_t _fs_closedir(fs_dir_t* dir)
{
    fs_errno_t err = FS_EOK;

    D( printf("ENTER: protectedfs.closedir()\n"); )

    FS_RAISE(FS_EINVAL);
    FS_RAISE(FS_EINVAL);

done:

    return err;
}

static fs_errno_t _fs_open(
    fs_t* fs,
    const char* path,
    int flags,
    uint32_t mode,
    fs_file_t** file_out)
{
    fs_errno_t err = FS_EOK;
    SGX_FILE* sgx_file = NULL;
    fs_file_t* file = NULL;
    const char* mode_string = NULL;
    if (!_valid_pfs(fs) || !path || !file_out)
        FS_RAISE(FS_EINVAL);

    /* Convert the flags to an fopen-style flags string. */
    {
        /*
         *  w  : FLAGS=00001101 FS_O_WRONLY (FS_O_TRUNC | FS_O_CREAT)
         *  a  : FLAGS=00002101 FS_O_WRONLY (FS_O_APPEND | FS_O_CREAT)
         *  w+ : FLAGS=00001102 FS_O_RDWR   (FS_O_TRUNC | FS_O_CREAT)
         *  a+ : FLAGS=00002102 FS_O_RDWR   (FS_O_APPEND | FS_O_CREAT)
         *  r+ : FLAGS=00000002 FS_O_RDWR
         *  r  : FLAGS=00000000 FS_O_RDONLY
         */

        if ((flags & FS_O_WRONLY))
        {
            if ((flags & FS_O_TRUNC) && (flags & FS_O_CREAT))
                mode_string = "w";
            else if ((flags & FS_O_APPEND) && (flags & FS_O_CREAT))
                mode_string = "a";
            else
                goto done;
        }
        else if ((flags & FS_O_RDWR))
        {
            if ((flags & FS_O_TRUNC) && (flags & FS_O_CREAT))
                mode_string = "w+";
            else if ((flags & FS_O_APPEND) && (flags & FS_O_CREAT))
                mode_string = "a+";
            else
                mode_string = "r+";
        }
        else /* FS_O_RDONLY */
        {
            mode_string = "r";
        }
    }

    errno = 0;

    if (!(sgx_file = sgx_fopen_auto_key(path, mode_string)))
    {
        if (errno)
            FS_RAISE(errno);

        FS_RAISE(FS_ENOENT);
    }

    if (!(file = calloc(1, sizeof(fs_file_t))))
        FS_RAISE(FS_ENOMEM);

    file->magic = FILE_MAGIC;
    file->sgx_file = sgx_file;

    *file_out = file;

done:

    return err;
}

static fs_errno_t _fs_mkdir(fs_t* fs, const char* path, uint32_t mode)
{
    fs_errno_t err = FS_EOK;

    D( printf("ENTER: protectedfs.mkdir()\n"); )

    FS_RAISE(FS_EINVAL);

done:

    return err;
}

static fs_errno_t _fs_creat(
    fs_t* fs,
    const char* path,
    uint32_t mode,
    fs_file_t** file_out)
{
    fs_errno_t err = FS_EOK;

    D( printf("ENTER: protectedfs.creat()\n"); )

    FS_RAISE(FS_EINVAL);

done:

    return err;
}

static fs_errno_t _fs_link(fs_t* fs, const char* old_path, const char* new_path)
{
    fs_errno_t err = FS_EOK;

    D( printf("ENTER: protectedfs.link()\n"); )

    FS_RAISE(FS_EINVAL);

done:

    return err;
}

static fs_errno_t _fs_rename(
    fs_t* fs,
    const char* old_path,
    const char* new_path)
{
    fs_errno_t err = FS_EOK;

    D( printf("ENTER: protectedfs.rename()\n"); )

    FS_RAISE(FS_EINVAL);

done:

    return err;
}

static fs_errno_t _fs_unlink(fs_t* fs, const char* path)
{
    fs_errno_t err = FS_EOK;

    D( printf("ENTER: protectedfs.unlink()\n"); )

    FS_RAISE(FS_EINVAL);

done:

    return err;
}

static fs_errno_t _fs_truncate(fs_t* fs, const char* path, ssize_t length)
{
    fs_errno_t err = FS_EOK;

    D( printf("ENTER: protectedfs.truncate()\n"); )

    FS_RAISE(FS_EINVAL);

done:

    return err;
}

static fs_errno_t _fs_rmdir(fs_t* fs, const char* path)
{
    fs_errno_t err = FS_EOK;

    D( printf("ENTER: protectedfs.rmdir()\n"); )

    FS_RAISE(FS_EINVAL);

done:

    return err;
}

static fs_errno_t _fs_stat(fs_t* fs, const char* path, fs_stat_t* stat)
{
    fs_errno_t err = FS_EOK;

    D( printf("ENTER: protectedfs.stat()\n"); )

    FS_RAISE(FS_EINVAL);

done:

    return err;
}

static fs_errno_t _fs_lseek(
    fs_file_t* file,
    ssize_t offset,
    int whence,
    ssize_t* offset_out)
{
    fs_errno_t err = FS_EOK;

    D( printf("ENTER: protectedfs.lseek()\n"); )

    FS_RAISE(FS_EINVAL);

done:

    return err;
}

/*
**==============================================================================
**
** Public interface:
**
**==============================================================================
*/

int protectedfs_new(fs_t** fs_out)
{
    int ret = -1;
    pfs_t* pfs = NULL;

    if (fs_out)
        *fs_out = NULL;

    if (!fs_out)
        goto done;

    if (!(pfs = calloc(1, sizeof(pfs_t))))
        goto done;

    pfs->base.fs_release = _fs_release;
    pfs->base.fs_add_ref = _fs_add_ref;
    pfs->base.fs_creat = _fs_creat;
    pfs->base.fs_open = _fs_open;
    pfs->base.fs_lseek = _fs_lseek;
    pfs->base.fs_read = _fs_read;
    pfs->base.fs_write = _fs_write;
    pfs->base.fs_close = _fs_close;
    pfs->base.fs_opendir = _fs_opendir;
    pfs->base.fs_readdir = _fs_readdir;
    pfs->base.fs_closedir = _fs_closedir;
    pfs->base.fs_stat = _fs_stat;
    pfs->base.fs_link = _fs_link;
    pfs->base.fs_unlink = _fs_unlink;
    pfs->base.fs_rename = _fs_rename;
    pfs->base.fs_truncate = _fs_truncate;
    pfs->base.fs_mkdir = _fs_mkdir;
    pfs->base.fs_rmdir = _fs_rmdir;
    pfs->magic = PFS_MAGIC;
    pfs->ref_count = 1;

    *fs_out = &pfs->base;
    pfs = NULL;

    ret = 0;

done:

    if (pfs)
        free(pfs);

    return ret;
}

int fs_mount_protectedfs(const char* target)
{
    int ret = -1;
    fs_t* fs = NULL;

    if (!target)
        goto done;

    if (protectedfs_new(&fs) != 0)
        goto done;

    if (fs_mount(fs, target) != 0)
        goto done;

    ret = 0;

done:

    if (fs)
        fs->fs_release(fs);

    return ret;
}
