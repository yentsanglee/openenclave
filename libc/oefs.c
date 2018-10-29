// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _GNU_SOURCE
#include "oefs.h"
#include <assert.h>
#include <openenclave/internal/enclavelibc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "buf.h"

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

#define TRACE printf("%s(%u): %s()\n", __FILE__, __LINE__, __FUNCTION__)

#define INLINE static __inline

#define COUNTOF(ARR) (sizeof(ARR) / sizeof((ARR)[0]))

// clang-format off
#define CHECK(ERR)                \
    do                            \
    {                             \
        oe_errno_t __err__ = ERR; \
        if (__err__ != OE_EOK)    \
        {                         \
            err = __err__;        \
            goto done;            \
        }                         \
    }                             \
    while (0)
// clang-format on

// clang-format off
#define RAISE(ERR) \
    do             \
    {              \
        err = ERR; \
        goto done; \
    }              \
    while (0)
// clang-format on

/* The logical block number of the root directory inode. */
#define OEFS_ROOT_INO 1

/* The physical block number of the super block. */
#define SUPER_BLOCK_PHYSICAL_BLKNO 2

/* The first physical block number of the block bitmap. */
#define BITMAP_PHYSICAL_BLKNO 3

#if 1

typedef struct _oefs_super_block
{
    /* Magic number: OEFS_SUPER_BLOCK_MAGIC. */
    uint32_t s_magic;

    /* The total number of blocks in the file system. */
    uint32_t s_nblocks;

    /* The number of free blocks. */
    uint32_t s_free_blocks;

    /* Reserved. */
    uint8_t s_reserved[500];
} oefs_super_block_t;

OE_STATIC_ASSERT(sizeof(oefs_super_block_t) == FS_BLOCK_SIZE);

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

    /* Total number of 512-byte blocks in this file. */
    uint32_t i_nblocks;

    /* The next blknos block. */
    uint32_t i_next;

    /* Reserved. */
    uint32_t i_reserved[6];

    /* Blocks comprising this file. */
    uint32_t i_blocks[112];

} oefs_inode_t;

OE_STATIC_ASSERT(sizeof(oefs_inode_t) == FS_BLOCK_SIZE);

typedef struct _oefs_bnode
{
    /* The next blknos block. */
    uint32_t b_next;

    /* Blocks comprising this file. */
    uint32_t b_blocks[127];

} oefs_bnode_t;

OE_STATIC_ASSERT(sizeof(oefs_bnode_t) == FS_BLOCK_SIZE);

OE_STATIC_ASSERT(sizeof(fs_dirent_t) == 268);

OE_STATIC_ASSERT(sizeof(fs_stat_t) == 48);

typedef struct _oefs
{
    fs_t base;
    /* ATTN: add magic number here. */
    oe_block_dev_t* dev;

    union
    {
        const oefs_super_block_t read;
        // This field should only be accessed with the _sb_write() method, 
        // which sets the oefs_t.dirty flag.
        oefs_super_block_t __write;
    }
    sb;

    union
    {
        // This field should only be accessed with the _bitmap_write() method,
        // which sets the oefs_t.dirty flag.
        uint8_t* __write;
        const uint8_t* read;
    }
    bitmap;

    oefs_super_block_t sb_copy;
    uint8_t* bitmap_copy;
    size_t bitmap_size;

    bool dirty;
} oefs_t;

typedef struct _oefs_block
{
    uint8_t data[FS_BLOCK_SIZE];
} oefs_block_t;

#endif

struct _fs_file
{
    oefs_t* oefs;

    /* The file's inode number. */
    uint32_t ino;

    /* The file's inode */
    oefs_inode_t inode;

    /* The block numbers that contain the file's data. */
    buf_u32_t blknos;

    /* The block numbers that contain the file's block numbers. */
    buf_u32_t bnode_blknos;

    /* The file offset (or current position). */
    uint32_t offset;

    bool eof;
};

struct _fs_dir
{
    /* The inode number of this directory. */
    uint32_t ino;

    /* The open file which contains the directory entries. */
    fs_file_t* file;

    /* The current directory entry. */
    fs_dirent_t ent;
};

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

/* Get the physical block number from a logical block number. */
INLINE uint32_t _get_physical_blkno(oefs_t* oefs, uint32_t blkno)
{
    /* Calculate the number of bitmap blocks. */
    size_t num_bitmap_blocks = oefs->sb.read.s_nblocks / OEFS_BITS_PER_BLOCK;
    return blkno + (3 + num_bitmap_blocks) - 1;
}

static oe_errno_t _read_block(oefs_t* oefs, size_t blkno, void* block)
{
    oe_errno_t err = OE_EOK;
    uint32_t physical_blkno;

    /* Check whether the block number is valid. */
    if (blkno == 0 || blkno > oefs->sb.read.s_nblocks)
        RAISE(OE_EIO);

    /* Sanity check: make sure the block is not free. */
    if (!_test_bit(oefs->bitmap.read, oefs->bitmap_size, blkno - 1))
        RAISE(OE_EIO);

    /* Convert the logical block number to a physical block number. */
    physical_blkno = _get_physical_blkno(oefs, blkno);

    /* Get the physical block. */
    if (oefs->dev->get(oefs->dev, physical_blkno, block) != 0)
        RAISE(OE_EIO);

done:
    return err;
}

static oe_errno_t _write_block(oefs_t* oefs, size_t blkno, const void* block)
{
    oe_errno_t err = OE_EOK;
    uint32_t physical_blkno;

    /* Check whether the block number is valid. */
    if (blkno == 0 || blkno > oefs->sb.read.s_nblocks)
        RAISE(OE_EINVAL);

    /* Sanity check: make sure the block is not free. */
    if (!_test_bit(oefs->bitmap.read, oefs->bitmap_size, blkno - 1))
        RAISE(OE_EIO);

    /* Convert the logical block number to a physical block number. */
    physical_blkno = _get_physical_blkno(oefs, blkno);

    /* Get the physical block. */
    if (oefs->dev->put(oefs->dev, physical_blkno, block) != 0)
        RAISE(OE_EIO);

done:
    return err;
}

static oe_errno_t _load_inode(
    oefs_t* oefs,
    uint32_t ino,
    oefs_inode_t* inode)
{
    oe_errno_t err = OE_EOK;

    /* Read this inode into memory. */
    CHECK(_read_block(oefs, ino, inode));

    /* Check the inode magic number. */
    if (inode->i_magic != OEFS_INODE_MAGIC)
        RAISE(OE_EIO);

done:
    return err;
}

static oe_errno_t _load_blknos(
    oefs_t* oefs,
    oefs_inode_t* inode,
    buf_u32_t* bnode_blknos,
    buf_u32_t* blknos)
{
    oe_errno_t err = OE_EOK;

    /* Collect direct block numbers from the inode. */
    {
        size_t n = sizeof(inode->i_blocks) / sizeof(uint32_t);

        for (size_t i = 0; i < n && inode->i_blocks[i]; i++)
        {
            if (buf_u32_append(blknos, &inode->i_blocks[i], 1) != 0)
                RAISE(OE_ENOMEM);
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
            CHECK(_read_block(oefs, next, &bnode));

            /* Append this bnode blkno. */
            if (buf_u32_append(bnode_blknos, &next, 1) != 0)
                RAISE(OE_ENOMEM);

            n = sizeof(bnode.b_blocks) / sizeof(uint32_t);

            /* Get all blocks from this bnode. */
            for (size_t i = 0; i < n && bnode.b_blocks[i]; i++)
            {
                if (buf_u32_append(blknos, &bnode.b_blocks[i], 1) != 0)
                    RAISE(OE_ENOMEM);
            }

            next = bnode.b_next;
        }
    }

done:
    return err;
}

static oe_errno_t _open_file(
    oefs_t* oefs,
    uint32_t ino,
    fs_file_t** file_out)
{
    oe_errno_t err = OE_EOK;
    fs_file_t* file = NULL;

    if (file_out)
        *file_out = NULL;

    if (!oefs || !ino || !file_out)
        RAISE(OE_EINVAL);

    if (!(file = calloc(1, sizeof(fs_file_t))))
        RAISE(OE_ENOMEM);

    file->oefs = oefs;
    file->ino = ino;

    /* Read this inode into memory. */
    CHECK(_load_inode(oefs, ino, &file->inode));

    /* Load the block numbers into memory. */
    CHECK(_load_blknos(oefs, &file->inode, &file->bnode_blknos, &file->blknos));

    *file_out = file;
    file = NULL;

done:

    if (file)
    {
        buf_u32_release(&file->blknos);
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

static oe_errno_t _assign_blkno(oefs_t* oefs, uint32_t* blkno)
{
    oe_errno_t err = OE_EOK;

    *blkno = 0;

    for (uint32_t i = 0; i < oefs->bitmap_size * 8; i++)
    {
        if (!_test_bit(oefs->bitmap.read, oefs->bitmap_size, i))
        {
            _set_bit(_bitmap_write(oefs), oefs->bitmap_size, i);
            *blkno = i + 1;
            _sb_write(oefs)->s_free_blocks--;
            RAISE(0);
        }
    }

    RAISE(OE_ENOSPC);

done:
    return err;
}

static oe_errno_t _unassign_blkno(oefs_t* oefs, uint32_t blkno)
{
    oe_errno_t err = OE_EOK;
    uint32_t nbits = oefs->bitmap_size * 8;
    uint32_t index = blkno - 1;

    if (blkno == 0 || index >= nbits)
        RAISE(OE_EINVAL);

    assert(_test_bit(oefs->bitmap.read, oefs->bitmap_size, index));

    _clr_bit(_bitmap_write(oefs), oefs->bitmap_size, index);
    _sb_write(oefs)->s_free_blocks++;

done:
    return err;
}

static oe_errno_t _flush_super_block(oefs_t* oefs)
{
    oe_errno_t err = OE_EOK;

    /* Flush the superblock if it changed. */
    if (memcmp(&oefs->sb, &oefs->sb_copy, sizeof(oefs_super_block_t)) != 0)
    {
        const uint32_t blkno = SUPER_BLOCK_PHYSICAL_BLKNO;

        if (oefs->dev->put(oefs->dev, blkno, &oefs->sb) != 0)
            RAISE(OE_EIO);

        memcpy(&oefs->sb_copy, &oefs->sb, sizeof(oefs_super_block_t));
    }

done:
    return err;
}

static oe_errno_t _flush_bitmap(oefs_t* oefs)
{
    oe_errno_t err = OE_EOK;
    size_t num_bitmap_blocks;

    /* Calculate the number of bitmap blocks. */
    num_bitmap_blocks = oefs->sb.read.s_nblocks / OEFS_BITS_PER_BLOCK;

    /* Flush each bitmap block that changed. */
    for (size_t i = 0; i < num_bitmap_blocks; i++)
    {
        /* Set pointer to the i-th block to write. */
        oefs_block_t* block = (oefs_block_t*)oefs->bitmap.read + i;
        oefs_block_t* block_copy = (oefs_block_t*)oefs->bitmap_copy + i;

        /* Flush this bitmap block if it changed. */
        if (memcmp(block, block_copy, FS_BLOCK_SIZE) != 0)
        {
            /* Calculate the block number to write. */
            uint32_t blkno = BITMAP_PHYSICAL_BLKNO + i;

            if (oefs->dev->put(oefs->dev, blkno, block) != 0)
                RAISE(OE_EIO);

            memcpy(block_copy, block, FS_BLOCK_SIZE);
        }
    }

done:
    return err;
}

static oe_errno_t _flush(oefs_t* oefs)
{
    oe_errno_t err = OE_EOK;

    if (oefs->dirty)
    {
        CHECK(_flush_bitmap(oefs));
        CHECK(_flush_super_block(oefs));
        oefs->dirty = false;
    }

done:
    return err;
}

static oe_errno_t _write_data(
    oefs_t* oefs,
    const void* data,
    uint32_t size,
    buf_u32_t* blknos)
{
    oe_errno_t err = OE_EOK;
    const uint8_t* ptr = (const uint8_t*)data;
    uint32_t remaining = size;

    /* While there is data remaining to be written. */
    while (remaining)
    {
        uint32_t blkno;
        oefs_block_t block;
        uint32_t copy_size;

        /* Assign a new block number from the in-memory bitmap. */
        CHECK(_assign_blkno(oefs, &blkno));

        /* Append the new block number to the array of blocks numbers. */
        if (buf_u32_append(blknos, &blkno, 1) != 0)
            RAISE(OE_ENOMEM);

        /* Calculate bytes to copy to the block. */
        copy_size = (remaining > FS_BLOCK_SIZE) ? FS_BLOCK_SIZE : remaining;

        /* Clear the block. */
        memset(block.data, 0, sizeof(oefs_block_t));

        /* Copy user data to the block. */
        memcpy(block.data, ptr, copy_size);

        /* Write the new block. */
        CHECK(_write_block(oefs, blkno, &block));

        /* Advance to next block of data to write. */
        ptr += copy_size;
        remaining -= copy_size;
    }

done:

    if (_flush(oefs) != 0)
        return OE_EIO;

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

static oe_errno_t _append_block_chain(
    fs_file_t* file,
    const buf_u32_t* blknos)
{
    oe_errno_t err = OE_EOK;
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
        count = COUNTOF(file->inode.i_blocks);
        blkno = file->ino;
        next = &file->inode.i_next;
    }
    else
    {
        assert(file->bnode_blknos.size != 0);

        if (file->bnode_blknos.size == 0)
            RAISE(OE_EIO);

        blkno = file->bnode_blknos.data[file->bnode_blknos.size-1];

        CHECK(_read_block(file->oefs, blkno, &bnode));

        object = &bnode;
        slots = bnode.b_blocks;
        count = COUNTOF(bnode.b_blocks);
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
            CHECK(_assign_blkno(file->oefs, &new_blkno));

            /* Append the bnode blkno to the file struct. */
            if (buf_u32_append(&file->bnode_blknos, &new_blkno, 1) != 0)
                RAISE(OE_ENOMEM);

            *next = new_blkno;

            /* Rewrite the current inode or bnode. */
            CHECK(_write_block(file->oefs, blkno, object));

            /* Initialize the new bnode with zeros. */
            memset(&bnode, 0, sizeof(oefs_bnode_t));

            /* Set up variables to refer to the new bnode. */
            object = &bnode;
            slots = bnode.b_blocks;
            count = COUNTOF(bnode.b_blocks);
            blkno = new_blkno;
            next = &bnode.b_next;
        }
        else
        {
            /* Rewrite the inode or bnode. */
            CHECK(_write_block(file->oefs, blkno, object));
        }
    }

done:
    return err;
}

static bool _sane_file(fs_file_t* file)
{
    if (!file->oefs)
        return false;

    if (!file->ino)
        return false;

    if (file->inode.i_nblocks != file->blknos.size)
        return false;

    return true;
}

static oe_errno_t _split_path(
    const char* path,
    char dirname[FS_PATH_MAX],
    char basename[FS_PATH_MAX])
{
    oe_errno_t err = OE_EOK;
    char* slash;

    /* Reject paths that are too long. */
    if (strlen(path) >= FS_PATH_MAX)
        RAISE(OE_EINVAL);

    /* Reject paths that are not absolute */
    if (path[0] != '/')
        RAISE(OE_EINVAL);

    /* Handle root directory up front */
    if (strcmp(path, "/") == 0)
    {
        strlcpy(dirname, "/", FS_PATH_MAX);
        strlcpy(basename, "/", FS_PATH_MAX);
        RAISE(0);
    }

    /* This cannot fail (prechecked) */
    if (!(slash = strrchr(path, '/')))
        RAISE(OE_EINVAL);

    /* If path ends with '/' character */
    if (!slash[1])
        RAISE(OE_EINVAL);

    /* Split the path */
    {
        if (slash == path)
        {
            strlcpy(dirname, "/", FS_PATH_MAX);
        }
        else
        {
            uint32_t index = slash - path;
            strlcpy(dirname, path, FS_PATH_MAX);

            if (index < FS_PATH_MAX)
                dirname[index] = '\0';
            else
                dirname[FS_PATH_MAX - 1] = '\0';
        }

        strlcpy(basename, slash + 1, FS_PATH_MAX);
    }

done:
    return err;
}

static oe_errno_t _create_file(
    oefs_t* oefs,
    uint32_t dir_ino,
    const char* name,
    uint8_t type,
    uint32_t* ino_out)
{
    oe_errno_t err = OE_EOK;
    uint32_t ino;
    fs_file_t* file = NULL;

    if (ino_out)
        *ino_out = 0;

    if (!oefs || !dir_ino || !name || strlen(name) >= FS_PATH_MAX)
        RAISE(OE_EINVAL);

    /* ATTN: should create replace an existing file? */

    /* Create an inode for the new file. */
    {
        oefs_inode_t inode;

        memset(&inode, 0, sizeof(oefs_inode_t));

        inode.i_magic = OEFS_INODE_MAGIC;
        inode.i_links = 1;

        if (type == FS_DT_DIR)
            inode.i_mode = FS_S_DIR_DEFAULT;
        else if (type == FS_DT_REG)
            inode.i_mode = FS_S_REG_DEFAULT;
        else
            RAISE(OE_EINVAL);

        CHECK(_assign_blkno(oefs, &ino));
        CHECK(_write_block(oefs, ino, &inode));
    }

    /* Add an entry to the directory for this inode. */
    {
        fs_dirent_t ent;
        int32_t n;

        CHECK(_open_file(oefs, dir_ino, &file));

        /* Check for duplicates. */
        for (;;)
        {
            CHECK(oefs_read(file, &ent, sizeof(ent), &n));

            if (n == 0)
                break;

            assert(n == sizeof(ent));

            if (n != sizeof(ent))
                RAISE(OE_EIO);

            if (strcmp(ent.d_name, name) == 0)
                RAISE(OE_EEXIST);
        }

        /* Append the entry to the directory. */
        {
            memset(&ent, 0, sizeof(ent));
            ent.d_ino = ino;
            ent.d_off = file->offset;
            ent.d_reclen = sizeof(ent);
            ent.d_type = type;
            strlcpy(ent.d_name, name, sizeof(ent.d_name));

            CHECK(oefs_write(file, &ent, sizeof(ent), &n));

            if (n != sizeof(ent))
                RAISE(OE_EIO);
        }
    }

    if (ino_out)
        *ino_out = ino;

done:

    if (file)
        oefs_close(file);

    if (_flush(oefs) != 0)
        return OE_EIO;

    return err;
}

static oe_errno_t _truncate_file(fs_file_t* file)
{
    oe_errno_t err = OE_EOK;

    if (!file)
        RAISE(OE_EINVAL);

    /* Release all the data blocks. */
    for (size_t i = 0; i < file->blknos.size; i++)
        CHECK(_unassign_blkno(file->oefs, file->blknos.data[i]));

    /* Release all the bnode blocks. */
    for (size_t i = 0; i < file->bnode_blknos.size; i++)
        CHECK(_unassign_blkno(file->oefs, file->bnode_blknos.data[i]));

    /* Update the inode. */
    file->inode.i_size = 0;
    file->inode.i_next = 0;
    file->inode.i_nblocks = 0;
    memset(file->inode.i_blocks, 0, sizeof(file->inode.i_blocks));

    /* Update the file struct. */
    buf_u32_clear(&file->blknos);
    buf_u32_clear(&file->bnode_blknos);
    file->offset = 0;

    /* Sync the inode to disk. */
    CHECK(_write_block(file->oefs, file->ino, &file->inode));

done:

    if (_flush(file->oefs) != 0)
        return OE_EIO;

    return err;
}

static oe_errno_t _load_file(
    fs_file_t* file,
    void** data_out,
    size_t* size_out)
{
    oe_errno_t err = OE_EOK;
    buf_t buf = BUF_INITIALIZER;
    char data[FS_BLOCK_SIZE];
    int32_t n;

    *data_out = NULL;
    *size_out = 0;

    for (;;)
    {
        CHECK(oefs_read(file, data, sizeof(data), &n)); 

        if (n == 0)
            break;

        if (buf_append(&buf, data, n) != 0)
            RAISE(OE_ENOMEM);
    }

    *data_out = buf.data;
    *size_out = buf.size;
    memset(&buf, 0, sizeof(buf));

done:

    buf_release(&buf);

    return err;
}

static oe_errno_t _release_inode(oefs_t* oefs, uint32_t ino)
{
    oe_errno_t err = OE_EOK;
    fs_file_t* file = NULL;

    CHECK(_open_file(oefs, ino, &file));
    CHECK(_truncate_file(file));
    CHECK(_unassign_blkno(oefs, file->ino));

done:

    if (file)
        oefs_close(file);

    if (_flush(oefs) != 0)
        return OE_EIO;

    return err;
}

static oe_errno_t _unlink_file(
    oefs_t* oefs,
    uint32_t dir_ino,
    uint32_t ino,
    const char* name)
{
    oe_errno_t err = OE_EOK;
    fs_file_t* dir = NULL;
    fs_file_t* file = NULL;
    void* data = NULL;
    size_t size = 0;
    oefs_inode_t inode;

    if (!oefs || !dir_ino || !ino)
        RAISE(OE_EINVAL);

    /* Open the directory file. */
    CHECK(_open_file(oefs, dir_ino, &dir));

    /* Load the contents of the parent directory into memory. */
    CHECK(_load_file(dir, &data, &size));

    /* File must be a multiple of the entry size. */
    {
        assert((size % sizeof(fs_dirent_t)) == 0);

        if (size % sizeof(fs_dirent_t))
            RAISE(OE_EIO);
    }

    /* Truncate the parent directory. */
    CHECK(_truncate_file(dir));

    /* Rewrite the directory entries but exclude the removed file. */
    {
        const fs_dirent_t* entries = (const fs_dirent_t*)data;
        size_t num_entries = size / sizeof(fs_dirent_t);

        for (size_t i = 0; i < num_entries; i++)
        {
            if (strcmp(entries[i].d_name, name) != 0)
            {
                const uint32_t n = sizeof(fs_dirent_t);
                int32_t nwritten;

                CHECK(oefs_write(dir, &entries[i], n, &nwritten));

                if (nwritten != n)
                    RAISE(OE_EIO);
            }
        }
    }

    /* Load the inode into memory. */
    CHECK(_load_inode(oefs, ino, &inode));

    /* If this is the only link to this file, then remove it. */
    if (inode.i_links == 1)
    {
        CHECK(_release_inode(oefs, ino));
    }
    else
    {
        /* Decrement the number of links. */
        inode.i_links--;

        /* Rewrite the inode. */
        CHECK(_write_block(oefs, ino, &inode));
    }

done:

    if (dir)
        oefs_close(dir);

    if (file)
        oefs_close(file);

    if (data)
        free(data);

    if (_flush(oefs) != 0)
        return OE_EIO;

    return err;
}

static oe_errno_t _opendir_by_ino(
    oefs_t* oefs, uint32_t ino, fs_dir_t** dir_out)
{
    oe_errno_t err = OE_EOK;
    fs_dir_t* dir = NULL;
    fs_file_t* file = NULL;

    if (dir_out)
        *dir_out = NULL;

    if (!oefs || !ino || !dir_out)
        RAISE(OE_EINVAL);

    CHECK(_open_file(oefs, ino, &file));

    if (!(dir = calloc(1, sizeof(fs_dir_t))))
        RAISE(OE_ENOMEM);

    dir->ino = ino;
    dir->file = file;
    file = NULL;

    *dir_out = dir;

done:

    if (file)
        oefs_close(file);

    return err;
}

static oe_errno_t _path_to_ino(
    oefs_t* oefs,
    const char* path,
    uint32_t* dir_ino_out,
    uint32_t* ino_out,
    uint8_t* type_out) /* fs_dir_t.d_type */
{
    oe_errno_t err = OE_EOK;
    char buf[FS_PATH_MAX];
    const char* elements[FS_PATH_MAX];
    const size_t MAX_ELEMENTS = COUNTOF(elements);
    size_t num_elements = 0;
    uint8_t i;
    uint32_t current_ino = 0;
    uint32_t dir_ino = 0;
    fs_dir_t* dir = NULL;

    if (dir_ino_out)
        *dir_ino_out = 0;

    if (ino_out)
        *ino_out = 0;

    if (type_out)
        *type_out = 0;

    /* Check for null parameters */
    if (!oefs || !path || !ino_out)
        RAISE(OE_EINVAL);

    /* Check path length */
    if (strlen(path) >= FS_PATH_MAX)
        RAISE(OE_ENAMETOOLONG);

    /* Make copy of the path. */
    strlcpy(buf, path, sizeof(buf));

    /* Be sure path begins with "/" */
    if (path[0] != '/')
        RAISE(OE_EINVAL);

    elements[num_elements++] = "/";

    /* Split the path into components */
    {
        char* p;
        char* save;

        for (p = strtok_r(buf, "/", &save); p; p = strtok_r(NULL, "/", &save))
        {
            assert(num_elements < MAX_ELEMENTS);
            elements[num_elements++] = p;
        }
    }

    /* Load each inode along the path until we find it. */
    for (i = 0; i < num_elements; i++)
    {
        bool final_element = (num_elements == i + 1);

        if (strcmp(elements[i], "/") == 0)
        {
            current_ino = OEFS_ROOT_INO;
            dir_ino = current_ino;

            if (final_element)
            {
                if (type_out)
                    *type_out = FS_DT_DIR;
            }
        }
        else
        {
            fs_dirent_t* ent;

            CHECK(_opendir_by_ino(oefs, current_ino, &dir));

            dir_ino = current_ino;
            current_ino = 0;

            for (;;)
            {
                CHECK(oefs_readdir(dir, &ent));

                if (!ent)
                    break;

                /* If final path element or a directory. */
                if (final_element || ent->d_type == FS_DT_DIR)
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
                RAISE(OE_ENOENT);

            CHECK(oefs_closedir(dir));
            dir = NULL;
        }
    }

    if (dir_ino_out)
        *dir_ino_out = dir_ino;

    *ino_out = current_ino;

done:

    if (dir)
        oefs_closedir(dir);

    return err;
}

/*
**==============================================================================
**
** Public interface:
**
**==============================================================================
*/

oe_errno_t oefs_mkfs(oe_block_dev_t* dev, size_t num_blocks)
{
    oe_errno_t err = OE_EOK;
    size_t num_bitmap_blocks;
    uint32_t blkno = 0;
    uint8_t empty_block[FS_BLOCK_SIZE];

    if (!dev || num_blocks < OEFS_BITS_PER_BLOCK)
        RAISE(OE_EINVAL);

    /* Fail if num_blocks is not a multiple of the block size. */
    if (num_blocks % OEFS_BITS_PER_BLOCK)
        RAISE(OE_EINVAL);

    /* Initialize an empty block. */
    memset(empty_block, 0, sizeof(empty_block));

    /* Calculate the number of bitmap blocks. */
    num_bitmap_blocks = num_blocks / OEFS_BITS_PER_BLOCK;

    /* Write empty block one. */
    if (dev->put(dev, blkno++, empty_block) != 0)
        RAISE(OE_EIO);

    /* Write empty block two. */
    if (dev->put(dev, blkno++, empty_block) != 0)
        RAISE(OE_EIO);

    /* Write the super block */
    {
        oefs_super_block_t sb;
        memset(&sb, 0, sizeof(sb));

        sb.s_magic = OEFS_SUPER_BLOCK_MAGIC;
        sb.s_nblocks = num_blocks;
        sb.s_free_blocks = num_blocks - 3;

        if (dev->put(dev, blkno++, &sb) != 0)
            RAISE(OE_EIO);
    }

    /* Write the first bitmap block. */
    {
        uint8_t block[FS_BLOCK_SIZE];

        memset(block, 0, sizeof(block));

        /* Set the bit for the root inode. */
        _set_bit(block, sizeof(block), 0);

        /* Set the bits for the root directory. */
        _set_bit(block, sizeof(block), 1);
        _set_bit(block, sizeof(block), 2);

        /* Write the block. */
        if (dev->put(dev, blkno++, &block) != 0)
            RAISE(OE_EIO);
    }

    /* Write the subsequent (empty) bitmap blocks. */
    for (size_t i = 1; i < num_bitmap_blocks; i++)
    {
        if (dev->put(dev, blkno++, &empty_block) != 0)
            RAISE(OE_EIO);
    }

    /* Write the root directory inode. */
    {
        oefs_inode_t root_inode = {
            .i_magic = OEFS_INODE_MAGIC,
            .i_mode = FS_S_DIR_DEFAULT,
            .i_uid = 0,
            .i_gid = 0,
            .i_links = 1,
            .i_size = 2 * sizeof(fs_dirent_t),
            .i_atime = 0,
            .i_ctime = 0,
            .i_mtime = 0,
            .i_dtime = 0,
            .i_nblocks = 2,
            .i_next = 0,
            .i_reserved = {0},
            .i_blocks = {2, 3},
        };

        if (dev->put(dev, blkno++, &root_inode) != 0)
            RAISE(OE_EIO);
    }

    /* Write the ".." and "." directory entries for the root directory.  */
    {
        uint8_t blocks[2][FS_BLOCK_SIZE];

        fs_dirent_t dir_entries[] = {
            {.d_ino = OEFS_ROOT_INO,
             .d_off = 0 * sizeof(fs_dirent_t),
             .d_reclen = sizeof(fs_dirent_t),
             .d_type = FS_DT_DIR,
             .d_name = ".."

            },
            {.d_ino = OEFS_ROOT_INO,
             .d_off = 1 * sizeof(fs_dirent_t),
             .d_reclen = sizeof(fs_dirent_t),
             .d_type = FS_DT_DIR,
             .d_name = "."},
        };

        memset(blocks, 0, sizeof(blocks));
        memcpy(blocks, dir_entries, sizeof(dir_entries));

        if (dev->put(dev, blkno++, &blocks[0]) != 0)
            RAISE(OE_EIO);

        if (dev->put(dev, blkno++, &blocks[1]) != 0)
            RAISE(OE_EIO);
    }

    /* Write the remaining empty blocks. */
    for (size_t i = 0; i < num_blocks - 3; i++)
    {
        if (dev->put(dev, blkno++, &empty_block) != 0)
            RAISE(OE_EIO);
    }

done:
    return err;
}

typedef struct _block_dev
{
    oe_block_dev_t base;
    size_t size;
} block_dev_t;

static int _block_dev_close(oe_block_dev_t* dev)
{
    return 0;
}

static int _block_dev_get(oe_block_dev_t* dev, uint32_t blkno, void* data)
{
    return -1;
}

int _block_dev_put(oe_block_dev_t* dev, uint32_t blkno, const void* data)
{
    block_dev_t* device = (block_dev_t*)dev;
    device->size += FS_BLOCK_SIZE;
    return 0;
}

oe_errno_t oefs_size(size_t num_blocks, size_t* size)
{
    oe_errno_t err = OE_EOK;
    block_dev_t dev;

    dev.base.close = _block_dev_close;
    dev.base.get = _block_dev_get;
    dev.base.put = _block_dev_put;
    dev.size = 0;

    if (size)
        *size = 0;

    CHECK(oefs_mkfs(&dev.base, num_blocks));

    if (size)
        *size = dev.size;

done:
    return err;
}

oe_errno_t oefs_read(
    fs_file_t* file,
    void* data,
    uint32_t size,
    int32_t* nread)
{
    oe_errno_t err = OE_EOK;
    uint32_t first;
    uint32_t i;
    uint32_t remaining;
    uint8_t* ptr = (uint8_t*)data;

    if (nread)
        *nread = 0;

    /* Check parameters */
    if (!file || !file->oefs || (!data && size))
        RAISE(OE_EINVAL);

    /* The index of the first block to read. */
    first = file->offset / FS_BLOCK_SIZE;

    /* The number of bytes remaining to be read */
    remaining = size;

    /* Read the data block-by-block */
    for (i = first; i < file->blknos.size && remaining > 0 && !file->eof; i++)
    {
        oefs_block_t block;
        uint32_t offset;

        CHECK(_read_block(file->oefs, file->blknos.data[i], &block));

        /* The offset of the data within this block */
        offset = file->offset % FS_BLOCK_SIZE;

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
                uint32_t block_bytes_remaining = FS_BLOCK_SIZE - offset;

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
            memcpy(ptr, block.data + offset, copy_bytes);

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
    return err;
}

oe_errno_t oefs_write(
    fs_file_t* file,
    const void* data,
    uint32_t size,
    int32_t* nwritten)
{
    oe_errno_t err = OE_EOK;
    uint32_t first;
    uint32_t i;
    uint32_t remaining;
    const uint8_t* ptr = (uint8_t*)data;
    uint32_t new_file_size;

    if (nwritten)
        *nwritten = 0;

    /* Check parameters */
    if (!file || !file->oefs || (!data && size) || !nwritten)
        RAISE(OE_EINVAL);

    assert(_sane_file(file));

    if (!_sane_file(file))
        RAISE(OE_EINVAL);

    /* The index of the first block to write. */
    first = file->offset / FS_BLOCK_SIZE;

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
        oefs_block_t block;
        uint32_t offset;

        CHECK(_read_block(file->oefs, file->blknos.data[i], &block));

        /* The offset of the data within this block */
        offset = file->offset % FS_BLOCK_SIZE;

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
                uint32_t block_bytes_remaining = FS_BLOCK_SIZE - offset;

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
            memcpy(block.data + offset, ptr, copy_bytes);

            /* Advance to next data to write. */
            ptr += copy_bytes;
            remaining -= copy_bytes;

            /* Advance the file position. */
            file->offset += copy_bytes;
        }

        /* Rewrite the block. */
        CHECK(_write_block(file->oefs, file->blknos.data[i], &block));
    }

    /* Append remaining data to the file. */
    if (remaining)
    {
        buf_u32_t blknos = BUF_U32_INITIALIZER;

        /* Write the new blocks. */
        CHECK(_write_data(file->oefs, ptr, remaining, &blknos));

        /* Append these block numbers to the block-numbers chain. */
        CHECK(_append_block_chain(file, &blknos));

        /* Update the inode's total_blocks */
        file->inode.i_nblocks += blknos.size;

        /* Append these block numbers to the file. */
        if (buf_u32_append(&file->blknos, blknos.data, blknos.size) != 0)
            RAISE(OE_ENOMEM);

        buf_u32_release(&blknos);

        file->offset += remaining;
        remaining = 0;
    }

    /* Update the file size. */
    file->inode.i_size = new_file_size;

    /* Flush the inode to disk. */
    CHECK(_write_block(file->oefs, file->ino, &file->inode));

    /* Calculate number of bytes written */
    *nwritten = size - remaining;

done:
    return err;
}

oe_errno_t oefs_close(fs_file_t* file)
{
    oe_errno_t err = OE_EOK;

    if (!file)
        RAISE(OE_EINVAL);

    buf_u32_release(&file->blknos);
    memset(file, 0, sizeof(fs_file_t));
    free(file);

done:
    return err;
}

oe_errno_t oefs_initialize(fs_t** fs_out, oe_block_dev_t* dev)
{
    oe_errno_t err = OE_EOK;
    size_t num_blocks;
    uint8_t* bitmap = NULL;
    uint8_t* bitmap_copy = NULL;
    oefs_t* oefs = NULL;

    if (fs_out)
        *fs_out = NULL;

    if (!fs_out || !dev)
        RAISE(OE_EINVAL);

    if (!(oefs = calloc(1, sizeof(oefs_t))))
        RAISE(OE_ENOMEM);

    /* Save pointer to device (caller is responsible for releasing it). */
    oefs->dev = dev;

    /* Read the super block from the file system. */
    if (dev->get(dev, SUPER_BLOCK_PHYSICAL_BLKNO, &oefs->sb) != 0)
        RAISE(OE_EIO);

    /* Check the superblock magic number. */
    if (oefs->sb.read.s_magic != OEFS_SUPER_BLOCK_MAGIC)
        RAISE(OE_EIO);

    /* Make copy of super block. */
    memcpy(&oefs->sb_copy, &oefs->sb, sizeof(oefs->sb_copy));

    /* Get the number of blocks. */
    num_blocks = oefs->sb.read.s_nblocks;

    /* Check that the number of blocks is correct. */
    if (!num_blocks || (num_blocks % OEFS_BITS_PER_BLOCK))
        RAISE(OE_EIO);

    /* Allocate space for the block bitmap. */
    {
        /* Calculate the number of bitmap blocks. */
        size_t num_bitmap_blocks = num_blocks / OEFS_BITS_PER_BLOCK;

        /* Calculate the size of the bitmap. */
        size_t bitmap_size = num_bitmap_blocks * FS_BLOCK_SIZE;

        /* Allocate the bitmap. */
        if (!(bitmap = calloc(1, bitmap_size)))
            RAISE(OE_ENOMEM);

        /* Allocate the bitmap. */
        if (!(bitmap_copy = calloc(1, bitmap_size)))
            RAISE(OE_ENOMEM);

        /* Read the bitset into memory */
        for (size_t i = 0; i < num_bitmap_blocks; i++)
        {
            uint8_t* ptr = bitmap + (i * FS_BLOCK_SIZE);

            if (dev->get(dev, BITMAP_PHYSICAL_BLKNO + i, ptr) != 0)
                RAISE(OE_EIO);
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
            RAISE(OE_EIO);
        }

        oefs->bitmap.read = bitmap;
        oefs->bitmap_copy = bitmap_copy;
        oefs->bitmap_size = bitmap_size;
        bitmap = NULL;
        bitmap_copy = NULL;
    }

    /* Check the root inode. */
    {
        oefs_inode_t inode;

        CHECK(_read_block(oefs, OEFS_ROOT_INO, &inode));

        if (inode.i_magic != OEFS_INODE_MAGIC)
            RAISE(OE_EIO);
    }

    oefs->base.fs_release = oefs_release;
    oefs->base.fs_read = oefs_read;
    oefs->base.fs_write = oefs_write;
    oefs->base.fs_close = oefs_close;
    oefs->base.fs_opendir = oefs_opendir;
    oefs->base.fs_readdir = oefs_readdir;
    oefs->base.fs_closedir = oefs_closedir;
    oefs->base.fs_open = oefs_open;
    oefs->base.fs_load = oefs_load;
    oefs->base.fs_mkdir = oefs_mkdir;
    oefs->base.fs_create = oefs_create;
    oefs->base.fs_link = oefs_link;
    oefs->base.fs_rename = oefs_rename;
    oefs->base.fs_unlink = oefs_unlink;
    oefs->base.fs_truncate = oefs_truncate;
    oefs->base.fs_rmdir = oefs_rmdir;
    oefs->base.fs_stat = oefs_stat;
    oefs->base.fs_lseek = oefs_lseek;

    *fs_out = &oefs->base;
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

oe_errno_t oefs_release(fs_t* fs)
{
    oe_errno_t err = OE_EOK;
    oefs_t* oefs = (oefs_t*)fs;

    if (!oefs || !oefs->bitmap.read || !oefs->bitmap_copy)
        RAISE(OE_EINVAL);

    free(_bitmap_write(oefs));
    free(oefs->bitmap_copy);
    memset(oefs, 0, sizeof(oefs_t));
    free(oefs);

done:
    return err;
}

oe_errno_t oefs_opendir(fs_t* fs, const char* path, fs_dir_t** dir)
{
    oe_errno_t err = OE_EOK;
    oefs_t* oefs = (oefs_t*)fs;
    uint32_t ino;

    if (dir)
        *dir = NULL;

    if (!oefs || !path || !dir)
        RAISE(OE_EINVAL);

    CHECK(_path_to_ino(oefs, path, NULL, &ino, NULL));
    CHECK(_opendir_by_ino(oefs, ino, dir));

done:

    return err;
}

oe_errno_t oefs_readdir(fs_dir_t* dir, fs_dirent_t** ent)
{
    oe_errno_t err = OE_EOK;
    int32_t nread = 0;

    if (ent)
        *ent = NULL;

    if (!dir || !dir->file)
        RAISE(OE_EINVAL);

    CHECK(oefs_read(dir->file, &dir->ent, sizeof(fs_dirent_t), &nread));

    /* Check for end of file. */
    if (nread == 0)
        RAISE(0);

    /* Check for an illegal read size. */
    if (nread != sizeof(fs_dirent_t))
        RAISE(OE_EBADF);

    *ent = &dir->ent;

done:

    return err;
}

oe_errno_t oefs_closedir(fs_dir_t* dir)
{
    oe_errno_t err = OE_EOK;

    if (!dir || !dir->file)
        RAISE(OE_EINVAL);

    oefs_close(dir->file);
    dir->file = NULL;
    free(dir);

done:
    return err;
}

oe_errno_t oefs_open(
    fs_t* fs,
    const char* path,
    int flags,
    uint32_t mode,
    fs_file_t** file_out)
{
    oefs_t* oefs = (oefs_t*)fs;
    oe_errno_t err = OE_EOK;
    fs_file_t* file = NULL;
    uint32_t ino = 0;

    if (file_out)
        *file_out = NULL;

    if (!oefs || !path || !file_out)
        RAISE(OE_EINVAL);

    CHECK(_path_to_ino(oefs, path, NULL, &ino, NULL));
    CHECK(_open_file(oefs, ino, &file));

    *file_out = file;

done:

    return err;
}

oe_errno_t oefs_load(
    fs_t* fs,
    const char* path,
    void** data_out,
    size_t* size_out)
{
    oefs_t* oefs = (oefs_t*)fs;
    oe_errno_t err = OE_EOK;
    fs_file_t* file = NULL;
    void* data = NULL;
    size_t size = 0;

    if (data_out)
        *data_out = NULL;

    if (size_out)
        *size_out = 0;

    if (!oefs || !path || !data_out || !size_out)
        RAISE(OE_EINVAL);

    CHECK(oefs_open(fs, path, 0, 0, &file));
    CHECK(_load_file(file, &data, &size));

    *data_out = data;
    *size_out = size;

    data = NULL;

done:

    if (file)
        oefs_close(file);

    if (data)
        free(data);

    return err;
}

oe_errno_t oefs_mkdir(fs_t* fs, const char* path, uint32_t mode)
{
    oefs_t* oefs = (oefs_t*)fs;
    oe_errno_t err = OE_EOK;
    char dirname[FS_PATH_MAX];
    char basename[FS_PATH_MAX];
    uint32_t dir_ino;
    uint32_t ino;
    fs_file_t* file = NULL;

    if (!oefs || !path)
        RAISE(OE_EINVAL);

    /* Split the path into parent path and final component. */
    CHECK(_split_path(path, dirname, basename));

    /* Get the inode of the parent directory. */
    CHECK(_path_to_ino(oefs, dirname, NULL, &dir_ino, NULL));

    /* Create the new directory file. */
    CHECK(_create_file(oefs, dir_ino, basename, FS_DT_DIR, &ino));

    /* Open the newly created directory */
    CHECK(_open_file(oefs, ino, &file));

    /* Write the empty directory contents. */
    {
        fs_dirent_t dirents[2];
        int32_t nwritten;

        /* Initialize the ".." directory. */
        dirents[0].d_ino = dir_ino;
        dirents[0].d_off = 0;
        dirents[0].d_reclen = sizeof(fs_dirent_t);
        dirents[0].d_type = FS_DT_DIR;
        strcpy(dirents[0].d_name, "..");

        /* Initialize the "." directory. */
        dirents[1].d_ino = ino;
        dirents[1].d_off = sizeof(fs_dirent_t);
        dirents[1].d_reclen = sizeof(fs_dirent_t);
        dirents[1].d_type = FS_DT_DIR;
        strcpy(dirents[1].d_name, ".");

        CHECK(oefs_write(file, &dirents, sizeof(dirents), &nwritten));

        if (nwritten != sizeof(dirents))
            RAISE(OE_EIO);
    }

done:

    if (file)
        oefs_close(file);

    return err;
}

oe_errno_t oefs_create(
    fs_t* fs,
    const char* path,
    uint32_t mode,
    fs_file_t** file_out)
{
    oefs_t* oefs = (oefs_t*)fs;
    oe_errno_t err = OE_EOK;
    char dirname[FS_PATH_MAX];
    char basename[FS_PATH_MAX];
    uint32_t dir_ino;
    uint32_t ino;
    fs_file_t* file = NULL;

    if (file_out)
        *file_out = NULL;

    if (!oefs || !path || !file_out)
        RAISE(OE_EINVAL);

    /* Split the path into parent directory and file name */
    CHECK(_split_path(path, dirname, basename));

    /* Get the inode of the parent directory. */
    CHECK(_path_to_ino(oefs, dirname, NULL, &dir_ino, NULL));

    /* Create the new file. */
    CHECK(_create_file(oefs, dir_ino, basename, FS_DT_REG, &ino));

    /* Open the new file. */
    CHECK(_open_file(oefs, ino, &file));

    *file_out = file;
    file = NULL;

done:

    return err;
}

oe_errno_t oefs_link(
    fs_t* fs,
    const char* old_path,
    const char* new_path)
{
    oefs_t* oefs = (oefs_t*)fs;
    oe_errno_t err = OE_EOK;
    uint32_t ino;
    char dirname[FS_PATH_MAX];
    char basename[FS_PATH_MAX];
    uint32_t dir_ino;
    fs_file_t* dir = NULL;
    uint32_t release_ino = 0;

    if (!old_path || !new_path)
        RAISE(OE_EINVAL);

    /* Get the inode number of the old path. */
    {
        uint8_t type;

        CHECK(_path_to_ino(oefs, old_path, NULL, &ino, &type));

        /* Only regular files can be linked. */
        if (type != FS_DT_REG)
            RAISE(OE_EINVAL);
    }

    /* Split the new path. */
    CHECK(_split_path(new_path, dirname, basename));

    /* Open the destination directory. */
    {
        uint8_t type;

        CHECK(_path_to_ino(oefs, dirname, NULL, &dir_ino, &type));

        if (type != FS_DT_DIR)
            RAISE(OE_EINVAL);

        CHECK(_open_file(oefs, dir_ino, &dir));
    }

    /* Replace the destination file if it already exists. */
    for (;;)
    {
        fs_dirent_t ent;
        int32_t n;

        CHECK(oefs_read(dir, &ent, sizeof(ent), &n));

        if (n == 0)
            break;

        if (n != sizeof(ent))
            RAISE(OE_EBADF);

        if (strcmp(ent.d_name, basename) == 0)
        {
            release_ino = ent.d_ino;

            if (ent.d_type != FS_DT_REG)
                RAISE(OE_EINVAL);

            CHECK(oefs_lseek(dir, -sizeof(ent), FS_SEEK_CUR, NULL));
            ent.d_ino = ino;
            CHECK(oefs_write(dir, &ent, sizeof(ent), &n));

            break;
        }
    }

    /* Append the entry to the directory. */
    if (!release_ino)
    {
        fs_dirent_t ent;
        int32_t n;

        memset(&ent, 0, sizeof(ent));
        ent.d_ino = ino;
        ent.d_off = dir->offset;
        ent.d_reclen = sizeof(ent);
        ent.d_type = FS_DT_REG;
        strlcpy(ent.d_name, basename, sizeof(ent.d_name));

        CHECK(oefs_write(dir, &ent, sizeof(ent), &n));

        if (n != sizeof(ent))
            RAISE(OE_EIO);
    }

    /* Increment the number of links to this file. */
    {
        oefs_inode_t inode;

        CHECK(_load_inode(oefs, ino, &inode));
        inode.i_links++;
        CHECK(_write_block(oefs, ino, &inode));
    }

    /* Remove the destination file if it existed above. */
    if (release_ino)
        CHECK(_release_inode(oefs, release_ino));

done:

    if (dir)
        oefs_close(dir);

    return err;
}

oe_errno_t oefs_rename(
    fs_t* fs,
    const char* old_path,
    const char* new_path)
{
    oe_errno_t err = OE_EOK;

    if (!old_path || !new_path)
        RAISE(OE_EINVAL);

    CHECK(oefs_link(fs, old_path, new_path));
    CHECK(oefs_unlink(fs, old_path));

done:

    return err;
}

oe_errno_t oefs_unlink(fs_t* fs, const char* path)
{
    oefs_t* oefs = (oefs_t*)fs;
    oe_errno_t err = OE_EOK;
    uint32_t dir_ino;
    uint32_t ino;
    uint8_t type;
    char dirname[FS_PATH_MAX];
    char basename[FS_PATH_MAX];

    if (!oefs || !path)
        RAISE(OE_EINVAL);

    CHECK(_path_to_ino(oefs, path, &dir_ino, &ino, &type));

    /* Only regular files can be removed. */
    if (type != FS_DT_REG)
        RAISE(OE_EINVAL);

    CHECK(_split_path(path, dirname, basename));
    CHECK(_unlink_file(oefs, dir_ino, ino, basename));

done:

    return err;
}

oe_errno_t oefs_truncate(fs_t* fs, const char* path)
{
    oefs_t* oefs = (oefs_t*)fs;
    oe_errno_t err = OE_EOK;
    fs_file_t* file = NULL;
    uint32_t ino;
    uint8_t type;

    if (!oefs || !path)
        RAISE(OE_EINVAL);

    CHECK(_path_to_ino(oefs, path, NULL, &ino, &type));

    /* Only regular files can be truncated. */
    if (type != FS_DT_REG)
        RAISE(OE_EINVAL);

    CHECK(_open_file(oefs, ino, &file));
    CHECK(_truncate_file(file));

done:

    if (file)
        oefs_close(file);

    return err;
}

oe_errno_t oefs_rmdir(fs_t* fs, const char* path)
{
    oefs_t* oefs = (oefs_t*)fs;
    oe_errno_t err = OE_EOK;
    uint32_t dir_ino;
    uint32_t ino;
    uint8_t type;
    char dirname[FS_PATH_MAX];
    char basename[FS_PATH_MAX];

    if (!oefs || !path)
        RAISE(OE_EINVAL);

    CHECK(_path_to_ino(oefs, path, &dir_ino, &ino, &type));

    /* The path must refer to a directory. */
    if (type != FS_DT_DIR)
        RAISE(OE_ENOTDIR);

    /* The inode must be a directory and the directory must be empty. */
    {
        oefs_inode_t inode;

        CHECK(_load_inode(oefs, ino, &inode));

        if (!(inode.i_mode & FS_S_IFDIR))
            RAISE(OE_ENOTDIR);

        /* The directory must contain two entries: "." and ".." */
        if (inode.i_size != 2 * sizeof(fs_dirent_t))
            RAISE(OE_ENOTEMPTY);
    }

    CHECK(_split_path(path, dirname, basename));
    CHECK(_unlink_file(oefs, dir_ino, ino, basename));

done:

    return err;
}

oe_errno_t oefs_stat(fs_t* fs, const char* path, fs_stat_t* stat)
{
    oefs_t* oefs = (oefs_t*)fs;
    oe_errno_t err = OE_EOK;
    oefs_inode_t inode;
    uint32_t ino;
    uint8_t type;

    if (stat)
        memset(stat, 0, sizeof(fs_stat_t));

    if (!oefs || !path || !stat)
        RAISE(OE_EINVAL);

    CHECK(_path_to_ino(oefs, path, NULL, &ino, &type));
    CHECK(_load_inode(oefs, ino, &inode));

    stat->st_dev = 0;
    stat->st_ino = ino;
    stat->st_mode = inode.i_mode;
    stat->__st_padding = 0;
    stat->st_nlink = inode.i_links;
    stat->st_uid = inode.i_uid;
    stat->st_gid = inode.i_gid;
    stat->st_rdev = 0;
    stat->st_size = inode.i_size;
    stat->st_blksize = FS_BLOCK_SIZE;
    stat->st_blocks = inode.i_nblocks;
    stat->st_atime = inode.i_atime;
    stat->st_mtime = inode.i_mtime;
    stat->st_ctime = inode.i_ctime;

done:
    return err;
}

oe_errno_t oefs_lseek(
    fs_file_t* file,
    ssize_t offset,
    int whence,
    ssize_t* offset_out)
{
    oe_errno_t err = OE_EOK;
    ssize_t new_offset;

    if (offset_out)
        *offset_out = 0;

    if (!file)
        RAISE(OE_EINVAL);

    switch (whence)
    {
        case FS_SEEK_SET:
        {
            new_offset = offset;
            break;
        }
        case FS_SEEK_CUR:
        {
            new_offset = file->offset + offset;
            break;
        }
        case FS_SEEK_END:
        {
            new_offset = file->inode.i_size + offset;
            break;
        }
        default:
        {
            RAISE(OE_EINVAL);
        }
    }

    /* Check whether the new offset if out of range. */
    if (new_offset < 0 || new_offset > file->offset)
        RAISE(OE_EINVAL);

    file->offset = new_offset;

    if (offset_out)
        *offset_out = new_offset;

done:
    return err;
}
