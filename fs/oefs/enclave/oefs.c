// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _GNU_SOURCE

#include "oefs.h"
#include <assert.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "buf.h"
#include "raise.h"
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

#define BITS_PER_BLK (OEFS_BLOCK_SIZE * 8)

#define SUPER_BLOCK_MAGIC 0x0EF51234

#define INODE_MAGIC 0xe3d6e3eb

#define OEFS_MAGIC 0xe56231bf

#define FILE_MAGIC 0x4adaee1c

#define DIR_MAGIC 0xdb9916db

/* The logical block number of the root directory inode. */
#define ROOT_INO 1

/* The physical block number of the super block. */
#define SUPER_BLOCK_PHYSICAL_BLKNO 2

/* The first physical block number of the block bitmap. */
#define BITMAP_PHYSICAL_BLKNO 3

#define BLKNOS_PER_INODE \
    (sizeof(((oefs_inode_t*)0)->i_blocks) / sizeof(uint32_t))

#define BLKNOS_PER_BNODE \
    (sizeof(((oefs_bnode_t*)0)->b_blocks) / sizeof(uint32_t))

typedef struct _oefs_super_block
{
    /* Magic number: SUPER_BLOCK_MAGIC. */
    uint32_t s_magic;

    /* The total number of blocks in the file system. */
    uint32_t s_nblks;

    /* The number of free blocks. */
    uint32_t s_free_blocks;

    /* (12) Reserved. */
    uint8_t s_reserved[OEFS_BLOCK_SIZE - 12];
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

    oe_blkdev_t* dev;

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
    oefs_bufu32_t blknos;

    /* The block numbers that contain the file's block numbers. */
    oefs_bufu32_t bnode_blknos;

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
    return blkno + (3 + num_bitmap_blocks) - 1;
}

static int _read_block(oefs_t* oefs, size_t blkno, oe_blk_t* blk)
{
    int err = 0;
    uint32_t physical_blkno;

    /* Check whether the block number is valid. */
    if (blkno == 0 || blkno > oefs->sb.read.s_nblks)
        RAISE(EIO);

    /* Sanity check: make sure the block is not free. */
    if (!_test_bit(oefs->bitmap.read, oefs->bitmap_size, blkno - 1))
        RAISE(EIO);

    /* Convert the logical block number to a physical block number. */
    physical_blkno = _get_physical_blkno(oefs, blkno);

    /* Get the physical block. */
    if (oefs->dev->get(oefs->dev, physical_blkno, blk) != 0)
        RAISE(EIO);

done:
    return err;
}

static int _write_block(oefs_t* oefs, size_t blkno, const oe_blk_t* blk)
{
    int err = 0;
    uint32_t physical_blkno;

    /* Check whether the block number is valid. */
    if (blkno == 0 || blkno > oefs->sb.read.s_nblks)
        RAISE(EINVAL);

    /* Sanity check: make sure the block is not free. */
    if (!_test_bit(oefs->bitmap.read, oefs->bitmap_size, blkno - 1))
        RAISE(EIO);

    /* Convert the logical block number to a physical block number. */
    physical_blkno = _get_physical_blkno(oefs, blkno);

    /* Get the physical block. */
    if (oefs->dev->put(oefs->dev, physical_blkno, blk) != 0)
        RAISE(EIO);

done:
    return err;
}

static int _load_inode(oefs_t* oefs, uint32_t ino, oefs_inode_t* inode)
{
    int err = 0;

    /* Read this inode into memory. */
    CHECK(_read_block(oefs, ino, (oe_blk_t*)inode));

    /* Check the inode magic number. */
    if (inode->i_magic != INODE_MAGIC)
        RAISE(EIO);

done:
    return err;
}

static int _load_blknos(
    oefs_t* oefs,
    oefs_inode_t* inode,
    oefs_bufu32_t* bnode_blknos,
    oefs_bufu32_t* blknos)
{
    int err = 0;

    /* Collect direct block numbers from the inode. */
    {
        size_t n = sizeof(inode->i_blocks) / sizeof(uint32_t);

        for (size_t i = 0; i < n && inode->i_blocks[i]; i++)
        {
            if (oefs_bufu32_append(blknos, &inode->i_blocks[i], 1) != 0)
                RAISE(ENOMEM);
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
            CHECK(_read_block(oefs, next, (oe_blk_t*)&bnode));

            /* Append this bnode blkno. */
            if (oefs_bufu32_append(bnode_blknos, &next, 1) != 0)
                RAISE(ENOMEM);

            n = sizeof(bnode.b_blocks) / sizeof(uint32_t);

            /* Get all blocks from this bnode. */
            for (size_t i = 0; i < n && bnode.b_blocks[i]; i++)
            {
                if (oefs_bufu32_append(blknos, &bnode.b_blocks[i], 1) != 0)
                    RAISE(ENOMEM);
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
        RAISE(EINVAL);

    if (!(file = calloc(1, sizeof(oefs_file_t))))
        RAISE(ENOMEM);

    file->magic = FILE_MAGIC;
    file->oefs = oefs;
    file->ino = ino;

    /* Read this inode into memory. */
    CHECK(_load_inode(oefs, ino, &file->inode));

    /* Load the block numbers into memory. */
    CHECK(
        _load_blknos(oefs, &file->inode, &file->bnode_blknos, &file->blknos));

    *file_out = file;
    file = NULL;

done:

    if (file)
    {
        oefs_bufu32_release(&file->blknos);
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

    for (uint32_t i = 0; i < oefs->nblks; i++)
    {
        if (!_test_bit(oefs->bitmap.read, oefs->bitmap_size, i))
        {
            _set_bit(_bitmap_write(oefs), oefs->bitmap_size, i);
            *blkno = i + 1;
            _sb_write(oefs)->s_free_blocks--;
            RAISE(0);
        }
    }

    RAISE(ENOSPC);

done:
    return err;
}

static int _unassign_blkno(oefs_t* oefs, uint32_t blkno)
{
    int err = 0;
    uint32_t index = blkno - 1;

    if (blkno == 0 || index >= oefs->nblks)
        RAISE(EINVAL);

    assert(_test_bit(oefs->bitmap.read, oefs->bitmap_size, index));

    _clr_bit(_bitmap_write(oefs), oefs->bitmap_size, index);
    _sb_write(oefs)->s_free_blocks++;

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

        if (oefs->dev->put(oefs->dev, blkno, (const oe_blk_t*)&oefs->sb) != 0)
            RAISE(EIO);

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
        oe_blk_t* blk = (oe_blk_t*)oefs->bitmap.read + i;
        oe_blk_t* blk_copy = (oe_blk_t*)oefs->bitmap_copy + i;

        /* Flush this bitmap block if it changed. */
        if (memcmp(blk, blk_copy, OEFS_BLOCK_SIZE) != 0)
        {
            /* Calculate the block number to write. */
            uint32_t blkno = BITMAP_PHYSICAL_BLKNO + i;

            if (oefs->dev->put(oefs->dev, blkno, blk) != 0)
                RAISE(EIO);

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
        RAISE(EINVAL);

    if (oefs->dirty)
    {
        CHECK(_flush_bitmap(oefs));
        CHECK(_flush_super_block(oefs));
        oefs->dirty = false;
    }

done:
    return err;
}

static int _write_data(
    oefs_t* oefs,
    const void* data,
    uint32_t size,
    oefs_bufu32_t* blknos)
{
    int err = 0;
    const uint8_t* ptr = (const uint8_t*)data;
    uint32_t remaining = size;

    /* While there is data remaining to be written. */
    while (remaining)
    {
        uint32_t blkno;
        oe_blk_t blk;
        uint32_t copy_size;

        /* Assign a new block number from the in-memory bitmap. */
        CHECK(_assign_blkno(oefs, &blkno));

        /* Append the new block number to the array of blocks numbers. */
        if (oefs_bufu32_append(blknos, &blkno, 1) != 0)
            RAISE(ENOMEM);

        /* Calculate bytes to copy to the block. */
        copy_size = (remaining > OEFS_BLOCK_SIZE) ? OEFS_BLOCK_SIZE : remaining;

        /* Clear the block. */
        memset(blk.data, 0, sizeof(oe_blk_t));

        /* Copy user data to the block. */
        memcpy(blk.data, ptr, copy_size);

        /* Write the new block. */
        CHECK(_write_block(oefs, blkno, &blk));

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

static int _append_block_chain(
    oefs_file_t* file,
    const oefs_bufu32_t* blknos)
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
            RAISE(EIO);

        blkno = file->bnode_blknos.data[file->bnode_blknos.size - 1];

        CHECK(_read_block(file->oefs, blkno, (oe_blk_t*)&bnode));

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
            CHECK(_assign_blkno(file->oefs, &new_blkno));

            /* Append the bnode blkno to the file struct. */
            if (oefs_bufu32_append(&file->bnode_blknos, &new_blkno, 1) != 0)
                RAISE(ENOMEM);

            *next = new_blkno;

            /* Rewrite the current inode or bnode. */
            CHECK(_write_block(file->oefs, blkno, object));

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
            CHECK(_write_block(file->oefs, blkno, object));
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
        RAISE(EINVAL);

    /* Reject paths that are not absolute */
    if (path[0] != '/')
        RAISE(EINVAL);

    /* Handle root directory up front */
    if (strcmp(path, "/") == 0)
    {
        strlcpy(dirname, "/", OEFS_PATH_MAX);
        strlcpy(basename, "/", OEFS_PATH_MAX);
        RAISE(0);
    }

    /* This cannot fail (prechecked) */
    if (!(slash = strrchr(path, '/')))
        RAISE(EINVAL);

    /* If path ends with '/' character */
    if (!slash[1])
        RAISE(EINVAL);

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
        RAISE(EINVAL);

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
            RAISE(EINVAL);

        CHECK(_assign_blkno(oefs, &ino));
        CHECK(_write_block(oefs, ino, (const oe_blk_t*)&inode));
    }

    /* Create an inode for the new file. */

    /* Add an entry to the directory for this inode. */
    {
        oefs_dirent_t ent;
        ssize_t n;

        CHECK(_open(oefs, dir_ino, &file));

        /* Check for duplicates. */
        for (;;)
        {
            CHECK(oefs_read(file, &ent, sizeof(ent), &n));

            if (n == 0)
                break;

            assert(n == sizeof(ent));

            if (n != sizeof(ent))
                RAISE(EIO);

            if (strcmp(ent.d_name, name) == 0)
                RAISE(EEXIST);
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
                RAISE(EIO);
        }
    }

    /* Create an inode for the new file. */

    if (ino_out)
        *ino_out = ino;

done:

    if (file)
        oefs_close(file);

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
        RAISE(EINVAL);

    assert(_sane_file(file));

    oefs = file->oefs;

    /* Fail if length is greater than the size of the file. */
    if (length > file->inode.i_size)
        RAISE(EFBIG);

    /* Calculate the index of the first block to be freed. */
    block_index = (length + OEFS_BLOCK_SIZE - 1) / OEFS_BLOCK_SIZE;

    /* Release the truncated data blocks. */
    {
        for (size_t i = block_index; i < file->blknos.size; i++)
        {
            CHECK(_unassign_blkno(oefs, file->blknos.data[i]));
            file->inode.i_nblks--;
        }

        if (oefs_bufu32_resize(&file->blknos, block_index) != 0)
            RAISE(ENOMEM);
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
            CHECK(_unassign_blkno(oefs, file->bnode_blknos.data[i]));
        }

        if (oefs_bufu32_resize(&file->bnode_blknos, 0) != 0)
            RAISE(ENOMEM);
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
                RAISE(EOVERFLOW);

            bnode_index--;
            slot_index = BLKNOS_PER_BNODE;
        }

        /* Load the bnode into memory. */
        bnode_blkno = file->bnode_blknos.data[bnode_index];
        CHECK(_read_block(oefs, bnode_blkno, (oe_blk_t*)&bnode));

        /* Clear any slots in the bnode. */
        for (size_t i = slot_index; i < BLKNOS_PER_BNODE; i++)
            bnode.b_blocks[i] = 0;

        /* Unlink this bnode from next bnode. */
        bnode.b_next = 0;

        /* Release unused bnode blocks. */
        for (size_t i = bnode_index + 1; i < file->bnode_blknos.size; i++)
        {
            CHECK(_unassign_blkno(oefs, file->bnode_blknos.data[i]));
        }

        if (oefs_bufu32_resize(&file->bnode_blknos, bnode_index + 1) != 0)
            RAISE(ENOMEM);

        /* Rewrite the bnode. */
        CHECK(_write_block(oefs, bnode_blkno, (const oe_blk_t*)&bnode));
    }

    /* Update the inode. */
    file->inode.i_size = length;

    /* Update the file offset. */
    file->offset = 0;

    /* Sync the inode to disk. */
    CHECK(
        _write_block(file->oefs, file->ino, (const oe_blk_t*)&file->inode));

done:

    assert(_sane_file(file));

    if (_flush(file->oefs) != 0)
        return EIO;

    return err;
}

static int _load_file(oefs_file_t* file, void** data_out, size_t* size_out)
{
    int err = 0;
    oefs_buf_t buf = BUF_INITIALIZER;
    char data[OEFS_BLOCK_SIZE];
    ssize_t n;

    *data_out = NULL;
    *size_out = 0;

    for (;;)
    {
        CHECK(oefs_read(file, data, sizeof(data), &n));

        if (n == 0)
            break;

        if (oefs_buf_append(&buf, data, n) != 0)
            RAISE(ENOMEM);
    }

    *data_out = buf.data;
    *size_out = buf.size;
    memset(&buf, 0, sizeof(buf));

done:

    oefs_buf_release(&buf);

    return err;
}

static int _release_inode(oefs_t* oefs, uint32_t ino)
{
    int err = 0;
    oefs_file_t* file = NULL;

    CHECK(_open(oefs, ino, &file));
    CHECK(_truncate(file, 0));
    CHECK(_unassign_blkno(oefs, file->ino));

done:

    if (file)
        oefs_close(file);

    if (_flush(oefs) != 0)
        return EIO;

    return err;
}

int _unlink(
    oefs_t* oefs,
    uint32_t dir_ino,
    uint32_t ino,
    const char* name)
{
    int err = 0;
    oefs_file_t* dir = NULL;
    oefs_file_t* file = NULL;
    void* data = NULL;
    size_t size = 0;
    oefs_inode_t inode;

    if (!oefs || !dir_ino || !ino)
        RAISE(EINVAL);

    /* Open the directory file. */
    CHECK(_open(oefs, dir_ino, &dir));

    /* Load the contents of the parent directory into memory. */
    CHECK(_load_file(dir, &data, &size));

    /* File must be a multiple of the entry size. */
    {
        assert((size % sizeof(oefs_dirent_t)) == 0);

        if (size % sizeof(oefs_dirent_t))
            RAISE(EIO);
    }

    /* Truncate the parent directory. */
    CHECK(_truncate(dir, 0));

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

                CHECK(oefs_write(dir, &entries[i], n, &nwritten));

                if (nwritten != n)
                    RAISE(EIO);
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
        CHECK(_write_block(oefs, ino, (const oe_blk_t*)&inode));
    }

done:

    if (dir)
        oefs_close(dir);

    if (file)
        oefs_close(file);

    if (data)
        free(data);

    if (_flush(oefs) != 0)
        return EIO;

    return err;
}

static int _opendir_by_ino(
    oefs_t* oefs,
    uint32_t ino,
    oefs_dir_t** dir_out)
{
    int err = 0;
    oefs_dir_t* dir = NULL;
    oefs_file_t* file = NULL;

    if (dir_out)
        *dir_out = NULL;

    if (!oefs || !ino || !dir_out)
        RAISE(EINVAL);

    CHECK(_open(oefs, ino, &file));

    if (!(dir = calloc(1, sizeof(oefs_dir_t))))
        RAISE(ENOMEM);

    dir->magic = DIR_MAGIC;
    dir->ino = ino;
    dir->file = file;
    file = NULL;

    *dir_out = dir;

done:

    if (file)
        oefs_close(file);

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
        RAISE(EINVAL);

    /* Check path length */
    if (strlen(path) >= OEFS_PATH_MAX)
        RAISE(ENAMETOOLONG);

    /* Make copy of the path. */
    strlcpy(buf, path, sizeof(buf));

    /* Be sure path begins with "/" */
    if (path[0] != '/')
        RAISE(EINVAL);

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

            CHECK(_opendir_by_ino(oefs, current_ino, &dir));

            dir_ino = current_ino;
            current_ino = 0;

            for (;;)
            {
                CHECK(oefs_readdir(dir, &ent));

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
                RAISE(ENOENT);

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

int oefs_read(
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
        RAISE(EINVAL);

    /* The index of the first block to read. */
    first = file->offset / OEFS_BLOCK_SIZE;

    /* The number of bytes remaining to be read */
    remaining = size;

    /* Read the data block-by-block */
    for (i = first; i < file->blknos.size && remaining > 0 && !file->eof; i++)
    {
        oe_blk_t blk;
        uint32_t offset;

        CHECK(_read_block(file->oefs, file->blknos.data[i], &blk));

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

int oefs_write(
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

    _begin(oefs);

    if (nwritten)
        *nwritten = 0;

    /* Check parameters */
    if (!_valid_file(file) || !file->oefs || (!data && size) || !nwritten)
        RAISE(EINVAL);

    assert(_sane_file(file));

    if (!_sane_file(file))
        RAISE(EINVAL);

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
        oe_blk_t blk;
        uint32_t offset;

        CHECK(_read_block(file->oefs, file->blknos.data[i], &blk));

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
        CHECK(_write_block(file->oefs, file->blknos.data[i], &blk));
    }

    /* Append remaining data to the file. */
    if (remaining)
    {
        oefs_bufu32_t blknos = BUF_U32_INITIALIZER;

        /* Write the new blocks. */
        CHECK(_write_data(file->oefs, ptr, remaining, &blknos));

        /* Append these block numbers to the block-numbers chain. */
        CHECK(_append_block_chain(file, &blknos));

        /* Update the inode's total_blocks */
        file->inode.i_nblks += blknos.size;

        /* Append these block numbers to the file. */
        if (oefs_bufu32_append(&file->blknos, blknos.data, blknos.size) != 0)
            RAISE(ENOMEM);

        oefs_bufu32_release(&blknos);

        file->offset += remaining;
        remaining = 0;
    }

    /* Update the file size. */
    file->inode.i_size = new_file_size;

    /* Flush the inode to disk. */
    CHECK(
        _write_block(file->oefs, file->ino, (const oe_blk_t*)&file->inode));

    /* Calculate number of bytes written */
    *nwritten = size - remaining;

done:

    if (_flush(oefs) != 0)
        return EIO;

    _end(oefs);

    return err;
}

int oefs_close(oefs_file_t* file)
{
    int err = 0;
    oefs_t* oefs = _oefs_from_file(file);

    if (!_valid_file(file))
        RAISE(EINVAL);

    oefs_bufu32_release(&file->blknos);
    oefs_bufu32_release(&file->bnode_blknos);
    memset(file, 0, sizeof(oefs_file_t));
    free(file);

done:

    if (_flush(oefs) != 0)
        return EIO;

    return err;
}

int oefs_release(oefs_t* oefs)
{
    int err = 0;

    if (!_valid_oefs(oefs) || !oefs->bitmap.read || !oefs->bitmap_copy)
        RAISE(EINVAL);

    oefs->dev->release(oefs->dev);
    free(_bitmap_write(oefs));
    free(oefs->bitmap_copy);
    memset(oefs, 0, sizeof(oefs_t));
    free(oefs);

done:

    return err;
}

int oefs_opendir(oefs_t* oefs, const char* path, oefs_dir_t** dir)
{
    int err = 0;
    uint32_t ino;

    if (dir)
        *dir = NULL;

    if (!_valid_oefs(oefs) || !path || !dir)
        RAISE(EINVAL);

    CHECK(_path_to_ino(oefs, path, NULL, &ino, NULL));
    CHECK(_opendir_by_ino(oefs, ino, dir));

done:

    if (_flush(oefs) != 0)
        return EIO;

    return err;
}

int oefs_readdir(oefs_dir_t* dir, oefs_dirent_t** ent)
{
    int err = 0;
    oefs_t* oefs = _oefs_from_dir(dir);
    ssize_t nread = 0;

    if (ent)
        *ent = NULL;

    if (!_valid_dir(dir) || !dir->file)
        RAISE(EINVAL);

    CHECK(oefs_read(dir->file, &dir->ent, sizeof(oefs_dirent_t), &nread));

    /* Check for end of file. */
    if (nread == 0)
        RAISE(0);

    /* Check for an illegal read size. */
    if (nread != sizeof(oefs_dirent_t))
        RAISE(EBADF);

    *ent = &dir->ent;

done:

    if (_flush(oefs) != 0)
        return EIO;

    return err;
}

int oefs_closedir(oefs_dir_t* dir)
{
    int err = 0;
    oefs_t* oefs = _oefs_from_dir(dir);

    if (!_valid_dir(dir) || !dir->file)
        RAISE(EINVAL);

    oefs_close(dir->file);
    dir->file = NULL;
    free(dir);

done:

    if (_flush(oefs) != 0)
        return EIO;

    return err;
}

int oefs_open(
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
        RAISE(EINVAL);

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
            RAISE(EINVAL);
    }

    /* Get the file's inode (only if it exists). */
    tmp_err = _path_to_ino(oefs, path, NULL, &ino, &type);

    /* If the file already exists. */
    if (tmp_err == 0)
    {
        if ((flags & OEFS_O_CREAT && flags & OEFS_O_EXCL))
            RAISE(EEXIST);

        if ((flags & OEFS_O_DIRECTORY) && (type != OEFS_DT_DIR))
            RAISE(ENOTDIR);

        CHECK(_open(oefs, ino, &file));

        if (flags & OEFS_O_TRUNC)
            CHECK(_truncate(file, 0));

        if (flags & OEFS_O_APPEND)
            file->offset = file->inode.i_size;
    }
    else if (tmp_err == ENOENT)
    {
        char dirname[OEFS_PATH_MAX];
        char basename[OEFS_PATH_MAX];
        uint32_t dir_ino;

        if (!(flags & OEFS_O_CREAT))
            RAISE(ENOENT);

        /* Split the path into parent directory and file name */
        CHECK(_split_path(path, dirname, basename));

        /* Get the inode of the parent directory. */
        CHECK(_path_to_ino(oefs, dirname, NULL, &dir_ino, NULL));

        /* Create the new file. */
        CHECK(_creat(oefs, dir_ino, basename, OEFS_DT_REG, &ino));

        /* Open the new file. */
        CHECK(_open(oefs, ino, &file));
    }
    else
    {
        RAISE(tmp_err);
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

int oefs_mkdir(oefs_t* oefs, const char* path, uint32_t mode)
{
    int err = 0;
    char dirname[OEFS_PATH_MAX];
    char basename[OEFS_PATH_MAX];
    uint32_t dir_ino;
    uint32_t ino;
    oefs_file_t* file = NULL;

    _begin(oefs);

    if (!_valid_oefs(oefs) || !path)
        RAISE(EINVAL);

    /* Split the path into parent path and final component. */
    CHECK(_split_path(path, dirname, basename));

    /* Get the inode of the parent directory. */
    CHECK(_path_to_ino(oefs, dirname, NULL, &dir_ino, NULL));

    /* Create the new directory file. */
    CHECK(_creat(oefs, dir_ino, basename, OEFS_DT_DIR, &ino));

    /* Open the newly created directory */
    CHECK(_open(oefs, ino, &file));

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

        CHECK(oefs_write(file, &dirents, sizeof(dirents), &nwritten));

        if (nwritten != sizeof(dirents))
            RAISE(EIO);
    }

done:

    if (file)
        oefs_close(file);

    if (_flush(oefs) != 0)
        return EIO;

    _end(oefs);

    return err;
}

int oefs_creat(
    oefs_t* oefs,
    const char* path,
    uint32_t mode,
    oefs_file_t** file_out)
{
    int err = 0;
    int flags = OEFS_O_CREAT | OEFS_O_WRONLY | OEFS_O_TRUNC;

    _begin(oefs);

    CHECK(oefs_open(oefs, path, flags, mode, file_out));

done:

    _end(oefs);

    return err;
}

int oefs_link(oefs_t* oefs, const char* old_path, const char* new_path)
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
        RAISE(EINVAL);

    /* Get the inode number of the old path. */
    {
        uint8_t type;

        CHECK(_path_to_ino(oefs, old_path, NULL, &ino, &type));

        /* Only regular files can be linked. */
        if (type != OEFS_DT_REG)
            RAISE(EINVAL);
    }

    /* Split the new path. */
    CHECK(_split_path(new_path, dirname, basename));

    /* Open the destination directory. */
    {
        uint8_t type;

        CHECK(_path_to_ino(oefs, dirname, NULL, &dir_ino, &type));

        if (type != OEFS_DT_DIR)
            RAISE(EINVAL);

        CHECK(_open(oefs, dir_ino, &dir));
    }

    /* Replace the destination file if it already exists. */
    for (;;)
    {
        oefs_dirent_t ent;
        ssize_t n;

        CHECK(oefs_read(dir, &ent, sizeof(ent), &n));

        if (n == 0)
            break;

        if (n != sizeof(ent))
        {
            RAISE(EBADF);
        }

        if (strcmp(ent.d_name, basename) == 0)
        {
            release_ino = ent.d_ino;

            if (ent.d_type != OEFS_DT_REG)
                RAISE(EINVAL);

            CHECK(oefs_lseek(dir, -sizeof(ent), OEFS_SEEK_CUR, NULL));
            ent.d_ino = ino;
            CHECK(oefs_write(dir, &ent, sizeof(ent), &n));

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

        CHECK(oefs_write(dir, &ent, sizeof(ent), &n));

        if (n != sizeof(ent))
            RAISE(EIO);
    }

    /* Increment the number of links to this file. */
    {
        oefs_inode_t inode;

        CHECK(_load_inode(oefs, ino, &inode));
        inode.i_links++;
        CHECK(_write_block(oefs, ino, (const oe_blk_t*)&inode));
    }

    /* Remove the destination file if it existed above. */
    if (release_ino)
        CHECK(_release_inode(oefs, release_ino));

done:

    if (dir)
        oefs_close(dir);

    if (_flush(oefs) != 0)
        return EIO;

    _end(oefs);

    return err;
}

int oefs_rename(oefs_t* oefs, const char* old_path, const char* new_path)
{
    int err = 0;

    _begin(oefs);

    if (!_valid_oefs(oefs) || !old_path || !new_path)
        RAISE(EINVAL);

    CHECK(oefs_link(oefs, old_path, new_path));
    CHECK(oefs_unlink(oefs, old_path));

done:

    if (_flush(oefs) != 0)
        return EIO;

    _end(oefs);

    return err;
}

int oefs_unlink(oefs_t* oefs, const char* path)
{
    int err = 0;
    uint32_t dir_ino;
    uint32_t ino;
    uint8_t type;
    char dirname[OEFS_PATH_MAX];
    char basename[OEFS_PATH_MAX];

    _begin(oefs);

    if (!_valid_oefs(oefs) || !path)
        RAISE(EINVAL);

    CHECK(_path_to_ino(oefs, path, &dir_ino, &ino, &type));

    /* Only regular files can be removed. */
    if (type != OEFS_DT_REG)
        RAISE(EINVAL);

    CHECK(_split_path(path, dirname, basename));
    CHECK(_unlink(oefs, dir_ino, ino, basename));

done:

    if (_flush(oefs) != 0)
        return EIO;

    _end(oefs);

    return err;
}

int oefs_truncate(oefs_t* oefs, const char* path, ssize_t length)
{
    int err = 0;
    oefs_file_t* file = NULL;
    uint32_t ino;
    uint8_t type;

    _begin(oefs);

    if (!_valid_oefs(oefs) || !path)
        RAISE(EINVAL);

    CHECK(_path_to_ino(oefs, path, NULL, &ino, &type));

    /* Only regular files can be truncated. */
    if (type != OEFS_DT_REG)
        RAISE(EINVAL);

    CHECK(_open(oefs, ino, &file));
    CHECK(_truncate(file, length));

done:

    if (file)
        oefs_close(file);

    if (_flush(oefs) != 0)
        return EIO;

    _end(oefs);

    return err;
}

int oefs_rmdir(oefs_t* oefs, const char* path)
{
    int err = 0;
    uint32_t dir_ino;
    uint32_t ino;
    uint8_t type;
    char dirname[OEFS_PATH_MAX];
    char basename[OEFS_PATH_MAX];

    _begin(oefs);

    if (!_valid_oefs(oefs) || !path)
        RAISE(EINVAL);

    CHECK(_path_to_ino(oefs, path, &dir_ino, &ino, &type));

    /* The path must refer to a directory. */
    if (type != OEFS_DT_DIR)
        RAISE(ENOTDIR);

    /* The inode must be a directory and the directory must be empty. */
    {
        oefs_inode_t inode;

        CHECK(_load_inode(oefs, ino, &inode));

        if (!(inode.i_mode & OEFS_S_IFDIR))
            RAISE(ENOTDIR);

        /* The directory must contain two entries: "." and ".." */
        if (inode.i_size != 2 * sizeof(oefs_dirent_t))
            RAISE(ENOTEMPTY);
    }

    CHECK(_split_path(path, dirname, basename));
    CHECK(_unlink(oefs, dir_ino, ino, basename));

done:

    if (_flush(oefs) != 0)
        return EIO;

    _end(oefs);

    return err;
}

int oefs_stat(oefs_t* oefs, const char* path, oefs_stat_t* stat)
{
    int err = 0;
    oefs_inode_t inode;
    uint32_t ino;
    uint8_t type;

    if (stat)
        memset(stat, 0, sizeof(oefs_stat_t));

    if (!_valid_oefs(oefs) || !path || !stat)
        RAISE(EINVAL);

    CHECK(_path_to_ino(oefs, path, NULL, &ino, &type));
    CHECK(_load_inode(oefs, ino, &inode));

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

int oefs_lseek(
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
        RAISE(EINVAL);

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
            RAISE(EINVAL);
        }
    }

    /* Check whether the new offset if out of range. */
    if (new_offset < 0 || new_offset > file->offset)
        RAISE(EINVAL);

    file->offset = new_offset;

    if (offset_out)
        *offset_out = new_offset;

done:

    if (_flush(oefs) != 0)
        return EIO;

    return err;
}

int oefs_mkfs(oe_blkdev_t* dev, size_t nblks)
{
    int err = 0;
    size_t num_bitmap_blocks;
    uint32_t blkno = 0;
    oe_blk_t empty_block;

    if (dev)
        dev->begin(dev);

    if (!dev || nblks < 2 || !oefs_is_pow_of_2(nblks))
        RAISE(EINVAL);

    /* Initialize an empty block. */
    memset(&empty_block, 0, sizeof(empty_block));

    /* Calculate the number of bitmap blocks. */
    num_bitmap_blocks = _num_bitmap_blocks(nblks);

    /* Write empty block one. */
    if (dev->put(dev, blkno++, &empty_block) != 0)
        RAISE(EIO);

    /* Write empty block two. */
    if (dev->put(dev, blkno++, &empty_block) != 0)
        RAISE(EIO);

    /* Write the super block */
    {
        oefs_super_block_t sb;
        memset(&sb, 0, sizeof(sb));

        sb.s_magic = SUPER_BLOCK_MAGIC;
        sb.s_nblks = nblks;
        sb.s_free_blocks = nblks - 3;

        if (dev->put(dev, blkno++, (const oe_blk_t*)&sb) != 0)
            RAISE(EIO);
    }

    /* Write the first bitmap block. */
    {
        oe_blk_t blk;

        memset(&blk, 0, sizeof(blk));

        /* Set the bit for the root inode. */
        _set_bit(blk.data, sizeof(blk), 0);

        /* Set the bits for the root directory. */
        _set_bit(blk.data, sizeof(blk), 1);
        _set_bit(blk.data, sizeof(blk), 2);

        /* Write the block. */
        if (dev->put(dev, blkno++, &blk) != 0)
            RAISE(EIO);
    }

    /* Write the subsequent (empty) bitmap blocks. */
    for (size_t i = 1; i < num_bitmap_blocks; i++)
    {
        if (dev->put(dev, blkno++, &empty_block) != 0)
            RAISE(EIO);
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

        if (dev->put(dev, blkno++, (const oe_blk_t*)&root_inode) != 0)
            RAISE(EIO);
    }

    /* Write the ".." and "." directory entries for the root directory.  */
    {
        oe_blk_t blocks[2];

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
            RAISE(EIO);

        if (dev->put(dev, blkno++, &blocks[1]) != 0)
            RAISE(EIO);
    }

    /* Count the remaining empty data blocks. */
    blkno += nblks - 3;

    /* Cross check the actual size against computed size. */
    {
        size_t size;
        CHECK(oefs_size(nblks, &size));

        if (size != (blkno * OEFS_BLOCK_SIZE))
            RAISE(EIO);
    }

done:

    if (dev)
        dev->begin(dev);

    return err;
}

int oefs_size(size_t nblks, size_t* size)
{
    int err = 0;
    size_t total_blocks = 0;

    if (size)
        *size = 0;

    if (nblks < 2 || !oefs_is_pow_of_2(nblks) || !size)
        goto done;

    /* Count the first two empty blocks. */
    total_blocks += 2;

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

int oefs_initialize(oefs_t** oefs_out, oe_blkdev_t* dev)
{
    int err = 0;
    size_t nblks;
    uint8_t* bitmap = NULL;
    uint8_t* bitmap_copy = NULL;
    oefs_t* oefs = NULL;

    if (oefs_out)
        *oefs_out = NULL;

    if (!oefs_out || !dev)
        RAISE(EINVAL);

    if (!(oefs = calloc(1, sizeof(oefs_t))))
        RAISE(ENOMEM);

    /* Save pointer to device (caller is responsible for releasing it). */
    oefs->dev = dev;

    /* Read the super block from the file system. */
    if (dev->get(dev, SUPER_BLOCK_PHYSICAL_BLKNO, (oe_blk_t*)&oefs->sb) != 0)
        RAISE(EIO);

    /* Check the superblock magic number. */
    if (oefs->sb.read.s_magic != SUPER_BLOCK_MAGIC)
        RAISE(EIO);

    /* Make copy of super block. */
    memcpy(&oefs->sb_copy, &oefs->sb, sizeof(oefs->sb_copy));

    /* Get the number of blocks. */
    nblks = oefs->sb.read.s_nblks;

    /* Check that the number of blocks is correct. */
    if (!nblks)
        RAISE(EIO);

    /* Allocate space for the block bitmap. */
    {
        /* Calculate the number of bitmap blocks. */
        size_t num_bitmap_blocks = _num_bitmap_blocks(nblks);

        /* Calculate the size of the bitmap. */
        size_t bitmap_size = num_bitmap_blocks * OEFS_BLOCK_SIZE;

        /* Allocate the bitmap. */
        if (!(bitmap = calloc(1, bitmap_size)))
            RAISE(ENOMEM);

        /* Allocate the bitmap. */
        if (!(bitmap_copy = calloc(1, bitmap_size)))
            RAISE(ENOMEM);

        /* Read the bitset into memory */
        for (size_t i = 0; i < num_bitmap_blocks; i++)
        {
            uint8_t* ptr = bitmap + (i * OEFS_BLOCK_SIZE);

            if (dev->get(dev, BITMAP_PHYSICAL_BLKNO + i, (oe_blk_t*)ptr) != 0)
                RAISE(EIO);
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
            RAISE(EIO);
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

        CHECK(_read_block(oefs, ROOT_INO, (oe_blk_t*)&inode));

        if (inode.i_magic != INODE_MAGIC)
            RAISE(EIO);
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

int oefs_new(
    oefs_t** oefs_out,
    const char* source,
    uint32_t flags,
    size_t nblks,
    const uint8_t key[OEFS_KEY_SIZE])
{
    int ret = -1;
    oe_blkdev_t* host_dev = NULL;
    oe_blkdev_t* crypto_dev = NULL;
    oe_blkdev_t* cache_dev = NULL;
    oe_blkdev_t* merkle_dev = NULL;
    oe_blkdev_t* dev = NULL;
    oe_blkdev_t* next = NULL;
    oefs_t* oefs = NULL;
    size_t extra_nblks = 0;

    (void)extra_nblks;

    if (oefs_out)
        *oefs_out = NULL;

    if (!oefs_out || !source || nblks < 2 || !oefs_is_pow_of_2(nblks))
        goto done;

    /* Open a host device. */
    if (oefs_host_blkdev_open(&host_dev, source) != 0)
        goto done;

    next = host_dev;

    /* Get the number of extra blocks needed by the Merkle block device. */
    if ((flags & OEFS_FLAG_INTEGRITY))
    {
        if (oefs_merkle_blkdev_get_extra_blocks(nblks, &extra_nblks) != 0)
            goto done;
    }

    /* Create an authenticated crypto block device. */
    if ((flags & OEFS_FLAG_ENCRYPTION) && (flags & OEFS_FLAG_AUTHENTICATION))
    {
        bool initialize = (flags & OEFS_FLAG_MKFS);

        if (oefs_auth_crypto_blkdev_open(
                &crypto_dev, initialize, nblks + extra_nblks, key, next) != 0)
        {
            goto done;
        }

        next = crypto_dev;
    }

    /* Create a crypto block device. */
    if ((flags & OEFS_FLAG_ENCRYPTION) && !(flags & OEFS_FLAG_AUTHENTICATION))
    {
        if (oefs_crypto_blkdev_open(&crypto_dev, key, next) != 0)
            goto done;

        next = crypto_dev;
    }

    /* Create a Merkle block device. */
    if ((flags & OEFS_FLAG_INTEGRITY))
    {
        bool initialize = (flags & OEFS_FLAG_MKFS);

        if (oefs_merkle_blkdev_open(&merkle_dev, initialize, nblks, next) != 0)
        {
            goto done;
        }

        next = merkle_dev;
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

    if (flags & OEFS_FLAG_MKFS)
    {
        if (oefs_mkfs(dev, nblks) != 0)
            goto done;
    }

    if (oefs_initialize(&oefs, dev) != 0)
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

    if (merkle_dev)
        merkle_dev->release(merkle_dev);

    if (oefs)
        oefs_release(oefs);

    return ret;
}

int oefs_ramfs_new(oefs_t** oefs_out, uint32_t flags, size_t nblks)
{
    int ret = -1;
    oe_blkdev_t* dev = NULL;
    oefs_t* oefs = NULL;
    size_t size;

    if (oefs_out)
        *oefs_out = NULL;

    if (!oefs_out || nblks < 2 || !oefs_is_pow_of_2(nblks))
        goto done;

    /* Fail if any of these flags are set. */
    {
        if (flags & OEFS_FLAG_ENCRYPTION)
            goto done;

        if (flags & OEFS_FLAG_AUTHENTICATION)
            goto done;

        if (flags & OEFS_FLAG_INTEGRITY)
            goto done;

        if (flags & OEFS_FLAG_CACHING)
            goto done;
    }

    /* Calculate the size in bytes of the RAMFS block device. */
    if (oefs_size(nblks, &size) != 0)
        goto done;

    /* Open the RAM block device. */
    if (oefs_ram_blkdev_open(&dev, size) != 0)
        goto done;

    if (flags & OEFS_FLAG_MKFS)
    {
        if (oefs_mkfs(dev, nblks) != 0)
            goto done;
    }

    if (oefs_initialize(&oefs, dev) != 0)
        goto done;

    *oefs_out = oefs;
    oefs = NULL;
    ret = 0;

done:

    if (dev)
        dev->release(dev);

    if (oefs)
        oefs_release(oefs);

    return ret;
}
