#ifndef _fs_h
#define _fs_h

#include "errno.h"
#include <stdint.h>
#include <stdio.h>

#define FS_PATH_MAX 256
#define FS_BLOCK_SIZE 512

/* fs_dirent_t.d_type -- the file type. */
#define FS_DT_UNKNOWN 0
#define FS_DT_FIFO 1 /* unused */
#define FS_DT_CHR 2  /* unused */
#define FS_DT_DIR 4
#define FS_DT_BLK 6 /* unused */
#define FS_DT_REG 8
#define FS_DT_LNK 10  /* unused */
#define FS_DT_SOCK 12 /* unused */
#define FS_DT_WHT 14  /* unused */

/* fs_inode_t.i_mode -- access rights. */
#define FS_S_IFSOCK 0xC000
#define FS_S_IFLNK 0xA000
#define FS_S_IFREG 0x8000
#define FS_S_IFBLK 0x6000
#define FS_S_IFDIR 0x4000
#define FS_S_IFCHR 0x2000
#define FS_S_IFIFO 0x1000
#define FS_S_ISUID 0x0800
#define FS_S_ISGID 0x0400
#define FS_S_ISVTX 0x0200
#define FS_S_IRUSR 0x0100
#define FS_S_IWUSR 0x0080
#define FS_S_IXUSR 0x0040
#define FS_S_IRGRP 0x0020
#define FS_S_IWGRP 0x0010
#define FS_S_IXGRP 0x0008
#define FS_S_IROTH 0x0004
#define FS_S_IWOTH 0x0002
#define FS_S_IXOTH 0x0001
#define FS_S_IRWXUSR (FS_S_IRUSR | FS_S_IWUSR | FS_S_IXUSR)
#define FS_S_IRWXGRP (FS_S_IRGRP | FS_S_IWGRP | FS_S_IXGRP)
#define FS_S_IRWXOTH (FS_S_IROTH | FS_S_IWOTH | FS_S_IXOTH)
#define FS_S_IRWXALL (FS_S_IRWXUSR | FS_S_IRWXGRP | FS_S_IRWXOTH)
#define FS_S_IRWUSR (FS_S_IRUSR | FS_S_IWUSR)
#define FS_S_IRWGRP (FS_S_IRGRP | FS_S_IWGRP)
#define FS_S_IRWOTH (FS_S_IROTH | FS_S_IWOTH)
#define FS_S_IRWALL (FS_S_IRWUSR | FS_S_IRWGRP | FS_S_IRWOTH)
#define FS_S_REG_DEFAULT (FS_S_IFREG | FS_S_IRWALL)
#define FS_S_DIR_DEFAULT (FS_S_IFDIR | FS_S_IRWXALL)

/* whence parameter for fs_lseek(). */
#define FS_SEEK_SET 0
#define FS_SEEK_CUR 1
#define FS_SEEK_END 2

typedef struct _fs fs_t;
typedef struct _fs_file fs_file_t;
typedef struct _fs_dir fs_dir_t;

typedef struct _fs_dirent
{
    uint32_t d_ino;
    uint32_t d_off;
    uint16_t d_reclen;
    uint8_t d_type;
    char d_name[FS_PATH_MAX];
    uint8_t __d_reserved;
} fs_dirent_t;

typedef struct _fs_stat
{
    uint32_t st_dev;
    uint32_t st_ino;
    uint16_t st_mode;
    uint16_t __st_padding;
    uint32_t st_nlink;
    uint16_t st_uid;
    uint16_t st_gid;
    uint32_t st_rdev;
    uint32_t st_size;
    uint32_t st_blksize;
    uint32_t st_blocks;
    uint32_t st_atime;
    uint32_t st_mtime;
    uint32_t st_ctime;
} fs_stat_t;

struct _fs
{
    oe_errno_t (*fs_release)(fs_t* fs);

    oe_errno_t (*fs_create)(
        fs_t* fs,
        const char* path,
        uint32_t mode,
        fs_file_t** file);

    oe_errno_t (*fs_open)(
        fs_t* fs,
        const char* pathname,
        int flags,
        uint32_t mode,
        fs_file_t** file_out);

    oe_errno_t (*fs_lseek)(
        fs_file_t* file,
        ssize_t offset,
        int whence,
        ssize_t* offset_out);

    oe_errno_t (
        *fs_read)(fs_file_t* file, void* data, uint32_t size, int32_t* nread);

    oe_errno_t (*fs_write)(
        fs_file_t* file,
        const void* data,
        uint32_t size,
        int32_t* nwritten);

    oe_errno_t (*fs_close)(fs_file_t* file);

    oe_errno_t (*fs_stat)(fs_t* fs, const char* path, fs_stat_t* stat);

    oe_errno_t (*fs_link)(fs_t* fs, const char* old_path, const char* new_path);

    oe_errno_t (*fs_unlink)(fs_t* fs, const char* path);

    oe_errno_t (
        *fs_rename)(fs_t* fs, const char* old_path, const char* new_path);

    oe_errno_t (*fs_truncate)(fs_t* fs, const char* path);

    oe_errno_t (*fs_mkdir)(fs_t* fs, const char* path, uint32_t mode);

    oe_errno_t (*fs_rmdir)(fs_t* fs, const char* path);

    oe_errno_t (*fs_opendir)(fs_t* fs, const char* path, fs_dir_t** dir);

    oe_errno_t (*fs_readdir)(fs_dir_t* dir, fs_dirent_t** dirent);

    oe_errno_t (*fs_closedir)(fs_dir_t* dir);
};

#endif /* _fs_h */
