// Licensed under the MIT License.

#define _GNU_SOURCE

#include <assert.h>
#include <errno.h>
#include <openenclave/bits/fs.h>
#include <openenclave/bits/safemath.h>
#include <openenclave/enclave.h>
#include <openenclave/internal/fs.h>
#include <openenclave/internal/hexdump.h>
#include <openenclave/internal/keys.h>
#include <openenclave/internal/oefs.h>
#include <openenclave/internal/raise.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "../../common/buf.h"
#include "blkdev.h"
#include "raise.h"
#include "sha.h"
#include "utils.h"

/*
**==============================================================================
**
** OEFS (OE File System)
**
**
**     Initial layout:
**
**         empty-block
**         empty-block
**         header-block (plain-text header block)
**         super-block
**         bitmap blocks
**         -------------
**         root-directory-inode block (block-1)
**         root-directory block (block-2)
**         root-directory block (block-3)
**         unassigned blocks (block-4 through N)
**
**==============================================================================
*/

/*
**==============================================================================
**
** Local definitions.
**
**==============================================================================
*/

#define INLINE static __inline

#define OEFS_FLAG_NONE 0
#define OEFS_FLAG_CRYPTO 2
#define OEFS_FLAG_AUTH_CRYPTO 4
#define OEFS_FLAG_INTEGRITY 8
#define OEFS_FLAG_CACHING 16

#define BITS_PER_BLK (OEFS_BLOCK_SIZE * 8)

#define HEADER_BLOCK_MAGIC 0X782056e2

#define SUPER_BLOCK_MAGIC 0x0EF51234

#define INODE_MAGIC 0xe3d6e3eb

#define OEFS_MAGIC 0xe56231bf

#define FILE_MAGIC 0x4adaee1c

#define DIR_MAGIC 0xdb9916db

#define DEFAULT_MODE 0666

/* The logical block number of the root directory inode. */
#define ROOT_INO 1

/* The physical block number of the plain-text header block. */
#define HEADER_BLOCK_PHYSICAL_BLKNO 2

/* The physical block number of the super block. */
#define SUPER_BLOCK_PHYSICAL_BLKNO 3

/* The first physical block number of the block bitmap. */
#define BITMAP_PHYSICAL_BLKNO 4

#define BLKNOS_PER_INODE \
    (sizeof(((oefs_inode_t*)0)->i_blocks) / sizeof(uint32_t))

#define BLKNOS_PER_BNODE \
    (sizeof(((oefs_bnode_t*)0)->b_blocks) / sizeof(uint32_t))

/* oefs_dirent_t.d_type -- the file type. */
#define OEFS_DT_UNKNOWN 0
#define OEFS_DT_FIFO 1 /* unused */
#define OEFS_DT_CHR 2  /* unused */
#define OEFS_DT_DIR 4
#define OEFS_DT_BLK 6 /* unused */
#define OEFS_DT_REG 8
#define OEFS_DT_LNK 10  /* unused */
#define OEFS_DT_SOCK 12 /* unused */
#define OEFS_DT_WHT 14  /* unused */

/* oefs_inode_t.i_mode -- access rights. */
#define OEFS_S_IFSOCK 0xC000
#define OEFS_S_IFLNK 0xA000
#define OEFS_S_IFREG 0x8000
#define OEFS_S_IFBLK 0x6000
#define OEFS_S_IFDIR 0x4000
#define OEFS_S_IFCHR 0x2000
#define OEFS_S_IFIFO 0x1000
#define OEFS_S_ISUID 0x0800
#define OEFS_S_ISGID 0x0400
#define OEFS_S_ISVTX 0x0200
#define OEFS_S_IRUSR 0x0100
#define OEFS_S_IWUSR 0x0080
#define OEFS_S_IXUSR 0x0040
#define OEFS_S_IRGRP 0x0020
#define OEFS_S_IWGRP 0x0010
#define OEFS_S_IXGRP 0x0008
#define OEFS_S_IROTH 0x0004
#define OEFS_S_IWOTH 0x0002
#define OEFS_S_IXOTH 0x0001
#define OEFS_S_IRWXUSR (OEFS_S_IRUSR | OEFS_S_IWUSR | OEFS_S_IXUSR)
#define OEFS_S_IRWXGRP (OEFS_S_IRGRP | OEFS_S_IWGRP | OEFS_S_IXGRP)
#define OEFS_S_IRWXOTH (OEFS_S_IROTH | OEFS_S_IWOTH | OEFS_S_IXOTH)
#define OEFS_S_IRWXALL (OEFS_S_IRWXUSR | OEFS_S_IRWXGRP | OEFS_S_IRWXOTH)
#define OEFS_S_IRWUSR (OEFS_S_IRUSR | OEFS_S_IWUSR)
#define OEFS_S_IRWGRP (OEFS_S_IRGRP | OEFS_S_IWGRP)
#define OEFS_S_IRWOTH (OEFS_S_IROTH | OEFS_S_IWOTH)
#define OEFS_S_IRWALL (OEFS_S_IRWUSR | OEFS_S_IRWGRP | OEFS_S_IRWOTH)
#define OEFS_S_REG_DEFAULT (OEFS_S_IFREG | OEFS_S_IRWALL)
#define OEFS_S_DIR_DEFAULT (OEFS_S_IFDIR | OEFS_S_IRWXALL)

// clang-format off
#define OEFS_O_RDONLY    000000000
#define OEFS_O_WRONLY    000000001
#define OEFS_O_RDWR      000000002
#define OEFS_O_CREAT     000000100
#define OEFS_O_EXCL      000000200
#define OEFS_O_NOCTTY    000000400
#define OEFS_O_TRUNC     000001000
#define OEFS_O_APPEND    000002000
#define OEFS_O_NONBLOCK  000004000
#define OEFS_O_DSYNC     000010000
#define OEFS_O_SYNC      004010000
#define OEFS_O_RSYNC     004010000
#define OEFS_O_DIRECTORY 000200000
#define OEFS_O_NOFOLLOW  000400000
#define OEFS_O_CLOEXEC   002000000
#define OEFS_O_ASYNC     000020000
#define OEFS_O_DIRECT    000040000
#define OEFS_O_LARGEFILE 000000000
#define OEFS_O_NOATIME   001000000
#define OEFS_O_PATH      010000000
#define OEFS_O_TMPFILE   020200000
#define OEFS_O_NDELAY    O_NONBLOCK
// clang-format on

/* whence parameter for oefs_lseek(). */
#define OEFS_SEEK_SET 0
#define OEFS_SEEK_CUR 1
#define OEFS_SEEK_END 2

typedef struct _oefs oefs_t;
typedef struct _oefs_file oefs_file_t;
typedef struct _oefs_dir oefs_dir_t;

typedef struct _oefs_dirent
{
    uint32_t d_ino;
    uint32_t d_off;
    uint16_t d_reclen;
    uint8_t d_type;
    char d_name[OEFS_PATH_MAX];
    uint8_t __d_reserved;
} oefs_dirent_t;

typedef struct _oefs_timespec
{
    long tv_sec;
    long tv_nsec;
} oefs_timespec_t;

typedef struct _oefs_stat
{
    uint32_t st_dev;
    uint32_t st_ino;
    uint16_t st_mode;
    uint16_t __st_padding1;
    uint32_t st_nlink;
    uint16_t st_uid;
    uint16_t st_gid;
    uint32_t st_rdev;
    uint32_t st_size;
    uint32_t st_blksize;
    uint32_t st_blocks;
    uint32_t __st_padding2;
    oefs_timespec_t st_atim;
    oefs_timespec_t st_mtim;
    oefs_timespec_t st_ctim;
} oefs_stat_t;

typedef struct _oefs_header_block
{
    /* Magic number: HEADER_BLOCK_MAGIC. */
    uint32_t h_magic;

    uint8_t h_key_id[OEFS_KEY_SIZE];

    uint8_t h_padding[OEFS_BLOCK_SIZE - sizeof(uint32_t) - OEFS_KEY_SIZE];

} oefs_header_block_t;

OE_STATIC_ASSERT(sizeof(oefs_header_block_t) == OEFS_BLOCK_SIZE);

typedef struct _oefs_super_block
{
    /* Magic number: SUPER_BLOCK_MAGIC. */
    uint32_t s_magic;

    /* The total number of physical blocks in the file system. */
    uint32_t s_nblks;

    /* The number of data blocks in the file system. */
    uint32_t s_num_data_blocks;

    /* The number of free data blocks. */
    uint32_t s_free_data_blocks;

    /* The SHA-256 hash of the header block. */
    uint8_t s_header_hash[32];

    /* (12) Reserved. */
    uint8_t s_reserved[OEFS_BLOCK_SIZE - 12 - 32 - 4];
} oefs_super_block_t;

OE_STATIC_ASSERT(sizeof(oefs_super_block_t) == OEFS_BLOCK_SIZE);

typedef struct _oefs_inode
{
    uint32_t i_magic;

    /* Access rights. */
    uint16_t i_mode;

    /* Owner user id */
    uint16_t i_uid;

    /* Owner group id */
    uint16_t i_gid;

    /* The number of links to this inode. */
    uint16_t i_links;

    /* Size of this file. */
    uint32_t i_size;

    /* File times. */
    uint32_t i_atime;
    uint32_t i_ctime;
    uint32_t i_mtime;
    uint32_t i_dtime;

    /* Total number of blocks in this file. */
    uint32_t i_nblks;

    /* The next blknos block. */
    uint32_t i_next;

    /* Reserved. */
    uint32_t i_reserved[6];

    /* (64) Blocks comprising this file. */
    uint32_t i_blocks[(OEFS_BLOCK_SIZE / 4) - 16];

} oefs_inode_t;

OE_STATIC_ASSERT(sizeof(oefs_inode_t) == OEFS_BLOCK_SIZE);

typedef struct _oefs_bnode
{
    /* The next blknos block. */
    uint32_t b_next;

    /* Blocks comprising this file. */
    uint32_t b_blocks[(OEFS_BLOCK_SIZE / 4) - 1];

} oefs_bnode_t;

OE_STATIC_ASSERT(sizeof(oefs_bnode_t) == OEFS_BLOCK_SIZE);

OE_STATIC_ASSERT(sizeof(oefs_dirent_t) == 268);

OE_STATIC_ASSERT(sizeof(oefs_timespec_t) == 16);
OE_STATIC_ASSERT(sizeof(oefs_stat_t) == (10 * 4 + 3 * 16));

struct _oefs
{
    /* Should contain the value of the OEFS_MAGIC macro. */
    uint32_t magic;

    /* "Cooked" crypto device. */
    oefs_blkdev_t* dev;

    union {
        const oefs_super_block_t read;
        // This field should only be accessed with the _sb_write() method,
        // which sets the oefs_t.dirty flag.
        oefs_super_block_t __write;
    } sb;

    union {
        // This field should only be accessed with the _bitmap_write() method,
        // which sets the oefs_t.dirty flag.
        uint8_t* __write;
        const uint8_t* read;
    } bitmap;

    oefs_super_block_t sb_copy;
    uint8_t* bitmap_copy;
    size_t bitmap_size;
    size_t nblks;

    bool dirty;
};

static bool _valid_oefs(oefs_t* oefs)
{
    return oefs && oefs->magic == OEFS_MAGIC;
}

struct _oefs_file
{
    uint32_t magic;
    oefs_t* oefs;

    /* The file's inode number. */
    uint32_t ino;

    /* The file's inode */
    oefs_inode_t inode;

    /* The block numbers that contain the file's data. */
    oe_bufu32_t blknos;

    /* The block numbers that contain the file's block numbers. */
    oe_bufu32_t bnode_blknos;

    /* The file offset (or current position). */
    uint32_t offset;

    /* Access flags: OEFS_O_RDONLY OEFS_O_RDWR, OEFS_O_WRONLY */
    int access;

    bool eof;
};

static bool _valid_file(oefs_file_t* file)
{
    return file && file->magic == FILE_MAGIC && file->oefs;
}

static oefs_t* _oefs_from_file(oefs_file_t* file)
{
    if (!_valid_file(file))
        return NULL;

    return file->oefs;
}

struct _oefs_dir
{
    uint32_t magic;

    /* The inode number of this directory. */
    uint32_t ino;

    /* The open file which contains the directory entries. */
    oefs_file_t* file;

    /* The current directory entry. */
    oefs_dirent_t ent;
};

static bool _valid_dir(oefs_dir_t* dir)
{
    return dir && dir->magic == DIR_MAGIC && dir->file && dir->file->oefs;
}

static oefs_t* _oefs_from_dir(oefs_dir_t* dir)
{
    if (!_valid_dir(dir))
        return NULL;

    return dir->file->oefs;
}

INLINE void _begin(oefs_t* oefs)
{
    if (oefs && oefs->magic == OEFS_MAGIC && oefs->dev)
        oefs->dev->begin(oefs->dev);
}

INLINE void _end(oefs_t* oefs)
{
    if (oefs && oefs->magic == OEFS_MAGIC && oefs->dev)
        oefs->dev->end(oefs->dev);
}

INLINE bool _test_bit(const uint8_t* data, uint32_t size, uint32_t index)
{
    uint32_t byte = index / 8;
    uint32_t bit = index % 8;
    assert(byte < size);
    return data[byte] & (1 << bit);
}

INLINE void _set_bit(uint8_t* data, uint32_t size, uint32_t index)
{
    uint32_t byte = index / 8;
    uint32_t bit = index % 8;
    assert(byte < size);
    data[byte] |= (1 << bit);
}

INLINE void _clr_bit(uint8_t* data, uint32_t size, uint32_t index)
{
    uint32_t byte = index / 8;
    uint32_t bit = index % 8;
    assert(byte < size);
    data[byte] &= ~(1 << bit);
}

INLINE size_t _num_bitmap_blocks(size_t nblks)
{
    return oefs_round_to_multiple(nblks, BITS_PER_BLK) / BITS_PER_BLK;
}

/* Get the physical block number from a logical block number. */
INLINE uint32_t _get_physical_blkno(oefs_t* oefs, uint32_t blkno)
{
    /* Calculate the number of bitmap blocks. */
    size_t num_bitmap_blocks = _num_bitmap_blocks(oefs->nblks);
    return blkno + (BITMAP_PHYSICAL_BLKNO + num_bitmap_blocks) - 1;
}

static bool _is_zero_filled(const void* data, size_t size)
{
    const uint8_t* p = (const uint8_t*)data;

    while (size--)
    {
        if (*p++ != '\0')
            return false;
    }

    return true;
}

static int _oefs_size(size_t nblks, size_t* size);

static int _oefs_mkfs(
    oefs_blkdev_t* host_dev,
    oefs_blkdev_t* dev,
    size_t nblks,
    uint8_t key_id[OEFS_KEY_SIZE]);

#if 0
static int _oefs_creat(
    oefs_t* fs,
    const char* path,
    uint32_t mode,
    oefs_file_t** file_out);
#endif

static int _oefs_open(
    oefs_t* fs,
    const char* path,
    int flags,
    uint32_t mode,
    oefs_file_t** file_out);

#if 0
static int _oefs_truncate(oefs_t* fs, const char* path, ssize_t length);
#endif

static int _oefs_lseek(
    oefs_file_t* file,
    ssize_t offset,
    int whence,
    ssize_t* offset_out);

static int _oefs_read(
    oefs_file_t* file,
    void* data,
    size_t size,
    ssize_t* nread);

static int _oefs_write(
    oefs_file_t* file,
    const void* data,
    size_t size,
    ssize_t* nwritten);

static int _oefs_close(oefs_file_t* file);

static int _oefs_opendir(oefs_t* fs, const char* path, oefs_dir_t** dir);

static int _oefs_readdir(oefs_dir_t* dir, oefs_dirent_t** ent);

static int _oefs_closedir(oefs_dir_t* dir);

static int _oefs_stat(oefs_t* fs, const char* path, oefs_stat_t* stat);

static int _oefs_link(oefs_t* fs, const char* old_path, const char* new_path);

static int _oefs_unlink(oefs_t* fs, const char* path);

static int _oefs_rename(oefs_t* fs, const char* old_path, const char* new_path);

static int _oefs_mkdir(oefs_t* fs, const char* path, uint32_t mode);

static int _oefs_rmdir(oefs_t* fs, const char* path);

static int _read_block(oefs_t* oefs, size_t blkno, oefs_blk_t* blk)
{
    int err = 0;
    uint32_t physical_blkno;

    /* Check whether the block number is valid. */
    if (blkno == 0 || blkno > oefs->sb.read.s_num_data_blocks)
        OEFS_RAISE(EIO);

    /* Sanity check: make sure the block is not free. */
    if (!_test_bit(oefs->bitmap.read, oefs->bitmap_size, blkno - 1))
        OEFS_RAISE(EIO);

    /* Convert the logical block number to a physical block number. */
    physical_blkno = _get_physical_blkno(oefs, blkno);

    /* Get the physical block. */
    if (oefs->dev->get(oefs->dev, physical_blkno, blk) != 0)
        OEFS_RAISE(EIO);

done:
    return err;
}

static int _oefs_write_block(oefs_t* oefs, size_t blkno, const oefs_blk_t* blk)
{
    int err = 0;
    uint32_t physical_blkno;

    /* Check whether the block number is valid. */
    if (blkno == 0 || blkno > oefs->sb.read.s_num_data_blocks)
        OEFS_RAISE(EINVAL);

    /* Sanity check: make sure the block is not free. */
    if (!_test_bit(oefs->bitmap.read, oefs->bitmap_size, blkno - 1))
        OEFS_RAISE(EIO);

    /* Convert the logical block number to a physical block number. */
    physical_blkno = _get_physical_blkno(oefs, blkno);

    /* Get the physical block. */
    if (oefs->dev->put(oefs->dev, physical_blkno, blk) != 0)
        OEFS_RAISE(EIO);

done:
    return err;
}

static int _load_inode(oefs_t* oefs, uint32_t ino, oefs_inode_t* inode)
{
    int err = 0;

    /* Read this inode into memory. */
    OEFS_CHECK(_read_block(oefs, ino, (oefs_blk_t*)inode));

    /* Check the inode magic number. */
    if (inode->i_magic != INODE_MAGIC)
        OEFS_RAISE(EIO);

done:
    return err;
}

static int _load_blknos(
    oefs_t* oefs,
    oefs_inode_t* inode,
    oe_bufu32_t* bnode_blknos,
    oe_bufu32_t* blknos)
{
    int err = 0;

    /* Collect direct block numbers from the inode. */
    {
        size_t n = sizeof(inode->i_blocks) / sizeof(uint32_t);

        for (size_t i = 0; i < n && inode->i_blocks[i]; i++)
        {
            if (oe_bufu32_append(blknos, &inode->i_blocks[i], 1) != 0)
                OEFS_RAISE(ENOMEM);
        }
    }

    /* Traverse linked list of blknos blocks. */
    {
        uint32_t next = inode->i_next;

        while (next)
        {
            oefs_bnode_t bnode;
            size_t n;

            /* Read this bnode into memory. */
            OEFS_CHECK(_read_block(oefs, next, (oefs_blk_t*)&bnode));

            /* Append this bnode blkno. */
            if (oe_bufu32_append(bnode_blknos, &next, 1) != 0)
                OEFS_RAISE(ENOMEM);

            n = sizeof(bnode.b_blocks) / sizeof(uint32_t);

            /* Get all blocks from this bnode. */
            for (size_t i = 0; i < n && bnode.b_blocks[i]; i++)
            {
                if (oe_bufu32_append(blknos, &bnode.b_blocks[i], 1) != 0)
                    OEFS_RAISE(ENOMEM);
            }

            next = bnode.b_next;
        }
    }

done:
    return err;
}

static int _open(oefs_t* oefs, uint32_t ino, oefs_file_t** file_out)
{
    int err = 0;
    oefs_file_t* file = NULL;

    if (file_out)
        *file_out = NULL;

    if (!oefs || !ino || !file_out)
        OEFS_RAISE(EINVAL);

    if (!(file = calloc(1, sizeof(oefs_file_t))))
        OEFS_RAISE(ENOMEM);

    file->magic = FILE_MAGIC;
    file->oefs = oefs;
    file->ino = ino;

    /* Read this inode into memory. */
    OEFS_CHECK(_load_inode(oefs, ino, &file->inode));

    /* Load the block numbers into memory. */
    OEFS_CHECK(
        _load_blknos(oefs, &file->inode, &file->bnode_blknos, &file->blknos));

    *file_out = file;
    file = NULL;

done:

    if (file)
    {
        oe_bufu32_release(&file->blknos);
        free(file);
    }

    return err;
}

static oefs_super_block_t* _sb_write(oefs_t* oefs)
{
    oefs->dirty = true;
    return &oefs->sb.__write;
}

static uint8_t* _bitmap_write(oefs_t* oefs)
{
    oefs->dirty = true;
    return oefs->bitmap.__write;
}

static int _assign_blkno(oefs_t* oefs, uint32_t* blkno)
{
    int err = 0;

    *blkno = 0;

    if (oefs->sb.read.s_free_data_blocks == 0)
        OEFS_RAISE(ENOSPC);

    for (uint32_t i = 0; i < oefs->nblks; i++)
    {
        if (!_test_bit(oefs->bitmap.read, oefs->bitmap_size, i))
        {
            _set_bit(_bitmap_write(oefs), oefs->bitmap_size, i);
            *blkno = i + 1;
            _sb_write(oefs)->s_free_data_blocks--;
            OEFS_RAISE(0);
        }
    }

    OEFS_RAISE(ENOSPC);

done:
    return err;
}

static int _unassign_blkno(oefs_t* oefs, uint32_t blkno)
{
    int err = 0;
    uint32_t index = blkno - 1;

    if (blkno == 0 || index >= oefs->nblks)
        OEFS_RAISE(EINVAL);

    assert(_test_bit(oefs->bitmap.read, oefs->bitmap_size, index));

    _clr_bit(_bitmap_write(oefs), oefs->bitmap_size, index);
    _sb_write(oefs)->s_free_data_blocks++;

done:
    return err;
}

static int _flush_super_block(oefs_t* oefs)
{
    int err = 0;

    /* Flush the superblock if it changed. */
    if (memcmp(&oefs->sb, &oefs->sb_copy, sizeof(oefs_super_block_t)) != 0)
    {
        const uint32_t blkno = SUPER_BLOCK_PHYSICAL_BLKNO;

        if (oefs->dev->put(oefs->dev, blkno, (const oefs_blk_t*)&oefs->sb) != 0)
            OEFS_RAISE(EIO);

        memcpy(&oefs->sb_copy, &oefs->sb, sizeof(oefs_super_block_t));
    }

done:
    return err;
}

static int _flush_bitmap(oefs_t* oefs)
{
    int err = 0;
    size_t num_bitmap_blocks;

    /* Calculate the number of bitmap blocks. */
    num_bitmap_blocks = _num_bitmap_blocks(oefs->nblks);

    /* Flush each bitmap block that changed. */
    for (size_t i = 0; i < num_bitmap_blocks; i++)
    {
        /* Set pointer to the i-th block to write. */
        oefs_blk_t* blk = (oefs_blk_t*)oefs->bitmap.read + i;
        oefs_blk_t* blk_copy = (oefs_blk_t*)oefs->bitmap_copy + i;

        /* Flush this bitmap block if it changed. */
        if (memcmp(blk, blk_copy, OEFS_BLOCK_SIZE) != 0)
        {
            /* Calculate the block number to write. */
            uint32_t blkno = BITMAP_PHYSICAL_BLKNO + i;

            if (oefs->dev->put(oefs->dev, blkno, blk) != 0)
                OEFS_RAISE(EIO);

            memcpy(blk_copy, blk, OEFS_BLOCK_SIZE);
        }
    }

done:
    return err;
}

static int _flush(oefs_t* oefs)
{
    int err = 0;

    if (!oefs)
        OEFS_RAISE(EINVAL);

    if (oefs->dirty)
    {
        OEFS_CHECK(_flush_bitmap(oefs));
        OEFS_CHECK(_flush_super_block(oefs));
        oefs->dirty = false;
    }

done:
    return err;
}

static int _oefs_write_data(
    oefs_t* oefs,
    const void* data,
    uint32_t size,
    oe_bufu32_t* blknos)
{
    int err = 0;
    const uint8_t* ptr = (const uint8_t*)data;
    uint32_t remaining = size;

    /* While there is data remaining to be written. */
    while (remaining)
    {
        uint32_t blkno;
        oefs_blk_t blk;
        uint32_t copy_size;

        /* Assign a new block number from the in-memory bitmap. */
        OEFS_CHECK(_assign_blkno(oefs, &blkno));

        /* Append the new block number to the array of blocks numbers. */
        if (oe_bufu32_append(blknos, &blkno, 1) != 0)
            OEFS_RAISE(ENOMEM);

        /* Calculate bytes to copy to the block. */
        copy_size = (remaining > OEFS_BLOCK_SIZE) ? OEFS_BLOCK_SIZE : remaining;

        /* Clear the block. */
        memset(blk.data, 0, sizeof(oefs_blk_t));

        /* Copy user data to the block. */
        memcpy(blk.data, ptr, copy_size);

        /* Write the new block. */
        OEFS_CHECK(_oefs_write_block(oefs, blkno, &blk));

        /* Advance to next block of data to write. */
        ptr += copy_size;
        remaining -= copy_size;
    }

done:

    if (_flush(oefs) != 0)
        return EIO;

    return err;
}

static void _fill_slots(
    uint32_t* slots,
    uint32_t num_slots,
    const uint32_t** ptr_in_out,
    uint32_t* rem_in_out)
{
    const uint32_t* ptr = *ptr_in_out;
    uint32_t rem = *rem_in_out;
    uint32_t i = 0;

    /* Find the first free slot if any. */
    while (i < num_slots && slots[i])
        i++;

    /* Copy into free slots. */
    for (uint32_t j = i; j < num_slots && rem; j++)
    {
        slots[j] = *ptr;
        ptr++;
        rem--;
    }

    *ptr_in_out = ptr;
    *rem_in_out = rem;
}

static int _append_block_chain(oefs_file_t* file, const oe_bufu32_t* blknos)
{
    int err = 0;
    const uint32_t* ptr = blknos->data;
    uint32_t rem = blknos->size;

    const void* object;
    uint32_t* slots;
    uint32_t count;
    uint32_t blkno;
    uint32_t* next;
    oefs_bnode_t bnode;

    if (file->inode.i_next == 0)
    {
        object = &file->inode;
        slots = file->inode.i_blocks;
        count = OE_COUNTOF(file->inode.i_blocks);
        blkno = file->ino;
        next = &file->inode.i_next;
    }
    else
    {
        assert(file->bnode_blknos.size != 0);

        if (file->bnode_blknos.size == 0)
            OEFS_RAISE(EIO);

        blkno = file->bnode_blknos.data[file->bnode_blknos.size - 1];

        OEFS_CHECK(_read_block(file->oefs, blkno, (oefs_blk_t*)&bnode));

        object = &bnode;
        slots = bnode.b_blocks;
        count = OE_COUNTOF(bnode.b_blocks);
        next = &bnode.b_next;
    }

    while (rem)
    {
        /* Fill the slots with as many block numbers as will fit. */
        _fill_slots(slots, count, &ptr, &rem);

        /* If the blocks overflowed the slots, then create a new bnode. */
        if (rem)
        {
            uint32_t new_blkno;

            /* Assign a block number for a new bnode. */
            OEFS_CHECK(_assign_blkno(file->oefs, &new_blkno));

            /* Append the bnode blkno to the file struct. */
            if (oe_bufu32_append(&file->bnode_blknos, &new_blkno, 1) != 0)
                OEFS_RAISE(ENOMEM);

            *next = new_blkno;

            /* Rewrite the current inode or bnode. */
            OEFS_CHECK(_oefs_write_block(file->oefs, blkno, object));

            /* Initialize the new bnode with zeros. */
            memset(&bnode, 0, sizeof(oefs_bnode_t));

            /* Set up variables to refer to the new bnode. */
            object = &bnode;
            slots = bnode.b_blocks;
            count = OE_COUNTOF(bnode.b_blocks);
            blkno = new_blkno;
            next = &bnode.b_next;
        }
        else
        {
            /* Rewrite the inode or bnode. */
            OEFS_CHECK(_oefs_write_block(file->oefs, blkno, object));
        }
    }

done:
    return err;
}

static bool _sane_file(oefs_file_t* file)
{
    if (!file->oefs)
        return false;

    if (!file->ino)
        return false;

    if (file->inode.i_nblks != file->blknos.size)
        return false;

    return true;
}

static int _split_path(
    const char* path,
    char dirname[OEFS_PATH_MAX],
    char basename[OEFS_PATH_MAX])
{
    int err = 0;
    char* slash;

    /* Reject paths that are too long. */
    if (strlen(path) >= OEFS_PATH_MAX)
        OEFS_RAISE(EINVAL);

    /* Reject paths that are not absolute */
    if (path[0] != '/')
        OEFS_RAISE(EINVAL);

    /* Handle root directory up front */
    if (strcmp(path, "/") == 0)
    {
        strlcpy(dirname, "/", OEFS_PATH_MAX);
        strlcpy(basename, "/", OEFS_PATH_MAX);
        OEFS_RAISE(0);
    }

    /* This cannot fail (prechecked) */
    if (!(slash = strrchr(path, '/')))
        OEFS_RAISE(EINVAL);

    /* If path ends with '/' character */
    if (!slash[1])
        OEFS_RAISE(EINVAL);

    /* Split the path */
    {
        if (slash == path)
        {
            strlcpy(dirname, "/", OEFS_PATH_MAX);
        }
        else
        {
            uint32_t index = slash - path;
            strlcpy(dirname, path, OEFS_PATH_MAX);

            if (index < OEFS_PATH_MAX)
                dirname[index] = '\0';
            else
                dirname[OEFS_PATH_MAX - 1] = '\0';
        }

        strlcpy(basename, slash + 1, OEFS_PATH_MAX);
    }

done:
    return err;
}

static int _creat(
    oefs_t* oefs,
    uint32_t dir_ino,
    const char* name,
    uint8_t type,
    uint32_t* ino_out)
{
    int err = 0;
    uint32_t ino;
    oefs_file_t* file = NULL;

    if (ino_out)
        *ino_out = 0;

    if (!oefs || !dir_ino || !name || strlen(name) >= OEFS_PATH_MAX)
        OEFS_RAISE(EINVAL);

    /* ATTN: should create replace an existing file? */

    /* Create an inode for the new file. */
    {
        oefs_inode_t inode;

        memset(&inode, 0, sizeof(oefs_inode_t));

        inode.i_magic = INODE_MAGIC;
        inode.i_links = 1;

        if (type == OEFS_DT_DIR)
            inode.i_mode = OEFS_S_DIR_DEFAULT;
        else if (type == OEFS_DT_REG)
            inode.i_mode = OEFS_S_REG_DEFAULT;
        else
            OEFS_RAISE(EINVAL);

        OEFS_CHECK(_assign_blkno(oefs, &ino));
        OEFS_CHECK(_oefs_write_block(oefs, ino, (const oefs_blk_t*)&inode));
    }

    /* Create an inode for the new file. */

    /* Add an entry to the directory for this inode. */
    {
        oefs_dirent_t ent;
        ssize_t n;

        OEFS_CHECK(_open(oefs, dir_ino, &file));

        /* Check for duplicates. */
        for (;;)
        {
            OEFS_CHECK(_oefs_read(file, &ent, sizeof(ent), &n));

            if (n == 0)
                break;

            assert(n == sizeof(ent));

            if (n != sizeof(ent))
                OEFS_RAISE(EIO);

            if (strcmp(ent.d_name, name) == 0)
                OEFS_RAISE(EEXIST);
        }

        /* Append the entry to the directory. */
        {
            memset(&ent, 0, sizeof(ent));
            ent.d_ino = ino;
            ent.d_off = file->offset;
            ent.d_reclen = sizeof(ent);
            ent.d_type = type;
            strlcpy(ent.d_name, name, sizeof(ent.d_name));

            OEFS_CHECK(_oefs_write(file, &ent, sizeof(ent), &n));

            if (n != sizeof(ent))
                OEFS_RAISE(EIO);
        }
    }

    /* Create an inode for the new file. */

    if (ino_out)
        *ino_out = ino;

done:

    if (file)
        _oefs_close(file);

    if (_flush(oefs) != 0)
        return EIO;

    return err;
}

static int _truncate(oefs_file_t* file, ssize_t length)
{
    int err = 0;
    oefs_t* oefs;
    size_t block_index;
    size_t bnode_index;

    if (!file)
        OEFS_RAISE(EINVAL);

    assert(_sane_file(file));

    oefs = file->oefs;

    /* Fail if length is greater than the size of the file. */
    if (length > file->inode.i_size)
        OEFS_RAISE(EFBIG);

    /* Calculate the index of the first block to be freed. */
    block_index = (length + OEFS_BLOCK_SIZE - 1) / OEFS_BLOCK_SIZE;

    /* Release the truncated data blocks. */
    {
        for (size_t i = block_index; i < file->blknos.size; i++)
        {
            OEFS_CHECK(_unassign_blkno(oefs, file->blknos.data[i]));
            file->inode.i_nblks--;
        }

        if (oe_bufu32_resize(&file->blknos, block_index) != 0)
            OEFS_RAISE(ENOMEM);
    }

    if (block_index <= BLKNOS_PER_INODE)
    {
        file->inode.i_next = 0;

        /* Clear any inode slots. */
        for (size_t i = block_index; i < BLKNOS_PER_INODE; i++)
            file->inode.i_blocks[i] = 0;

        /* Release all the bnode blocks. */
        for (size_t i = 0; i < file->bnode_blknos.size; i++)
        {
            OEFS_CHECK(_unassign_blkno(oefs, file->bnode_blknos.data[i]));
        }

        if (oe_bufu32_resize(&file->bnode_blknos, 0) != 0)
            OEFS_RAISE(ENOMEM);
    }
    else
    {
        size_t slot_index;
        oefs_bnode_t bnode;
        uint32_t bnode_blkno;

        /* Adjust the block index relative to the bnodes. */
        block_index -= BLKNOS_PER_INODE;

        /* Find the index of the associated bnode. */
        bnode_index = block_index / BLKNOS_PER_BNODE;

        /* Find the slot index within this bnode. */
        slot_index = block_index % BLKNOS_PER_BNODE;

        /* Adjust the indices if this is the first slot. */
        if (slot_index == 0)
        {
            if (bnode_index == 0)
                OEFS_RAISE(EOVERFLOW);

            bnode_index--;
            slot_index = BLKNOS_PER_BNODE;
        }

        /* Load the bnode into memory. */
        bnode_blkno = file->bnode_blknos.data[bnode_index];
        OEFS_CHECK(_read_block(oefs, bnode_blkno, (oefs_blk_t*)&bnode));

        /* Clear any slots in the bnode. */
        for (size_t i = slot_index; i < BLKNOS_PER_BNODE; i++)
            bnode.b_blocks[i] = 0;

        /* Unlink this bnode from next bnode. */
        bnode.b_next = 0;

        /* Release unused bnode blocks. */
        for (size_t i = bnode_index + 1; i < file->bnode_blknos.size; i++)
        {
            OEFS_CHECK(_unassign_blkno(oefs, file->bnode_blknos.data[i]));
        }

        if (oe_bufu32_resize(&file->bnode_blknos, bnode_index + 1) != 0)
            OEFS_RAISE(ENOMEM);

        /* Rewrite the bnode. */
        OEFS_CHECK(
            _oefs_write_block(oefs, bnode_blkno, (const oefs_blk_t*)&bnode));
    }

    /* Update the inode. */
    file->inode.i_size = length;

    /* Update the file offset. */
    file->offset = 0;

    /* Sync the inode to disk. */
    OEFS_CHECK(
        _oefs_write_block(
            file->oefs, file->ino, (const oefs_blk_t*)&file->inode));

done:

    assert(_sane_file(file));

    if (_flush(file->oefs) != 0)
        return EIO;

    return err;
}

static int _load_file(oefs_file_t* file, void** data_out, size_t* size_out)
{
    int err = 0;
    oe_buf_t buf = OE_BUF_INITIALIZER;
    char data[OEFS_BLOCK_SIZE];
    ssize_t n;

    *data_out = NULL;
    *size_out = 0;

    for (;;)
    {
        OEFS_CHECK(_oefs_read(file, data, sizeof(data), &n));

        if (n == 0)
            break;

        if (oe_buf_append(&buf, data, n) != 0)
            OEFS_RAISE(ENOMEM);
    }

    *data_out = buf.data;
    *size_out = buf.size;
    memset(&buf, 0, sizeof(buf));

done:

    oe_buf_release(&buf);

    return err;
}

static int _release_inode(oefs_t* oefs, uint32_t ino)
{
    int err = 0;
    oefs_file_t* file = NULL;

    OEFS_CHECK(_open(oefs, ino, &file));
    OEFS_CHECK(_truncate(file, 0));
    OEFS_CHECK(_unassign_blkno(oefs, file->ino));

done:

    if (file)
        _oefs_close(file);

    if (_flush(oefs) != 0)
        return EIO;

    return err;
}

int _unlink(oefs_t* oefs, uint32_t dir_ino, uint32_t ino, const char* name)
{
    int err = 0;
    oefs_file_t* dir = NULL;
    oefs_file_t* file = NULL;
    void* data = NULL;
    size_t size = 0;
    oefs_inode_t inode;

    if (!oefs || !dir_ino || !ino)
        OEFS_RAISE(EINVAL);

    /* Open the directory file. */
    OEFS_CHECK(_open(oefs, dir_ino, &dir));

    /* Load the contents of the parent directory into memory. */
    OEFS_CHECK(_load_file(dir, &data, &size));

    /* File must be a multiple of the entry size. */
    {
        assert((size % sizeof(oefs_dirent_t)) == 0);

        if (size % sizeof(oefs_dirent_t))
            OEFS_RAISE(EIO);
    }

    /* Truncate the parent directory. */
    OEFS_CHECK(_truncate(dir, 0));

    /* Rewrite the directory entries but exclude the removed file. */
    {
        const oefs_dirent_t* entries = (const oefs_dirent_t*)data;
        size_t num_entries = size / sizeof(oefs_dirent_t);

        for (size_t i = 0; i < num_entries; i++)
        {
            if (strcmp(entries[i].d_name, name) != 0)
            {
                const uint32_t n = sizeof(oefs_dirent_t);
                ssize_t nwritten;

                OEFS_CHECK(_oefs_write(dir, &entries[i], n, &nwritten));

                if (nwritten != n)
                    OEFS_RAISE(EIO);
            }
        }
    }

    /* Load the inode into memory. */
    OEFS_CHECK(_load_inode(oefs, ino, &inode));

    /* If this is the only link to this file, then remove it. */
    if (inode.i_links == 1)
    {
        OEFS_CHECK(_release_inode(oefs, ino));
    }
    else
    {
        /* Decrement the number of links. */
        inode.i_links--;

        /* Rewrite the inode. */
        OEFS_CHECK(_oefs_write_block(oefs, ino, (const oefs_blk_t*)&inode));
    }

done:

    if (dir)
        _oefs_close(dir);

    if (file)
        _oefs_close(file);

    if (data)
        free(data);

    if (_flush(oefs) != 0)
        return EIO;

    return err;
}

static int _opendir_by_ino(oefs_t* oefs, uint32_t ino, oefs_dir_t** dir_out)
{
    int err = 0;
    oefs_dir_t* dir = NULL;
    oefs_file_t* file = NULL;

    if (dir_out)
        *dir_out = NULL;

    if (!oefs || !ino || !dir_out)
        OEFS_RAISE(EINVAL);

    OEFS_CHECK(_open(oefs, ino, &file));

    if (!(dir = calloc(1, sizeof(oefs_dir_t))))
        OEFS_RAISE(ENOMEM);

    dir->magic = DIR_MAGIC;
    dir->ino = ino;
    dir->file = file;
    file = NULL;

    *dir_out = dir;

done:

    if (file)
        _oefs_close(file);

    return err;
}

static int _path_to_ino(
    oefs_t* oefs,
    const char* path,
    uint32_t* dir_ino_out,
    uint32_t* ino_out,
    uint8_t* type_out) /* oefs_dir_t.d_type */
{
    int err = 0;
    char buf[OEFS_PATH_MAX];
    const char* elements[OEFS_PATH_MAX];
    size_t num_elements = 0;
    uint8_t i;
    uint32_t current_ino = 0;
    uint32_t dir_ino = 0;
    oefs_dir_t* dir = NULL;

    if (dir_ino_out)
        *dir_ino_out = 0;

    if (ino_out)
        *ino_out = 0;

    if (type_out)
        *type_out = 0;

    /* Check for null parameters */
    if (!oefs || !path || !ino_out)
        OEFS_RAISE(EINVAL);

    /* Check path length */
    if (strlen(path) >= OEFS_PATH_MAX)
        OEFS_RAISE(ENAMETOOLONG);

    /* Make copy of the path. */
    strlcpy(buf, path, sizeof(buf));

    /* Be sure path begins with "/" */
    if (path[0] != '/')
        OEFS_RAISE(EINVAL);

    elements[num_elements++] = "/";

    /* Split the path into components */
    {
        char* p;
        char* save;

        for (p = strtok_r(buf, "/", &save); p; p = strtok_r(NULL, "/", &save))
        {
            assert(num_elements < OE_COUNTOF(elements));
            elements[num_elements++] = p;
        }
    }

    /* Load each inode along the path until we find it. */
    for (i = 0; i < num_elements; i++)
    {
        bool final_element = (num_elements == i + 1);

        if (strcmp(elements[i], "/") == 0)
        {
            current_ino = ROOT_INO;
            dir_ino = current_ino;

            if (final_element)
            {
                if (type_out)
                    *type_out = OEFS_DT_DIR;
            }
        }
        else
        {
            oefs_dirent_t* ent;

            OEFS_CHECK(_opendir_by_ino(oefs, current_ino, &dir));

            dir_ino = current_ino;
            current_ino = 0;

            for (;;)
            {
                OEFS_CHECK(_oefs_readdir(dir, &ent));

                if (!ent)
                    break;

                /* If final path element or a directory. */
                if (final_element || ent->d_type == OEFS_DT_DIR)
                {
                    if (strcmp(ent->d_name, elements[i]) == 0)
                    {
                        current_ino = ent->d_ino;

                        if (final_element)
                        {
                            if (type_out)
                                *type_out = ent->d_type;
                        }
                        break;
                    }
                }
            }

            if (!current_ino)
                OEFS_RAISE(ENOENT);

            OEFS_CHECK(_oefs_closedir(dir));
            dir = NULL;
        }
    }

    if (dir_ino_out)
        *dir_ino_out = dir_ino;

    *ino_out = current_ino;

done:

    if (dir)
        _oefs_closedir(dir);

    return err;
}

/*
**==============================================================================
**
** Public interface:
**
**==============================================================================
*/

static int _oefs_read(
    oefs_file_t* file,
    void* data,
    size_t size,
    ssize_t* nread)
{
    int err = 0;
    oefs_t* oefs = _oefs_from_file(file);
    uint32_t first;
    uint32_t i;
    uint32_t remaining;
    uint8_t* ptr = (uint8_t*)data;

    if (nread)
        *nread = 0;

    /* Check parameters */
    if (!_valid_file(file) || !file->oefs || (!data && size))
        OEFS_RAISE(EINVAL);

    /* The index of the first block to read. */
    first = file->offset / OEFS_BLOCK_SIZE;

    /* The number of bytes remaining to be read */
    remaining = size;

    /* Read the data block-by-block */
    for (i = first; i < file->blknos.size && remaining > 0 && !file->eof; i++)
    {
        oefs_blk_t blk;
        uint32_t offset;

        OEFS_CHECK(_read_block(file->oefs, file->blknos.data[i], &blk));

        /* The offset of the data within this block */
        offset = file->offset % OEFS_BLOCK_SIZE;

        /* Copy the minimum of these to the caller's buffer:
         *     remaining
         *     block-bytes-remaining
         *     file-bytes-remaining
         */
        {
            uint32_t copy_bytes;

            /* Bytes to copy to user buffer */
            copy_bytes = remaining;

            /* Reduce copy_bytes to bytes remaining in the block. */
            {
                uint32_t block_bytes_remaining = OEFS_BLOCK_SIZE - offset;

                if (block_bytes_remaining < copy_bytes)
                    copy_bytes = block_bytes_remaining;
            }

            /* Reduce copy_bytes to bytes remaining in the file. */
            {
                uint32_t file_bytes_remaining =
                    file->inode.i_size - file->offset;

                if (file_bytes_remaining < copy_bytes)
                {
                    copy_bytes = file_bytes_remaining;
                    file->eof = true;
                }
            }

            /* Copy data to user buffer */
            memcpy(ptr, blk.data + offset, copy_bytes);

            /* Advance to the next data to write. */
            remaining -= copy_bytes;
            ptr += copy_bytes;

            /* Advance the file offset. */
            file->offset += copy_bytes;
        }
    }

    /* Calculate number of bytes read */
    *nread = size - remaining;

done:

    if (_flush(oefs) != 0)
        return EIO;

    return err;
}

static int _oefs_write(
    oefs_file_t* file,
    const void* data,
    size_t size,
    ssize_t* nwritten)
{
    int err = 0;
    oefs_t* oefs = _oefs_from_file(file);
    uint32_t first;
    uint32_t i;
    uint32_t remaining;
    const uint8_t* ptr = (uint8_t*)data;
    uint32_t new_file_size;

    if (nwritten)
        *nwritten = 0;

    /* Check parameters */
    if (!_valid_file(file) || !file->oefs || (!data && size) || !nwritten)
        OEFS_RAISE(EINVAL);

    assert(_sane_file(file));

    if (!_sane_file(file))
        OEFS_RAISE(EINVAL);

    oefs = file->oefs;

    /* The index of the first block to write. */
    first = file->offset / OEFS_BLOCK_SIZE;

    /* The number of bytes remaining to be read */
    remaining = size;

    /* Calculate the new size of the file (bigger or unchanged). */
    {
        new_file_size = file->inode.i_size;

        if (new_file_size < file->offset + size)
            new_file_size = file->offset + size;
    }

    /* Write the data block-by-block */
    for (i = first; i < file->blknos.size && remaining; i++)
    {
        oefs_blk_t blk;
        uint32_t offset;

        OEFS_CHECK(_read_block(file->oefs, file->blknos.data[i], &blk));

        /* The offset of the data within this block */
        offset = file->offset % OEFS_BLOCK_SIZE;

        /* Copy the minimum of these to the caller's buffer:
         *     remaining
         *     block-bytes-remaining
         *     file-bytes-remaining
         */
        {
            uint32_t copy_bytes;

            /* Bytes to copy to user buffer */
            copy_bytes = remaining;

            /* Reduce copy_bytes to bytes remaining in the block. */
            {
                uint32_t block_bytes_remaining = OEFS_BLOCK_SIZE - offset;

                if (block_bytes_remaining < copy_bytes)
                    copy_bytes = block_bytes_remaining;
            }

            /* Reduce copy_bytes to bytes remaining in the file. */
            {
                uint32_t file_bytes_remaining = new_file_size - file->offset;

                if (file_bytes_remaining < copy_bytes)
                {
                    copy_bytes = file_bytes_remaining;
                }
            }

            /* Copy user data into block. */
            memcpy(blk.data + offset, ptr, copy_bytes);

            /* Advance to next data to write. */
            ptr += copy_bytes;
            remaining -= copy_bytes;

            /* Advance the file position. */
            file->offset += copy_bytes;
        }

        /* Rewrite the block. */
        OEFS_CHECK(_oefs_write_block(file->oefs, file->blknos.data[i], &blk));
    }

    /* Append remaining data to the file. */
    if (remaining)
    {
        oe_bufu32_t blknos = OE_BUF_U32_INITIALIZER;

        /* Write the new blocks. */
        OEFS_CHECK(_oefs_write_data(file->oefs, ptr, remaining, &blknos));

        /* Append these block numbers to the block-numbers chain. */
        OEFS_CHECK(_append_block_chain(file, &blknos));

        /* Update the inode's total_blocks */
        file->inode.i_nblks += blknos.size;

        /* Append these block numbers to the file. */
        if (oe_bufu32_append(&file->blknos, blknos.data, blknos.size) != 0)
            OEFS_RAISE(ENOMEM);

        oe_bufu32_release(&blknos);

        file->offset += remaining;
        remaining = 0;
    }

    /* Update the file size. */
    file->inode.i_size = new_file_size;

    /* Flush the inode to disk. */
    OEFS_CHECK(
        _oefs_write_block(
            file->oefs, file->ino, (const oefs_blk_t*)&file->inode));

    /* Calculate number of bytes written */
    *nwritten = size - remaining;

done:

    if (_flush(oefs) != 0)
        return EIO;

    return err;
}

static int _oefs_close(oefs_file_t* file)
{
    int err = 0;
    oefs_t* oefs = _oefs_from_file(file);

    if (!_valid_file(file))
        OEFS_RAISE(EINVAL);

    oe_bufu32_release(&file->blknos);
    oe_bufu32_release(&file->bnode_blknos);
    memset(file, 0, sizeof(oefs_file_t));
    free(file);

done:

    if (_flush(oefs) != 0)
        return EIO;

    return err;
}

static int _oefs_release(oefs_t* oefs)
{
    int err = 0;

    if (!_valid_oefs(oefs) || !oefs->bitmap.read || !oefs->bitmap_copy)
        OEFS_RAISE(EINVAL);

    oefs->dev->release(oefs->dev);
    free(_bitmap_write(oefs));
    free(oefs->bitmap_copy);
    memset(oefs, 0, sizeof(oefs_t));
    free(oefs);

done:

    return err;
}

static int _oefs_opendir(oefs_t* oefs, const char* path, oefs_dir_t** dir)
{
    int err = 0;
    uint32_t ino;

    if (dir)
        *dir = NULL;

    if (!_valid_oefs(oefs) || !path || !dir)
        OEFS_RAISE(EINVAL);

    OEFS_CHECK(_path_to_ino(oefs, path, NULL, &ino, NULL));
    OEFS_CHECK(_opendir_by_ino(oefs, ino, dir));

done:

    if (_flush(oefs) != 0)
        return EIO;

    return err;
}

static int _oefs_readdir(oefs_dir_t* dir, oefs_dirent_t** ent)
{
    int err = 0;
    oefs_t* oefs = _oefs_from_dir(dir);
    ssize_t nread = 0;

    if (ent)
        *ent = NULL;

    if (!_valid_dir(dir) || !dir->file)
        OEFS_RAISE(EINVAL);

    OEFS_CHECK(_oefs_read(dir->file, &dir->ent, sizeof(oefs_dirent_t), &nread));

    /* Check for end of file. */
    if (nread == 0)
        OEFS_RAISE(0);

    /* Check for an illegal read size. */
    if (nread != sizeof(oefs_dirent_t))
        OEFS_RAISE(EBADF);

    *ent = &dir->ent;

done:

    if (_flush(oefs) != 0)
        return EIO;

    return err;
}

static int _oefs_closedir(oefs_dir_t* dir)
{
    int err = 0;
    oefs_t* oefs = _oefs_from_dir(dir);

    if (!_valid_dir(dir) || !dir->file)
        OEFS_RAISE(EINVAL);

    _oefs_close(dir->file);
    dir->file = NULL;
    free(dir);

done:

    if (_flush(oefs) != 0)
        return EIO;

    return err;
}

static int _oefs_open(
    oefs_t* oefs,
    const char* path,
    int flags,
    uint32_t mode,
    oefs_file_t** file_out)
{
    int err = 0;
    int tmp_err;
    oefs_file_t* file = NULL;
    uint32_t ino = 0;
    uint8_t type;

    _begin(oefs);

    if (file_out)
        *file_out = NULL;

    if (!_valid_oefs(oefs) || !path || !file_out)
        OEFS_RAISE(EINVAL);

    /* Reject unsupported flags. */
    {
        int supported_flags = 0;

        supported_flags |= OEFS_O_RDONLY;
        supported_flags |= OEFS_O_RDWR;
        supported_flags |= OEFS_O_WRONLY;
        supported_flags |= OEFS_O_CREAT;
        supported_flags |= OEFS_O_EXCL;
        supported_flags |= OEFS_O_TRUNC;
        supported_flags |= OEFS_O_APPEND;
        supported_flags |= OEFS_O_DIRECTORY;
        supported_flags |= OEFS_O_CLOEXEC;

        if (flags & ~supported_flags)
            OEFS_RAISE(EINVAL);
    }

    /* Get the file's inode (only if it exists). */
    tmp_err = _path_to_ino(oefs, path, NULL, &ino, &type);

    /* If the file already exists. */
    if (tmp_err == 0)
    {
        if ((flags & OEFS_O_CREAT && flags & OEFS_O_EXCL))
            OEFS_RAISE(EEXIST);

        if ((flags & OEFS_O_DIRECTORY) && (type != OEFS_DT_DIR))
            OEFS_RAISE(ENOTDIR);

        OEFS_CHECK(_open(oefs, ino, &file));

        if (flags & OEFS_O_TRUNC)
            OEFS_CHECK(_truncate(file, 0));

        if (flags & OEFS_O_APPEND)
            file->offset = file->inode.i_size;
    }
    else if (tmp_err == ENOENT)
    {
        char dirname[OEFS_PATH_MAX];
        char basename[OEFS_PATH_MAX];
        uint32_t dir_ino;

        if (!(flags & OEFS_O_CREAT))
            OEFS_RAISE(ENOENT);

        /* Split the path into parent directory and file name */
        OEFS_CHECK(_split_path(path, dirname, basename));

        /* Get the inode of the parent directory. */
        OEFS_CHECK(_path_to_ino(oefs, dirname, NULL, &dir_ino, NULL));

        /* Create the new file. */
        OEFS_CHECK(_creat(oefs, dir_ino, basename, OEFS_DT_REG, &ino));

        /* Open the new file. */
        OEFS_CHECK(_open(oefs, ino, &file));
    }
    else
    {
        OEFS_RAISE(tmp_err);
    }

    /* Save the access flags. */
    file->access = (flags & (OEFS_O_RDONLY | OEFS_O_RDWR | OEFS_O_WRONLY));

    *file_out = file;

done:

    if (_flush(oefs) != 0)
        return EIO;

    _end(oefs);

    return err;
}

static int _oefs_mkdir(oefs_t* oefs, const char* path, uint32_t mode)
{
    int err = 0;
    char dirname[OEFS_PATH_MAX];
    char basename[OEFS_PATH_MAX];
    uint32_t dir_ino;
    uint32_t ino;
    oefs_file_t* file = NULL;

    _begin(oefs);

    if (!_valid_oefs(oefs) || !path)
        OEFS_RAISE(EINVAL);

    /* Split the path into parent path and final component. */
    OEFS_CHECK(_split_path(path, dirname, basename));

    /* Get the inode of the parent directory. */
    OEFS_CHECK(_path_to_ino(oefs, dirname, NULL, &dir_ino, NULL));

    /* Create the new directory file. */
    OEFS_CHECK(_creat(oefs, dir_ino, basename, OEFS_DT_DIR, &ino));

    /* Open the newly created directory */
    OEFS_CHECK(_open(oefs, ino, &file));

    /* Write the empty directory contents. */
    {
        oefs_dirent_t dirents[2];
        ssize_t nwritten;

        /* Initialize the ".." directory. */
        dirents[0].d_ino = dir_ino;
        dirents[0].d_off = 0;
        dirents[0].d_reclen = sizeof(oefs_dirent_t);
        dirents[0].d_type = OEFS_DT_DIR;
        strcpy(dirents[0].d_name, "..");

        /* Initialize the "." directory. */
        dirents[1].d_ino = ino;
        dirents[1].d_off = sizeof(oefs_dirent_t);
        dirents[1].d_reclen = sizeof(oefs_dirent_t);
        dirents[1].d_type = OEFS_DT_DIR;
        strcpy(dirents[1].d_name, ".");

        OEFS_CHECK(_oefs_write(file, &dirents, sizeof(dirents), &nwritten));

        if (nwritten != sizeof(dirents))
            OEFS_RAISE(EIO);
    }

done:

    if (file)
        _oefs_close(file);

    if (_flush(oefs) != 0)
        return EIO;

    _end(oefs);

    return err;
}

#if 0
static int _oefs_creat(
    oefs_t* oefs,
    const char* path,
    uint32_t mode,
    oefs_file_t** file_out)
{
    int err = 0;
    int flags = OEFS_O_CREAT | OEFS_O_WRONLY | OEFS_O_TRUNC;

    _begin(oefs);

    OEFS_CHECK(_oefs_open(oefs, path, flags, mode, file_out));

done:

    _end(oefs);

    return err;
}
#endif

static int _oefs_link(oefs_t* oefs, const char* old_path, const char* new_path)
{
    int err = 0;
    uint32_t ino;
    char dirname[OEFS_PATH_MAX];
    char basename[OEFS_PATH_MAX];
    uint32_t dir_ino;
    oefs_file_t* dir = NULL;
    uint32_t release_ino = 0;

    _begin(oefs);

    if (!old_path || !new_path)
        OEFS_RAISE(EINVAL);

    /* Get the inode number of the old path. */
    {
        uint8_t type;

        OEFS_CHECK(_path_to_ino(oefs, old_path, NULL, &ino, &type));

        /* Only regular files can be linked. */
        if (type != OEFS_DT_REG)
            OEFS_RAISE(EINVAL);
    }

    /* Split the new path. */
    OEFS_CHECK(_split_path(new_path, dirname, basename));

    /* Open the destination directory. */
    {
        uint8_t type;

        OEFS_CHECK(_path_to_ino(oefs, dirname, NULL, &dir_ino, &type));

        if (type != OEFS_DT_DIR)
            OEFS_RAISE(EINVAL);

        OEFS_CHECK(_open(oefs, dir_ino, &dir));
    }

    /* Replace the destination file if it already exists. */
    for (;;)
    {
        oefs_dirent_t ent;
        ssize_t n;

        OEFS_CHECK(_oefs_read(dir, &ent, sizeof(ent), &n));

        if (n == 0)
            break;

        if (n != sizeof(ent))
        {
            OEFS_RAISE(EBADF);
        }

        if (strcmp(ent.d_name, basename) == 0)
        {
            release_ino = ent.d_ino;

            if (ent.d_type != OEFS_DT_REG)
                OEFS_RAISE(EINVAL);

            OEFS_CHECK(_oefs_lseek(dir, -sizeof(ent), OEFS_SEEK_CUR, NULL));
            ent.d_ino = ino;
            OEFS_CHECK(_oefs_write(dir, &ent, sizeof(ent), &n));

            break;
        }
    }

    /* Append the entry to the directory. */
    if (!release_ino)
    {
        oefs_dirent_t ent;
        ssize_t n;

        memset(&ent, 0, sizeof(ent));
        ent.d_ino = ino;
        ent.d_off = dir->offset;
        ent.d_reclen = sizeof(ent);
        ent.d_type = OEFS_DT_REG;
        strlcpy(ent.d_name, basename, sizeof(ent.d_name));

        OEFS_CHECK(_oefs_write(dir, &ent, sizeof(ent), &n));

        if (n != sizeof(ent))
            OEFS_RAISE(EIO);
    }

    /* Increment the number of links to this file. */
    {
        oefs_inode_t inode;

        OEFS_CHECK(_load_inode(oefs, ino, &inode));
        inode.i_links++;
        OEFS_CHECK(_oefs_write_block(oefs, ino, (const oefs_blk_t*)&inode));
    }

    /* Remove the destination file if it existed above. */
    if (release_ino)
        OEFS_CHECK(_release_inode(oefs, release_ino));

done:

    if (dir)
        _oefs_close(dir);

    if (_flush(oefs) != 0)
        return EIO;

    _end(oefs);

    return err;
}

static int _oefs_rename(
    oefs_t* oefs,
    const char* old_path,
    const char* new_path)
{
    int err = 0;

    _begin(oefs);

    if (!_valid_oefs(oefs) || !old_path || !new_path)
        OEFS_RAISE(EINVAL);

    OEFS_CHECK(_oefs_link(oefs, old_path, new_path));
    OEFS_CHECK(_oefs_unlink(oefs, old_path));

done:

    if (_flush(oefs) != 0)
        return EIO;

    _end(oefs);

    return err;
}

static int _oefs_unlink(oefs_t* oefs, const char* path)
{
    int err = 0;
    uint32_t dir_ino;
    uint32_t ino;
    uint8_t type;
    char dirname[OEFS_PATH_MAX];
    char basename[OEFS_PATH_MAX];

    _begin(oefs);

    if (!_valid_oefs(oefs) || !path)
        OEFS_RAISE(EINVAL);

    OEFS_CHECK(_path_to_ino(oefs, path, &dir_ino, &ino, &type));

    /* Only regular files can be removed. */
    if (type != OEFS_DT_REG)
        OEFS_RAISE(EINVAL);

    OEFS_CHECK(_split_path(path, dirname, basename));
    OEFS_CHECK(_unlink(oefs, dir_ino, ino, basename));

done:

    if (_flush(oefs) != 0)
        return EIO;

    _end(oefs);

    return err;
}

#if 0
static int _oefs_truncate(oefs_t* oefs, const char* path, ssize_t length)
{
    int err = 0;
    oefs_file_t* file = NULL;
    uint32_t ino;
    uint8_t type;

    _begin(oefs);

    if (!_valid_oefs(oefs) || !path)
        OEFS_RAISE(EINVAL);

    OEFS_CHECK(_path_to_ino(oefs, path, NULL, &ino, &type));

    /* Only regular files can be truncated. */
    if (type != OEFS_DT_REG)
        OEFS_RAISE(EINVAL);

    OEFS_CHECK(_open(oefs, ino, &file));
    OEFS_CHECK(_truncate(file, length));

done:

    if (file)
        _oefs_close(file);

    if (_flush(oefs) != 0)
        return EIO;

    _end(oefs);

    return err;
}
#endif

static int _oefs_rmdir(oefs_t* oefs, const char* path)
{
    int err = 0;
    uint32_t dir_ino;
    uint32_t ino;
    uint8_t type;
    char dirname[OEFS_PATH_MAX];
    char basename[OEFS_PATH_MAX];

    _begin(oefs);

    if (!_valid_oefs(oefs) || !path)
        OEFS_RAISE(EINVAL);

    OEFS_CHECK(_path_to_ino(oefs, path, &dir_ino, &ino, &type));

    /* The path must refer to a directory. */
    if (type != OEFS_DT_DIR)
        OEFS_RAISE(ENOTDIR);

    /* The inode must be a directory and the directory must be empty. */
    {
        oefs_inode_t inode;

        OEFS_CHECK(_load_inode(oefs, ino, &inode));

        if (!(inode.i_mode & OEFS_S_IFDIR))
            OEFS_RAISE(ENOTDIR);

        /* The directory must contain two entries: "." and ".." */
        if (inode.i_size != 2 * sizeof(oefs_dirent_t))
            OEFS_RAISE(ENOTEMPTY);
    }

    OEFS_CHECK(_split_path(path, dirname, basename));
    OEFS_CHECK(_unlink(oefs, dir_ino, ino, basename));

done:

    if (_flush(oefs) != 0)
        return EIO;

    _end(oefs);

    return err;
}

static int _oefs_stat(oefs_t* oefs, const char* path, oefs_stat_t* stat)
{
    int err = 0;
    oefs_inode_t inode;
    uint32_t ino;
    uint8_t type;

    if (stat)
        memset(stat, 0, sizeof(oefs_stat_t));

    if (!_valid_oefs(oefs) || !path || !stat)
        OEFS_RAISE(EINVAL);

    OEFS_CHECK(_path_to_ino(oefs, path, NULL, &ino, &type));
    OEFS_CHECK(_load_inode(oefs, ino, &inode));

    stat->st_dev = 0;
    stat->st_ino = ino;
    stat->st_mode = inode.i_mode;
    stat->__st_padding1 = 0;
    stat->st_nlink = inode.i_links;
    stat->st_uid = inode.i_uid;
    stat->st_gid = inode.i_gid;
    stat->st_rdev = 0;
    stat->st_size = inode.i_size;
    stat->st_blksize = OEFS_BLOCK_SIZE;
    stat->st_blocks = inode.i_nblks;
    stat->__st_padding2 = 0;
    stat->st_atim.tv_sec = inode.i_atime;
    stat->st_atim.tv_nsec = 0;
    stat->st_mtim.tv_sec = inode.i_mtime;
    stat->st_mtim.tv_nsec = 0;
    stat->st_ctim.tv_sec = inode.i_ctime;
    stat->st_ctim.tv_nsec = 0;

done:

    if (_flush(oefs) != 0)
        return EIO;

    return err;
}

static int _oefs_lseek(
    oefs_file_t* file,
    ssize_t offset,
    int whence,
    ssize_t* offset_out)
{
    oefs_t* oefs = _oefs_from_file(file);
    int err = 0;
    ssize_t new_offset;

    if (offset_out)
        *offset_out = 0;

    if (!_valid_file(file))
        OEFS_RAISE(EINVAL);

    switch (whence)
    {
        case OEFS_SEEK_SET:
        {
            new_offset = offset;
            break;
        }
        case OEFS_SEEK_CUR:
        {
            new_offset = file->offset + offset;
            break;
        }
        case OEFS_SEEK_END:
        {
            new_offset = file->inode.i_size + offset;
            break;
        }
        default:
        {
            OEFS_RAISE(EINVAL);
        }
    }

    /* Check whether the new offset if out of range. */
    if (new_offset < 0 || new_offset > file->inode.i_size)
        OEFS_RAISE(EINVAL);

    file->offset = new_offset;

    if (offset_out)
        *offset_out = new_offset;

done:

    if (_flush(oefs) != 0)
        return EIO;

    return err;
}

static void _initialize_header_block(
    oefs_header_block_t* hb,
    const uint8_t key_id[OEFS_KEY_SIZE])
{
    memset(hb, 0, sizeof(oefs_header_block_t));
    hb->h_magic = HEADER_BLOCK_MAGIC;
    memcpy(hb->h_key_id, key_id, sizeof(hb->h_key_id));
}

static int _oefs_mkfs(
    oefs_blkdev_t* host_dev,
    oefs_blkdev_t* dev,
    size_t nblks,
    uint8_t key_id[OEFS_KEY_SIZE])
{
    int err = 0;
    size_t num_bitmap_blocks;
    uint32_t blkno = 0;
    oefs_blk_t empty_block;
    oefs_header_block_t hb;
    size_t num_data_blocks;

    if (dev)
        dev->begin(dev);

    if (!dev || nblks < 2 || !oefs_is_pow_of_2(nblks))
        OEFS_RAISE(EINVAL);

    /* Initialize an empty block. */
    memset(&empty_block, 0, sizeof(empty_block));

    /* Calculate the number of bitmap blocks. */
    num_bitmap_blocks = _num_bitmap_blocks(nblks);

    /* Write empty block one. */
    if (dev->put(dev, blkno++, &empty_block) != 0)
        OEFS_RAISE(EIO);

    /* Write empty block two. */
    if (dev->put(dev, blkno++, &empty_block) != 0)
        OEFS_RAISE(EIO);

    /* Write the plain-text header block. */
    {
        _initialize_header_block(&hb, key_id);

        if (host_dev->put(host_dev, blkno++, (const oefs_blk_t*)&hb) != 0)
            OEFS_RAISE(EIO);
    }

    /* Calculate the number of data blocks. */
    {
        num_data_blocks = nblks;

        /* Exclude the first two empty blocks. */
        num_data_blocks -= 2;

        /* Exclude the header block. */
        num_data_blocks--;

        /* Exclude the superblock. */
        num_data_blocks--;

        /* Exclude the bitmap blocks. */
        num_data_blocks -= num_bitmap_blocks;
    }

    /* Write the super block. */
    {
        oefs_super_block_t sb;
        oefs_sha256_t hash;

        memset(&sb, 0, sizeof(sb));

        sb.s_magic = SUPER_BLOCK_MAGIC;
        sb.s_nblks = nblks;
        sb.s_num_data_blocks = num_data_blocks;
        sb.s_free_data_blocks = num_data_blocks - 3;

        if (oefs_sha256(&hash, &hb, sizeof(hb)) != 0)
            OEFS_RAISE(EIO);

        memcpy(sb.s_header_hash, &hash, sizeof(sb.s_header_hash));

        if (dev->put(dev, blkno++, (const oefs_blk_t*)&sb) != 0)
            OEFS_RAISE(EIO);
    }

    /* Write the first bitmap block. */
    {
        oefs_blk_t blk;

        memset(&blk, 0, sizeof(blk));

        /* Set the bit for the root inode. */
        _set_bit(blk.data, sizeof(blk), 0);

        /* Set the bits for the root directory. */
        _set_bit(blk.data, sizeof(blk), 1);
        _set_bit(blk.data, sizeof(blk), 2);

        /* Write the block. */
        if (dev->put(dev, blkno++, &blk) != 0)
            OEFS_RAISE(EIO);
    }

    /* Write the subsequent (empty) bitmap blocks. */
    for (size_t i = 1; i < num_bitmap_blocks; i++)
    {
        if (dev->put(dev, blkno++, &empty_block) != 0)
            OEFS_RAISE(EIO);
    }

    /* Write the root directory inode. */
    {
        oefs_inode_t root_inode = {
            .i_magic = INODE_MAGIC,
            .i_mode = OEFS_S_DIR_DEFAULT,
            .i_uid = 0,
            .i_gid = 0,
            .i_links = 1,
            .i_size = 2 * sizeof(oefs_dirent_t),
            .i_atime = 0,
            .i_ctime = 0,
            .i_mtime = 0,
            .i_dtime = 0,
            .i_nblks = 2,
            .i_next = 0,
            .i_reserved = {0},
            .i_blocks = {2, 3},
        };

        if (dev->put(dev, blkno++, (const oefs_blk_t*)&root_inode) != 0)
            OEFS_RAISE(EIO);
    }

    /* Write the ".." and "." directory entries for the root directory.  */
    {
        oefs_blk_t blocks[2];

        oefs_dirent_t dir_entries[] = {
            {.d_ino = ROOT_INO,
             .d_off = 0 * sizeof(oefs_dirent_t),
             .d_reclen = sizeof(oefs_dirent_t),
             .d_type = OEFS_DT_DIR,
             .d_name = ".."

            },
            {.d_ino = ROOT_INO,
             .d_off = 1 * sizeof(oefs_dirent_t),
             .d_reclen = sizeof(oefs_dirent_t),
             .d_type = OEFS_DT_DIR,
             .d_name = "."},
        };

        memset(blocks, 0, sizeof(blocks));
        memcpy(blocks, dir_entries, sizeof(dir_entries));

        if (dev->put(dev, blkno++, &blocks[0]) != 0)
            OEFS_RAISE(EIO);

        if (dev->put(dev, blkno++, &blocks[1]) != 0)
            OEFS_RAISE(EIO);
    }

    /* Count the remaining empty data blocks. */
    blkno += nblks - 3;

    /* Cross check the actual size against computed size. */
    {
        size_t size;
        OEFS_CHECK(_oefs_size(nblks, &size));

        if (size != (blkno * OEFS_BLOCK_SIZE))
            OEFS_RAISE(EIO);
    }

done:

    if (dev)
        dev->begin(dev);

    return err;
}

static int _oefs_size(size_t nblks, size_t* size)
{
    int err = 0;
    size_t total_blocks = 0;

    if (size)
        *size = 0;

    if (nblks < 2 || !oefs_is_pow_of_2(nblks) || !size)
        goto done;

    /* Count the first two empty blocks. */
    total_blocks += 2;

    /* Count the header block. */
    total_blocks++;

    /* Count the super block */
    total_blocks++;

    /* Count the bitmap blocks. */
    total_blocks += oefs_round_to_multiple(nblks, BITS_PER_BLK) / BITS_PER_BLK;

    /* Count the data blocks. */
    total_blocks += nblks;

    if (size)
        *size = total_blocks * OEFS_BLOCK_SIZE;

    goto done;

done:
    return err;
}

static int _oefs_initialize(
    oefs_t** oefs_out,
    oefs_blkdev_t* host_dev,
    oefs_blkdev_t* dev,
    const oefs_header_block_t* hb)
{
    int err = 0;
    size_t nblks;
    uint8_t* bitmap = NULL;
    uint8_t* bitmap_copy = NULL;
    oefs_t* oefs = NULL;
    oefs_super_block_t sb;

    if (oefs_out)
        *oefs_out = NULL;

    if (!oefs_out || !dev)
        OEFS_RAISE(EINVAL);

    if (!(oefs = calloc(1, sizeof(oefs_t))))
        OEFS_RAISE(ENOMEM);

    /* Save pointer to device (caller is responsible for releasing it). */
    oefs->dev = dev;

    /* Read the super block from the file system. */
    if (dev->get(dev, SUPER_BLOCK_PHYSICAL_BLKNO, (oefs_blk_t*)&sb) != 0)
        OEFS_RAISE(EIO);

    /* Check that the hash of the header matches the one in the super block. */
    {
        oefs_sha256_t hash;

        if (oefs_sha256(&hash, hb, sizeof(oefs_header_block_t)) != 0)
            OEFS_RAISE(EIO);

        if (memcmp(sb.s_header_hash, &hash, sizeof(sb.s_header_hash)) != 0)
            OEFS_RAISE(EIO);
    }

    /* Check the super block magic number. */
    if (sb.s_magic != SUPER_BLOCK_MAGIC)
        OEFS_RAISE(EIO);

    oefs->sb.__write = sb;

    /* Make copy of super block. */
    memcpy(&oefs->sb_copy, &oefs->sb, sizeof(oefs->sb_copy));

    /* Get the number of physical blocks. */
    nblks = oefs->sb.read.s_nblks;

    /* Check that the number of blocks is correct. */
    if (!nblks)
        OEFS_RAISE(EIO);

    /* Allocate space for the block bitmap. */
    {
        /* Calculate the number of bitmap blocks. */
        size_t num_bitmap_blocks = _num_bitmap_blocks(nblks);

        /* Calculate the size of the bitmap. */
        size_t bitmap_size = num_bitmap_blocks * OEFS_BLOCK_SIZE;

        /* Allocate the bitmap. */
        if (!(bitmap = calloc(1, bitmap_size)))
            OEFS_RAISE(ENOMEM);

        /* Allocate the bitmap. */
        if (!(bitmap_copy = calloc(1, bitmap_size)))
            OEFS_RAISE(ENOMEM);

        /* Read the bitset into memory */
        for (size_t i = 0; i < num_bitmap_blocks; i++)
        {
            uint8_t* ptr = bitmap + (i * OEFS_BLOCK_SIZE);

            if (dev->get(dev, BITMAP_PHYSICAL_BLKNO + i, (oefs_blk_t*)ptr) != 0)
                OEFS_RAISE(EIO);
        }

        /* Make copy of the bitmap. */
        memcpy(bitmap_copy, bitmap, bitmap_size);

        /* The first three bits should always be set. These include:
         * (1) The root inode block.
         * (2) The first root directory block.
         * (3) The second root directory block.
         */
        if (!_test_bit(bitmap, bitmap_size, 0) ||
            !_test_bit(bitmap, bitmap_size, 1) ||
            !_test_bit(bitmap, bitmap_size, 2))
        {
            OEFS_RAISE(EIO);
        }

        oefs->bitmap.read = bitmap;
        oefs->bitmap_copy = bitmap_copy;
        oefs->bitmap_size = bitmap_size;
        oefs->nblks = nblks;
        bitmap = NULL;
        bitmap_copy = NULL;
    }

    /* Check the root inode. */
    {
        oefs_inode_t inode;

        OEFS_CHECK(_read_block(oefs, ROOT_INO, (oefs_blk_t*)&inode));

        if (inode.i_magic != INODE_MAGIC)
            OEFS_RAISE(EIO);
    }

    oefs->magic = OEFS_MAGIC;
    oefs->dev->add_ref(dev);

    *oefs_out = oefs;
    oefs = NULL;

done:

    if (oefs)
    {
        free(_bitmap_write(oefs));
        free(oefs->bitmap_copy);
        free(oefs);
    }

    if (bitmap)
        free(bitmap);

    if (bitmap_copy)
        free(bitmap_copy);

    return err;
}

int oefs_calculate_total_blocks(size_t nblks, size_t* total_nblks)
{
    int ret = -1;

    if (total_nblks)
        *total_nblks = 0;

    if (!total_nblks)
        goto done;

    /* There must be at least 8 blocks. */
    if (nblks < 8)
        goto done;

    /* The number of blocks must be a power of two. */
    if (!oefs_is_pow_of_2(nblks))
        goto done;

    /* Add extra blocks for the Merkle-tree overhead bytes. */
    {
        size_t extra_nblks;

#if defined(USE_MERKLE_BLKDDEV)
        if (oefs_merkle_blkdev_get_extra_blocks(nblks, &extra_nblks) != 0)
            goto done;
#else
        if (oefs_hash_list_blkdev_get_extra_blocks(nblks, &extra_nblks) != 0)
            goto done;
#endif

        nblks += extra_nblks;
    }

    /* Add extra blocks for the auth-crypto overhead bytes. */
    {
        size_t extra_nblks;

        if (oefs_auth_crypto_blkdev_get_extra_blocks(nblks, &extra_nblks) != 0)
            goto done;

        nblks += extra_nblks;
    }

    *total_nblks = nblks;

    ret = 0;

done:
    return ret;
}

static int _generate_sgx_key(sgx_key_t* key, uint8_t key_id[SGX_KEYID_SIZE])
{
    sgx_key_request_t kr;

    memset(&kr, 0, sizeof(sgx_key_request_t));

    kr.key_name = SGX_KEYSELECT_SEAL;
    kr.key_policy = SGX_KEYPOLICY_MRSIGNER;

    memset(&kr.cpu_svn, 0, sizeof(kr.cpu_svn));
    memset(&kr.isv_svn, 0, sizeof(kr.isv_svn));

    kr.attribute_mask.flags = OE_SEALKEY_DEFAULT_FLAGSMASK;
    kr.attribute_mask.xfrm = 0x0;
    kr.misc_attribute_mask = OE_SEALKEY_DEFAULT_MISCMASK;

    memcpy(kr.key_id, key_id, sizeof(kr.key_id));

    if (oe_get_key(&kr, key) != 0)
        return -1;

    return 0;
}

static int _generate_master_key(
    const uint8_t key_id[OEFS_KEY_SIZE],
    uint8_t key[OEFS_KEY_SIZE])
{
    sgx_key_t lo;
    sgx_key_t hi;
    uint8_t id_lo[SGX_KEYID_SIZE];
    uint8_t id_hi[SGX_KEYID_SIZE];

    memcpy(id_lo, &key_id[0], sizeof(id_lo));
    memcpy(id_hi, &key_id[SGX_KEYID_SIZE], sizeof(id_hi));

    if (_generate_sgx_key(&lo, id_lo) != 0)
        return -1;

    if (_generate_sgx_key(&hi, id_hi) != 0)
        return -1;

    memcpy(&key[0], &hi, sizeof(hi));
    memcpy(&key[sizeof(hi)], &lo, sizeof(lo));

    return 0;
}

static int _oefs_new(
    oefs_t** oefs_out,
    const char* source,
    uint32_t flags,
    const uint8_t key_in[OEFS_KEY_SIZE])
{
    int ret = -1;
    oefs_blkdev_t* host_dev = NULL;
    oefs_blkdev_t* crypto_dev = NULL;
    oefs_blkdev_t* cache_dev = NULL;
    oefs_blkdev_t* integ_dev = NULL;
    oefs_blkdev_t* dev = NULL;
    oefs_blkdev_t* next = NULL;
    oefs_t* oefs = NULL;
    size_t device_size;
    size_t n0;
    size_t n1;
    size_t n2;
    uint8_t key_id[OEFS_KEY_SIZE];
    uint8_t key[OEFS_KEY_SIZE];
    oefs_header_block_t hb;
    bool do_mkfs;

    if (oefs_out)
        *oefs_out = NULL;

    if (!oefs_out || !source)
        goto done;

    /* Open a host device. */
    if (oefs_host_blkdev_open(&host_dev, source) != 0)
        goto done;

    next = host_dev;

    /* Fetch header and set do_mkfs to true if header is zero-filled. */
    {
        if (host_dev->get(
                host_dev, HEADER_BLOCK_PHYSICAL_BLKNO, (oefs_blk_t*)&hb) != 0)
        {
            goto done;
        }

        if (_is_zero_filled(&hb, sizeof(hb)))
        {
            do_mkfs = true;
        }
        else
        {
            if (hb.h_magic != HEADER_BLOCK_MAGIC)
                goto done;

            memcpy(key_id, hb.h_key_id, sizeof(key_id));

            do_mkfs = false;
        }
    }

    if (key_in)
    {
        memset(key_id, 0, sizeof(key_id));
        memcpy(key, key_in, sizeof(key));
        _initialize_header_block(&hb, key_id);
    }
    else
    {
        /* Generate a key id or read it from the header. */
        if (do_mkfs)
        {
            if (oe_random(key_id, sizeof(key_id)) != OE_OK)
                goto done;

            _initialize_header_block(&hb, key_id);
        }

        /* Generate a key from the key id. */
        if (_generate_master_key(key_id, key) != 0)
            goto done;
    }

    /* Get the size of the host device. */
    {
        oefs_blkdev_stat_t stat;

        if (host_dev->stat(host_dev, &stat) != 0)
            goto done;

        device_size = stat.total_size;

        if ((device_size % OEFS_BLOCK_SIZE))
            goto done;

        n0 = device_size / OEFS_BLOCK_SIZE;
    }

    /* Block size assumption. */
    OE_STATIC_ASSERT(OEFS_BLOCK_SIZE >= 1024);

    /*
     * Calculate n0, n1, and n2 such that:
     *
     *                                    n2         n1         n0
     *                                     |          |          |
     *                                     V          V          V
     * +-----------------------------------+----------+----------+
     * | Data blocks                       | Merkle   | Auth     |
     * |                                   | Tree     | Crypto   |
     * |                                   | overhead | overhead |
     * |                                   | blocks   | blocks   |
     * +-----------------------------------+----------+----------+
     *
     */

    /* Calculate the largest power of two less than n0. */
    {
        size_t r = 1;
        n2 = r;

        while (r < n0)
        {
            n2 = r;
            r *= 2;
        }
    }

    /* Calculate extra bytes for the Merkle tree block device. */
    {
        size_t extra_nblks;

#if defined(USE_MERKLE_BLKDDEV)
        if (oefs_merkle_blkdev_get_extra_blocks(n2, &extra_nblks) != 0)
            goto done;
#else
        if (oefs_hash_list_blkdev_get_extra_blocks(n2, &extra_nblks) != 0)
            goto done;
#endif

        n1 = n2 + extra_nblks;
    }

    /* Calculate extra bytes for the auth-crypto block device. */
    {
        size_t extra_nblks;

        if (oefs_auth_crypto_blkdev_get_extra_blocks(n1, &extra_nblks) != 0)
            goto done;

        size_t end = n1 + extra_nblks;

        if (end > n0)
            goto done;
    }

    /* Create an authenticated crypto block device. */
    if ((flags & OEFS_FLAG_AUTH_CRYPTO))
    {
        bool initialize = do_mkfs;

        if (oefs_auth_crypto_blkdev_open(
                &crypto_dev, initialize, n1, key, next) != 0)
        {
            goto done;
        }

        next = crypto_dev;
    }

    if ((flags & OEFS_FLAG_CRYPTO))
    {
        if (oefs_crypto_blkdev_open(&crypto_dev, key, next) != 0)
            goto done;

        next = crypto_dev;
    }

    /* Create a Merkle block device. */
    if ((flags & OEFS_FLAG_INTEGRITY))
    {
        bool initialize = do_mkfs;

#if defined(USE_MERKLE_BLKDDEV)
        if (oefs_merkle_blkdev_open(&integ_dev, initialize, n2, next) != 0)
            goto done;
#else
        if (oefs_hash_list_blkdev_open(&integ_dev, initialize, n2, next) != 0)
            goto done;
#endif

        next = integ_dev;
    }

    /* Create a cache block device. */
    if ((flags & OEFS_FLAG_CACHING))
    {
        if (oefs_cache_blkdev_open(&cache_dev, next) != 0)
            goto done;

        next = cache_dev;
    }

    /* Set the top-level block device. */
    dev = next;

    if (do_mkfs)
    {
        if (_oefs_mkfs(host_dev, dev, n2, key_id) != 0)
            goto done;
    }

    if (_oefs_initialize(&oefs, host_dev, dev, &hb) != 0)
        goto done;

    *oefs_out = oefs;

    oefs = NULL;
    ret = 0;

done:

    if (host_dev)
        host_dev->release(host_dev);

    if (crypto_dev)
        crypto_dev->release(crypto_dev);

    if (cache_dev)
        cache_dev->release(cache_dev);

    if (integ_dev)
        integ_dev->release(integ_dev);

    if (oefs)
        _oefs_release(oefs);

    return ret;
}

#if 0
static int _oefs_ramfs_new(oefs_t** oefs_out, uint32_t flags, size_t nblks)
{
    int ret = -1;
    oefs_blkdev_t* dev = NULL;
    oefs_t* oefs = NULL;
    size_t size;

    if (oefs_out)
        *oefs_out = NULL;

    if (!oefs_out || nblks < 2 || !oefs_is_pow_of_2(nblks))
        goto done;

    /* Fail if any of these flags are set. */
    {
        if (flags & OEFS_FLAG_CRYPTO)
            goto done;

        if (flags & OEFS_FLAG_AUTH_CRYPTO)
            goto done;

        if (flags & OEFS_FLAG_INTEGRITY)
            goto done;

        if (flags & OEFS_FLAG_CACHING)
            goto done;
    }

    /* Calculate the size in bytes of the RAMFS block device. */
    if (_oefs_size(nblks, &size) != 0)
        goto done;

    /* Open the RAM block device. */
    if (oefs_ram_blkdev_open(&dev, size) != 0)
        goto done;

    if (flags & OEFS_FLAG_MKFS)
    {
        if (_oefs_mkfs(dev, nblks) != 0)
            goto done;
    }

    if (_oefs_initialize(&oefs, dev) != 0)
        goto done;

    *oefs_out = oefs;
    oefs = NULL;
    ret = 0;

done:

    if (dev)
        dev->release(dev);

    if (oefs)
        _oefs_release(oefs);

    return ret;
}
#endif

/*
**==============================================================================
**
** The oe_fs_t implementation of OEFS.
**
**==============================================================================
*/

/* This implementation overlays oe_fs_t. */
typedef struct _oefs_fs_impl
{
    oe_fs_base_t base;
    uint64_t magic;
    oefs_t* oefs;
    uint64_t padding[12];
} fs_impl_t;

OE_STATIC_ASSERT(sizeof(fs_impl_t) == sizeof(oe_fs_t));

#define FILE_BUF_SIZE (32 * 1024)

typedef struct _file
{
    FILE base;
    bool f_err;
    bool f_eof;
    oefs_file_t* oefs_file;
    uint8_t buf[FILE_BUF_SIZE];
    size_t buf_size;
} file_t;

typedef struct _dir
{
    DIR base;
    oefs_dir_t* oefs_dir;
    struct dirent entry;
} dir_t;

static oefs_t* _get_oefs(oe_fs_t* fs)
{
    fs_impl_t* impl = (fs_impl_t*)fs;

    if (!fs || oe_fs_magic(fs) != OE_FS_MAGIC)
        return NULL;

    if (impl->magic != OEFS_MAGIC || !impl->oefs)
        return NULL;

    if (impl->oefs->magic != OEFS_MAGIC)
        return NULL;

    return impl->oefs;
}

static oefs_file_t* _get_oefs_file(file_t* file)
{
    if (!file || file->base.magic != OE_FILE_MAGIC)
        return NULL;

    if (!_valid_file(file->oefs_file))
        return NULL;

    return file->oefs_file;
}

static oefs_dir_t* _get_oefs_dir(dir_t* dir)
{
    if (!dir || !_valid_dir(dir->oefs_dir))
        return NULL;

    return dir->oefs_dir;
}

static int _oefs_f_fflush(FILE* base);

static int _oefs_f_fclose(FILE* base)
{
    int ret = -1;
    file_t* file = (file_t*)base;
    oefs_file_t* oefs_file = _get_oefs_file(file);

    if (!oefs_file)
        goto done;

    if (_oefs_f_fflush(base) != 0)
        goto done;

    if (_oefs_close(oefs_file) != 0)
        goto done;

    free(base);

    ret = 0;

done:

    return ret;
}

static size_t _oefs_f_fread(void* ptr, size_t size, size_t nmemb, FILE* base)
{
    size_t ret = 0;
    file_t* file = (file_t*)base;
    oefs_file_t* oefs_file = _get_oefs_file(file);
    size_t n = size * nmemb;
    ssize_t nread;
    int err;

    if (!ptr || oe_safe_mul_u64(size, nmemb, &n) != OE_OK || !oefs_file)
    {
        file->f_err = true;
        goto done;
    }

    /* Flush the write buffer before reading. */
    if (_oefs_f_fflush(base) != 0)
        goto done;

    if ((err = _oefs_read(oefs_file, ptr, n, &nread)) != 0)
    {
        file->f_err = true;
        goto done;
    }

    ret = (size_t)nread;

    if (ret == 0)
        file->f_eof = true;

done:

    return ret;
}

static size_t _oefs_f_fwrite(
    const void* ptr,
    size_t size,
    size_t nmemb,
    FILE* base)
{
    size_t ret = 0;
    file_t* file = (file_t*)base;
    oefs_file_t* oefs_file = _get_oefs_file(file);
    size_t count = size * nmemb;
    ssize_t nwritten = 0;
    int err;
    const uint8_t* p = (const uint8_t*)ptr;

    if (!ptr || oe_safe_mul_u64(size, nmemb, &count) != OE_OK || !oefs_file)
    {
        file->f_err = true;
        goto done;
    }

    _begin(oefs_file->oefs);

    /* While there are more bytes to be written. */
    while (count > 0)
    {
        size_t remaining;
        size_t min;

        /* Calculate the number of bytes remaining in the write buffer. */
        remaining = FILE_BUF_SIZE - file->buf_size;

        /* Copy more caller data into the write buffer. */
        min = (count < remaining) ? count : remaining;
        memcpy(file->buf + file->buf_size, p, min);
        p += min;
        count -= min;
        file->buf_size += min;

        /* Flush the write buffer if it is full. */
        if (file->buf_size == FILE_BUF_SIZE)
        {
            ssize_t n;

            if ((err = _oefs_write(oefs_file, file->buf, file->buf_size, &n)) !=
                0)
            {
                file->f_err = true;
                goto done;
            }

            assert(n == file->buf_size);

            if (n != file->buf_size)
            {
                file->f_err = true;
                goto done;
            }

            file->buf_size = 0;
        }

        nwritten += min;
    }

    ret = (size_t)nwritten;

done:

    _end(oefs_file->oefs);

    return ret;
}

static int64_t _oefs_f_ftell(FILE* base)
{
    int64_t ret = -1;
    file_t* file = (file_t*)base;
    oefs_file_t* oefs_file = _get_oefs_file(file);
    int err;
    ssize_t offset;

    if (!oefs_file)
    {
        errno = EINVAL;
        goto done;
    }

    /* Flush the write buffer. */
    if (_oefs_f_fflush(base) != 0)
        goto done;

    if ((err = _oefs_lseek(oefs_file, 0, SEEK_CUR, &offset)) != 0)
    {
        errno = err;
        goto done;
    }

    ret = offset;

done:

    return ret;
}

static int _oefs_f_fseek(FILE* base, int64_t offset, int whence)
{
    int ret = -1;
    file_t* file = (file_t*)base;
    oefs_file_t* oefs_file = _get_oefs_file(file);
    int err;
    ssize_t new_offset;

    if (!oefs_file)
    {
        errno = EINVAL;
        goto done;
    }

    /* Flush the write buffer. */
    if (_oefs_f_fflush(base) != 0)
        goto done;

    if ((err = _oefs_lseek(oefs_file, offset, whence, &new_offset)) != 0)
    {
        errno = err;
        goto done;
    }

    ret = 0;

done:

    return ret;
}

static int _oefs_f_fflush(FILE* base)
{
    int ret = EOF;
    file_t* file = (file_t*)base;
    oefs_file_t* oefs_file = _get_oefs_file(file);
    int err;
    ssize_t n;

    if (!oefs_file)
    {
        errno = EINVAL;
        file->f_err = true;
        goto done;
    }

    _begin(oefs_file->oefs);

    if (file->buf_size)
    {
        if ((err = _oefs_write(oefs_file, file->buf, file->buf_size, &n)) != 0)
        {
            errno = err;
            file->f_err = true;
            goto done;
        }

        file->buf_size = 0;
    }

    ret = 0;

done:

    _end(oefs_file->oefs);

    return ret;
}

static int _oefs_f_ferror(FILE* base)
{
    int ret = 1;
    file_t* file = (file_t*)base;
    oefs_file_t* oefs_file = _get_oefs_file(file);

    if (!oefs_file)
        goto done;

    if (file->f_err)
        goto done;

    ret = 0;

done:

    return ret;
}

static int _oefs_f_feof(FILE* base)
{
    int ret = 1;
    file_t* file = (file_t*)base;
    oefs_file_t* oefs_file = _get_oefs_file(file);

    if (!oefs_file)
        goto done;

    if (file->f_eof)
        goto done;

    ret = 0;

done:

    return ret;
}

static void _oefs_f_clearerr(FILE* base)
{
    file_t* file = (file_t*)base;
    oefs_file_t* oefs_file = _get_oefs_file(file);

    if (oefs_file)
    {
        file->f_err = false;
        file->f_eof = false;
    }
}

static FILE* _oefs_fs_fopen(
    oe_fs_t* fs,
    const char* path,
    const char* mode_in,
    va_list ap)
{
    FILE* ret = NULL;
    oefs_t* oefs = _get_oefs(fs);
    int flags;
    char mode[8];
    file_t* file = NULL;
    oefs_file_t* oefs_file = NULL;

    if (!oefs || !path || !mode_in || strlen(mode_in) >= sizeof(mode))
    {
        errno = EINVAL;
        goto done;
    }

    /* Strip 'b' from the mode parameter. */
    {
        char* out = mode;
        *out = '\0';

        for (const char* in = mode_in; *in; in++)
        {
            if (*in != 'b')
            {
                *out++ = *in;
                *out = '\0';
            }
        }
    }

    if (strcmp(mode, "r") == 0)
    {
        flags = OEFS_O_RDONLY;
    }
    else if (strcmp(mode, "r+") == 0)
    {
        flags = OEFS_O_RDWR;
    }
    else if (strcmp(mode, "w") == 0)
    {
        flags = OEFS_O_WRONLY | OEFS_O_TRUNC | OEFS_O_CREAT;
    }
    else if (strcmp(mode, "w+") == 0)
    {
        flags = OEFS_O_RDWR | OEFS_O_TRUNC | OEFS_O_CREAT;
    }
    else if (strcmp(mode, "a") == 0)
    {
        flags = OEFS_O_WRONLY | OEFS_O_APPEND | OEFS_O_CREAT;
    }
    else if (strcmp(mode, "a+") == 0)
    {
        flags = OEFS_O_RDWR | OEFS_O_APPEND | OEFS_O_CREAT;
    }
    else
    {
        errno = EINVAL;
        goto done;
    }

    /* Create the file object. */
    if (!(file = calloc(1, sizeof(file_t))))
    {
        errno = ENOMEM;
        goto done;
    }

    /* Open the OEFS file. */
    {
        int err = _oefs_open(oefs, path, flags, DEFAULT_MODE, &oefs_file);

        if (err != 0)
        {
            errno = err;
            goto done;
        }
    }

    file->base.magic = OE_FILE_MAGIC;
    file->base.f_fclose = _oefs_f_fclose;
    file->base.f_fread = _oefs_f_fread;
    file->base.f_fwrite = _oefs_f_fwrite;
    file->base.f_ftell = _oefs_f_ftell;
    file->base.f_fseek = _oefs_f_fseek;
    file->base.f_fflush = _oefs_f_fflush;
    file->base.f_ferror = _oefs_f_ferror;
    file->base.f_feof = _oefs_f_feof;
    file->base.f_clearerr = _oefs_f_clearerr;
    file->oefs_file = oefs_file;
    ret = &file->base;
    file = NULL;
    oefs_file = NULL;

done:

    if (file)
        free(file);

    if (oefs_file)
        _oefs_close(oefs_file);

    return ret;
}

static struct dirent* _oefs_d_readdir(DIR* base)
{
    struct dirent* ret = NULL;
    dir_t* dir = (dir_t*)base;
    oefs_dir_t* oefs_dir = _get_oefs_dir(dir);
    int err;
    oefs_dirent_t* entry;

    if (!oefs_dir)
    {
        errno = EINVAL;
        goto done;
    }

    if ((err = _oefs_readdir(oefs_dir, &entry)) != 0)
    {
        errno = err;
        goto done;
    }

    if (entry)
    {
        dir->entry.d_ino = entry->d_ino;
        dir->entry.d_off = entry->d_off;
        dir->entry.d_reclen = entry->d_reclen;
        dir->entry.d_type = entry->d_type;
        strlcpy(dir->entry.d_name, entry->d_name, sizeof(dir->entry.d_name));
        ret = &dir->entry;
    }

done:

    return ret;
}

static int _oefs_d_closedir(DIR* base)
{
    int ret = -1;
    dir_t* dir = (dir_t*)base;
    oefs_dir_t* oefs_dir = _get_oefs_dir(dir);
    int err;

    if (!oefs_dir)
    {
        errno = EINVAL;
        goto done;
    }

    if ((err = _oefs_closedir(oefs_dir)) != 0)
    {
        errno = err;
        goto done;
    }

    free(dir);

    ret = 0;

done:

    return ret;
}

static DIR* _oefs_fs_opendir(oe_fs_t* fs, const char* name)
{
    DIR* ret = NULL;
    oefs_t* oefs = _get_oefs(fs);
    dir_t* dir = NULL;
    oefs_dir_t* oefs_dir = NULL;
    int err;

    if (!oefs || !name)
    {
        errno = EINVAL;
        goto done;
    }

    if (!(dir = calloc(1, sizeof(dir_t))))
    {
        errno = EINVAL;
        goto done;
    }

    if ((err = _oefs_opendir(oefs, name, &oefs_dir)) != 0)
    {
        errno = err;
        goto done;
    }

    dir->base.d_readdir = _oefs_d_readdir;
    dir->base.d_closedir = _oefs_d_closedir;
    dir->oefs_dir = oefs_dir;
    ret = &dir->base;
    dir = NULL;
    oefs_dir = NULL;

done:

    if (dir)
        free(dir);

    if (oefs_dir)
        _oefs_closedir(oefs_dir);

    return ret;
}

static int _oefs_fs_release(oe_fs_t* fs)
{
    uint32_t ret = -1;
    oefs_t* oefs = _get_oefs(fs);
    int err;

    if (!oefs)
    {
        errno = EINVAL;
        goto done;
    }

    if ((err = _oefs_release(oefs)) != 0)
    {
        errno = err;
        goto done;
    }

    ret = 0;

done:
    return ret;
}

static int _oefs_fs_stat(oe_fs_t* fs, const char* path, struct stat* stat)
{
    int ret = -1;
    oefs_t* oefs = _get_oefs(fs);
    oefs_stat_t buf;
    int err;

    if (!oefs)
    {
        errno = EINVAL;
        goto done;
    }

    if ((err = _oefs_stat(oefs, path, &buf)) != 0)
    {
        errno = err;
        goto done;
    }

    stat->st_dev = buf.st_dev;
    stat->st_ino = buf.st_ino;
    stat->st_mode = buf.st_mode;
    stat->st_nlink = buf.st_nlink;
    stat->st_uid = buf.st_uid;
    stat->st_gid = buf.st_gid;
    stat->st_rdev = buf.st_rdev;
    stat->st_size = buf.st_size;
    stat->st_blksize = buf.st_blksize;
    stat->st_blocks = buf.st_blocks;

    ret = 0;

done:

    return ret;
}

static int _oefs_fs_rename(
    oe_fs_t* fs,
    const char* old_path,
    const char* new_path)
{
    int ret = -1;
    oefs_t* oefs = _get_oefs(fs);
    int err;

    if (!oefs)
    {
        errno = EINVAL;
        goto done;
    }

    if ((err = _oefs_rename(oefs, old_path, new_path)) != 0)
    {
        errno = err;
        goto done;
    }

    ret = 0;

done:

    return ret;
}

static int _oefs_fs_remove(oe_fs_t* fs, const char* path)
{
    int ret = -1;
    oefs_t* oefs = _get_oefs(fs);
    int err;

    if (!oefs)
    {
        errno = EINVAL;
        goto done;
    }

    if ((err = _oefs_unlink(oefs, path)) != 0)
    {
        errno = err;
        goto done;
    }

    ret = 0;

done:

    return ret;
}

static int _oefs_fs_mkdir(oe_fs_t* fs, const char* path, unsigned int mode)
{
    int ret = -1;
    oefs_t* oefs = _get_oefs(fs);
    int err;

    if (!oefs)
    {
        errno = EINVAL;
        goto done;
    }

    if ((err = _oefs_mkdir(oefs, path, mode)) != 0)
    {
        errno = err;
        goto done;
    }

    ret = 0;

done:

    return ret;
}

static int _oefs_fs_rmdir(oe_fs_t* fs, const char* path)
{
    int ret = -1;
    oefs_t* oefs = _get_oefs(fs);
    int err;

    if (!oefs)
    {
        errno = EINVAL;
        goto done;
    }

    if ((err = _oefs_rmdir(oefs, path)) != 0)
    {
        errno = err;
        goto done;
    }

    ret = 0;

done:

    return ret;
}

static oe_fs_ft_t _ft = {
    .fs_release = _oefs_fs_release,
    .fs_fopen = _oefs_fs_fopen,
    .fs_opendir = _oefs_fs_opendir,
    .fs_stat = _oefs_fs_stat,
    .fs_remove = _oefs_fs_remove,
    .fs_rename = _oefs_fs_rename,
    .fs_mkdir = _oefs_fs_mkdir,
    .fs_rmdir = _oefs_fs_rmdir,
};

int oe_oefs_initialize(
    oe_fs_t* fs,
    const char* source,
    const uint8_t key[OEFS_KEY_SIZE])
{
    int ret = -1;
    oefs_t* oefs = NULL;
    fs_impl_t* impl = NULL;
    uint32_t flags = 0;

    if (fs)
        memset(fs, 0, sizeof(oe_fs_t));

    if (!fs)
        goto done;

    flags |= OEFS_FLAG_CACHING;
    flags |= OEFS_FLAG_INTEGRITY;
    flags |= OEFS_FLAG_AUTH_CRYPTO;
    //flags |= OEFS_FLAG_CRYPTO;

    if (_oefs_new(&oefs, source, flags, key) != 0)
        goto done;

    impl = (fs_impl_t*)fs;
    impl->base.magic = OE_FS_MAGIC;
    impl->base.ft = &_ft;
    impl->magic = OEFS_MAGIC;
    impl->oefs = oefs;
    oefs = NULL;

    ret = 0;

done:

    if (oefs)
        _oefs_release(oefs);

    return ret;
}
