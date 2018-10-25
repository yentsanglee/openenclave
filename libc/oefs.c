#include <openenclave/internal/enclavelibc.h>
#include <openenclave/internal/hexdump.h>
#include "oefs.h"
#include <stdio.h>
#include <stdlib.h>
#include "buf.h"

/*
**==============================================================================
**
** Local definitions.
**
**==============================================================================
*/

#define SUPER_BLOCK_PHYSICAL_BLKNO 2
#define BITMAP_PHYSICAL_BLKNO 3

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

static oefs_result_t _load_inode_blknos(
    oefs_t* oefs,
    uint32_t inode_blkno,
    buf_u32_t* blknos)
{
    oefs_result_t result = OEFS_FAILED;
    oefs_inode_t inode;

    /* Read this inode into memory. */
    if (_read_block(oefs, inode_blkno, &inode) != OEFS_OK)
        goto done;

    /* Check the inode magic number. */
    if (inode.i_magic != OEFS_INODE_MAGIC)
        goto done;

    /* Collect direct block numbers. */
    {
        size_t n = sizeof(inode.i_blocks) / sizeof(uint32_t);

        for (size_t i = 0; i < n && inode.i_blocks[i]; i++)
        {
            if (buf_u32_append(blknos, &inode.i_blocks[i], 1) != 0)
                goto done;
        }
    }

    /* Traverse linked list of blknos blocks. */
    {
        uint32_t next = inode.i_next;

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

            next = bnode.b_next;
        }
    }

    result = OEFS_OK;

done:
    return result;
}

/*
**==============================================================================
**
** oefs_initialize()
**
**     Initialize the blocks of a new file system. Create the following blocks.
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

oefs_result_t oefs_initialize(oe_block_device_t* dev, size_t num_blocks)
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
    oe_memset(empty_block, 0, sizeof(empty_block));

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
        oe_memset(&sb, 0, sizeof(sb));

        sb.s_magic = OEFS_SUPER_BLOCK_MAGIC;
        sb.s_num_blocks = num_blocks;
        sb.s_free_blocks = num_blocks - 2;

        if (dev->put(dev, blkno++, &sb) != 0)
        {
            result = OEFS_FAILED;
            goto done;
        }
    }

    /* Write the first bitmap block. */
    {
        uint8_t block[OEFS_BLOCK_SIZE];

        oe_memset(block, 0, sizeof(block));

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
        oefs_inode_t root_inode =
        {
            .i_magic = OEFS_INODE_MAGIC,
            .i_mode = 0,
            .i_uid = 0,
            .i_gid = 0,
            .i_links = 0,
            .i_size = 2 * sizeof(oefs_dir_entry_t),
            .i_atime = 0,
            .i_ctime = 0,
            .i_mtime = 0,
            .i_dtime = 0,
            .i_total_blocks = 2,
            .i_next = 0,
            .i_reserved = { 0 },
            .i_blocks = { 2, 3 },
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

        oefs_dir_entry_t dir_entries[] =
        {
            {
                .d_inode = OEFS_ROOT_INODE_BLKNO,
                .d_type = OEFS_DT_DIR,
                .d_name = ".."

            },
            {
                .d_inode = OEFS_ROOT_INODE_BLKNO,
                .d_type = OEFS_DT_DIR,
                .d_name = "."
            },
        };

        oe_memset(blocks, 0, sizeof(blocks));
        oe_memcpy(blocks, dir_entries, sizeof(dir_entries));

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

/*
**==============================================================================
**
** oefs_compute_size()
**
**     Compute the size requirements for the given file system.
**
**==============================================================================
*/

typedef struct _block_device
{
    oe_block_device_t base;
    size_t size;
}
block_device_t;

static int _block_device_close(oe_block_device_t* dev)
{
    return 0;
}

static int _block_device_get(
    oe_block_device_t* dev, uint32_t blkno, void* data)
{
    return -1;
}

int _block_device_put(oe_block_device_t* dev, uint32_t blkno, const void* data)
{
    block_device_t* device = (block_device_t*)dev;
    device->size += OEFS_BLOCK_SIZE;
    return 0;
}

oefs_result_t oefs_compute_size(size_t num_blocks, size_t* total_bytes)
{
    oefs_result_t result = OEFS_FAILED;

    block_device_t dev =
    {
        .base =
        {
            .close = _block_device_close,
            .get = _block_device_get,
            .put = _block_device_put,
        },
        .size = 0
    };

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

/*
**==============================================================================
**
** oefs_open()
**
**     Open an OEFS file system.
**
**==============================================================================
*/

oefs_result_t oefs_open(oefs_t* oefs, oe_block_device_t* dev)
{
    oefs_result_t result = OEFS_FAILED;
    size_t num_blocks;
    uint8_t* bitmap = NULL;

    if (oefs)
        oe_memset(oefs, 0, sizeof(oefs_t));

    if (!dev || !oefs)
    {
        result = OEFS_BAD_PARAMETER;
        goto done;
    }

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
        if (!_test_bit(bitmap, 0) ||
            !_test_bit(bitmap, 0) ||
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

        if (_read_block(oefs, OEFS_ROOT_INODE_BLKNO, &inode) != OEFS_OK)
            goto done;

        if (inode.i_magic != OEFS_INODE_MAGIC)
            goto done;
    }

#if 1
    /* Load the block numbers for the root inode. */
    {
        buf_u32_t blknos = BUF_U32_INITIALIZER;
        _load_inode_blknos(oefs, OEFS_ROOT_INODE_BLKNO, &blknos);

        for (size_t i = 0; i < blknos.size; i++)
        {
            printf("blknos{i}=%u\n", blknos.data[i]);
        }
    }
#endif

    result = OEFS_OK;

done:

    if (bitmap)
        free(bitmap);

    return result;
}
