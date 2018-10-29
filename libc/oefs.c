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
#define CHECK(RESULT)                      \
    do                                     \
    {                                      \
        oefs_result_t __result__ = RESULT; \
        if (__result__ != OEFS_OK)         \
        {                                  \
            result = __result__;           \
            goto done;                     \
        }                                  \
    }                                      \
    while (0)
// clang-format on

// clang-format off
#define RAISE(RESULT)    \
    do                   \
    {                    \
        result = RESULT; \
        goto done;       \
    }                    \
    while (0)
// clang-format on

/* The logical block number of the root directory inode. */
#define OEFS_ROOT_INO 1

/* The physical block number of the super block. */
#define SUPER_BLOCK_PHYSICAL_BLKNO 2

/* The first physical block number of the block bitmap. */
#define BITMAP_PHYSICAL_BLKNO 3

struct _oefs_file
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

struct _oefs_dir
{
    /* The inode number of this directory. */
    uint32_t ino;

    /* The open file which contains the directory entries. */
    oefs_file_t* file;

    /* The current directory entry. */
    oefs_dirent_t ent;
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
    size_t num_bitmap_blocks = oefs->sb.s_nblocks / OEFS_BITS_PER_BLOCK;
    return blkno + (3 + num_bitmap_blocks) - 1;
}

static oefs_result_t _read_block(oefs_t* oefs, size_t blkno, void* block)
{
    oefs_result_t result = OEFS_FAILED;
    uint32_t physical_blkno;

    /* Check whether the block number is valid. */
    if (blkno == 0 || blkno > oefs->sb.s_nblocks)
        RAISE(OEFS_OUT_OF_BOUNDS);

    /* Sanity check: make sure the block is not free. */
    if (!_test_bit(oefs->bitmap, oefs->bitmap_size, blkno - 1))
        RAISE(OEFS_SANITY);

    /* Convert the logical block number to a physical block number. */
    physical_blkno = _get_physical_blkno(oefs, blkno);

    /* Get the physical block. */
    if (oefs->dev->get(oefs->dev, physical_blkno, block) != 0)
        RAISE(OEFS_FAILED);

    result = OEFS_OK;

done:
    return result;
}

static oefs_result_t _write_block(oefs_t* oefs, size_t blkno, const void* block)
{
    oefs_result_t result = OEFS_FAILED;
    uint32_t physical_blkno;

    /* Check whether the block number is valid. */
    if (blkno == 0 || blkno > oefs->sb.s_nblocks)
        RAISE(OEFS_BAD_PARAMETER);

    /* Sanity check: make sure the block is not free. */
    if (!_test_bit(oefs->bitmap, oefs->bitmap_size, blkno - 1))
        RAISE(OEFS_SANITY);

    /* Convert the logical block number to a physical block number. */
    physical_blkno = _get_physical_blkno(oefs, blkno);

    /* Get the physical block. */
    if (oefs->dev->put(oefs->dev, physical_blkno, block) != 0)
        RAISE(OEFS_FAILED);

    result = OEFS_OK;

done:
    return result;
}

static oefs_result_t _load_inode(
    oefs_t* oefs,
    uint32_t ino,
    oefs_inode_t* inode)
{
    oefs_result_t result = OEFS_FAILED;

    /* Read this inode into memory. */
    CHECK(_read_block(oefs, ino, inode));

    /* Check the inode magic number. */
    if (inode->i_magic != OEFS_INODE_MAGIC)
        RAISE(OEFS_SANITY);

    result = OEFS_OK;

done:
    return result;
}

static oefs_result_t _load_blknos(
    oefs_t* oefs,
    oefs_inode_t* inode,
    buf_u32_t* bnode_blknos,
    buf_u32_t* blknos)
{
    oefs_result_t result = OEFS_FAILED;

    /* Collect direct block numbers from the inode. */
    {
        size_t n = sizeof(inode->i_blocks) / sizeof(uint32_t);

        for (size_t i = 0; i < n && inode->i_blocks[i]; i++)
        {
            if (buf_u32_append(blknos, &inode->i_blocks[i], 1) != 0)
                RAISE(OEFS_OUT_OF_MEMORY);
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
                RAISE(OEFS_OUT_OF_MEMORY);

            n = sizeof(bnode.b_blocks) / sizeof(uint32_t);

            /* Get all blocks from this bnode. */
            for (size_t i = 0; i < n && bnode.b_blocks[i]; i++)
            {
                if (buf_u32_append(blknos, &bnode.b_blocks[i], 1) != 0)
                    RAISE(OEFS_OUT_OF_MEMORY);
            }

            next = bnode.b_next;
        }
    }

    result = OEFS_OK;

done:
    return result;
}

/*
BOOKMARK:
*/

static oefs_result_t _open_file(
    oefs_t* oefs,
    uint32_t ino,
    oefs_file_t** file_out)
{
    oefs_result_t result = OEFS_FAILED;
    oefs_file_t* file = NULL;

    if (file_out)
        *file_out = NULL;

    if (!oefs || !ino || !file_out)
        RAISE(OEFS_BAD_PARAMETER);

    if (!(file = calloc(1, sizeof(oefs_file_t))))
        RAISE(OEFS_OUT_OF_MEMORY);

    file->oefs = oefs;
    file->ino = ino;

    /* Read this inode into memory. */
    if (_load_inode(oefs, ino, &file->inode) != OEFS_OK)
        RAISE(OEFS_FAILED);

    /* Load the block numbers into memory. */
    if (_load_blknos(
            oefs,
            &file->inode,
            &file->bnode_blknos,
            &file->blknos) != OEFS_OK)
    {
        RAISE(OEFS_FAILED);
    }

    *file_out = file;
    file = NULL;

    result = OEFS_OK;

done:

    if (file)
    {
        buf_u32_release(&file->blknos);
        free(file);
    }

    return result;
}

static oefs_result_t _assign_blkno(oefs_t* oefs, uint32_t* blkno)
{
    oefs_result_t result = OEFS_FAILED;

    *blkno = 0;

    for (uint32_t i = 0; i < oefs->bitmap_size * 8; i++)
    {
        if (!_test_bit(oefs->bitmap, oefs->bitmap_size, i))
        {
            _set_bit(oefs->bitmap, oefs->bitmap_size, i);
            *blkno = i + 1;
            oefs->sb.s_free_blocks--;
            oefs->dirty = true;
            RAISE(OEFS_OK);
        }
    }

    RAISE(OEFS_NOT_FOUND);

done:
    return result;
}

static oefs_result_t _unassign_blkno(oefs_t* oefs, uint32_t blkno)
{
    oefs_result_t result = OEFS_FAILED;
    uint32_t nbits = oefs->bitmap_size * 8;
    uint32_t index = blkno - 1;

    if (blkno == 0 || index >= nbits)
        RAISE(OEFS_BAD_PARAMETER);

    if (!_test_bit(oefs->bitmap, oefs->bitmap_size, index))
        RAISE(OEFS_SANITY);

    _clr_bit(oefs->bitmap, oefs->bitmap_size, index);
    oefs->sb.s_free_blocks++;

    oefs->dirty = true;

    result = OEFS_OK;

done:
    return result;
}

static oefs_result_t _flush_super_block(oefs_t* oefs)
{
    oefs_result_t result = OEFS_FAILED;

    if (oefs->dev->put(oefs->dev, SUPER_BLOCK_PHYSICAL_BLKNO, &oefs->sb) != 0)
        RAISE(OEFS_FAILED);

    result = OEFS_OK;

done:
    return result;
}

/* TODO: optimize to use partial bitmap flushing */
static oefs_result_t _flush_bitmap(oefs_t* oefs)
{
    oefs_result_t result = OEFS_FAILED;
    size_t num_bitmap_blocks;

    /* Calculate the number of bitmap blocks. */
    num_bitmap_blocks = oefs->sb.s_nblocks / OEFS_BITS_PER_BLOCK;

    /* Flush each bitmap block. */
    for (size_t i = 0; i < num_bitmap_blocks; i++)
    {
        /* Set pointer to the i-th block to write. */
        const oefs_block_t* block = (const oefs_block_t*)oefs->bitmap + i;

        /* Calculate the block number to write. */
        uint32_t blkno = BITMAP_PHYSICAL_BLKNO + i;

        if (oefs->dev->put(oefs->dev, blkno, block) != 0)
            RAISE(OEFS_FAILED);
    }

    result = OEFS_OK;

done:
    return result;
}

static oefs_result_t _flush(oefs_t* oefs)
{
    oefs_result_t result = OEFS_FAILED;

    if (oefs->dirty)
    {
        if (_flush_bitmap(oefs) != OEFS_OK)
            RAISE(OEFS_FAILED);

        if (_flush_super_block(oefs) != OEFS_OK)
            RAISE(OEFS_FAILED);

        oefs->dirty = false;
    }

    result = OEFS_OK;

done:
    return result;
}

static oefs_result_t _write_data(
    oefs_t* oefs,
    const void* data,
    uint32_t size,
    buf_u32_t* blknos)
{
    oefs_result_t result = OEFS_FAILED;
    const uint8_t* ptr = (const uint8_t*)data;
    uint32_t remaining = size;

    /* While there is data remaining to be written. */
    while (remaining)
    {
        uint32_t blkno;
        oefs_block_t block;
        uint32_t copy_size;

        /* Assign a new block number from the in-memory bitmap. */
        if (_assign_blkno(oefs, &blkno) != OEFS_OK)
            RAISE(OEFS_FAILED);

        /* Append the new block number to the array of blocks numbers. */
        if (buf_u32_append(blknos, &blkno, 1) != 0)
            RAISE(OEFS_OUT_OF_MEMORY);

        /* Calculate bytes to copy to the block. */
        copy_size = (remaining > OEFS_BLOCK_SIZE) ? OEFS_BLOCK_SIZE : remaining;

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

    result = OEFS_OK;

done:

    if (_flush(oefs) != OEFS_OK)
        return OEFS_FAILED;

    return result;
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

static oefs_result_t _append_block_chain(
    oefs_file_t* file,
    const buf_u32_t* blknos)
{
    oefs_result_t result = OEFS_FAILED;
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
        if (file->bnode_blknos.size == 0)
            RAISE(OEFS_SANITY);

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
            if (_assign_blkno(file->oefs, &new_blkno) != OEFS_OK)
                RAISE(OEFS_FAILED);

            /* Append the bnode blkno to the file struct. */
            if (buf_u32_append(&file->bnode_blknos, &new_blkno, 1) != 0)
                RAISE(OEFS_OUT_OF_MEMORY);

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

    result = OEFS_OK;

done:
    return result;
}

static bool _sane_file(oefs_file_t* file)
{
    if (!file->oefs)
        return false;

    if (!file->ino)
        return false;

    if (file->inode.i_nblocks != file->blknos.size)
        return false;

    return true;
}

static oefs_result_t _split_path(
    const char* path,
    char dirname[OEFS_PATH_MAX],
    char basename[OEFS_PATH_MAX])
{
    oefs_result_t result = OEFS_FAILED;
    char* slash;

    /* Reject paths that are too long. */
    if (strlen(path) >= OEFS_PATH_MAX)
        RAISE(OEFS_BAD_PARAMETER);

    /* Reject paths that are not absolute */
    if (path[0] != '/')
        RAISE(OEFS_BAD_PARAMETER);

    /* Handle root directory up front */
    if (strcmp(path, "/") == 0)
    {
        strlcpy(dirname, "/", OEFS_PATH_MAX);
        strlcpy(basename, "/", OEFS_PATH_MAX);
        RAISE(OEFS_OK);
    }

    /* This cannot fail (prechecked) */
    if (!(slash = strrchr(path, '/')))
        RAISE(OEFS_UNEXPECTED);

    /* If path ends with '/' character */
    if (!slash[1])
        RAISE(OEFS_BAD_PATH);

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

    result = OEFS_OK;

done:
    return result;
}

static oefs_result_t _create_file(
    oefs_t* oefs,
    uint32_t dir_ino,
    const char* name,
    uint8_t type,
    uint32_t* ino_out)
{
    oefs_result_t result = OEFS_FAILED;
    uint32_t ino;
    oefs_file_t* file = NULL;

    if (ino_out)
        *ino_out = 0;

    if (!oefs || !dir_ino || !name || strlen(name) >= OEFS_PATH_MAX)
        RAISE(OEFS_BAD_PARAMETER);

    /* ATTN: should create replace an existing file? */

    /* Create an inode for the new file. */
    {
        oefs_inode_t inode;

        memset(&inode, 0, sizeof(oefs_inode_t));

        inode.i_magic = OEFS_INODE_MAGIC;
        inode.i_links = 1;

        if (type == OEFS_DT_DIR)
            inode.i_mode = OEFS_S_DIR_DEFAULT;
        else if (type == OEFS_DT_REG)
            inode.i_mode = OEFS_S_REG_DEFAULT;
        else
            RAISE(OEFS_BAD_PARAMETER);

        CHECK(_assign_blkno(oefs, &ino));
        CHECK(_write_block(oefs, ino, &inode));
    }

    /* Add an entry to the directory for this inode. */
    {
        oefs_dirent_t ent;
        int32_t n;

        CHECK(_open_file(oefs, dir_ino, &file));

        /* Check for duplicates. */
        for (;;)
        {
            CHECK(oefs_read(file, &ent, sizeof(ent), &n));

            if (n == 0)
                break;

            if (n != sizeof(ent))
                RAISE(OEFS_SANITY);

            if (strcmp(ent.d_name, name) == 0)
                RAISE(OEFS_ALREADY_EXISTS);
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
                RAISE(OEFS_FAILED);
        }
    }

    if (ino_out)
        *ino_out = ino;

    result = OEFS_OK;

done:

    if (file)
        oefs_close(file);

    if (_flush(oefs) != OEFS_OK)
        return OEFS_FAILED;

    return result;
}

static oefs_result_t _truncate_file(oefs_file_t* file)
{
    oefs_result_t result = OEFS_FAILED;

    if (!file)
        RAISE(OEFS_BAD_PARAMETER);

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

    result = OEFS_OK;

done:

    if (_flush(file->oefs) != OEFS_OK)
        return OEFS_FAILED;

    return result;
}

static oefs_result_t _load_file(
    oefs_file_t* file,
    void** data_out,
    size_t* size_out)
{
    oefs_result_t result = OEFS_FAILED;
    buf_t buf = BUF_INITIALIZER;
    char data[OEFS_BLOCK_SIZE];
    int32_t n;

    *data_out = NULL;
    *size_out = 0;

    for (;;)
    {
        CHECK(oefs_read(file, data, sizeof(data), &n)); 

        if (n == 0)
            break;

        if (buf_append(&buf, data, n) != 0)
            RAISE(OEFS_OUT_OF_MEMORY);
    }

    *data_out = buf.data;
    *size_out = buf.size;
    memset(&buf, 0, sizeof(buf));

    result = OEFS_OK;

done:

    buf_release(&buf);

    return result;
}

static oefs_result_t _release_inode(oefs_t* oefs, uint32_t ino)
{
    oefs_result_t result = OEFS_FAILED;
    oefs_file_t* file = NULL;

    CHECK(_open_file(oefs, ino, &file));
    CHECK(_truncate_file(file));
    CHECK(_unassign_blkno(oefs, file->ino));

    result = OEFS_OK;

done:

    if (file)
        oefs_close(file);

    if (_flush(oefs) != OEFS_OK)
        return OEFS_FAILED;

    return result;
}

static oefs_result_t _unlink_file(
    oefs_t* oefs,
    uint32_t dir_ino,
    uint32_t ino,
    const char* name)
{
    oefs_result_t result = OEFS_FAILED;
    oefs_file_t* dir = NULL;
    oefs_file_t* file = NULL;
    void* data = NULL;
    size_t size = 0;
    oefs_inode_t inode;

    if (!oefs || !dir_ino || !ino)
        RAISE(OEFS_BAD_PARAMETER);

    /* Open the directory file. */
    CHECK(_open_file(oefs, dir_ino, &dir));

    /* Load the contents of the parent directory into memory. */
    CHECK(_load_file(dir, &data, &size));

    /* File must be a multiple of the entry size. */
    if (size % sizeof(oefs_dirent_t))
        RAISE(OEFS_SANITY);

    /* Truncate the parent directory. */
    CHECK(_truncate_file(dir));

    /* Rewrite the directory entries but exclude the removed file. */
    {
        const oefs_dirent_t* entries = (const oefs_dirent_t*)data;
        size_t num_entries = size / sizeof(oefs_dirent_t);

        for (size_t i = 0; i < num_entries; i++)
        {
            if (strcmp(entries[i].d_name, name) != 0)
            {
                const uint32_t n = sizeof(oefs_dirent_t);
                int32_t nwritten;

                CHECK(oefs_write(dir, &entries[i], n, &nwritten));

                if (nwritten != n)
                    RAISE(OEFS_FAILED);
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

    result = OEFS_OK;

done:

    if (dir)
        oefs_close(dir);

    if (file)
        oefs_close(file);

    if (data)
        free(data);

    if (_flush(oefs) != OEFS_OK)
        return OEFS_FAILED;

    return result;
}

static oefs_result_t _opendir_by_ino(
    oefs_t* oefs, uint32_t ino, oefs_dir_t** dir_out)
{
    oefs_result_t result = OEFS_FAILED;
    oefs_dir_t* dir = NULL;
    oefs_file_t* file = NULL;

    if (dir_out)
        *dir_out = NULL;

    if (!oefs || !ino || !dir_out)
        RAISE(OEFS_BAD_PARAMETER);

    CHECK(_open_file(oefs, ino, &file));

    if (!(dir = calloc(1, sizeof(oefs_dir_t))))
        RAISE(OEFS_OUT_OF_MEMORY);

    dir->ino = ino;
    dir->file = file;
    file = NULL;

    *dir_out = dir;

    result = OEFS_OK;

done:

    if (file)
        oefs_close(file);

    return result;
}

static oefs_result_t _path_to_ino(
    oefs_t* oefs,
    const char* path,
    uint32_t* dir_ino_out,
    uint32_t* ino_out,
    uint8_t* type_out) /* oefs_dir_t.d_type */
{
    oefs_result_t result = OEFS_FAILED;
    char buf[OEFS_PATH_MAX];
    const char* elements[128];
    const uint8_t MAX_ELEMENTS = COUNTOF(elements);
    uint8_t num_elements = 0;
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
        RAISE(OEFS_BAD_PARAMETER);

    /* Check path length */
    if (strlen(path) >= OEFS_PATH_MAX)
        RAISE(OEFS_BAD_PATH);

    /* Make copy of the path. */
    strlcpy(buf, path, sizeof(buf));

    /* Be sure path begins with "/" */
    if (path[0] != '/')
        RAISE(OEFS_BAD_PATH);

    elements[num_elements++] = "/";

    /* Split the path into components */
    {
        char* p;
        char* save;

        for (p = strtok_r(buf, "/", &save); p; p = strtok_r(NULL, "/", &save))
        {
            if (num_elements == MAX_ELEMENTS)
                RAISE(OEFS_BUFFER_OVERFLOW);

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
                RAISE(OEFS_NOT_FOUND);

            CHECK(oefs_closedir(dir));
            dir = NULL;
        }
    }

    if (dir_ino_out)
        *dir_ino_out = dir_ino;

    *ino_out = current_ino;

    result = OEFS_OK;

done:

    if (dir)
        oefs_closedir(dir);

    return result;
}

/*
**==============================================================================
**
** Public interface:
**
**==============================================================================
*/

oefs_result_t oefs_mkfs(oe_block_dev_t* dev, size_t num_blocks)
{
    oefs_result_t result = OEFS_FAILED;
    size_t num_bitmap_blocks;
    uint32_t blkno = 0;
    uint8_t empty_block[OEFS_BLOCK_SIZE];

    if (!dev || num_blocks < OEFS_BITS_PER_BLOCK)
        RAISE(OEFS_BAD_PARAMETER);

    /* Fail if num_blocks is not a multiple of the block size. */
    if (num_blocks % OEFS_BITS_PER_BLOCK)
        RAISE(OEFS_BAD_PARAMETER);

    /* Initialize an empty block. */
    memset(empty_block, 0, sizeof(empty_block));

    /* Calculate the number of bitmap blocks. */
    num_bitmap_blocks = num_blocks / OEFS_BITS_PER_BLOCK;

    /* Write empty block one. */
    if (dev->put(dev, blkno++, empty_block) != 0)
        RAISE(OEFS_FAILED);

    /* Write empty block two. */
    if (dev->put(dev, blkno++, empty_block) != 0)
        RAISE(OEFS_FAILED);

    /* Write the super block */
    {
        oefs_super_block_t sb;
        memset(&sb, 0, sizeof(sb));

        sb.s_magic = OEFS_SUPER_BLOCK_MAGIC;
        sb.s_nblocks = num_blocks;
        sb.s_free_blocks = num_blocks - 3;

        if (dev->put(dev, blkno++, &sb) != 0)
            RAISE(OEFS_FAILED);
    }

    /* Write the first bitmap block. */
    {
        uint8_t block[OEFS_BLOCK_SIZE];

        memset(block, 0, sizeof(block));

        /* Set the bit for the root inode. */
        _set_bit(block, sizeof(block), 0);

        /* Set the bits for the root directory. */
        _set_bit(block, sizeof(block), 1);
        _set_bit(block, sizeof(block), 2);

        /* Write the block. */
        if (dev->put(dev, blkno++, &block) != 0)
            RAISE(OEFS_FAILED);
    }

    /* Write the subsequent (empty) bitmap blocks. */
    for (size_t i = 1; i < num_bitmap_blocks; i++)
    {
        if (dev->put(dev, blkno++, &empty_block) != 0)
            RAISE(OEFS_FAILED);
    }

    /* Write the root directory inode. */
    {
        oefs_inode_t root_inode = {
            .i_magic = OEFS_INODE_MAGIC,
            .i_mode = OEFS_S_DIR_DEFAULT,
            .i_uid = 0,
            .i_gid = 0,
            .i_links = 1,
            .i_size = 2 * sizeof(oefs_dirent_t),
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
            RAISE(OEFS_FAILED);
    }

    /* Write the ".." and "." directory entries for the root directory.  */
    {
        uint8_t blocks[2][OEFS_BLOCK_SIZE];

        oefs_dirent_t dir_entries[] = {
            {.d_ino = OEFS_ROOT_INO,
             .d_off = 0 * sizeof(oefs_dirent_t),
             .d_reclen = sizeof(oefs_dirent_t),
             .d_type = OEFS_DT_DIR,
             .d_name = ".."

            },
            {.d_ino = OEFS_ROOT_INO,
             .d_off = 1 * sizeof(oefs_dirent_t),
             .d_reclen = sizeof(oefs_dirent_t),
             .d_type = OEFS_DT_DIR,
             .d_name = "."},
        };

        memset(blocks, 0, sizeof(blocks));
        memcpy(blocks, dir_entries, sizeof(dir_entries));

        if (dev->put(dev, blkno++, &blocks[0]) != 0)
            RAISE(OEFS_FAILED);

        if (dev->put(dev, blkno++, &blocks[1]) != 0)
            RAISE(OEFS_FAILED);
    }

    /* Write the remaining empty blocks. */
    for (size_t i = 0; i < num_blocks - 3; i++)
    {
        if (dev->put(dev, blkno++, &empty_block) != 0)
            RAISE(OEFS_FAILED);
    }

    result = OEFS_OK;

done:
    return result;
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
    device->size += OEFS_BLOCK_SIZE;
    return 0;
}

oefs_result_t oefs_size(size_t num_blocks, size_t* size)
{
    oefs_result_t result = OEFS_FAILED;
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

    /* Double check the size by computing it differently. */
    {
        size_t num_bitmap_blocks = num_blocks / OEFS_BITS_PER_BLOCK;
        size_t size = (3 + num_bitmap_blocks + num_blocks) * OEFS_BLOCK_SIZE;

        if (size != dev.size)
            RAISE(OEFS_SANITY);
    }

    result = OEFS_OK;

done:
    return result;
}

oefs_result_t oefs_read(
    oefs_file_t* file,
    void* data,
    uint32_t size,
    int32_t* nread)
{
    oefs_result_t result = OEFS_FAILED;
    uint32_t first;
    uint32_t i;
    uint32_t remaining;
    uint8_t* ptr = (uint8_t*)data;

    if (nread)
        *nread = 0;

    /* Check parameters */
    if (!file || !file->oefs || (!data && size))
        RAISE(OEFS_BAD_PARAMETER);

    /* The index of the first block to read. */
    first = file->offset / OEFS_BLOCK_SIZE;

    /* The number of bytes remaining to be read */
    remaining = size;

    /* Read the data block-by-block */
    for (i = first; i < file->blknos.size && remaining > 0 && !file->eof; i++)
    {
        oefs_block_t block;
        uint32_t offset;

        CHECK(_read_block(file->oefs, file->blknos.data[i], &block));

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

    result = OEFS_OK;

done:
    return result;
}

oefs_result_t oefs_write(
    oefs_file_t* file,
    const void* data,
    uint32_t size,
    int32_t* nwritten)
{
    oefs_result_t result = OEFS_FAILED;
    uint32_t first;
    uint32_t i;
    uint32_t remaining;
    const uint8_t* ptr = (uint8_t*)data;
    uint32_t new_file_size;

    if (nwritten)
        *nwritten = 0;

    /* Check parameters */
    if (!file || !file->oefs || (!data && size) || !nwritten)
        RAISE(OEFS_BAD_PARAMETER);

    if (!_sane_file(file))
        RAISE(OEFS_SANITY);

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
        oefs_block_t block;
        uint32_t offset;

        CHECK(_read_block(file->oefs, file->blknos.data[i], &block));

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
            RAISE(OEFS_OUT_OF_MEMORY);

        buf_u32_release(&blknos);

        file->offset += remaining;
        remaining = 0;
    }

    /* Update the file size. */
    file->inode.i_size = new_file_size;

    /* Flush the inode to disk. */
    CHECK(_write_block(file->oefs, file->ino, &file->inode));

    result = OEFS_OK;

    /* Calculate number of bytes written */
    *nwritten = size - remaining;

done:
    return result;
}

oefs_result_t oefs_close(oefs_file_t* file)
{
    oefs_result_t result = OEFS_FAILED;

    if (!file)
        RAISE(OEFS_BAD_PARAMETER);

    buf_u32_release(&file->blknos);
    memset(file, 0, sizeof(oefs_file_t));
    free(file);

    result = OEFS_OK;

done:
    return result;
}

oefs_result_t oefs_initialize(oefs_t** oefs_out, oe_block_dev_t* dev)
{
    oefs_result_t result = OEFS_FAILED;
    size_t num_blocks;
    uint8_t* bitmap = NULL;
    uint8_t* bitmap_copy = NULL;
    oefs_t* oefs = NULL;

    if (oefs_out)
        *oefs_out = NULL;

    if (!dev || !oefs_out)
        RAISE(OEFS_BAD_PARAMETER);

    if (!(oefs = calloc(1, sizeof(oefs_t))))
        RAISE(OEFS_OUT_OF_MEMORY);

    /* Save pointer to device (caller is responsible for releasing it). */
    oefs->dev = dev;

    /* Read the super block from the file system. */
    if (dev->get(dev, SUPER_BLOCK_PHYSICAL_BLKNO, &oefs->sb) != 0)
        RAISE(OEFS_FAILED);

    /* Check the superblock magic number. */
    if (oefs->sb.s_magic != OEFS_SUPER_BLOCK_MAGIC)
        RAISE(OEFS_SANITY);

    /* Make copy of super block. */
    memcpy(&oefs->sb_copy, &oefs->sb, sizeof(oefs->sb_copy));

    /* Get the number of blocks. */
    num_blocks = oefs->sb.s_nblocks;

    /* Check that the number of blocks is correct. */
    if (!num_blocks || (num_blocks % OEFS_BITS_PER_BLOCK))
        RAISE(OEFS_SANITY);

    /* Allocate space for the block bitmap. */
    {
        /* Calculate the number of bitmap blocks. */
        size_t num_bitmap_blocks = num_blocks / OEFS_BITS_PER_BLOCK;

        /* Calculate the size of the bitmap. */
        size_t bitmap_size = num_bitmap_blocks * OEFS_BLOCK_SIZE;

        /* Allocate the bitmap. */
        if (!(bitmap = calloc(1, bitmap_size)))
            RAISE(OEFS_OUT_OF_MEMORY);

        /* Allocate the bitmap. */
        if (!(bitmap_copy = calloc(1, bitmap_size)))
            RAISE(OEFS_OUT_OF_MEMORY);

        /* Read the bitset into memory */
        for (size_t i = 0; i < num_bitmap_blocks; i++)
        {
            uint8_t* ptr = bitmap + (i * OEFS_BLOCK_SIZE);

            if (dev->get(dev, BITMAP_PHYSICAL_BLKNO + i, ptr) != 0)
                RAISE(OEFS_FAILED);
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
            RAISE(OEFS_SANITY);
        }

        oefs->bitmap = bitmap;
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
            RAISE(OEFS_SANITY);
    }

    *oefs_out = oefs;
    oefs = NULL;

    result = OEFS_OK;

done:

    if (oefs)
    {
        free(oefs->bitmap);
        free(oefs);
    }

    if (bitmap)
        free(bitmap);

    if (bitmap_copy)
        free(bitmap_copy);

    return result;
}

oefs_result_t oefs_release(oefs_t* oefs)
{
    oefs_result_t result = OEFS_FAILED;

    if (!oefs || !oefs->bitmap || !oefs->bitmap_copy)
        RAISE(OEFS_BAD_PARAMETER);

    free(oefs->bitmap);
    free(oefs->bitmap_copy);
    memset(oefs, 0, sizeof(oefs_t));
    free(oefs);

    result = OEFS_OK;

done:
    return result;
}

oefs_result_t oefs_opendir(oefs_t* oefs, const char* path, oefs_dir_t** dir)
{
    oefs_result_t result = OEFS_FAILED;
    uint32_t ino;

    if (dir)
        *dir = NULL;

    if (!oefs || !path || !dir)
        RAISE(OEFS_BAD_PARAMETER);

    CHECK(_path_to_ino(oefs, path, NULL, &ino, NULL));
    CHECK(_opendir_by_ino(oefs, ino, dir));

    result = OEFS_OK;

done:

    return result;
}

oefs_result_t oefs_readdir(oefs_dir_t* dir, oefs_dirent_t** ent)
{
    oefs_result_t result = OEFS_FAILED;
    int32_t nread = 0;

    if (ent)
        *ent = NULL;

    if (!dir || !dir->file)
        RAISE(OEFS_BAD_PARAMETER);

    CHECK(oefs_read(dir->file, &dir->ent, sizeof(oefs_dirent_t), &nread));

    /* Check for end of file. */
    if (nread == 0)
        RAISE(OEFS_OK);

    /* Check for an illegal read size. */
    if (nread != sizeof(oefs_dirent_t))
        RAISE(OEFS_SANITY);

    *ent = &dir->ent;
    result = OEFS_OK;

done:

    return result;
}

oefs_result_t oefs_closedir(oefs_dir_t* dir)
{
    oefs_result_t result = OEFS_FAILED;

    if (!dir || !dir->file)
        RAISE(OEFS_BAD_PARAMETER);

    oefs_close(dir->file);
    dir->file = NULL;
    free(dir);

    result = OEFS_OK;

done:
    return result;
}

oefs_result_t oefs_open(
    oefs_t* oefs,
    const char* path,
    int flags,
    uint32_t mode,
    oefs_file_t** file_out)
{
    oefs_result_t result = OEFS_FAILED;
    oefs_file_t* file = NULL;
    uint32_t ino = 0;

    if (file_out)
        *file_out = NULL;

    if (!oefs || !path || !file_out)
        RAISE(OEFS_BAD_PARAMETER);

    CHECK(_path_to_ino(oefs, path, NULL, &ino, NULL));
    CHECK(_open_file(oefs, ino, &file));

    *file_out = file;

    result = OEFS_OK;

done:

    return result;
}

oefs_result_t oefs_load(
    oefs_t* oefs,
    const char* path,
    void** data_out,
    size_t* size_out)
{
    oefs_result_t result = OEFS_FAILED;
    oefs_file_t* file = NULL;
    void* data = NULL;
    size_t size = 0;

    if (data_out)
        *data_out = NULL;

    if (size_out)
        *size_out = 0;

    if (!oefs || !path || !data_out || !size_out)
        RAISE(OEFS_BAD_PARAMETER);

    CHECK(oefs_open(oefs, path, 0, 0, &file));
    CHECK(_load_file(file, &data, &size));

    *data_out = data;
    *size_out = size;

    data = NULL;

    result = OEFS_OK;

done:

    if (file)
        oefs_close(file);

    if (data)
        free(data);

    return result;
}

oefs_result_t oefs_mkdir(oefs_t* oefs, const char* path, uint32_t mode)
{
    oefs_result_t result = OEFS_FAILED;
    char dirname[OEFS_PATH_MAX];
    char basename[OEFS_PATH_MAX];
    uint32_t dir_ino;
    uint32_t ino;
    oefs_file_t* file = NULL;

    if (!oefs || !path)
        RAISE(OEFS_BAD_PARAMETER);

    /* Split the path into parent path and final component. */
    CHECK(_split_path(path, dirname, basename));

    /* Get the inode of the parent directory. */
    CHECK(_path_to_ino(oefs, dirname, NULL, &dir_ino, NULL));

    /* Create the new directory file. */
    CHECK(_create_file(oefs, dir_ino, basename, OEFS_DT_DIR, &ino));

    /* Open the newly created directory */
    CHECK(_open_file(oefs, ino, &file));

    /* Write the empty directory contents. */
    {
        oefs_dirent_t dirents[2];
        int32_t nwritten;

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
            RAISE(OEFS_FAILED);
    }

    result = OEFS_OK;

done:

    if (file)
        oefs_close(file);

    return result;
}

oefs_result_t oefs_create(
    oefs_t* oefs,
    const char* path,
    uint32_t mode,
    oefs_file_t** file_out)
{
    oefs_result_t result = OEFS_FAILED;
    char dirname[OEFS_PATH_MAX];
    char basename[OEFS_PATH_MAX];
    uint32_t dir_ino;
    uint32_t ino;
    oefs_file_t* file = NULL;

    if (file_out)
        *file_out = NULL;

    if (!oefs || !path || !file_out)
        RAISE(OEFS_BAD_PARAMETER);

    /* Split the path into parent directory and file name */
    CHECK(_split_path(path, dirname, basename));

    /* Get the inode of the parent directory. */
    CHECK(_path_to_ino(oefs, dirname, NULL, &dir_ino, NULL));

    /* Create the new file. */
    CHECK(_create_file(oefs, dir_ino, basename, OEFS_DT_REG, &ino));

    /* Open the new file. */
    CHECK(_open_file(oefs, ino, &file));

    *file_out = file;
    file = NULL;

    result = OEFS_OK;

done:

    return result;
}

oefs_result_t oefs_link(
    oefs_t* oefs,
    const char* old_path,
    const char* new_path)
{
    oefs_result_t result = OEFS_FAILED;
    uint32_t ino;
    char dirname[OEFS_PATH_MAX];
    char basename[OEFS_PATH_MAX];
    uint32_t dir_ino;
    oefs_file_t* dir = NULL;
    uint32_t release_ino = 0;

    if (!old_path || !new_path)
        RAISE(OEFS_BAD_PARAMETER);

    /* Get the inode number of the old path. */
    {
        uint8_t type;

        CHECK(_path_to_ino(oefs, old_path, NULL, &ino, &type));

        /* Only regular files can be linked. */
        if (type != OEFS_DT_REG)
            RAISE(OEFS_BAD_PARAMETER);
    }

    /* Split the new path. */
    CHECK(_split_path(new_path, dirname, basename));

    /* Open the destination directory. */
    {
        uint8_t type;

        CHECK(_path_to_ino(oefs, dirname, NULL, &dir_ino, &type));

        if (type != OEFS_DT_DIR)
            RAISE(OEFS_BAD_PARAMETER);

        CHECK(_open_file(oefs, dir_ino, &dir));
    }

    /* Replace the destination file if it already exists. */
    for (;;)
    {
        oefs_dirent_t ent;
        int32_t n;

        CHECK(oefs_read(dir, &ent, sizeof(ent), &n));

        if (n == 0)
            break;

        if (n != sizeof(ent))
            RAISE(OEFS_SANITY);

        if (strcmp(ent.d_name, basename) == 0)
        {
            release_ino = ent.d_ino;

            if (ent.d_type != OEFS_DT_REG)
                RAISE(OEFS_BAD_PARAMETER);

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
        int32_t n;

        memset(&ent, 0, sizeof(ent));
        ent.d_ino = ino;
        ent.d_off = dir->offset;
        ent.d_reclen = sizeof(ent);
        ent.d_type = OEFS_DT_REG;
        strlcpy(ent.d_name, basename, sizeof(ent.d_name));

        CHECK(oefs_write(dir, &ent, sizeof(ent), &n));

        if (n != sizeof(ent))
            RAISE(OEFS_FAILED);
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

    result = OEFS_OK;

done:

    if (dir)
        oefs_close(dir);

    return result;
}

oefs_result_t oefs_rename(
    oefs_t* oefs,
    const char* old_path,
    const char* new_path)
{
    oefs_result_t result = OEFS_FAILED;

    if (!old_path || !new_path)
        RAISE(OEFS_BAD_PARAMETER);

    CHECK(oefs_link(oefs, old_path, new_path));
    CHECK(oefs_unlink(oefs, old_path));

    result = OEFS_OK;

done:

    return result;
}

oefs_result_t oefs_unlink(oefs_t* oefs, const char* path)
{
    oefs_result_t result = OEFS_FAILED;
    uint32_t dir_ino;
    uint32_t ino;
    uint8_t type;
    char dirname[OEFS_PATH_MAX];
    char basename[OEFS_PATH_MAX];

    if (!oefs || !path)
        RAISE(OEFS_BAD_PARAMETER);

    CHECK(_path_to_ino(oefs, path, &dir_ino, &ino, &type));

    /* Only regular files can be removed. */
    if (type != OEFS_DT_REG)
        RAISE(OEFS_BAD_PARAMETER);

    CHECK(_split_path(path, dirname, basename));
    CHECK(_unlink_file(oefs, dir_ino, ino, basename));

    result = OEFS_OK;

done:

    return result;
}

oefs_result_t oefs_truncate(oefs_t* oefs, const char* path)
{
    oefs_result_t result = OEFS_FAILED;
    oefs_file_t* file = NULL;
    uint32_t ino;
    uint8_t type;

    if (!oefs || !path)
        RAISE(OEFS_BAD_PARAMETER);

    CHECK(_path_to_ino(oefs, path, NULL, &ino, &type));

    /* Only regular files can be truncated. */
    if (type != OEFS_DT_REG)
        RAISE(OEFS_BAD_PARAMETER);

    CHECK(_open_file(oefs, ino, &file));
    CHECK(_truncate_file(file));

    result = OEFS_OK;

done:

    if (file)
        oefs_close(file);

    return result;
}

oefs_result_t oefs_rmdir(oefs_t* oefs, const char* path)
{
    oefs_result_t result = OEFS_FAILED;
    uint32_t dir_ino;
    uint32_t ino;
    uint8_t type;
    char dirname[OEFS_PATH_MAX];
    char basename[OEFS_PATH_MAX];

    if (!oefs || !path)
        RAISE(OEFS_BAD_PARAMETER);

    CHECK(_path_to_ino(oefs, path, &dir_ino, &ino, &type));

    /* The path must refer to a directory. */
    if (type != OEFS_DT_DIR)
        RAISE(OEFS_BAD_PARAMETER);

    /* The inode must be a directory and the directory must be empty. */
    {
        oefs_inode_t inode;

        CHECK(_load_inode(oefs, ino, &inode));

        if (!(inode.i_mode & OEFS_S_IFDIR))
            RAISE(OEFS_SANITY);

        /* The directory must contain two entries: "." and ".." */
        if (inode.i_size != 2 * sizeof(oefs_dirent_t))
            RAISE(OEFS_SANITY);
    }

    CHECK(_split_path(path, dirname, basename));
    CHECK(_unlink_file(oefs, dir_ino, ino, basename));

    result = OEFS_OK;

done:

    return result;
}

oefs_result_t oefs_stat(oefs_t* oefs, const char* path, oefs_stat_t* stat)
{
    oefs_result_t result = OEFS_FAILED;
    oefs_inode_t inode;
    uint32_t ino;
    uint8_t type;

    if (stat)
        memset(stat, 0, sizeof(oefs_stat_t));

    if (!oefs || !path || !stat)
        RAISE(OEFS_BAD_PARAMETER);

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
    stat->st_blksize = OEFS_BLOCK_SIZE;
    stat->st_blocks = inode.i_nblocks;
    stat->st_atime = inode.i_atime;
    stat->st_mtime = inode.i_mtime;
    stat->st_ctime = inode.i_ctime;

    result = OEFS_OK;

done:
    return result;
}

oefs_result_t oefs_lseek(
    oefs_file_t* file,
    ssize_t offset,
    int whence,
    ssize_t* offset_out)
{
    oefs_result_t result = OEFS_FAILED;
    ssize_t new_offset;

    if (offset_out)
        *offset_out = 0;

    if (!file)
        RAISE(OEFS_BAD_PARAMETER);

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
            RAISE(OEFS_BAD_PARAMETER);
        }
    }

    /* Check whether the new offset if out of range. */
    if (new_offset < 0 || new_offset > file->offset)
        RAISE(OEFS_OUT_OF_BOUNDS);

    file->offset = new_offset;

    if (offset_out)
        *offset_out = new_offset;

    result = OEFS_OK;

done:
    return result;
}
