#define _GNU_SOURCE
#include "oefs.h"
#include <openenclave/internal/enclavelibc.h>
#include <openenclave/internal/hexdump.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

#define COUNTOF(ARR) (sizeof(ARR) / sizeof((ARR)[0]))

#define SUPER_BLOCK_PHYSICAL_BLKNO 2

#define BITMAP_PHYSICAL_BLKNO 3

struct _oefs_file
{
    oefs_t* oefs;

    uint32_t ino;
    oefs_inode_t inode;

    uint32_t last_bnode_blkno;
    oefs_bnode_t last_bnode;

    buf_u32_t blknos;

    uint32_t offset;

    bool eof;
};

struct _oefs_dir
{
    oefs_file_t* file;
    oefs_dirent_t dirent;
};

static __inline bool _test_bit(const uint8_t* data, uint32_t index)
{
    uint32_t byte = index / 8;
    uint32_t bit = index % 8;
    return data[byte] & (1 << bit);
}

static __inline void _set_bit(uint8_t* data, uint32_t index)
{
    uint32_t byte = index / 8;
    uint32_t bit = index % 8;
    data[byte] |= (1 << bit);
}

static __inline void _clr_bit(uint8_t* data, uint32_t index)
{
    uint32_t byte = index / 8;
    uint32_t bit = index % 8;
    data[byte] &= ~(1 << bit);
}

/* Get the physical block number from a logical block number. */
static __inline uint32_t _get_physical_blkno(oefs_t* oefs, uint32_t blkno)
{
    /* Calculate the number of bitmap blocks. */
    size_t num_bitmap_blocks = oefs->sb.s_num_blocks / OEFS_BITS_PER_BLOCK;
    return blkno + (3 + num_bitmap_blocks) - 1;
}

static oefs_result_t _read_block(oefs_t* oefs, size_t blkno, void* block)
{
    oefs_result_t result = OEFS_FAILED;
    uint32_t physical_blkno;

    /* Check whether the block number is valid. */
    if (blkno == 0 || blkno > oefs->sb.s_num_blocks)
        goto done;

    /* Sanity check: make sure the block is not free. */
    if (!_test_bit(oefs->bitmap, blkno - 1))
        goto done;

    /* Convert the logical block number to a physical block number. */
    physical_blkno = _get_physical_blkno(oefs, blkno);

    /* Get the physical block. */
    if (oefs->dev->get(oefs->dev, physical_blkno, block) != 0)
        goto done;

    result = OEFS_OK;

done:
    return result;
}

static oefs_result_t _write_block(oefs_t* oefs, size_t blkno, const void* block)
{
    oefs_result_t result = OEFS_FAILED;
    uint32_t physical_blkno;

    /* Check whether the block number is valid. */
    if (blkno == 0 || blkno > oefs->sb.s_num_blocks)
        goto done;

    /* Sanity check: make sure the block is not free. */
    if (!_test_bit(oefs->bitmap, blkno - 1))
        goto done;

    /* Convert the logical block number to a physical block number. */
    physical_blkno = _get_physical_blkno(oefs, blkno);

    /* Get the physical block. */
    if (oefs->dev->put(oefs->dev, physical_blkno, block) != 0)
        goto done;

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
    if (_read_block(oefs, ino, inode) != OEFS_OK)
        goto done;

    /* Check the inode magic number. */
    if (inode->i_magic != OEFS_INODE_MAGIC)
        goto done;

    result = OEFS_OK;

done:
    return result;
}

static oefs_result_t _load_blknos(
    oefs_t* oefs,
    oefs_inode_t* inode,
    buf_u32_t* blknos,
    uint32_t* last_bnode_blkno,
    oefs_bnode_t* last_bnode)
{
    oefs_result_t result = OEFS_FAILED;

    if (last_bnode_blkno)
        *last_bnode_blkno = 0;

    if (last_bnode)
        memset(last_bnode, 0, sizeof(oefs_bnode_t));

    /* Collect direct block numbers. */
    {
        size_t n = sizeof(inode->i_blocks) / sizeof(uint32_t);

        for (size_t i = 0; i < n && inode->i_blocks[i]; i++)
        {
            if (buf_u32_append(blknos, &inode->i_blocks[i], 1) != 0)
                goto done;
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
            if (_read_block(oefs, next, &bnode) != OEFS_OK)
                goto done;

            n = sizeof(bnode.b_blocks) / sizeof(uint32_t);

            /* Get all blocks from this bnode. */
            for (size_t i = 0; i < n && bnode.b_blocks[i]; i++)
            {
                if (buf_u32_append(blknos, &bnode.b_blocks[i], 1) != 0)
                    goto done;
            }

            /* Save the last bnode in the chain. */
            if (!bnode.b_next)
            {
                if (last_bnode_blkno)
                    *last_bnode_blkno = next;

                if (last_bnode)
                    *last_bnode = bnode;
            }

            next = bnode.b_next;
        }
    }

    result = OEFS_OK;

done:
    return result;
}

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
        goto done;

    if (!(file = calloc(1, sizeof(oefs_file_t))))
        goto done;

    file->oefs = oefs;
    file->ino = ino;

    /* Read this inode into memory. */
    if (_load_inode(oefs, ino, &file->inode) != OEFS_OK)
        goto done;

    /* Load the block numbers into memory. */
    if (_load_blknos(
            oefs,
            &file->inode,
            &file->blknos,
            &file->last_bnode_blkno,
            &file->last_bnode) != OEFS_OK)
    {
        goto done;
    }

    *file_out = file;
    file = NULL;

    result = OEFS_OK;

done:

    if (file)
    {
        buf_u32_release(&file->blknos);
        memcpy(file, 0, sizeof(oefs_file_t));
        free(file);
    }

    return result;
}

oefs_result_t oefs_load_file(
    oefs_t* oefs,
    uint32_t ino,
    void** data,
    size_t* size)
{
    oefs_result_t result = OEFS_FAILED;
    buf_u32_t blknos = BUF_U32_INITIALIZER;
    oefs_inode_t inode;
    buf_t buf = BUF_INITIALIZER;

    if (data)
        *data = NULL;

    if (size)
        *size = 0;

    /* Read this inode into memory. */
    if (_load_inode(oefs, ino, &inode) != OEFS_OK)
        goto done;

    /* Load the block numbers into memory. */
    if (_load_blknos(oefs, &inode, &blknos, NULL, NULL) != OEFS_OK)
        goto done;

    /* Load each block into memory. */
    for (size_t i = 0; i < blknos.size; i++)
    {
        uint8_t block[OEFS_BLOCK_SIZE];

        if (_read_block(oefs, blknos.data[i], block) != OEFS_OK)
            goto done;

        if (buf_append(&buf, block, sizeof(block)) != 0)
            goto done;
    }

    /* Adjust the size of the file. */
    buf.size = inode.i_size;

    *data = buf.data;
    *size = buf.size;

    result = OEFS_OK;

done:

    if (result != OEFS_OK)
        buf_release(&buf);

    buf_u32_release(&blknos);

    return result;
}

static __inline void _dump_dirent(oefs_dirent_t* entry)
{
    printf("=== oefs_dirent_t\n");
    printf("d_ino=%u\n", entry->d_ino);
    printf("d_type=%u\n", entry->d_type);
    printf("d_name=%s\n", entry->d_name);
}

static oefs_result_t _assign_blkno(oefs_t* oefs, uint32_t* blkno)
{
    oefs_result_t result = OEFS_FAILED;

    *blkno = 0;

    for (uint32_t i = 0; i < oefs->bitmap_size * 8; i++)
    {
        if (!_test_bit(oefs->bitmap, i))
        {
            _set_bit(oefs->bitmap, i);
            *blkno = i + 1;
            oefs->sb.s_free_blocks--;
            result = OEFS_OK;
            goto done;
        }
    }

    result = OEFS_NOT_FOUND;

done:
    return result;
}

static oefs_result_t _unassign_blkno(oefs_t* oefs, uint32_t blkno)
{
    oefs_result_t result = OEFS_FAILED;
    uint32_t nbits = oefs->bitmap_size * 8;
    uint32_t index = blkno - 1;

    if (blkno == 0 || index >= nbits)
        goto done;

    if (!_test_bit(oefs->bitmap, index))
        goto done;

    _clr_bit(oefs->bitmap, index);
    oefs->sb.s_free_blocks++;

    result = OEFS_OK;

done:
    return result;
}

static oefs_result_t _flush_super_block(oefs_t* oefs)
{
    oefs_result_t result = OEFS_OK;

    if (oefs->dev->put(oefs->dev, SUPER_BLOCK_PHYSICAL_BLKNO, &oefs->sb) != 0)
        goto done;

    result = OEFS_OK;

done:
    return result;
}

/* TODO: optimize to use partial bitmap flushing */
static oefs_result_t _flush_bitmap(oefs_t* oefs)
{
    oefs_result_t result = OEFS_OK;
    size_t num_bitmap_blocks;

    /* Calculate the number of bitmap blocks. */
    num_bitmap_blocks = oefs->sb.s_num_blocks / OEFS_BITS_PER_BLOCK;

    /* Flush each bitmap block. */
    for (size_t i = 0; i < num_bitmap_blocks; i++)
    {
        /* Set pointer to the i-th block to write. */
        const oefs_block_t* block = (const oefs_block_t*)oefs->bitmap + i;

        /* Calculate the block number to write. */
        uint32_t blkno = BITMAP_PHYSICAL_BLKNO + i;

        if (oefs->dev->put(oefs->dev, blkno, block) != 0)
            goto done;
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
    bool changed = false;

    /* While there is data remaining to be written. */
    while (remaining)
    {
        uint32_t blkno;
        oefs_block_t block;
        uint32_t copy_size;

        /* Assign a new block number from the in-memory bitmap. */
        if (_assign_blkno(oefs, &blkno) != OEFS_OK)
            goto done;

        /* Append the new block number to the array of blocks numbers. */
        if (buf_u32_append(blknos, &blkno, 1) != 0)
            goto done;

        /* Calculate bytes to copy to the block. */
        copy_size = (remaining > OEFS_BLOCK_SIZE) ? OEFS_BLOCK_SIZE : remaining;

        /* Clear the block. */
        memset(block.data, 0, sizeof(oefs_block_t));

        /* Copy user data to the block. */
        memcpy(block.data, ptr, copy_size);

        /* Write the new block. */
        if (_write_block(oefs, blkno, &block))
            goto done;

        /* Advance to next block of data to write. */
        ptr += copy_size;
        remaining -= copy_size;

        /* Set this flag to the memory structures will be flushed. */
        changed = true;
    }

    /* If anything changed, then flush memory structures to disk. */
    if (changed)
    {
        if (_flush_super_block(oefs) != OEFS_OK)
            goto done;

        if (_flush_bitmap(oefs) != OEFS_OK)
            goto done;
    }

    result = OEFS_OK;

done:
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
    oefs_bnode_t bnode = file->last_bnode;

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
        object = &file->last_bnode;
        slots = file->last_bnode.b_blocks;
        count = COUNTOF(file->last_bnode.b_blocks);
        blkno = file->last_bnode_blkno;
        next = &file->last_bnode.b_next;
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
                goto done;

            *next = new_blkno;

            /* Rewrite the current inode or bnode. */
            if (_write_block(file->oefs, blkno, object) != 0)
                goto done;

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
            if (_write_block(file->oefs, blkno, object) != 0)
                goto done;
        }
    }

    /* Update the last bnode if it changed. */
    if (object == &bnode)
    {
        file->last_bnode = bnode;
        file->last_bnode_blkno = blkno;
    }

    result = OEFS_OK;

done:
    return result;
}

static oefs_result_t _dump_directory(oefs_t* oefs, uint32_t ino)
{
    oefs_result_t result = OEFS_FAILED;
    oefs_file_t* file;
    oefs_dirent_t entry;
    int32_t n;

    if (_open_file(oefs, ino, &file) != 0)
        goto done;

    while ((n = oefs_read_file(file, &entry, sizeof(entry))) > 0)
    {
        _dump_dirent(&entry);
    }

    oefs_close_file(file);

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

    if (file->inode.i_total_blocks != file->blknos.size)
        return false;

    return true;
}

/*
**==============================================================================
**
** Public interface:
**
**==============================================================================
*/

oefs_result_t oefs_initialize(oe_block_dev_t* dev, size_t num_blocks)
{
    oefs_result_t result = OEFS_FAILED;
    size_t num_bitmap_blocks;
    uint32_t blkno = 0;
    uint8_t empty_block[OEFS_BLOCK_SIZE];

    if (!dev || num_blocks < OEFS_MIN_BLOCKS)
    {
        result = OEFS_BAD_PARAMETER;
        goto done;
    }

    /* Fail if num_blocks is not a multiple of the block size. */
    if (num_blocks % OEFS_BITS_PER_BLOCK)
    {
        result = OEFS_BAD_PARAMETER;
        goto done;
    }

    /* Initialize an empty block. */
    memset(empty_block, 0, sizeof(empty_block));

    /* Calculate the number of bitmap blocks. */
    num_bitmap_blocks = num_blocks / OEFS_BITS_PER_BLOCK;

    /* Write empty block one. */
    if (dev->put(dev, blkno++, empty_block) != 0)
    {
        result = OEFS_FAILED;
        goto done;
    }

    /* Write empty block two. */
    if (dev->put(dev, blkno++, empty_block) != 0)
    {
        result = OEFS_FAILED;
        goto done;
    }

    /* Write the super block */
    {
        oefs_super_block_t sb;
        memset(&sb, 0, sizeof(sb));

        sb.s_magic = OEFS_SUPER_BLOCK_MAGIC;
        sb.s_num_blocks = num_blocks;
        sb.s_free_blocks = num_blocks - 3;

        if (dev->put(dev, blkno++, &sb) != 0)
        {
            result = OEFS_FAILED;
            goto done;
        }
    }

    /* Write the first bitmap block. */
    {
        uint8_t block[OEFS_BLOCK_SIZE];

        memset(block, 0, sizeof(block));

        /* Set the bit for the root inode. */
        _set_bit(block, 0);

        /* Set the bits for the root directory. */
        _set_bit(block, 1);
        _set_bit(block, 2);

        /* Write the block. */
        if (dev->put(dev, blkno++, &block) != 0)
        {
            result = OEFS_FAILED;
            goto done;
        }
    }

    /* Write the subsequent (empty) bitmap blocks. */
    for (size_t i = 1; i < num_bitmap_blocks; i++)
    {
        if (dev->put(dev, blkno++, &empty_block) != 0)
        {
            result = OEFS_FAILED;
            goto done;
        }
    }

    /* Write the root directory inode. */
    {
        oefs_inode_t root_inode = {
            .i_magic = OEFS_INODE_MAGIC,
            .i_mode = 0,
            .i_uid = 0,
            .i_gid = 0,
            .i_links = 0,
            .i_size = 2 * sizeof(oefs_dirent_t),
            .i_atime = 0,
            .i_ctime = 0,
            .i_mtime = 0,
            .i_dtime = 0,
            .i_total_blocks = 2,
            .i_next = 0,
            .i_reserved = {0},
            .i_blocks = {2, 3},
        };

        if (dev->put(dev, blkno++, &root_inode) != 0)
        {
            result = OEFS_FAILED;
            goto done;
        }
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
        {
            result = OEFS_FAILED;
            goto done;
        }

        if (dev->put(dev, blkno++, &blocks[1]) != 0)
        {
            result = OEFS_FAILED;
            goto done;
        }
    }

    /* Write the remaining empty blocks. */
    for (size_t i = 0; i < num_blocks - 3; i++)
    {
        if (dev->put(dev, blkno++, &empty_block) != 0)
        {
            result = OEFS_FAILED;
            goto done;
        }
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

oefs_result_t oefs_compute_size(size_t num_blocks, size_t* total_bytes)
{
    oefs_result_t result = OEFS_FAILED;

    block_dev_t dev = {.base =
                              {
                                  .close = _block_dev_close,
                                  .get = _block_dev_get,
                                  .put = _block_dev_put,
                              },
                          .size = 0};

    if (total_bytes)
        *total_bytes = 0;

    if ((result = oefs_initialize(&dev.base, num_blocks)) != OEFS_OK)
        goto done;

    if (total_bytes)
        *total_bytes = dev.size;

    /* Double check the size by computing it differently. */
    {
        size_t num_bitmap_blocks = num_blocks / OEFS_BITS_PER_BLOCK;
        size_t size = (3 + num_bitmap_blocks + num_blocks) * OEFS_BLOCK_SIZE;

        if (size != dev.size)
            goto done;
    }

    result = OEFS_OK;

done:
    return result;
}

int32_t oefs_read_file(oefs_file_t* file, void* data, uint32_t size)
{
    int32_t ret = -1;
    uint32_t first;
    uint32_t i;
    uint32_t remaining;
    uint8_t* ptr = (uint8_t*)data;

    /* Check parameters */
    if (!file || !file->oefs || (!data && size))
        goto done;

    /* The index of the first block to read. */
    first = file->offset / OEFS_BLOCK_SIZE;

    /* The number of bytes remaining to be read */
    remaining = size;

    /* Read the data block-by-block */
    for (i = first; i < file->blknos.size && remaining > 0 && !file->eof; i++)
    {
        oefs_block_t block;
        uint32_t offset;

        if (_read_block(file->oefs, file->blknos.data[i], &block) != OEFS_OK)
            goto done;

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
    ret = size - remaining;

done:
    return ret;
}

int32_t oefs_write_file(oefs_file_t* file, const void* data, uint32_t size)
{
    int32_t ret = -1;
    uint32_t first;
    uint32_t i;
    uint32_t remaining;
    const uint8_t* ptr = (uint8_t*)data;
    uint32_t new_file_size;

    /* Check parameters */
    if (!file || !file->oefs || (!data && size))
        goto done;

    if (!_sane_file(file))
        goto done;

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

        if (_read_block(file->oefs, file->blknos.data[i], &block) != OEFS_OK)
            goto done;

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
        if (_write_block(file->oefs, file->blknos.data[i], &block) != OEFS_OK)
            goto done;
    }

    /* Append remaining data to the file. */
    if (remaining)
    {
        buf_u32_t blknos = BUF_U32_INITIALIZER;

        /* Write the new blocks. */
        if (_write_data(file->oefs, ptr, remaining, &blknos) != OEFS_OK)
            goto done;

        /* Append these block numbers to the block-numbers chain. */
        if (_append_block_chain(file, &blknos) != OEFS_OK)
            goto done;

        /* Update the inode's total_blocks */
        file->inode.i_total_blocks += blknos.size;

        /* Append these block numbers to the file. */
        buf_u32_append(&file->blknos, blknos.data, blknos.size);

        buf_u32_release(&blknos);

        file->offset += remaining;
        remaining = 0;
    }

    /* Update the file size. */
    file->inode.i_size = new_file_size;

    /* Flush the inode to disk. */
    if (_write_block(file->oefs, file->ino, &file->inode) != OEFS_OK)
        goto done;

    /* Calculate number of bytes read */
    ret = size - remaining;

done:
    return ret;
}

int oefs_close_file(oefs_file_t* file)
{
    int ret = -1;

    if (!file)
        goto done;

    buf_u32_release(&file->blknos);
    memset(file, 0, sizeof(oefs_file_t));
    free(file);

done:
    return ret;
}

oefs_result_t oefs_new(oefs_t** oefs_out, oe_block_dev_t* dev)
{
    oefs_result_t result = OEFS_FAILED;
    size_t num_blocks;
    uint8_t* bitmap = NULL;
    oefs_t* oefs = NULL;

    if (oefs_out)
        *oefs_out = NULL;

    if (!dev || !oefs_out)
    {
        result = OEFS_BAD_PARAMETER;
        goto done;
    }

    if (!(oefs = calloc(1, sizeof(oefs_t))))
        goto done;

    /* Save pointer to device (caller is responsible for releasing it). */
    oefs->dev = dev;

    /* Read the super block from the file system. */
    if (dev->get(dev, SUPER_BLOCK_PHYSICAL_BLKNO, &oefs->sb) != 0)
        goto done;

    /* Check the superblock magic number. */
    if (oefs->sb.s_magic != OEFS_SUPER_BLOCK_MAGIC)
        goto done;

    /* Get the number of blocks. */
    num_blocks = oefs->sb.s_num_blocks;

    /* Check that the number of blocks is correct. */
    if (!num_blocks || (num_blocks % OEFS_BITS_PER_BLOCK))
        goto done;

    /* Allocate space for the block bitmap. */
    {
        /* Calculate the number of bitmap blocks. */
        size_t num_bitmap_blocks = num_blocks / OEFS_BITS_PER_BLOCK;

        /* Calculate the size of the bitmap. */
        size_t bitmap_size = num_bitmap_blocks * OEFS_BLOCK_SIZE;

        /* Allocate the bitmap. */
        if (!(bitmap = calloc(1, bitmap_size)))
            goto done;

        /* Read the bitset into memory */
        for (size_t i = 0; i < num_bitmap_blocks; i++)
        {
            uint8_t* ptr = bitmap + (i * OEFS_BLOCK_SIZE);

            if (dev->get(dev, BITMAP_PHYSICAL_BLKNO + i, ptr) != 0)
                goto done;
        }

        /* The first three bits should always be set. These include:
         * (1) The root inode block.
         * (2) The first root directory block.
         * (3) The second root directory block.
         */
        if (!_test_bit(bitmap, 0) || !_test_bit(bitmap, 0) ||
            !_test_bit(bitmap, 0))
        {
            goto done;
        }

        oefs->bitmap = bitmap;
        oefs->bitmap_size = bitmap_size;
        bitmap = NULL;
    }

    /* Check the root inode. */
    {
        oefs_inode_t inode;

        if (_read_block(oefs, OEFS_ROOT_INO, &inode) != OEFS_OK)
            goto done;

        if (inode.i_magic != OEFS_INODE_MAGIC)
            goto done;
    }

#if 0
    {
        printf("<<<<<<<<<<\n");

        if (_dump_directory(oefs, OEFS_ROOT_INO) != OEFS_OK)
            goto done;

        printf(">>>>>>>>>>\n");
    }
#endif

#if 0
    {
        oefs_file_t* file;
        int32_t n;
        uint32_t count = 0;
        oefs_dirent_t entry;

        if (_open_file(oefs, OEFS_ROOT_INO, &file) != 0)
            goto done;

        printf("<<<<<<<<<<\n");
        {
            memset(&entry, 0, sizeof(entry));

            while ((n = oefs_read_file(file, &entry, sizeof(entry))) > 0)
            {
                count++;
            }

            printf("count=%d\n", count);
        }
        printf(">>>>>>>>>>\n");

        buf_t buf = BUF_INITIALIZER;

        /* Write directory entries. */
        const uint32_t N = 6000;
        for (uint32_t i = 0; i < N; i++)
        {
            entry.d_ino = OEFS_ROOT_INO;
            entry.d_type = OEFS_DT_REG;
            sprintf(entry.d_name, "filename-%u", count + i);

            if (buf_append(&buf, &entry, sizeof(entry)) != 0)
            {
                fprintf(stderr, "EEEEEEEEEEEEEEEEEEEEEEE\n");
                oe_abort();
            }

            printf("name{%s}\n", entry.d_name);

            const size_t r = sizeof(entry);

            if ((n = oefs_write_file(file, &entry, sizeof(entry))) != r)
            {
                fprintf(stderr, "error: oops\n");
                goto done;
            }
        }

        if ((n = oefs_write_file(file, buf.data, buf.size)) != buf.size)
        {
            fprintf(stderr, "error: oops\n");
            goto done;
        }

        buf_release(&buf);

        oefs_close_file(file);
    }
#endif

#if 0
    {
        printf("<<<<<<<<<<\n");

        if (_dump_directory(oefs, OEFS_ROOT_INO) != OEFS_OK)
            goto done;

        printf(">>>>>>>>>>\n");
    }
#endif

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

    return result;
}

oefs_result_t oefs_delete(oefs_t* oefs)
{
    oefs_result_t result = OEFS_FAILED;

    if (!oefs)
        goto done;

    free(oefs->bitmap);
    memset(oefs, 0, sizeof(oefs_t));
    free(oefs);

    result = OEFS_OK;

done:
    return result;
}

oefs_dir_t* oefs_opendir(oefs_t* oefs, const char* name)
{
    oefs_dir_t* dir = NULL;
    oefs_file_t* file = NULL;
    uint32_t ino = 0;

    if (!oefs || !name)
        goto done;

    if (strcmp(name, "/") == 0)
    {
        ino = OEFS_ROOT_INO;
    }
    else
    {
        /* ATTN: */
        goto done;
    }

    if (_open_file(oefs, ino, &file) != 0)
        goto done;

    if (!(dir = calloc(1, sizeof(oefs_dir_t))))
        goto done;

    dir->file = file;
    file = NULL;

done:

    if (file)
        oefs_close_file(file);

    return dir;
}

oefs_dirent_t* oefs_readdir(oefs_dir_t* dir)
{
    oefs_dirent_t* ret = NULL;
    int32_t n;

    if (!dir || !dir->file)
        goto done;

    n = oefs_read_file(dir->file, &dir->dirent, sizeof(oefs_dirent_t));

    if (n != sizeof(oefs_dirent_t))
        goto done;

    ret = &dir->dirent;

done:

    return ret;
}

int oefs_closedir(oefs_dir_t* dir)
{
    int ret = -1;

    if (!dir || !dir->file)
        goto done;

    oefs_close_file(dir->file);
    dir->file = NULL;

    free(dir);

done:
    return ret;
}

oefs_result_t __oefs_create_file(
    oefs_t* oefs,
    uint32_t dir_ino,
    const char* name,
    oefs_file_t** file_out)
{
    oefs_result_t result = OEFS_FAILED;
    uint32_t ino;
    oefs_file_t* file = NULL;

    if (file_out)
        *file_out = NULL;

    if (!oefs || !dir_ino || !name || strlen(name) >= OEFS_PATH_MAX ||
        !file_out)
    {
        result = OEFS_BAD_PARAMETER;
        goto done;
    }

    /* Create an inode for the new file. */
    {
        oefs_inode_t inode;

        memset(&inode, 0, sizeof(oefs_inode_t));

        inode.i_links = 1;

        if (_assign_blkno(oefs, &ino) != OEFS_OK)
            goto done;

        if (_flush_bitmap(oefs) != OEFS_OK)
            goto done;

        if (_flush_super_block(oefs) != OEFS_OK)
            goto done;

        if (_write_block(oefs, ino, &inode))
            goto done;
    }

    /* Add an entry to the directory for this file. */
    {
        oefs_dirent_t dirent;
        int32_t n;

        if (_open_file(oefs, dir_ino, &file) != 0)
            goto done;

        /* Check for duplicates. */
        while ((n = oefs_read_file(file, &dirent, sizeof(dirent))) > 0)
        {
            if (n != sizeof(dirent))
                goto done;

            if (strcmp(dirent.d_name, name) == 0)
            {
                result = OEFS_ALREADY_EXISTS;
                goto done;
            }
        }

        /* Append the entry to the directory. */
        {
            memset(&dirent, 0, sizeof(dirent));
            dirent.d_ino = ino;
            dirent.d_off = file->offset;
            dirent.d_reclen = sizeof(dirent);
            dirent.d_type = OEFS_DT_REG;
            strlcpy(dirent.d_name, name, sizeof(dirent.d_name));

            n = oefs_write_file(file, &dirent, sizeof(dirent));

            if (n != sizeof(dirent))
                goto done;
        }
    }

    *file_out = file;
    file = NULL;

    result = OEFS_OK;

done:

    /* TODO: should blocks be unassigned on failure? */

    if (file)
        oefs_close_file(file);

    return result;
}
