/*
**==============================================================================
**
** LSVMTools
**
** MIT License
**
** Copyright (c) Microsoft Corporation. All rights reserved.
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
** SOFTWARE
**
**==============================================================================
*/

#define _GNU_SOURCE
#include "ext2.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "buf.h"

/*
**==============================================================================
**
** local definitions:
**
**==============================================================================
*/

#define EXT2_SECTOR_SIZE 512

static __inline bool _is_valid(const ext2_t* ext2)
{
    return ext2 != NULL && ext2->sb.s_magic == EXT2_S_MAGIC;
}

typedef struct _dir_ent dir_ent_t;

struct _dir_ent
{
    uint32_t inode;
    uint16_t rec_len;
    uint8_t name_len;
    uint8_t file_type;
    char name[EXT2_PATH_MAX];
};

static __inline bool _if_err(ext2_err_t err)
{
    return err != EXT2_ERR_NONE;
}

static char* _strncat(
    char* dest,
    uint32_t dest_size,
    const char* src,
    uint32_t len)
{
    uint32_t n = strlen(dest);
    uint32_t i;

    for (i = 0; (i < len) && (n + i < dest_size); i++)
    {
        dest[n + i] = src[i];
    }

    if (n + i < dest_size)
        dest[n + i] = '\0';
    else if (dest_size > 0)
        dest[dest_size - 1] = '\0';

    return dest;
}

static __inline uint32_t _next_mult(uint32_t x, uint32_t m)
{
    return (x + m - 1) / m * m;
}

static __inline uint32_t _min(uint32_t x, uint32_t y)
{
    return x < y ? x : y;
}

static void _hex_dump(const uint8_t* data, uint32_t size, bool printables)
{
    uint32_t i;

    printf("%u bytes\n", size);

    for (i = 0; i < size; i++)
    {
        unsigned char c = data[i];

        if (printables && (c >= ' ' && c < '~'))
            printf("'%c", c);
        else
            printf("%02X", c);

        if ((i + 1) % 16)
        {
            printf(" ");
        }
        else
        {
            printf("\n");
        }
    }

    printf("\n");
}

#if defined(_WIN32)
typedef unsigned long ssize_t;
#endif

static ssize_t _read(
    oe_block_device_t* dev,
    size_t offset,
    void* data,
    size_t size)
{
    uint32_t blkno;
    uint32_t i;
    uint32_t rem;
    uint8_t* ptr;

    if (!dev || !data)
        return -1;

    blkno = offset / EXT2_SECTOR_SIZE;

    for (i = blkno, rem = size, ptr = (uint8_t*)data; rem; i++)
    {
        uint8_t blk[EXT2_SECTOR_SIZE];
        uint32_t off; /* offset into this block */
        uint32_t len; /* bytes to read from this block */

        if (dev->get(dev, i, blk) != 0)
            return -1;

        /* If first block */
        if (i == blkno)
            off = offset % EXT2_SECTOR_SIZE;
        else
            off = 0;

        len = EXT2_SECTOR_SIZE - off;

        if (len > rem)
            len = rem;

        memcpy(ptr, &blk[off], len);
        rem -= len;
        ptr += len;
    }

    return size;
}

static ssize_t _write(
    oe_block_device_t* dev,
    size_t offset,
    const void* data,
    size_t size)
{
    uint32_t blkno;
    uint32_t i;
    uint32_t rem;
    uint8_t* ptr;

    if (!dev || !data)
        return -1;

    blkno = offset / EXT2_SECTOR_SIZE;

    for (i = blkno, rem = size, ptr = (uint8_t*)data; rem; i++)
    {
        uint8_t blk[EXT2_SECTOR_SIZE];
        uint32_t off; /* offset into this block */
        uint32_t len; /* bytes to write from this block */

        /* Fetch the block */
        if (dev->get(dev, i, blk) != 0)
            return -1;

        /* If first block */
        if (i == blkno)
            off = offset % EXT2_SECTOR_SIZE;
        else
            off = 0;

        len = EXT2_SECTOR_SIZE - off;

        if (len > rem)
            len = rem;

        memcpy(&blk[off], ptr, len);
        rem -= len;
        ptr += len;

        /* Rewrite the block */
        if (dev->put(dev, i, blk) != 0)
            return -1;
    }

    return size;
}

static void _dump_block_numbers(const uint32_t* data, uint32_t size)
{
    uint32_t i;

    printf("%u blocks\n", size);

    for (i = 0; i < size; i++)
    {
        printf("%08X", data[i]);

        if ((i + 1) % 8)
        {
            printf(" ");
        }
        else
        {
            printf("\n");
        }
    }

    printf("\n");
}

static void _ascii_dump(const uint8_t* data, uint32_t size)
{
    uint32_t i;

    printf("=== ASCII dump:\n");

    for (i = 0; i < size; i++)
    {
        unsigned char c = data[i];

        if (c >= ' ' && c <= '~')
            printf("%c", c);
        else
            printf(".");

        if (i + 1 != size && !((i + 1) % 80))
            printf("\n");
    }

    printf("\n");
}

static bool _zero_filled(const uint8_t* data, uint32_t size)
{
    uint32_t i;

    for (i = 0; i < size; i++)
    {
        if (data[i])
            return 0;
    }

    return 1;
}

static uint32_t _count_bits(uint8_t byte)
{
    uint8_t i;
    uint8_t n = 0;

    for (i = 0; i < 8; i++)
    {
        if (byte & (1 << i))
            n++;
    }

    return n;
}

static uint32_t _count_bits_n(const uint8_t* data, uint32_t size)
{
    uint32_t i;
    uint32_t n = 0;

    for (i = 0; i < size; i++)
    {
        n += _count_bits(data[i]);
    }

    return n;
}

static __inline bool _test_bit(
    const uint8_t* data,
    uint32_t size,
    uint32_t index)
{
    uint32_t byte = index / 8;
    uint32_t bit = index % 8;

    if (byte >= size)
        return 0;

    return ((uint32_t)(data[byte]) & (1 << bit)) ? 1 : 0;
}

static __inline void _set_bit(uint8_t* data, uint32_t size, uint32_t index)
{
    uint32_t byte = index / 8;
    uint32_t bit = index % 8;

    if (byte >= size)
        return;

    data[byte] |= (1 << bit);
}

static __inline void _clear_bit(uint8_t* data, uint32_t size, uint32_t index)
{
    uint32_t byte = index / 8;
    uint32_t bit = index % 8;

    if (byte >= size)
        return;

    data[byte] &= ~(1 << bit);
}

static void _dump_bitmap(const ext2_block_t* block)
{
    if (_zero_filled(block->data, block->size))
    {
        printf("...\n\n");
    }
    else
    {
        _hex_dump(block->data, block->size, 0);
    }
}

static bool _is_big_endian()
{
    union {
        unsigned short x;
        unsigned char bytes[2];
    } u;
    u.x = 0xABCD;
    return u.bytes[0] == 0xAB ? 1 : 0;
}

ext2_err_t ext2_load_file(const char* path, void** data, uint32_t* size)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    FILE* is = NULL;
    buf_t buf = BUF_INITIALIZER;

    /* Clear output parameters */
    *data = NULL;
    *size = 0;

    /* Open the file */
    if (!(is = fopen(path, "rb")))
    {
        err = EXT2_ERR_OPEN_FAILED;
        goto done;
    }

    /* Read file into memory */
    for (;;)
    {
        char tmp[4096];
        size_t n;

        n = fread(tmp, 1, sizeof(tmp), is);

        if (n <= 0)
            break;

        if (buf_append(&buf, tmp, n) != 0)
        {
            goto done;
        }
    }

    *data = buf.data;
    *size = buf.size;

    err = EXT2_ERR_NONE;

done:

    if (is)
        fclose(is);

    if (_if_err(err))
        buf_release(&buf);

    return err;
}

/*
**==============================================================================
**
** errors:
**
**==============================================================================
*/

#if defined(EXT2_USE_TYPESAFE_ERRORS)
#define EXT2_DEFINE_ERR(TAG, NUM) const ext2_err_t TAG = {NUM};

EXT2_DEFINE_ERR(EXT2_ERR_NONE, 0);
EXT2_DEFINE_ERR(EXT2_ERR_FAILED, 1);
EXT2_DEFINE_ERR(EXT2_ERR_INVALID_PARAMETER, 2);
EXT2_DEFINE_ERR(EXT2_ERR_FILE_NOT_FOUND, 3);
EXT2_DEFINE_ERR(EXT2_ERR_BAD_MAGIC, 4);
EXT2_DEFINE_ERR(EXT2_ERR_UNSUPPORTED, 5);
EXT2_DEFINE_ERR(EXT2_ERR_OUT_OF_MEMORY, 6);
EXT2_DEFINE_ERR(EXT2_ERR_FAILED_TO_READ_SUPERBLOCK, 7);
EXT2_DEFINE_ERR(EXT2_ERR_FAILED_TO_READ_GROUPS, 8);
EXT2_DEFINE_ERR(EXT2_ERR_FAILED_TO_READ_INODE, 9);
EXT2_DEFINE_ERR(EXT2_ERR_UNSUPPORTED_REVISION, 10);
EXT2_DEFINE_ERR(EXT2_ERR_OPEN_FAILED, 11);
EXT2_DEFINE_ERR(EXT2_ERR_BUFFER_OVERFLOW, 12);
EXT2_DEFINE_ERR(EXT2_ERR_SEEK_FAILED, 13);
EXT2_DEFINE_ERR(EXT2_ERR_READ_FAILED, 14);
EXT2_DEFINE_ERR(EXT2_ERR_WRITE_FAILED, 15);
EXT2_DEFINE_ERR(EXT2_ERR_UNEXPECTED, 16);
EXT2_DEFINE_ERR(EXT2_ERR_SANITY_CHECK_FAILED, 17);
EXT2_DEFINE_ERR(EXT2_ERR_BAD_BLKNO, 18);
EXT2_DEFINE_ERR(EXT2_ERR_BAD_INO, 19);
EXT2_DEFINE_ERR(EXT2_ERR_BAD_GRPNO, 18);
EXT2_DEFINE_ERR(EXT2_ERR_BAD_MULTIPLE, 19);
EXT2_DEFINE_ERR(EXT2_ERR_EXTRANEOUS_DATA, 20);
EXT2_DEFINE_ERR(EXT2_ERR_BAD_SIZE, 21);
EXT2_DEFINE_ERR(EXT2_ERR_PATH_TOO_LONG, 22);

#endif /* define(EXT2_USE_TYPESAFE_ERRORS) */

const char* ext2_err_str(ext2_err_t err)
{
    if (err == EXT2_ERR_NONE)
        return "ok";

    if (err == EXT2_ERR_FAILED)
        return "failed";

    if (err == EXT2_ERR_INVALID_PARAMETER)
        return "invalid parameter";

    if (err == EXT2_ERR_FILE_NOT_FOUND)
        return "file not found";

    if (err == EXT2_ERR_BAD_MAGIC)
        return "bad magic";

    if (err == EXT2_ERR_UNSUPPORTED)
        return "unsupported";

    if (err == EXT2_ERR_OUT_OF_MEMORY)
        return "out of memory";

    if (err == EXT2_ERR_FAILED_TO_READ_SUPERBLOCK)
        return "failed to read superblock";

    if (err == EXT2_ERR_FAILED_TO_READ_GROUPS)
        return "failed to read groups";

    if (err == EXT2_ERR_FAILED_TO_READ_INODE)
        return "failed to read inode";

    if (err == EXT2_ERR_UNSUPPORTED_REVISION)
        return "unsupported revision";

    if (err == EXT2_ERR_OPEN_FAILED)
        return "open failed";

    if (err == EXT2_ERR_BUFFER_OVERFLOW)
        return "buffer overflow";

    if (err == EXT2_ERR_SEEK_FAILED)
        return "buffer overflow";

    if (err == EXT2_ERR_READ_FAILED)
        return "read failed";

    if (err == EXT2_ERR_WRITE_FAILED)
        return "write failed";

    if (err == EXT2_ERR_UNEXPECTED)
        return "unexpected";

    if (err == EXT2_ERR_SANITY_CHECK_FAILED)
        return "sanity check failed";

    if (err == EXT2_ERR_BAD_BLKNO)
        return "bad block number";

    if (err == EXT2_ERR_BAD_INO)
        return "bad inode number";

    if (err == EXT2_ERR_BAD_GRPNO)
        return "bad group number";

    if (err == EXT2_ERR_BAD_MULTIPLE)
        return "bad multiple";

    if (err == EXT2_ERR_EXTRANEOUS_DATA)
        return "extraneous data";

    if (err == EXT2_ERR_BAD_SIZE)
        return "bad size";

    if (err == EXT2_ERR_PATH_TOO_LONG)
        return "path too long";

    return "unknown";
}

/*
**==============================================================================
**
** blocks:
**
**==============================================================================
*/

/* Byte offset of this block (block 0 is the null block) */
static uint32_t BlockOffset(uint32_t blkno, uint32_t block_size)
{
    return blkno * block_size;
}

static uint32_t MakeBlkno(const ext2_t* ext2, uint32_t grpno, uint32_t lblkno)
{
    const uint32_t first = ext2->sb.s_first_data_block;
    return (grpno * ext2->sb.s_blocks_per_group) + (lblkno + first);
}

static uint32_t _blokno_to_grpno(const ext2_t* ext2, uint32_t blkno)
{
    const uint32_t first = ext2->sb.s_first_data_block;

    if (first && blkno == 0)
        return 0;

    return (blkno - first) / ext2->sb.s_blocks_per_group;
}

static uint32_t _blkno_to_lblkno(const ext2_t* ext2, uint32_t blkno)
{
    const uint32_t first = ext2->sb.s_first_data_block;

    if (first && blkno == 0)
        return 0;

    return (blkno - first) % ext2->sb.s_blocks_per_group;
}

static ext2_err_t _read_blocks(
    const ext2_t* ext2,
    uint32_t blkno,
    uint32_t nblks,
    buf_t* buf)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    uint32_t bytes;

    /* Check for null parameters */
    if (!_is_valid(ext2) || !buf)
    {
        err = EXT2_ERR_INVALID_PARAMETER;
        goto done;
    }

    /* Expand the allocation to hold the new data */
    {
        bytes = nblks * ext2->block_size;

        if (buf_reserve(buf, buf->size + bytes) != 0)
            goto done;
    }

    /* Read the blocks */
    if (_read(
            ext2->dev,
            BlockOffset(blkno, ext2->block_size),
            (unsigned char*)buf->data + buf->size,
            bytes) != bytes)
    {
        err = EXT2_ERR_READ_FAILED;
        goto done;
    }

    buf->size += bytes;

    err = EXT2_ERR_NONE;

done:

    return err;
}

ext2_err_t ext2_read_block(
    const ext2_t* ext2,
    uint32_t blkno,
    ext2_block_t* block)
{
    ext2_err_t err = EXT2_ERR_FAILED;

    /* Check for null parameters */
    if (!_is_valid(ext2) || !block)
    {
        err = EXT2_ERR_INVALID_PARAMETER;
        goto done;
    }

    memset(block, 0xAA, sizeof(ext2_block_t));

    /* Is block size too big for buffer? */
    if (ext2->block_size > sizeof(block->data))
    {
        err = EXT2_ERR_BUFFER_OVERFLOW;
        goto done;
    }

    /* Set the size of the block */
    block->size = ext2->block_size;

    /* Read the block */
    if (_read(
            ext2->dev,
            BlockOffset(blkno, ext2->block_size),
            block->data,
            block->size) != block->size)
    {
        err = EXT2_ERR_READ_FAILED;
        goto done;
    }

    err = EXT2_ERR_NONE;

done:

    return err;
}

ext2_err_t ext2_write_block(
    const ext2_t* ext2,
    uint32_t blkno,
    const ext2_block_t* block)
{
    ext2_err_t err = EXT2_ERR_FAILED;

    /* Check for null parameters */
    if (!_is_valid(ext2) || !block)
    {
        err = EXT2_ERR_INVALID_PARAMETER;
        goto done;
    }

    /* Is block size too big for buffer? */
    if (block->size > ext2->block_size)
    {
        err = EXT2_ERR_BUFFER_OVERFLOW;
        goto done;
    }

    /* Write the block */
    if (_write(
            ext2->dev,
            BlockOffset(blkno, ext2->block_size),
            block->data,
            block->size) != block->size)
    {
        err = EXT2_ERR_WRITE_FAILED;
        goto done;
    }

    err = EXT2_ERR_NONE;

done:

    return err;
}

static ext2_err_t _append_direct_block_numbers(
    const uint32_t* blocks,
    uint32_t num_blocks,
    buf_u32_t* buf)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    uint32_t i;
    uint32_t n = 0;

    /* Determine size of blocks array */
    for (i = 0; i < num_blocks && blocks[i]; i++)
        n++;

    if (n == 0)
    {
        err = EXT2_ERR_NONE;
        goto done;
    }

    /* Append the blocks to the 'data' array */
    if (_if_err(err = buf_u32_append(buf, blocks, n)))
        goto done;

    err = EXT2_ERR_NONE;

done:
    return err;
}

static ext2_err_t _append_indirect_block_numbers(
    const ext2_t* ext2,
    const uint32_t* blocks,
    uint32_t num_blocks,
    bool include_block_blocks,
    buf_u32_t* buf)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    ext2_block_t block;
    uint32_t i;

    if (include_block_blocks)
    {
        if (_if_err(
                err = _append_direct_block_numbers(blocks, num_blocks, buf)))
        {
            goto done;
        }
    }

    /* Handle the direct blocks */
    for (i = 0; i < num_blocks && blocks[i]; i++)
    {
        uint32_t block_no = blocks[i];

        /* Read the next block */
        if (_if_err(err = ext2_read_block(ext2, block_no, &block)))
        {
            goto done;
        }

        if (_if_err(
                err = _append_direct_block_numbers(
                    (const uint32_t*)block.data,
                    block.size / sizeof(uint32_t),
                    buf)))
        {
            goto done;
        }
    }

    err = EXT2_ERR_NONE;

done:
    return err;
}

static ext2_err_t _append_double_indirect_block_numbers(
    const ext2_t* ext2,
    const uint32_t* blocks,
    uint32_t num_blocks,
    bool include_block_blocks,
    buf_u32_t* buf)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    ext2_block_t block;
    uint32_t i;

    if (include_block_blocks)
    {
        if (_if_err(
                err = _append_direct_block_numbers(blocks, num_blocks, buf)))
        {
            goto done;
        }
    }

    /* Handle the direct blocks */
    for (i = 0; i < num_blocks && blocks[i]; i++)
    {
        uint32_t block_no = blocks[i];

        /* Read the next block */
        if (_if_err(err = ext2_read_block(ext2, block_no, &block)))
        {
            goto done;
        }

        if (_if_err(
                err = _append_indirect_block_numbers(
                    ext2,
                    (const uint32_t*)block.data,
                    block.size / sizeof(uint32_t),
                    include_block_blocks,
                    buf)))
        {
            goto done;
        }
    }

    err = EXT2_ERR_NONE;

done:
    return err;
}

static ext2_err_t _load_block_numbers_from_inode(
    const ext2_t* ext2,
    const ext2_inode_t* inode,
    bool include_block_blocks,
    buf_u32_t* buf)
{
    ext2_err_t err = EXT2_ERR_FAILED;

    /* Check parameters */
    if (!_is_valid(ext2) || !inode || !buf)
    {
        err = EXT2_ERR_INVALID_PARAMETER;
        goto done;
    }

    /* Handle the direct blocks */
    if (_if_err(
            err = _append_direct_block_numbers(
                inode->i_block, EXT2_SINGLE_INDIRECT_BLOCK, buf)))
    {
        goto done;
    }

    /* Handle single-indirect blocks */
    if (inode->i_block[EXT2_SINGLE_INDIRECT_BLOCK])
    {
        ext2_block_t block;
        uint32_t block_no = inode->i_block[EXT2_SINGLE_INDIRECT_BLOCK];

        /* Read the next block */
        if (_if_err(err = ext2_read_block(ext2, block_no, &block)))
        {
            goto done;
        }

        if (include_block_blocks)
        {
            if (_if_err(err = buf_u32_append(buf, &block_no, 1)))
            {
                goto done;
            }
        }

        /* Append the block numbers from this block */
        if (_if_err(
                err = _append_direct_block_numbers(
                    (const uint32_t*)block.data,
                    block.size / sizeof(uint32_t),
                    buf)))
        {
            goto done;
        }
    }

    /* Handle double-indirect blocks */
    if (inode->i_block[EXT2_DOUBLE_INDIRECT_BLOCK])
    {
        ext2_block_t block;
        uint32_t block_no = inode->i_block[EXT2_DOUBLE_INDIRECT_BLOCK];

        /* Read the next block */
        if (_if_err(err = ext2_read_block(ext2, block_no, &block)))
        {
            goto done;
        }

        if (include_block_blocks)
        {
            if (_if_err(err = buf_u32_append(buf, &block_no, 1)))
                goto done;
        }

        if (_if_err(
                err = _append_indirect_block_numbers(
                    ext2,
                    (const uint32_t*)block.data,
                    block.size / sizeof(uint32_t),
                    include_block_blocks,
                    buf)))
        {
            goto done;
        }
    }

    /* Handle triple-indirect blocks */
    if (inode->i_block[EXT2_TRIPLE_INDIRECT_BLOCK])
    {
        ext2_block_t block;
        uint32_t block_no = inode->i_block[EXT2_TRIPLE_INDIRECT_BLOCK];

        /* Read the next block */
        if (_if_err(err = ext2_read_block(ext2, block_no, &block)))
        {
            goto done;
        }

        if (include_block_blocks)
        {
            if (_if_err(err = buf_u32_append(buf, &block_no, 1)))
                goto done;
        }

        if (_if_err(
                err = _append_double_indirect_block_numbers(
                    ext2,
                    (const uint32_t*)block.data,
                    block.size / sizeof(uint32_t),
                    include_block_blocks,
                    buf)))
        {
            goto done;
        }
    }

    err = EXT2_ERR_NONE;

done:

    return err;
}

static ext2_err_t _write_group(const ext2_t* ext2, uint32_t grpno);

static ext2_err_t _write_super_block(const ext2_t* ext2);

static ext2_err_t _check_block_number(
    ext2_t* ext2,
    uint32_t blkno,
    uint32_t grpno,
    uint32_t lblkno)
{
    ext2_err_t err = EXT2_ERR_FAILED;

    /* Check parameters */
    if (!ext2)
    {
        err = EXT2_ERR_INVALID_PARAMETER;
        goto done;
    }

    /* Sanity check */
    if (MakeBlkno(ext2, grpno, lblkno) != blkno)
    {
        err = EXT2_ERR_BAD_BLKNO;
        goto done;
    }

    /* See if 'grpno' is out of range */
    if (grpno > ext2->group_count)
    {
        err = EXT2_ERR_BAD_GRPNO;
        goto done;
    }

    err = EXT2_ERR_NONE;

done:
    return err;
}

static ext2_err_t _write_group_with_bitmap(
    ext2_t* ext2,
    uint32_t grpno,
    ext2_block_t* bitmap)
{
    ext2_err_t err = EXT2_ERR_FAILED;

    /* Check parameters */
    if (!ext2 || !bitmap)
    {
        err = EXT2_ERR_INVALID_PARAMETER;
        goto done;
    }

    /* Write the group */
    if (_if_err(err = _write_group(ext2, grpno)))
    {
        goto done;
    }

    /* Write the bitmap */
    if (_if_err(err = ext2_write_block_bitmap(ext2, grpno, bitmap)))
    {
        goto done;
    }

    err = EXT2_ERR_NONE;

done:
    return err;
}

static int _compare_uint32(const void* p1, const void* p2)
{
    uint32_t v1 = *(uint32_t*)p1;
    uint32_t v2 = *(uint32_t*)p2;
    if (v1 < v2)
        return -1;
    else if (v1 > v2)
        return 1;
    return 0;
}

static ext2_err_t _put_blocks(
    ext2_t* ext2,
    const uint32_t* blknos,
    uint32_t nblknos)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    uint32_t i;
    uint32_t* temp = NULL;

    /* Sort the block numbers, so we can just iterate through the group list
       and make changes there. So we do I/O operations = to the group list
       rather than the number of blocks.
       NOTE: Hash table should be faster, but not a big deal for small files
       (initrd).
    */
    temp = (uint32_t*)malloc(nblknos * sizeof(uint32_t));
    if (temp == NULL)
    {
        err = EXT2_ERR_OUT_OF_MEMORY;
        goto done;
    }
    for (i = 0; i < nblknos; i++)
    {
        temp[i] = blknos[i];
    }

    qsort(temp, nblknos, sizeof(uint32_t), _compare_uint32);

    /* Loop through the groups. */
    ext2_block_t bitmap;
    uint32_t prevgrpno = 0;
    for (i = 0; i < nblknos; i++)
    {
        uint32_t grpno = _blokno_to_grpno(ext2, temp[i]);
        uint32_t lblkno = _blkno_to_lblkno(ext2, temp[i]);

        if (_if_err(err = _check_block_number(ext2, temp[i], grpno, lblkno)))
        {
            goto done;
        }

        if (i == 0)
        {
            if (_if_err(err = ext2_read_block_bitmap(ext2, grpno, &bitmap)))
            {
                goto done;
            }
        }
        else if (prevgrpno != grpno)
        {
            /* Advance to next group so write old bitmap and read new one. */
            if (_if_err(
                    err = _write_group_with_bitmap(ext2, prevgrpno, &bitmap)))
            {
                goto done;
            }
            if (_if_err(err = ext2_read_block_bitmap(ext2, grpno, &bitmap)))
            {
                goto done;
            }
        }

        /* Sanity check */
        if (!_test_bit(bitmap.data, bitmap.size, lblkno))
        {
            err = EXT2_ERR_SANITY_CHECK_FAILED;
            goto done;
        }

        /* Update in memory structs. */
        _clear_bit(bitmap.data, bitmap.size, lblkno);
        ext2->sb.s_free_blocks_count++;
        ext2->groups[grpno].bg_free_blocks_count++;
        prevgrpno = grpno;

        /* Always write final block. */
        if (i + 1 == nblknos)
        {
            if (_if_err(err = _write_group_with_bitmap(ext2, grpno, &bitmap)))
            {
                goto done;
            }
        }
    }

    /* Update super block. */
    if (_if_err(err = _write_super_block(ext2)))
    {
        goto done;
    }

    err = EXT2_ERR_NONE;

done:
    free(temp);
    return err;
}

static ext2_err_t _get_block(ext2_t* ext2, uint32_t* blkno)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    ext2_block_t bitmap;
    uint32_t grpno;

    /* Check parameters */
    if (!ext2 || !blkno)
    {
        err = EXT2_ERR_INVALID_PARAMETER;
        goto done;
    }

    /* Clear any block number */
    *blkno = 0;

    /* Use brute force search for a free block */
    for (grpno = 0; grpno < ext2->group_count; grpno++)
    {
        uint32_t lblkno;

        /* Read the bitmap */

        if (_if_err(err = ext2_read_block_bitmap(ext2, grpno, &bitmap)))
        {
            goto done;
        }

        /* Scan the bitmap, looking for free bit */
        for (lblkno = 0; lblkno < bitmap.size * 8; lblkno++)
        {
            if (!_test_bit(bitmap.data, bitmap.size, lblkno))
            {
                _set_bit(bitmap.data, bitmap.size, lblkno);
                *blkno = MakeBlkno(ext2, grpno, lblkno);
                break;
            }
        }

        if (*blkno)
            break;
    }

    /* If no free blocks found */
    if (!*blkno)
    {
        goto done;
    }

    /* Write the superblock */
    {
        ext2->sb.s_free_blocks_count--;

        if (_if_err(err = _write_super_block(ext2)))
        {
            goto done;
        }
    }

    /* Write the group */
    {
        ext2->groups[grpno].bg_free_blocks_count--;

        if (_if_err(err = _write_group(ext2, grpno)))
        {
            goto done;
        }
    }

    /* Write the bitmap */
    if (_if_err(err = ext2_write_block_bitmap(ext2, grpno, &bitmap)))
    {
        goto done;
    }

    err = EXT2_ERR_NONE;

done:
    return err;
}

/*
**==============================================================================
**
** super block:
**
**==============================================================================
*/

void ext2_dump_super_block(const ext2_super_block_t* sb)
{
    printf("=== ext2_super_block_t:\n");
    printf("s_inodes_count=%u\n", sb->s_inodes_count);
    printf("s_blocks_count=%u\n", sb->s_blocks_count);
    printf("s_r_blocks_count=%u\n", sb->s_r_blocks_count);
    printf("s_free_blocks_count=%u\n", sb->s_free_blocks_count);
    printf("s_free_inodes_count=%u\n", sb->s_free_inodes_count);
    printf("s_first_data_block=%u\n", sb->s_first_data_block);
    printf("s_log_block_size=%u\n", sb->s_log_block_size);
    printf("s_log_frag_size=%u\n", sb->s_log_frag_size);
    printf("s_blocks_per_group=%u\n", sb->s_blocks_per_group);
    printf("s_frags_per_group=%u\n", sb->s_frags_per_group);
    printf("s_inodes_per_group=%u\n", sb->s_inodes_per_group);
    printf("s_mtime=%u\n", sb->s_mtime);
    printf("s_wtime=%u\n", sb->s_wtime);
    printf("s_mnt_count=%u\n", sb->s_mnt_count);
    printf("s_max_mnt_count=%u\n", sb->s_max_mnt_count);
    printf("s_magic=%X\n", sb->s_magic);
    printf("s_state=%u\n", sb->s_state);
    printf("s_errors=%u\n", sb->s_errors);
    printf("s_minor_rev_level=%u\n", sb->s_minor_rev_level);
    printf("s_lastcheck=%u\n", sb->s_lastcheck);
    printf("s_checkinterval=%u\n", sb->s_checkinterval);
    printf("s_creator_os=%u\n", sb->s_creator_os);
    printf("s_rev_level=%u\n", sb->s_rev_level);
    printf("s_def_resuid=%u\n", sb->s_def_resuid);
    printf("s_def_resgid=%u\n", sb->s_def_resgid);
    printf("s_first_ino=%u\n", sb->s_first_ino);
    printf("s_inode_size=%u\n", sb->s_inode_size);
    printf("s_block_group_nr=%u\n", sb->s_block_group_nr);
    printf("s_feature_compat=%u\n", sb->s_feature_compat);
    printf("s_feature_incompat=%u\n", sb->s_feature_incompat);
    printf("s_feature_ro_compat=%u\n", sb->s_feature_ro_compat);
    printf(
        "s_uuid="
        "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n",
        sb->s_uuid[0],
        sb->s_uuid[1],
        sb->s_uuid[2],
        sb->s_uuid[3],
        sb->s_uuid[4],
        sb->s_uuid[5],
        sb->s_uuid[6],
        sb->s_uuid[7],
        sb->s_uuid[8],
        sb->s_uuid[9],
        sb->s_uuid[10],
        sb->s_uuid[11],
        sb->s_uuid[12],
        sb->s_uuid[13],
        sb->s_uuid[14],
        sb->s_uuid[15]);
    printf("s_volume_name=%s\n", sb->s_volume_name);
    printf("s_last_mounted=%s\n", sb->s_last_mounted);
    printf("s_algo_bitmap=%u\n", sb->s_algo_bitmap);
    printf("s_prealloc_blocks=%u\n", sb->s_prealloc_blocks);
    printf("s_prealloc_dir_blocks=%u\n", sb->s_prealloc_dir_blocks);
    printf(
        "s_journal_uuid="
        "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n",
        sb->s_journal_uuid[0],
        sb->s_journal_uuid[1],
        sb->s_journal_uuid[2],
        sb->s_journal_uuid[3],
        sb->s_journal_uuid[4],
        sb->s_journal_uuid[5],
        sb->s_journal_uuid[6],
        sb->s_journal_uuid[7],
        sb->s_journal_uuid[8],
        sb->s_journal_uuid[9],
        sb->s_journal_uuid[10],
        sb->s_journal_uuid[11],
        sb->s_journal_uuid[12],
        sb->s_journal_uuid[13],
        sb->s_journal_uuid[14],
        sb->s_journal_uuid[15]);
    printf("s_journal_inum=%u\n", sb->s_journal_inum);
    printf("s_journal_dev=%u\n", sb->s_journal_dev);
    printf("s_last_orphan=%u\n", sb->s_last_orphan);
    printf(
        "s_hash_seed={%02X,%02X,%02X,%02X}\n",
        sb->s_hash_seed[0],
        sb->s_hash_seed[1],
        sb->s_hash_seed[2],
        sb->s_hash_seed[3]);
    printf("s_def_hash_version=%u\n", sb->s_def_hash_version);
    printf("s_default_mount_options=%u\n", sb->s_default_mount_options);
    printf("s_first_meta_bg=%u\n", sb->s_first_meta_bg);
    printf("\n");
}

static ext2_err_t _read_super_block(
    oe_block_device_t* dev,
    ext2_super_block_t* sb)
{
    ext2_err_t err = EXT2_ERR_FAILED;

    /* Read the superblock */
    if (_read(dev, EXT2_BASE_OFFSET, sb, sizeof(ext2_super_block_t)) !=
        sizeof(ext2_super_block_t))
    {
        goto done;
    }

    err = EXT2_ERR_NONE;

done:
    return err;
}

static ext2_err_t _write_super_block(const ext2_t* ext2)
{
    ext2_err_t err = EXT2_ERR_FAILED;

    /* Read the superblock */
    if (_write(
            ext2->dev,
            EXT2_BASE_OFFSET,
            &ext2->sb,
            sizeof(ext2_super_block_t)) != sizeof(ext2_super_block_t))
    {
        goto done;
    }

    err = EXT2_ERR_NONE;

done:
    return err;
}

/*
**==============================================================================
**
** groups:
**
**==============================================================================
*/

static void _dump_group_desc(const ext2_group_desc_t* gd)
{
    printf("=== ext2_group_desc_t\n");
    printf("bg_block_bitmap=%u\n", gd->bg_block_bitmap);
    printf("bg_inode_bitmap=%u\n", gd->bg_inode_bitmap);
    printf("bg_inode_table=%u\n", gd->bg_inode_table);
    printf("bg_free_blocks_count=%u\n", gd->bg_free_blocks_count);
    printf("bg_free_inodes_count=%u\n", gd->bg_free_inodes_count);
    printf("bg_used_dirs_count=%u\n", gd->bg_used_dirs_count);
    printf("\n");
}

static void _dump_group_descs(
    const ext2_group_desc_t* groups,
    uint32_t group_count)
{
    const ext2_group_desc_t* p = groups;
    const ext2_group_desc_t* end = groups + group_count;

    while (p != end)
    {
        _dump_group_desc(p);
        p++;
    }
}

static ext2_group_desc_t* _read_groups(const ext2_t* ext2)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    ext2_group_desc_t* groups = NULL;
    uint32_t groups_size = 0;
    uint32_t blkno;

    /* Check the file system argument */
    if (!_is_valid(ext2))
    {
        err = EXT2_ERR_INVALID_PARAMETER;
        goto done;
    }

    /* Allocate the groups list */
    {
        groups_size = ext2->group_count * sizeof(ext2_group_desc_t);

        if (!(groups = (ext2_group_desc_t*)malloc(groups_size)))
        {
            goto done;
        }

        memset(groups, 0xAA, groups_size);
    }

    /* Determine the block where group table starts */
    if (ext2->block_size == 1024)
        blkno = 2;
    else
        blkno = 1;

    /* Read the block */
    if (_read(
            ext2->dev,
            BlockOffset(blkno, ext2->block_size),
            groups,
            groups_size) != groups_size)
    {
        goto done;
    }

    err = EXT2_ERR_NONE;

done:

    if (_if_err(err))
    {
        if (groups)
        {
            free(groups);
            groups = NULL;
        }
    }

    return groups;
}

static ext2_err_t _write_group(const ext2_t* ext2, uint32_t grpno)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    uint32_t blkno;

    /* Check the file system argument */
    if (!_is_valid(ext2))
    {
        err = EXT2_ERR_INVALID_PARAMETER;
        goto done;
    }

    if (ext2->block_size == 1024)
        blkno = 2;
    else
        blkno = 1;

    /* Read the block */
    if (_write(
            ext2->dev,
            BlockOffset(blkno, ext2->block_size) +
                (grpno * sizeof(ext2_group_desc_t)),
            &ext2->groups[grpno],
            sizeof(ext2_group_desc_t)) != sizeof(ext2_group_desc_t))
    {
        err = EXT2_ERR_WRITE_FAILED;
        goto done;
    }

    err = EXT2_ERR_NONE;

done:

    return err;
}

/*
**==============================================================================
**
** inodes:
**
**==============================================================================
*/

static uint32_t _make_ino(const ext2_t* ext2, uint32_t grpno, uint32_t lino)
{
    return (grpno * ext2->sb.s_inodes_per_group) + (lino + 1);
}

static uint32_t _ino_to_grpno(const ext2_t* ext2, ext2_ino_t ino)
{
    if (ino == 0)
        return 0;

    return (ino - 1) / ext2->sb.s_inodes_per_group;
}

static uint32_t _ino_to_lino(const ext2_t* ext2, ext2_ino_t ino)
{
    if (ino == 0)
        return 0;

    return (ino - 1) % ext2->sb.s_inodes_per_group;
}

void ext2_dump_inode(const ext2_t* ext2, const ext2_inode_t* inode)
{
    uint32_t i;
    uint32_t n;
    (void)_hex_dump;
    (void)_ascii_dump;

    printf("=== ext2_inode_t\n");
    printf("i_mode=%u (%X)\n", inode->i_mode, inode->i_mode);
    printf("i_uid=%u\n", inode->i_uid);
    printf("i_size=%u\n", inode->i_size);
    printf("i_atime=%u\n", inode->i_atime);
    printf("i_ctime=%u\n", inode->i_ctime);
    printf("i_mtime=%u\n", inode->i_mtime);
    printf("i_dtime=%u\n", inode->i_dtime);
    printf("i_gid=%u\n", inode->i_gid);
    printf("i_links_count=%u\n", inode->i_links_count);
    printf("i_blocks=%u\n", inode->i_blocks);
    printf("i_flags=%u\n", inode->i_flags);
    printf("i_osd1=%u\n", inode->i_osd1);

    {
        printf("i_block[]={");
        n = sizeof(inode->i_block) / sizeof(inode->i_block[0]);

        for (i = 0; i < n; i++)
        {
            printf("%X", inode->i_block[i]);

            if (i + 1 != n)
                printf(", ");
        }

        printf("}\n");
    }

    printf("i_generation=%u\n", inode->i_generation);
    printf("i_file_acl=%u\n", inode->i_file_acl);
    printf("i_dir_acl=%u\n", inode->i_dir_acl);
    printf("i_faddr=%u\n", inode->i_faddr);

    {
        printf("i_osd2[]={");
        n = sizeof(inode->i_osd2) / sizeof(inode->i_osd2[0]);

        for (i = 0; i < n; i++)
        {
            printf("%u", inode->i_osd2[i]);

            if (i + 1 != n)
                printf(", ");
        }

        printf("}\n");
    }

    printf("\n");

    if (inode->i_block[0])
    {
        ext2_block_t block;

        if (!_if_err(ext2_read_block(ext2, inode->i_block[0], &block)))
        {
            _ascii_dump(block.data, block.size);
        }
    }
}

static ext2_err_t _write_block_bitmap(
    const ext2_t* ext2,
    uint32_t group_index,
    const ext2_block_t* block)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    uint32_t bitmap_size_bytes;

    if (!_is_valid(ext2) || !block)
    {
        err = EXT2_ERR_INVALID_PARAMETER;
        goto done;
    }

    bitmap_size_bytes = ext2->sb.s_inodes_per_group / 8;

    if (block->size != bitmap_size_bytes)
    {
        err = EXT2_ERR_BAD_SIZE;
        goto done;
    }

    if (group_index > ext2->group_count)
    {
        err = EXT2_ERR_BAD_GRPNO;
        goto done;
    }

    if (_if_err(
            err = ext2_write_block(
                ext2, ext2->groups[group_index].bg_inode_bitmap, block)))
    {
        goto done;
    }

    err = EXT2_ERR_NONE;

done:
    return err;
}

static ext2_err_t _get_ino(ext2_t* ext2, ext2_ino_t* ino)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    ext2_block_t bitmap;
    uint32_t grpno;

    /* Check parameters */
    if (!ext2 || !ino)
    {
        err = EXT2_ERR_INVALID_PARAMETER;
        goto done;
    }

    /* Clear the node number */
    *ino = 0;

    /* Use brute force search for a free inode number */
    for (grpno = 0; grpno < ext2->group_count; grpno++)
    {
        uint32_t lino;

        /* Read the bitmap */
        if (_if_err(err = ext2_read_read_inode_bitmap(ext2, grpno, &bitmap)))
        {
            goto done;
        }

        /* Scan the bitmap, looking for free bit */
        for (lino = 0; lino < bitmap.size * 8; lino++)
        {
            if (!_test_bit(bitmap.data, bitmap.size, lino))
            {
                _set_bit(bitmap.data, bitmap.size, lino);
                *ino = _make_ino(ext2, grpno, lino);
                break;
            }
        }

        if (*ino)
            break;
    }

    /* If no free inode numbers */
    if (!*ino)
    {
        goto done;
    }

    /* Write the superblock */
    {
        ext2->sb.s_free_inodes_count--;

        if (_if_err(err = _write_super_block(ext2)))
        {
            goto done;
        }
    }

    /* Write the group */
    {
        ext2->groups[grpno].bg_free_inodes_count--;

        if (_if_err(err = _write_group(ext2, grpno)))
        {
            goto done;
        }
    }

    /* Write the bitmap */
    if (_if_err(err = _write_block_bitmap(ext2, grpno, &bitmap)))
    {
        goto done;
    }

    err = EXT2_ERR_NONE;

done:
    return err;
}

ext2_err_t ext2_read_inode(
    const ext2_t* ext2,
    ext2_ino_t ino,
    ext2_inode_t* inode)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    uint32_t lino = _ino_to_lino(ext2, ino);
    uint32_t grpno = _ino_to_grpno(ext2, ino);
    const ext2_group_desc_t* group = &ext2->groups[grpno];
    uint32_t inode_size = ext2->sb.s_inode_size;
    uint32_t offset;

    if (ino == 0)
    {
        err = EXT2_ERR_BAD_INO;
        goto done;
    }

    /* Check the reverse mapping */
    {
        ext2_ino_t tmp;
        tmp = _make_ino(ext2, grpno, lino);
        assert(tmp == ino);
    }

    offset = BlockOffset(group->bg_inode_table, ext2->block_size) +
             lino * inode_size;

    /* Read the inode */
    if (_read(ext2->dev, offset, inode, inode_size) != inode_size)
    {
        goto done;
    }

    err = EXT2_ERR_NONE;

done:
    return err;
}

static ext2_err_t _write_inode(
    const ext2_t* ext2,
    ext2_ino_t ino,
    const ext2_inode_t* inode)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    uint32_t lino = _ino_to_lino(ext2, ino);
    uint32_t grpno = _ino_to_grpno(ext2, ino);
    const ext2_group_desc_t* group = &ext2->groups[grpno];
    uint32_t inode_size = ext2->sb.s_inode_size;
    uint32_t offset;

    /* Check the reverse mapping */
    {
        ext2_ino_t tmp;
        tmp = _make_ino(ext2, grpno, lino);
        assert(tmp == ino);
    }

    offset = BlockOffset(group->bg_inode_table, ext2->block_size) +
             lino * inode_size;

    /* Read the inode */
    if (_write(ext2->dev, offset, inode, inode_size) != inode_size)
    {
        goto done;
    }

    err = EXT2_ERR_NONE;

done:
    return err;
}

ext2_err_t ext2_path_to_ino(
    const ext2_t* ext2,
    const char* path,
    ext2_ino_t* ino)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    char buf[EXT2_PATH_MAX];
    const char* elements[32];
    const uint8_t NELEMENTS = sizeof(elements) / sizeof(elements[0]);
    uint8_t nelements = 0;
    char* p;
    char* save;
    uint8_t i;
    ext2_ino_t current_ino = 0;

    /* Check for null parameters */
    if (!ext2 || !path || !ino)
    {
        err = EXT2_ERR_INVALID_PARAMETER;
        goto done;
    }

    /* Initialize to null inode for now */
    *ino = 0;

    /* Check path length */
    if (strlen(path) >= EXT2_PATH_MAX)
    {
        err = EXT2_ERR_PATH_TOO_LONG;
        goto done;
    }

    /* Copy path */
    strlcpy(buf, path, sizeof(buf));

    /* Be sure path begins with "/" */
    if (path[0] != '/')
    {
        err = EXT2_ERR_INVALID_PARAMETER;
        goto done;
    }

    elements[nelements++] = "/";

    /* Split the path into components */
    for (p = strtok_r(buf, "/", &save); p; p = strtok_r(NULL, "/", &save))
    {
        if (nelements == NELEMENTS)
        {
            err = EXT2_ERR_BUFFER_OVERFLOW;
            goto done;
        }

        elements[nelements++] = p;
    }

    /* Load each inode along the path until we find it */
    for (i = 0; i < nelements; i++)
    {
        if (strcmp(elements[i], "/") == 0)
        {
            current_ino = EXT2_ROOT_INO;
        }
        else
        {
            ext2_dir_ent_t* entries = NULL;
            uint32_t nentries = 0;
            uint32_t j;

            if (_if_err(
                    err = ext2_list_dir_inode(
                        ext2, current_ino, &entries, &nentries)))
            {
                goto done;
            }

            current_ino = 0;

            for (j = 0; j < nentries; j++)
            {
                const ext2_dir_ent_t* ent = &entries[j];

                if (i + 1 == nelements || ent->d_type == EXT2_DT_DIR)
                {
                    if (strcmp(ent->d_name, elements[i]) == 0)
                    {
                        current_ino = ent->d_ino;
                        break;
                    }
                }
            }

            if (entries)
                free(entries);

            if (!current_ino)
            {
                /* Not found case */
                err = EXT2_ERR_FILE_NOT_FOUND;
                goto done;
            }
        }
    }

    *ino = current_ino;

    err = EXT2_ERR_NONE;

done:
    return err;
}

ext2_err_t ext2_path_to_inode(
    const ext2_t* ext2,
    const char* path,
    ext2_ino_t* ino,
    ext2_inode_t* inode)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    ext2_ino_t tmp_ino;

    /* Check parameters */
    if (!ext2 || !path || !inode)
    {
        err = EXT2_ERR_INVALID_PARAMETER;
        goto done;
    }

    /* Find the ino for this path */
    if (_if_err(err = ext2_path_to_ino(ext2, path, &tmp_ino)))
    {
        /* Not found case */
        goto done;
    }

    /* Read the inode into memory */
    if (_if_err(err = ext2_read_inode(ext2, tmp_ino, inode)))
    {
        goto done;
    }

    if (ino)
        *ino = tmp_ino;

    err = EXT2_ERR_NONE;

done:
    return err;
}

/*
**==============================================================================
**
** bitmaps:
**
**==============================================================================
*/

ext2_err_t ext2_read_block_bitmap(
    const ext2_t* ext2,
    uint32_t group_index,
    ext2_block_t* block)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    uint32_t bitmap_size_bytes;

    if (!_is_valid(ext2) || !block)
    {
        err = EXT2_ERR_INVALID_PARAMETER;
        goto done;
    }

    memset(block, 0, sizeof(ext2_block_t));

    bitmap_size_bytes = ext2->sb.s_blocks_per_group / 8;

    if (group_index > ext2->group_count)
    {
        err = EXT2_ERR_BAD_GRPNO;
        goto done;
    }

    if (_if_err(
            err = ext2_read_block(
                ext2, ext2->groups[group_index].bg_block_bitmap, block)))
    {
        goto done;
    }

    if (block->size > bitmap_size_bytes)
        block->size = bitmap_size_bytes;

    err = EXT2_ERR_NONE;

done:
    return err;
}

ext2_err_t ext2_write_block_bitmap(
    const ext2_t* ext2,
    uint32_t group_index,
    const ext2_block_t* block)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    uint32_t bitmap_size_bytes;

    if (!_is_valid(ext2) || !block)
    {
        err = EXT2_ERR_INVALID_PARAMETER;
        goto done;
    }

    bitmap_size_bytes = ext2->sb.s_blocks_per_group / 8;

    if (block->size != bitmap_size_bytes)
    {
        err = EXT2_ERR_BAD_SIZE;
        goto done;
    }

    if (group_index > ext2->group_count)
    {
        err = EXT2_ERR_BAD_GRPNO;
        goto done;
    }

    if (_if_err(
            err = ext2_write_block(
                ext2, ext2->groups[group_index].bg_block_bitmap, block)))
    {
        goto done;
    }

    err = EXT2_ERR_NONE;

done:
    return err;
}

ext2_err_t ext2_read_read_inode_bitmap(
    const ext2_t* ext2,
    uint32_t group_index,
    ext2_block_t* block)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    uint32_t bitmap_size_bytes;

    if (!_is_valid(ext2) || !block)
    {
        err = EXT2_ERR_INVALID_PARAMETER;
        goto done;
    }

    memset(block, 0, sizeof(ext2_block_t));

    bitmap_size_bytes = ext2->sb.s_inodes_per_group / 8;

    if (group_index > ext2->group_count)
    {
        err = EXT2_ERR_BAD_GRPNO;
        goto done;
    }

    if (_if_err(
            err = ext2_read_block(
                ext2, ext2->groups[group_index].bg_inode_bitmap, block)))
    {
        goto done;
    }

    if (block->size > bitmap_size_bytes)
        block->size = bitmap_size_bytes;

    err = EXT2_ERR_NONE;

done:
    return err;
}

/*
**==============================================================================
**
** directory:
**
**==============================================================================
*/

static void _dump_directory_entry(const dir_ent_t* dirent)
{
    printf("=== dir_ent_t:\n");
    printf("inode=%u\n", dirent->inode);
    printf("rec_len=%u\n", dirent->rec_len);
    printf("name_len=%u\n", dirent->name_len);
    printf("file_type=%u\n", dirent->file_type);
    printf("name={%.*s}\n", dirent->name_len, dirent->name);
}

static ext2_err_t _count_directory_entries(
    const ext2_t* ext2,
    const void* data,
    uint32_t size,
    uint32_t* count)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    const uint8_t* p = (uint8_t*)data;
    const uint8_t* end = (uint8_t*)data + size;

    /* Check parameters */
    if (!_is_valid(ext2) || !data || !size || !count)
    {
        err = EXT2_ERR_INVALID_PARAMETER;
        goto done;
    }

    /* Initilize the count */
    *count = 0;

    /* Must be divisiable by block size */
    if ((end - p) % ext2->block_size)
    {
        err = EXT2_ERR_BAD_MULTIPLE;
        goto done;
    }

    while (p < end)
    {
        const dir_ent_t* ent = (const dir_ent_t*)p;

        if (ent->name_len)
        {
            (*count)++;
        }

        p += ent->rec_len;
    }

    if (p != end)
    {
        err = EXT2_ERR_EXTRANEOUS_DATA;
        goto done;
    }

    err = EXT2_ERR_NONE;

done:
    return err;
}

struct _ext2_dir
{
    void* data;
    uint32_t size;
    const void* next;
    ext2_dir_ent_t ent;
};

ext2_dir_t* ext2_open_dir(const ext2_t* ext2, const char* path)
{
    ext2_dir_t* dir = NULL;
    ext2_ino_t ino;

    /* Check parameters */
    if (!ext2 || !path)
    {
        goto done;
    }

    /* Find inode number for this directory */
    if (_if_err(ext2_path_to_ino(ext2, path, &ino)))
        goto done;

    /* Open directory from this inode */
    if (!(dir = ext2_open_dir_ino(ext2, ino)))
        goto done;

done:
    return dir;
}

ext2_dir_t* ext2_open_dir_ino(const ext2_t* ext2, ext2_ino_t ino)
{
    ext2_dir_t* dir = NULL;
    ext2_inode_t inode;

    /* Check parameters */
    if (!ext2 || !ino)
    {
        goto done;
    }

    /* Read the inode into memory */
    if (_if_err(ext2_read_inode(ext2, ino, &inode)))
    {
        goto done;
    }

    /* Fail if not a directory */
    if (!(inode.i_mode & EXT2_S_IFDIR))
    {
        goto done;
    }

    /* Allocate directory object */
    if (!(dir = (ext2_dir_t*)calloc(1, sizeof(ext2_dir_t))))
    {
        goto done;
    }

    /* Load the blocks for this inode into memory */
    if (_if_err(
            ext2_load_file_from_inode(ext2, &inode, &dir->data, &dir->size)))
    {
        free(dir);
        dir = NULL;
        goto done;
    }

    /* Set pointer to current directory */
    dir->next = dir->data;

done:
    return dir;
}

ext2_dir_ent_t* ext2_read_dir(ext2_dir_t* dir)
{
    ext2_dir_ent_t* ent = NULL;

    if (!dir || !dir->data || !dir->next)
        goto done;

    /* Find the next entry (possibly skipping padding entries) */
    {
        const void* end = (void*)((char*)dir->data + dir->size);

        while (!ent && dir->next < end)
        {
            const dir_ent_t* de = (dir_ent_t*)dir->next;

            if (de->rec_len == 0)
                break;

            if (de->name_len > 0)
            {
                /* Found! */

                /* Set ext2_dir_ent_t.d_ino */
                dir->ent.d_ino = de->inode;

                /* Set ext2_dir_ent_t.d_off (not used) */
                dir->ent.d_off = 0;

                /* Set ext2_dir_ent_t.d_reclen (not used) */
                dir->ent.d_reclen = sizeof(ext2_dir_ent_t);

                /* Set ext2_dir_ent_t.type */
                switch (de->file_type)
                {
                    case EXT2_FT_UNKNOWN:
                        dir->ent.d_type = EXT2_DT_UNKNOWN;
                        break;
                    case EXT2_FT_REG_FILE:
                        dir->ent.d_type = EXT2_DT_REG;
                        break;
                    case EXT2_FT_DIR:
                        dir->ent.d_type = EXT2_DT_DIR;
                        break;
                    case EXT2_FT_CHRDEV:
                        dir->ent.d_type = EXT2_DT_CHR;
                        break;
                    case EXT2_FT_BLKDEV:
                        dir->ent.d_type = EXT2_DT_BLK;
                        break;
                    case EXT2_FT_FIFO:
                        dir->ent.d_type = EXT2_DT_FIFO;
                        break;
                    case EXT2_FT_SOCK:
                        dir->ent.d_type = EXT2_DT_SOCK;
                        break;
                    case EXT2_FT_SYMLINK:
                        dir->ent.d_type = EXT2_DT_LNK;
                        break;
                    default:
                        dir->ent.d_type = EXT2_DT_UNKNOWN;
                        break;
                }

                /* Set ext2_dir_ent_t.d_name */
                dir->ent.d_name[0] = '\0';

                _strncat(
                    dir->ent.d_name,
                    sizeof(dir->ent.d_name),
                    de->name,
                    _min(EXT2_PATH_MAX - 1, de->name_len));

                /* Success! */
                ent = &dir->ent;
            }

            /* Position to the next entry (for next call to readdir) */
            dir->next = (void*)((char*)dir->next + de->rec_len);
        }
    }

done:
    return ent;
}

ext2_err_t ext2_close_dir(ext2_dir_t* dir)
{
    ext2_err_t err = EXT2_ERR_FAILED;

    if (!dir)
    {
        goto done;
    }

    free(dir->data);
    free(dir);

    err = EXT2_ERR_NONE;

done:
    return err;
}

ext2_err_t ext2_list_dir_inode(
    const ext2_t* ext2,
    ext2_ino_t ino,
    ext2_dir_ent_t** entries,
    uint32_t* nentries)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    ext2_dir_t* dir = NULL;
    ext2_dir_ent_t* ent;
    buf_t buf = BUF_INITIALIZER;

    /* Check parameters */
    if (!ext2 || !ino || !entries || !nentries)
    {
        err = EXT2_ERR_INVALID_PARAMETER;
        goto done;
    }

    *entries = NULL;
    *nentries = 0;

    /* Open the directory */
    if (!(dir = ext2_open_dir_ino(ext2, ino)))
    {
        goto done;
    }

    /* Add entries to array */
    while ((ent = ext2_read_dir(dir)))
    {
        /* Append to buffer */
        if (_if_err(err = buf_append(&buf, ent, sizeof(ext2_dir_ent_t))))
        {
            goto done;
        }
    }

    (*entries) = (ext2_dir_ent_t*)buf.data;
    (*nentries) = buf.size / sizeof(ext2_dir_ent_t);

    err = EXT2_ERR_NONE;

done:

    if (_if_err(err))
        buf_release(&buf);

    /* Close the directory */
    if (dir)
        ext2_close_dir(dir);

    return err;
}

/*
**==============================================================================
**
** files:
**
**==============================================================================
*/

static ext2_err_t _split_full_path(
    const char* path,
    char dirname[EXT2_PATH_MAX],
    char basename[EXT2_PATH_MAX])
{
    char* slash;

    /* Reject paths that are not absolute */
    if (path[0] != '/')
        return EXT2_ERR_FAILED;

    /* Handle root directory up front */
    if (strcmp(path, "/") == 0)
    {
        strlcpy(dirname, "/", EXT2_PATH_MAX);
        strlcpy(basename, "/", EXT2_PATH_MAX);
        return EXT2_ERR_NONE;
    }

    /* This cannot fail (prechecked) */
    if (!(slash = strrchr(path, '/')))
        return EXT2_ERR_FAILED;

    /* If path ends with '/' character */
    if (!slash[1])
        return EXT2_ERR_FAILED;

    /* Split the path */
    {
        if (slash == path)
        {
            strlcpy(dirname, "/", EXT2_PATH_MAX);
        }
        else
        {
            uint32_t index = slash - path;
            strlcpy(dirname, path, EXT2_PATH_MAX);

            if (index < EXT2_PATH_MAX)
                dirname[index] = '\0';
            else
                dirname[EXT2_PATH_MAX - 1] = '\0';
        }

        strlcpy(basename, slash + 1, EXT2_PATH_MAX);
    }

    return EXT2_ERR_NONE;
}

ext2_err_t ext2_load_file_from_inode(
    const ext2_t* ext2,
    const ext2_inode_t* inode,
    void** data,
    uint32_t* size)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    buf_u32_t blknos = BUF_U32_INITIALIZER;
    buf_t buf = BUF_INITIALIZER;
    uint32_t i;

    /* Check parameters */
    if (!_is_valid(ext2) || !inode || !data || !size)
    {
        err = EXT2_ERR_INVALID_PARAMETER;
        goto done;
    }

    /* Initialize the output */
    *data = NULL;
    *size = 0;

    /* Form a list of block-numbers for this file */
    if (_if_err(
            err = _load_block_numbers_from_inode(
                ext2,
                inode,
                0, /* include_block_blocks */
                &blknos)))
    {
        goto done;
    }

    /* Read and append each block */
    for (i = 0; i < blknos.size;)
    {
        uint32_t nblks = 1;
        uint32_t j;
        ext2_block_t block;

        /* Count the number of consecutive blocks: nblks */
        for (j = i + 1; j < blknos.size; j++)
        {
            if (blknos.data[j] != blknos.data[j - 1] + 1)
                break;

            nblks++;
        }

        if (nblks == 1)
        {
            /* Read the next block */
            if (_if_err(err = ext2_read_block(ext2, blknos.data[i], &block)))
            {
                goto done;
            }

            /* Append block to end of buffer */
            if (_if_err(err = buf_append(&buf, block.data, block.size)))
                goto done;
        }
        else
        {
            if (_if_err(_read_blocks(ext2, blknos.data[i], nblks, &buf)))
            {
                goto done;
            }
        }

        i += nblks;
    }

    *data = buf.data;
    *size = inode->i_size; /* data may be smaller than block multiple */

    err = EXT2_ERR_NONE;

done:

    if (_if_err(err))
        buf_release(&buf);

    buf_u32_release(&blknos);

    return err;
}

ext2_err_t ext2_load_file_from_path(
    const ext2_t* ext2,
    const char* path,
    void** data,
    uint32_t* size)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    ext2_inode_t inode;

    if (ext2_path_to_inode(ext2, path, NULL, &inode) != EXT2_ERR_NONE)
        goto done;

    if (ext2_load_file_from_inode(ext2, &inode, data, size) != EXT2_ERR_NONE)
        goto done;

    err = EXT2_ERR_NONE;

done:
    return err;
}

/*
**==============================================================================
**
** ext2_t
**
**==============================================================================
*/

ext2_err_t ext2_new(oe_block_device_t* dev, ext2_t** ext2_out)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    ext2_t* ext2 = NULL;

    /* Check parameters */
    if (!dev || !ext2_out)
    {
        err = EXT2_ERR_INVALID_PARAMETER;
        goto done;
    }

    /* Initialize output parameters */
    *ext2_out = NULL;

    /* Bit endian is not supported */
    if (_is_big_endian())
    {
        err = EXT2_ERR_UNSUPPORTED;
        goto done;
    }

    /* Allocate the file system object */
    if (!(ext2 = (ext2_t*)calloc(1, sizeof(ext2_t))))
    {
        err = EXT2_ERR_OUT_OF_MEMORY;
        goto done;
    }

    /* Set the file object */
    ext2->dev = dev;

    /* Read the superblock */
    if (_if_err(err = _read_super_block(ext2->dev, &ext2->sb)))
    {
        err = EXT2_ERR_FAILED_TO_READ_SUPERBLOCK;
        goto done;
    }

    /* Check the superblock magic number */
    if (ext2->sb.s_magic != EXT2_S_MAGIC)
    {
        err = EXT2_ERR_BAD_MAGIC;
        goto done;
    }

    /* Reject revision 0 file systems */
    if (ext2->sb.s_rev_level == EXT2_GOOD_OLD_REV)
    {
        /* Revision 0 not supported */
        err = EXT2_ERR_UNSUPPORTED_REVISION;
        goto done;
    }

    /* Accept revision 1 file systems */
    if (ext2->sb.s_rev_level < EXT2_DYNAMIC_REV)
    {
        /* Revision 1 and up supported */
        err = EXT2_ERR_UNSUPPORTED_REVISION;
        goto done;
    }

    /* Check inode size */
    if (ext2->sb.s_inode_size > sizeof(ext2_inode_t))
    {
        err = EXT2_ERR_UNEXPECTED;
        goto done;
    }

    /* Calcualte the block size in bytes */
    ext2->block_size = 1024 << ext2->sb.s_log_block_size;

    /* Calculate the number of block groups */
    ext2->group_count =
        1 + (ext2->sb.s_blocks_count - 1) / ext2->sb.s_blocks_per_group;

    /* Get the groups list */
    if (!(ext2->groups = _read_groups(ext2)))
    {
        err = EXT2_ERR_FAILED_TO_READ_GROUPS;
        goto done;
    }

    /* Read the root inode */
    if (_if_err(err = ext2_read_inode(ext2, EXT2_ROOT_INO, &ext2->root_inode)))
    {
        err = EXT2_ERR_FAILED_TO_READ_INODE;
        goto done;
    }

    err = EXT2_ERR_NONE;

done:

    if (_if_err(err))
    {
        /* Caller must dispose of this! */
        ext2->dev = NULL;
        ext2_delete(ext2);
    }
    else
        *ext2_out = ext2;

    return err;
}

void ext2_delete(ext2_t* ext2)
{
    if (ext2)
    {
        if (ext2->dev)
            ext2->dev->close(ext2->dev);

        if (ext2->groups)
            free(ext2->groups);

        free(ext2);
    }
}

ext2_err_t ext2_dump(const ext2_t* ext2)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    uint32_t grpno;

    /* Print the superblock */
    ext2_dump_super_block(&ext2->sb);

    printf("block_size=%u\n", ext2->block_size);
    printf("group_count=%u\n", ext2->group_count);

    /* Print the groups */
    _dump_group_descs(ext2->groups, ext2->group_count);

    /* Print out the bitmaps for the data blocks */
    {
        for (grpno = 0; grpno < ext2->group_count; grpno++)
        {
            ext2_block_t bitmap;

            if (_if_err(err = ext2_read_block_bitmap(ext2, grpno, &bitmap)))
            {
                goto done;
            }

            printf("=== block bitmap:\n");
            _dump_bitmap(&bitmap);
        }
    }

    /* Print the inode bitmaps */
    for (grpno = 0; grpno < ext2->group_count; grpno++)
    {
        ext2_block_t bitmap;

        if (_if_err(err = ext2_read_read_inode_bitmap(ext2, grpno, &bitmap)))
        {
            goto done;
        }

        printf("=== inode bitmap:\n");
        _dump_bitmap(&bitmap);
    }

    /* dump the inodes */
    {
        uint32_t nbits = 0;
        uint32_t mbits = 0;

        /* Print the inode tables */
        for (grpno = 0; grpno < ext2->group_count; grpno++)
        {
            ext2_block_t bitmap;
            uint32_t lino;

            /* Get inode bitmap for this group */
            if (_if_err(
                    err = ext2_read_read_inode_bitmap(ext2, grpno, &bitmap)))
            {
                goto done;
            }

            nbits += _count_bits_n(bitmap.data, bitmap.size);

            /* For each bit set in the bit map */
            for (lino = 0; lino < ext2->sb.s_inodes_per_group; lino++)
            {
                ext2_inode_t inode;
                ext2_ino_t ino;

                if (!_test_bit(bitmap.data, bitmap.size, lino))
                    continue;

                mbits++;

                if ((lino + 1) < EXT2_FIRST_INO && (lino + 1) != EXT2_ROOT_INO)
                    continue;

                ino = _make_ino(ext2, grpno, lino);

                if (_if_err(err = ext2_read_inode(ext2, ino, &inode)))
                {
                    goto done;
                }

                printf("INODE{%u}\n", ino);
                ext2_dump_inode(ext2, &inode);
            }
        }

        printf("nbits{%u}\n", nbits);
        printf("mbits{%u}\n", mbits);
    }

    /* dump the root inode */
    ext2_dump_inode(ext2, &ext2->root_inode);

    err = EXT2_ERR_NONE;

done:
    return err;
}

ext2_err_t ext2_check(const ext2_t* ext2)
{
    ext2_err_t err = EXT2_ERR_FAILED;

    /* Check the block bitmaps */
    {
        uint32_t i;
        uint32_t n = 0;
        uint32_t nused = 0;
        uint32_t nfree = 0;

        for (i = 0; i < ext2->group_count; i++)
        {
            ext2_block_t bitmap;

            nfree += ext2->groups[i].bg_free_blocks_count;

            if (_if_err(err = ext2_read_block_bitmap(ext2, i, &bitmap)))
            {
                goto done;
            }

            nused += _count_bits_n(bitmap.data, bitmap.size);
            n += bitmap.size * 8;
        }

        if (ext2->sb.s_free_blocks_count != nfree)
        {
            printf(
                "s_free_blocks_count{%u}, nfree{%u}\n",
                ext2->sb.s_free_blocks_count,
                nfree);
            goto done;
        }

        if (ext2->sb.s_free_blocks_count != n - nused)
        {
            goto done;
        }
    }

    /* Check the inode bitmaps */
    {
        uint32_t i;
        uint32_t n = 0;
        uint32_t nused = 0;
        uint32_t nfree = 0;

        /* Check the bitmaps for the inodes */
        for (i = 0; i < ext2->group_count; i++)
        {
            ext2_block_t bitmap;

            nfree += ext2->groups[i].bg_free_inodes_count;

            if (_if_err(err = ext2_read_read_inode_bitmap(ext2, i, &bitmap)))
            {
                goto done;
            }

            nused += _count_bits_n(bitmap.data, bitmap.size);
            n += bitmap.size * 8;
        }

        if (ext2->sb.s_free_inodes_count != n - nused)
        {
            goto done;
        }

        if (ext2->sb.s_free_inodes_count != nfree)
        {
            goto done;
        }
    }

    /* Check the inodes */
    {
        uint32_t grpno;
        uint32_t nbits = 0;
        uint32_t mbits = 0;

        /* Check the inode tables */
        for (grpno = 0; grpno < ext2->group_count; grpno++)
        {
            ext2_block_t bitmap;
            uint32_t lino;

            /* Get inode bitmap for this group */
            if (_if_err(
                    err = ext2_read_read_inode_bitmap(ext2, grpno, &bitmap)))
            {
                goto done;
            }

            nbits += _count_bits_n(bitmap.data, bitmap.size);

            /* For each bit set in the bit map */
            for (lino = 0; lino < ext2->sb.s_inodes_per_group; lino++)
            {
                ext2_inode_t inode;
                ext2_ino_t ino;

                if (!_test_bit(bitmap.data, bitmap.size, lino))
                    continue;

                mbits++;

                if ((lino + 1) < EXT2_FIRST_INO && (lino + 1) != EXT2_ROOT_INO)
                    continue;

                ino = _make_ino(ext2, grpno, lino);

                if (_if_err(err = ext2_read_inode(ext2, ino, &inode)))
                {
                    goto done;
                }

                /* Mode can never be zero */
                if (inode.i_mode == 0)
                {
                    goto done;
                }

                /* If file is not zero size but no blocks, then fail */
                if (inode.i_size && !inode.i_block[0])
                {
                    goto done;
                }
            }
        }

        /* The number of bits in bitmap must match number of active inodes */
        if (nbits != mbits)
        {
            goto done;
        }
    }

    err = EXT2_ERR_NONE;

done:
    return err;
}

ext2_err_t ext2_trunc(ext2_t* ext2, const char* path)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    char dirname[EXT2_PATH_MAX];
    char basename[EXT2_PATH_MAX];
    ext2_ino_t dir_ino;
    ext2_inode_t dir_inode;
    ext2_ino_t file_ino;
    ext2_inode_t file_inode;
    buf_u32_t blknos = BUF_U32_INITIALIZER;

    /* Check parameters */
    if (!ext2 || !path)
    {
        err = EXT2_ERR_INVALID_PARAMETER;
        goto done;
    }

    /* Split the path */
    if (_if_err(err = _split_full_path(path, dirname, basename)))
    {
        goto done;
    }

    /* Find the inode of the dirname */
    if (_if_err(err = ext2_path_to_inode(ext2, dirname, &dir_ino, &dir_inode)))
    {
        goto done;
    }

    /* Find the inode of the basename */
    if (_if_err(err = ext2_path_to_inode(ext2, path, &file_ino, &file_inode)))
    {
        goto done;
    }

    /* Form a list of block-numbers for this file */
    if (_if_err(
            err = _load_block_numbers_from_inode(
                ext2,
                &file_inode,
                1, /* include_block_blocks */
                &blknos)))
    {
        goto done;
    }

    /* If this file is a directory, then fail */
    if (file_inode.i_mode & EXT2_S_IFDIR)
    {
        goto done;
    }

    /* Return the blocks to the free list */
    {
        if (_if_err(err = _put_blocks(ext2, blknos.data, blknos.size)))
            goto done;
    }

    /* Rewrite the inode */
    {
        file_inode.i_size = 0;
        memset(file_inode.i_block, 0, sizeof(file_inode.i_block));

        if (_if_err(err = _write_inode(ext2, file_ino, &file_inode)))
        {
            goto done;
        }
    }

    /* Update the super block */
    if (_if_err(err = _write_super_block(ext2)))
    {
        goto done;
    }

    err = EXT2_ERR_NONE;

done:

    buf_u32_release(&blknos);

    return err;
}

static ext2_err_t _write_single_direct_block_numbers(
    ext2_t* ext2,
    const uint32_t* blknos,
    uint32_t nblknos,
    uint32_t* blkno)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    ext2_block_t block;

    /* Check parameters */
    if (!_is_valid(ext2) || !blknos || !nblknos)
    {
        err = EXT2_ERR_INVALID_PARAMETER;
        goto done;
    }

    /* If no room in a single block for the block numbers */
    if (nblknos > ext2->block_size / sizeof(uint32_t))
    {
        err = EXT2_ERR_BUFFER_OVERFLOW;
        goto done;
    }

    /* Assign an available block */
    if (_if_err(err = _get_block(ext2, blkno)))
    {
        goto done;
    }

    /* Copy block numbers into block */
    memset(&block, 0, sizeof(ext2_block_t));
    memcpy(block.data, blknos, nblknos * sizeof(uint32_t));
    block.size = ext2->block_size;

    /* Write the block */
    if (_if_err(err = ext2_write_block(ext2, *blkno, &block)))
    {
        goto done;
    }

    err = EXT2_ERR_NONE;

done:
    return err;
}

static ext2_err_t _write_indirect_block_numbers(
    ext2_t* ext2,
    uint32_t indirection, /* level of indirection: 2=double, 3=triple */
    const uint32_t* blknos,
    uint32_t nblknos,
    uint32_t* blkno)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    ext2_block_t block;
    uint32_t blknos_per_block = ext2->block_size / sizeof(uint32_t);

    /* Check parameters */
    if (!_is_valid(ext2) || !blknos || !nblknos || !blkno)
    {
        err = EXT2_ERR_INVALID_PARAMETER;
        goto done;
    }

    /* Check indirection level */
    if (indirection != 2 && indirection != 3)
    {
        err = EXT2_ERR_UNEXPECTED;
        goto done;
    }

    /* If double indirection */
    if (indirection == 2)
    {
        /* If too many indirect blocks */
        if (nblknos > blknos_per_block * blknos_per_block)
        {
            err = EXT2_ERR_BUFFER_OVERFLOW;
            goto done;
        }
    }

    /* Assign an available block */
    if (_if_err(err = _get_block(ext2, blkno)))
    {
        goto done;
    }

    /* Allocate indirect block (to hold block numbers) */
    memset(&block, 0, sizeof(ext2_block_t));
    block.size = ext2->block_size;

    /* Write each of the indirect blocks */
    {
        const uint32_t* p = blknos;
        uint32_t r = nblknos;
        uint32_t i;

        /* For each block */
        for (i = 0; r > 0; i++)
        {
            uint32_t n;

            /* If double indirection */
            if (indirection == 2)
            {
                n = _min(r, blknos_per_block);

                if (_if_err(
                        err = _write_single_direct_block_numbers(
                            ext2, p, n, (uint32_t*)block.data + i)))
                {
                    goto done;
                }
            }
            else
            {
                n = _min(r, blknos_per_block * blknos_per_block);

                /* Write the block numbers for this block */
                if (_if_err(
                        err = _write_indirect_block_numbers(
                            ext2,
                            2, /* double indirection */
                            p,
                            n,
                            (uint32_t*)block.data + i)))
                {
                    goto done;
                }
            }

            p += n;
            r -= n;
        }

        /* Check that the blocks were exhausted */
        if (r != 0)
        {
            err = EXT2_ERR_EXTRANEOUS_DATA;
            goto done;
        }
    }

    /* Write the indirect block */
    if (_if_err(err = ext2_write_block(ext2, *blkno, &block)))
    {
        goto done;
    }

    err = EXT2_ERR_NONE;

done:
    return err;
}

static ext2_err_t _write_data(
    ext2_t* ext2,
    const void* data,
    uint32_t size,
    buf_u32_t* blknos)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    uint32_t blksize = ext2->block_size;
    uint32_t nblks = (size + blksize - 1) / blksize;
    uint32_t rem = size % blksize;
    uint32_t i = 0;
    uint32_t grpno;

    for (grpno = 0; grpno < ext2->group_count && i < nblks; grpno++)
    {
        ext2_block_t bitmap;
        ext2_block_t block;
        uint32_t blkno;
        uint32_t lblkno;
        int changed = 0;

        /* Read the bitmap */
        if (_if_err(err = ext2_read_block_bitmap(ext2, grpno, &bitmap)))
        {
            goto done;
        }

        /* Scan the bitmap, looking for free bit */
        for (lblkno = 0; lblkno < bitmap.size * 8 && i < nblks; lblkno++)
        {
            if (!_test_bit(bitmap.data, bitmap.size, lblkno))
            {
                _set_bit(bitmap.data, bitmap.size, lblkno);
                blkno = MakeBlkno(ext2, grpno, lblkno);

                /* Append this to the array of blocks */
                if (_if_err(err = buf_u32_append(blknos, &blkno, 1)))
                    goto done;

                /* If this is the final block */
                if (i + 1 == nblks && rem)
                {
                    memcpy(block.data, (char*)data + (i * blksize), rem);
                    block.size = rem;
                }
                else
                {
                    memcpy(block.data, (char*)data + (i * blksize), blksize);
                    block.size = blksize;
                }

                /* Write the block */
                if (_if_err(err = ext2_write_block(ext2, blkno, &block)))
                {
                    goto done;
                }

                /* Update the in memory structs. */
                ext2->sb.s_free_blocks_count--;
                ext2->groups[grpno].bg_free_blocks_count--;
                i++;
                changed = 1;
            }
        }

        if (changed == 1)
        {
            /* Write group and bitmap. */
            if (_if_err(err = _write_group(ext2, grpno)))
            {
                goto done;
            }

            if (_if_err(err = ext2_write_block_bitmap(ext2, grpno, &bitmap)))
            {
                goto done;
            }
        }
    }

    /* Ran out of disk space. */
    if (i != nblks)
    {
        goto done;
    }

    /* Write super block data. */
    if (_if_err(err = _write_super_block(ext2)))
    {
        goto done;
    }

    err = EXT2_ERR_NONE;

done:
    return err;
}

static ext2_err_t _update_inode_block_pointers(
    ext2_t* ext2,
    ext2_ino_t ino,
    ext2_inode_t* inode,
    uint32_t size,
    const uint32_t* blknos,
    uint32_t nblknos)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    uint32_t i;
    const uint32_t* p = blknos;
    uint32_t r = nblknos;
    uint32_t blknos_per_block = ext2->block_size / sizeof(uint32_t);

    /* Update the inode size */
    inode->i_size = size;

    /* Update the direct inode blocks */
    if (r)
    {
        uint32_t n = _min(r, EXT2_SINGLE_INDIRECT_BLOCK);

        for (i = 0; i < n; i++)
        {
            inode->i_block[i] = p[i];
        }

        p += n;
        r -= n;
    }

    /* Write the indirect block numbers */
    if (r)
    {
        uint32_t n = _min(r, blknos_per_block);

        if (_if_err(
                err = _write_single_direct_block_numbers(
                    ext2, p, n, &inode->i_block[EXT2_SINGLE_INDIRECT_BLOCK])))
        {
            goto done;
        }

        p += n;
        r -= n;
    }

    /* Write the double indirect block numbers */
    if (r)
    {
        uint32_t n = _min(r, blknos_per_block * blknos_per_block);

        if (_if_err(
                err = _write_indirect_block_numbers(
                    ext2,
                    2, /* double indirection */
                    p,
                    n,
                    &inode->i_block[EXT2_DOUBLE_INDIRECT_BLOCK])))
        {
            goto done;
        }

        p += n;
        r -= n;
    }

    /* Write the triple indirect block numbers */
    if (r)
    {
        uint32_t n = r;

        if (_if_err(
                err = _write_indirect_block_numbers(
                    ext2,
                    3, /* triple indirection */
                    p,
                    n,
                    &inode->i_block[EXT2_TRIPLE_INDIRECT_BLOCK])))
        {
            goto done;
        }

        p += n;
        r -= n;
    }

    /* Note: triple indirect block numbers not handled! */
    if (r > 0)
    {
        goto done;
    }

    /* Rewrite the inode */
    if (_if_err(err = _write_inode(ext2, ino, inode)))
    {
        goto done;
    }

    err = EXT2_ERR_NONE;

done:
    return err;
}

static ext2_err_t _update_inode_data_blocks(
    ext2_t* ext2,
    ext2_ino_t ino,
    ext2_inode_t* inode,
    const void* data,
    uint32_t size,
    bool is_dir)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    buf_u32_t blknos = BUF_U32_INITIALIZER;
    uint32_t count = 0;
    void* tmp_data = NULL;

    if (_if_err(err = _write_data(ext2, data, size, &blknos)))
        goto done;

    if (is_dir)
    {
        if (_if_err(err = _count_directory_entries(ext2, data, size, &count)))
        {
            goto done;
        }
    }

    inode->i_osd1 = count + 1;

    if (_if_err(
            err = _update_inode_block_pointers(
                ext2, ino, inode, size, blknos.data, blknos.size)))
    {
        goto done;
    }

    err = EXT2_ERR_NONE;

done:

    buf_u32_release(&blknos);

    if (tmp_data)
        free(tmp_data);

    return err;
}

ext2_err_t ext2_update(
    ext2_t* ext2,
    const void* data,
    uint32_t size,
    const char* path)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    ext2_ino_t ino;
    ext2_inode_t inode;

    /* Check parameters */
    if (!_is_valid(ext2) || !data || !path)
    {
        err = EXT2_ERR_INVALID_PARAMETER;
        goto done;
    }

    /* Truncate the destination file */
    if (_if_err(err = ext2_trunc(ext2, path)))
    {
        goto done;
    }

    /* Read inode for this file */
    if (_if_err(err = ext2_path_to_inode(ext2, path, &ino, &inode)))
    {
        goto done;
    }

    /* Update the inode blocks */
    if (_if_err(
            err = _update_inode_data_blocks(
                ext2, ino, &inode, data, size, 0))) /* !is_dir */
    {
        goto done;
    }

    err = EXT2_ERR_NONE;

done:

    return err;
}

static ext2_err_t _check_directory_entries(
    const ext2_t* ext2,
    const void* data,
    uint32_t size)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    const uint8_t* p = (uint8_t*)data;
    const uint8_t* end = (uint8_t*)data + size;

    /* Must be divisiable by block size */
    if ((end - p) % ext2->block_size)
    {
        goto done;
    }

    while (p < end)
    {
        uint32_t n;
        const dir_ent_t* ent = (const dir_ent_t*)p;

        n = sizeof(dir_ent_t) - EXT2_PATH_MAX + ent->name_len;
        n = _next_mult(n, 4);

        if (n != ent->rec_len)
        {
            uint32_t offset = ((char*)p - (char*)data) % ext2->block_size;
            uint32_t rem = ext2->block_size - offset;

            if (rem != ent->rec_len)
            {
                goto done;
            }
        }

        p += ent->rec_len;
    }

    if (p != end)
    {
        goto done;
    }

    err = EXT2_ERR_NONE;

done:
    return err;
}

static void _dump_directory_entries(
    const ext2_t* ext2,
    const void* data,
    uint32_t size)
{
    const uint8_t* p = (uint8_t*)data;
    const uint8_t* end = (uint8_t*)data + size;

    while (p < end)
    {
        uint32_t n;
        const dir_ent_t* ent = (const dir_ent_t*)p;

        _dump_directory_entry(ent);

        n = sizeof(dir_ent_t) - EXT2_PATH_MAX + ent->name_len;
        n = _next_mult(n, 4);

        if (n != ent->rec_len)
        {
            uint32_t gap = ent->rec_len - n;
            uint32_t offset = ((char*)p - (char*)data) % ext2->block_size;
            uint32_t rem = ext2->block_size - offset;

            printf("gap: %u\n", gap);
            printf("offset: %u\n", offset);
            printf("remaing: %u\n", rem);
        }

        p += ent->rec_len;
    }
}

static const dir_ent_t* _find_directory_entry(
    const char* name,
    const void* data,
    uint32_t size)
{
    const uint8_t* p = (uint8_t*)data;
    const uint8_t* end = (uint8_t*)data + size;

    while (p < end)
    {
        const dir_ent_t* ent = (const dir_ent_t*)p;
        char tmp[EXT2_PATH_MAX];

        /* Create zero-terminated name */
        tmp[0] = '\0';
        _strncat(
            tmp,
            sizeof(tmp),
            ent->name,
            _min(ent->name_len, EXT2_PATH_MAX - 1));

        if (strcmp(tmp, name) == 0)
            return ent;

        p += ent->rec_len;
    }

    /* Not found */
    return NULL;
}

static ext2_err_t _count_directory_entries_ino(
    ext2_t* ext2,
    ext2_ino_t ino,
    uint32_t* count)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    ext2_dir_t* dir = NULL;
    ext2_dir_ent_t* ent;

    /* Check parameters */
    if (!_is_valid(ext2) || !ino || !count)
    {
        err = EXT2_ERR_INVALID_PARAMETER;
        goto done;
    }

    /* Initialize count */
    *count = 0;

    /* Open directory */
    if (!(dir = ext2_open_dir_ino(ext2, ino)))
    {
        goto done;
    }

    /* Count the entries (notes that "." and ".." will always be present). */
    while ((ent = ext2_read_dir(dir)))
    {
        (*count)++;
    }

    err = EXT2_ERR_NONE;

done:

    if (dir)
        ext2_close_dir(dir);

    return err;
}

ext2_err_t ext2_rm(ext2_t* ext2, const char* path)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    char dirname[EXT2_PATH_MAX];
    char filename[EXT2_PATH_MAX];
    void* blocks = NULL;
    uint32_t blocks_size = 0;
    void* new_blocks = NULL;
    uint32_t new_blocks_size = 0;
    buf_u32_t blknos = BUF_U32_INITIALIZER;
    ext2_ino_t ino;
    ext2_inode_t inode;
    const dir_ent_t* ent = NULL;

    /* Check parameters */
    if (!_is_valid(ext2) && !path)
    {
        err = EXT2_ERR_INVALID_PARAMETER;
        goto done;
    }

    /* Truncate the file first */
    if (_if_err(err = ext2_trunc(ext2, path)))
    {
        goto done;
    }

    /* Split the path */
    if (_if_err(err = _split_full_path(path, dirname, filename)))
    {
        goto done;
    }

    /* Load the directory inode */
    if (_if_err(err = ext2_path_to_inode(ext2, dirname, &ino, &inode)))
    {
        goto done;
    }

    /* Load the directory file */
    if (_if_err(
            err =
                ext2_load_file_from_inode(ext2, &inode, &blocks, &blocks_size)))
    {
        goto done;
    }

    /* Load the block numbers (including the block blocks) */
    if (_if_err(
            err = _load_block_numbers_from_inode(
                ext2,
                &inode,
                1, /* include_block_blocks */
                &blknos)))
    {
        goto done;
    }

    /* Find 'filename' within this directory */
    if (!(ent = _find_directory_entry(filename, blocks, blocks_size)))
    {
        goto done;
    }

    /* Allow removal of empty directories only */
    if (ent->file_type == EXT2_FT_DIR)
    {
        ext2_ino_t dir_ino;
        ext2_inode_t dir_inode;
        uint32_t count;

        /* Find the inode of the filename */
        if (_if_err(
                err = ext2_path_to_inode(ext2, filename, &dir_ino, &dir_inode)))
        {
            goto done;
        }

        /* Disallow removal if directory is non empty */
        if (_if_err(err = _count_directory_entries_ino(ext2, dir_ino, &count)))
        {
            goto done;
        }

        /* Expect just "." and ".." entries */
        if (count != 2)
        {
            goto done;
        }
    }

    /* Convert from 'indexed' to 'linked list' directory format */
    {
        new_blocks_size = blocks_size;
        const char* src = (const char*)blocks;
        const char* src_end = (const char*)blocks + inode.i_size;
        char* dest = NULL;

        /* Allocate a buffer to hold 'linked list' directory */
        if (!(new_blocks = calloc(new_blocks_size, 1)))
        {
            goto done;
        }

        /* Set current and end pointers to new buffer */
        dest = (char*)new_blocks;

        /* Copy over directory entries (skipping removed entry) */
        {
            dir_ent_t* prev = NULL;

            while (src < src_end)
            {
                const dir_ent_t* curr_ent = (const dir_ent_t*)src;
                uint32_t rec_len;
                uint32_t offset;

                /* Skip the removed directory entry */
                if (curr_ent == ent || ent->name[0] == '\0')
                {
                    src += curr_ent->rec_len;
                    continue;
                }

                /* Compute size of the new directory entry */
                rec_len =
                    sizeof(*curr_ent) - EXT2_PATH_MAX + curr_ent->name_len;
                rec_len = _next_mult(rec_len, 4);

                /* Compute byte offset into current block */
                offset = (dest - (char*)new_blocks) % ext2->block_size;

                /* If new entry would overflow the block */
                if (offset + rec_len > ext2->block_size)
                {
                    uint32_t rem = ext2->block_size - offset;

                    if (!prev)
                    {
                        goto done;
                    }

                    /* Adjust previous entry to point to next block */
                    prev->rec_len += rem;
                    dest += rem;
                }

                /* Copy this entry into new buffer */
                {
                    dir_ent_t* new_ent = (dir_ent_t*)dest;
                    memset(new_ent, 0, rec_len);
                    memcpy(
                        new_ent,
                        curr_ent,
                        sizeof(*curr_ent) + curr_ent->name_len);

                    new_ent->rec_len = rec_len;
                    prev = new_ent;
                    dest += rec_len;
                }

                src += curr_ent->rec_len;
            }

            /* Set final entry to point to end of the block */
            if (prev)
            {
                uint32_t offset;
                uint32_t rem;

                /* Compute byte offset into current block */
                offset = (dest - (char*)new_blocks) % ext2->block_size;

                /* Compute remaining bytes */
                rem = ext2->block_size - offset;

                /* Set record length of final entry to end of block */
                prev->rec_len += rem;

                /* Advance dest to block boundary */
                dest += rem;
            }

            /* Size down the new blocks size */
            new_blocks_size = (uint32_t)(dest - (char*)new_blocks);

            if (_if_err(
                    err = _check_directory_entries(
                        ext2, new_blocks, new_blocks_size)))
            {
                goto done;
            }
        }
    }

    /* Count directory entries before and after */
    {
        uint32_t count;
        uint32_t new_count;

        if (_if_err(
                err = _count_directory_entries(
                    ext2, blocks, blocks_size, &count)))
        {
            goto done;
        }

        if (_if_err(
                err = _count_directory_entries(
                    ext2, new_blocks, new_blocks_size, &new_count)))
        {
            goto done;
        }
    }

    /* Return all directory blocks to the free list */
    if (_if_err(err = _put_blocks(ext2, blknos.data, blknos.size)))
    {
        goto done;
    }

    /* Update the inode blocks */
    if (_if_err(
            err = _update_inode_data_blocks(
                ext2, ino, &inode, new_blocks, new_blocks_size, 1))) /* is_dir
                                                                        */
    {
        goto done;
    }

    err = EXT2_ERR_NONE;

done:

    if (blocks)
        free(blocks);

    if (new_blocks)
        free(new_blocks);

    buf_u32_release(&blknos);

    return err;
}

static ext2_err_t _create_file_inode(
    ext2_t* ext2,
    const void* data,
    uint32_t size,
    uint16_t mode,
    uint32_t* blknos,
    uint32_t nblknos,
    uint32_t* ino)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    ext2_inode_t inode;

    /* Check parameters */
    if (!_is_valid(ext2) || !data || !blknos)
    {
        err = EXT2_ERR_INVALID_PARAMETER;
        goto done;
    }

    /* Initialize the inode */
    {
        /* Uses posix_time() for EFI */
        const uint32_t t = time(NULL);

        memset(&inode, 0, sizeof(ext2_inode_t));

        /* Set the mode of the new file */
        inode.i_mode = mode;

        /* Set the uid and gid to root */
        inode.i_uid = 0;
        inode.i_gid = 0;

        /* Set the size of this file */
        inode.i_size = size;

        /* Set the access, creation, and mtime to the same value */
        inode.i_atime = t;
        inode.i_ctime = t;
        inode.i_mtime = t;

        /* Linux-specific value */
        inode.i_osd1 = 1;

        /* The number of links is initially 1 */
        inode.i_links_count = 1;

        /* Set the number of 512 byte blocks */
        inode.i_blocks = (nblknos * ext2->block_size) / 512;
    }

    /* Assign an inode number */
    if (_if_err(err = _get_ino(ext2, ino)))
    {
        goto done;
    }

    /* Update the inode block pointers and write the inode to disk */
    if (_if_err(
            err = _update_inode_block_pointers(
                ext2, *ino, &inode, size, blknos, nblknos)))
    {
        goto done;
    }

    err = EXT2_ERR_NONE;

done:
    return err;
}

typedef struct _ext2_directory_ent_buf
{
    dir_ent_t base;
    char buf[EXT2_PATH_MAX];
} ext2_directory_ent_buf_t;

static ext2_err_t _create_dir_inode_and_block(
    ext2_t* ext2,
    ext2_ino_t parent_ino,
    uint16_t mode,
    ext2_ino_t* ino)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    ext2_inode_t inode;
    uint32_t blkno;
    ext2_block_t block;

    /* Check parameters */
    if (!_is_valid(ext2) || !mode || !parent_ino || !ino)
    {
        err = EXT2_ERR_INVALID_PARAMETER;
        goto done;
    }

    /* Initialize the inode */
    {
        const uint32_t t = time(NULL);

        memset(&inode, 0, sizeof(ext2_inode_t));

        /* Set the mode of the new file */
        inode.i_mode = mode;

        /* Set the uid and gid to root */
        inode.i_uid = 0;
        inode.i_gid = 0;

        /* Set the size of this file */
        inode.i_size = ext2->block_size;

        /* Set the access, creation, and mtime to the same value */
        inode.i_atime = t;
        inode.i_ctime = t;
        inode.i_mtime = t;

        /* Linux-specific value */
        inode.i_osd1 = 1;

        /* The number of links is initially 2 */
        inode.i_links_count = 2;

        /* Set the number of 512 byte blocks */
        inode.i_blocks = ext2->block_size / 512;
    }

    /* Assign an inode number */
    if (_if_err(err = _get_ino(ext2, ino)))
    {
        goto done;
    }

    /* Assign a block number */
    if (_if_err(err = _get_block(ext2, &blkno)))
    {
        goto done;
    }

    /* Create a block to hold the two directory entries */
    {
        ext2_directory_ent_buf_t dot1;
        ext2_directory_ent_buf_t dot2;
        dir_ent_t* ent;

        /* The "." directory */
        memset(&dot1, 0, sizeof(dot1));
        dot1.base.inode = *ino;
        dot1.base.name_len = 1;
        dot1.base.file_type = EXT2_DT_DIR;
        dot1.base.name[0] = '.';
        uint16_t d1len = sizeof(dir_ent_t) - EXT2_PATH_MAX + dot1.base.name_len;
        dot1.base.rec_len = _next_mult(d1len, 4);

        /* The ".." directory */
        memset(&dot2, 0, sizeof(dot2));
        dot2.base.inode = parent_ino;
        dot2.base.name_len = 2;
        dot2.base.file_type = EXT2_DT_DIR;
        dot2.base.name[0] = '.';
        dot2.base.name[1] = '.';
        uint16_t d2len = sizeof(dir_ent_t) - EXT2_PATH_MAX + dot2.base.name_len;
        dot2.base.rec_len = _next_mult(d2len, 4);

        /* Initialize the directory entries block */
        memset(&block, 0, sizeof(ext2_block_t));
        memcpy(block.data, &dot1, dot1.base.rec_len);
        memcpy(block.data + dot1.base.rec_len, &dot2, dot2.base.rec_len);
        block.size = ext2->block_size;

        /* Adjust dot2.rec_len to point to end of block */
        ent = (dir_ent_t*)(block.data + dot1.base.rec_len);

        ent->rec_len +=
            ext2->block_size - (dot1.base.rec_len + dot2.base.rec_len);

        /* Write the block */
        if (_if_err(err = ext2_write_block(ext2, blkno, &block)))
        {
            goto done;
        }
    }

    /* Update the inode block pointers and write the inode to disk */
    if (_if_err(
            err = _update_inode_block_pointers(
                ext2, *ino, &inode, ext2->block_size, &blkno, 1)))
    {
        goto done;
    }

    err = EXT2_ERR_NONE;

done:
    return err;
}

static ext2_err_t _append_directory_entry(
    ext2_t* ext2,
    void* data,
    uint32_t size, /* unused */
    char** current,
    dir_ent_t** prev,
    const dir_ent_t* ent)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    uint32_t rec_len;
    uint32_t offset;
    (void)size;

    /* Compute size of the new directory entry */
    rec_len = sizeof(*ent) - EXT2_PATH_MAX + ent->name_len;
    rec_len = _next_mult(rec_len, 4);

    /* Compute byte offset into current block */
    offset = ((char*)(*current) - (char*)data) % ext2->block_size;

    /* If new entry would overflow the block */
    if (offset + rec_len > ext2->block_size)
    {
        uint32_t rem = ext2->block_size - offset;

        if (!(*prev))
        {
            goto done;
        }

        /* Adjust previous entry to point to next block */
        (*prev)->rec_len += rem;
        (*current) += rem;
    }

    /* Copy this entry into new buffer */
    {
        dir_ent_t* tmp_ent;

        /* Set pointer to next new entry */
        tmp_ent = (dir_ent_t*)(*current);

        /* Copy over new entry */
        memset(tmp_ent, 0, rec_len);
        memcpy(tmp_ent, ent, sizeof(*ent) - EXT2_PATH_MAX + ent->name_len);
        tmp_ent->rec_len = rec_len;
        (*prev) = tmp_ent;
        (*current) += rec_len;
    }

    err = EXT2_ERR_NONE;

done:
    return err;
}

static ext2_err_t _index_directory_to_linked_list_directory(
    ext2_t* ext2,
    const char* rm_name, /* may be null */
    dir_ent_t* new_ent,  /* may be null */
    const void* data,
    uint32_t size,
    void** new_data,
    uint32_t* new_size)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    const char* src = (const char*)data;
    const char* src_end = (const char*)data + size;
    char* dest = NULL;

    /* Check parameters */
    if (!_is_valid(ext2) || !data || !size || !new_data || !new_size)
    {
        err = EXT2_ERR_INVALID_PARAMETER;
        goto done;
    }

    /* Linked list directory will be smaller than this */
    *new_size = size;

    /* Allocate a buffer to hold 'linked list' directory */
    if (!(*new_data = calloc(*new_size, 1)))
    {
        goto done;
    }

    /* Set current and end pointers to new buffer */
    dest = (char*)*new_data;

    /* Copy over directory entries (skipping removed entry) */
    {
        dir_ent_t* prev = NULL;

        while (src < src_end)
        {
            const dir_ent_t* ent;

            /* Set pointer to current entry */
            ent = (const dir_ent_t*)src;

            /* Skip blank directory entries */
            if (ent->name[0] == '\0')
            {
                src += ent->rec_len;
                continue;
            }

            /* Skip optional entry to be removed */
            if (rm_name && strncmp(rm_name, ent->name, ent->name_len) == 0)
            {
                src += ent->rec_len;
                continue;
            }

            if (_if_err(
                    err = _append_directory_entry(
                        ext2, *new_data, *new_size, &dest, &prev, ent)))
            {
                goto done;
            }

            src += ent->rec_len;
        }

        if (new_ent)
        {
            if (_if_err(
                    err = _append_directory_entry(
                        ext2, *new_data, *new_size, &dest, &prev, new_ent)))
            {
                goto done;
            }
        }

        /* Set final entry to point to end of the block */
        if (prev)
        {
            uint32_t offset;
            uint32_t rem;

            /* Compute byte offset into current block */
            offset = (dest - (char*)*new_data) % ext2->block_size;

            /* Compute remaining bytes */
            rem = ext2->block_size - offset;

            /* Set record length of final entry to end of block */
            prev->rec_len += rem;

            /* Advance dest to block boundary */
            dest += rem;
        }

        /* Size down the new blocks size */
        *new_size = (uint32_t)(dest - (char*)*new_data);

        /* Perform a sanity check on the new entries */
        if (_if_err(err = _check_directory_entries(ext2, *new_data, *new_size)))
        {
            goto done;
        }
    }

    err = EXT2_ERR_NONE;

done:
    return err;
}

static ext2_err_t _add_directory_entry(
    ext2_t* ext2,
    ext2_ino_t dir_ino,
    ext2_inode_t* dir_inode,
    const char* filename,
    dir_ent_t* new_ent)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    void* blocks = NULL;
    uint32_t blocks_size = 0;
    void* new_blocks = NULL;
    uint32_t new_blocks_size = 0;
    buf_u32_t blknos = BUF_U32_INITIALIZER;

    /* Load the directory file */
    if (_if_err(
            err = ext2_load_file_from_inode(
                ext2, dir_inode, &blocks, &blocks_size)))
    {
        goto done;
    }

    /* Load the block numbers (including the block blocks) */
    if (_if_err(
            err = _load_block_numbers_from_inode(
                ext2,
                dir_inode,
                1, /* include_block_blocks */
                &blknos)))
    {
        goto done;
    }

    /* Find 'filename' within this directory */
    if ((_find_directory_entry(filename, blocks, blocks_size)))
    {
        err = EXT2_ERR_FILE_NOT_FOUND;
        goto done;
    }

    /* Convert from 'indexed' to 'linked list' directory format */
    if (_if_err(
            err = _index_directory_to_linked_list_directory(
                ext2,
                NULL,    /* rm_name */
                new_ent, /* new_entry */
                blocks,
                dir_inode->i_size,
                &new_blocks,
                &new_blocks_size)))
    {
        goto done;
    }

    /* Count directory entries before and after */
    {
        uint32_t count;
        uint32_t new_count;

        if (_if_err(
                err = _count_directory_entries(
                    ext2, blocks, blocks_size, &count)))
        {
            goto done;
        }

        if (_if_err(
                err = _count_directory_entries(
                    ext2, new_blocks, new_blocks_size, &new_count)))
        {
            goto done;
        }

        if (count + 1 != new_count)
        {
            goto done;
        }
    }

    /* Return all directory blocks to the free list */
    if (_if_err(err = _put_blocks(ext2, blknos.data, blknos.size)))
    {
        goto done;
    }

    /* Update the directory inode blocks */
    if (_if_err(
            err = _update_inode_data_blocks(
                ext2,
                dir_ino,
                dir_inode,
                new_blocks,
                new_blocks_size,
                1))) /* is_dir */
    {
        goto done;
    }

    err = EXT2_ERR_NONE;

done:

    if (blocks)
        free(blocks);

    if (new_blocks)
        free(new_blocks);

    buf_u32_release(&blknos);

    return err;
}

ext2_err_t ext2_put(
    ext2_t* ext2,
    const void* data,
    uint32_t size,
    const char* path,
    uint16_t mode)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    char dirname[EXT2_PATH_MAX];
    char filename[EXT2_PATH_MAX];
    ext2_ino_t file_ino;
    ext2_inode_t file_inode;
    ext2_ino_t dir_ino;
    ext2_inode_t dir_inode;
    buf_u32_t blknos = BUF_U32_INITIALIZER;
    ext2_directory_ent_buf_t ent_buf;
    dir_ent_t* ent = &ent_buf.base;

    /* Check parameters */
    if (!_is_valid(ext2) || !data || !size || !path)
    {
        err = EXT2_ERR_INVALID_PARAMETER;
        goto done;
    }

    /* Reject attempts to copy directories */
    if (mode & EXT2_S_IFDIR)
    {
        goto done;
    }

    /* Split the path */
    if (_if_err(err = _split_full_path(path, dirname, filename)))
    {
        goto done;
    }

    /* Read inode for the directory */
    if (_if_err(err = ext2_path_to_inode(ext2, dirname, &dir_ino, &dir_inode)))
    {
        goto done;
    }

    /* Read inode for the file (if any) */
    if (!_if_err(err = ext2_path_to_inode(ext2, path, &file_ino, &file_inode)))
    {
        /* Disallow overwriting of directory */
        if (!(file_inode.i_mode & EXT2_S_IFDIR))
        {
            goto done;
        }

        /* Delete the file if it exists */
        if (_if_err(err = ext2_rm(ext2, path)))
        {
            goto done;
        }
    }

    /* Write the blocks of the file */
    if (_if_err(err = _write_data(ext2, data, size, &blknos)))
        goto done;

    /* Create an inode for this new file */
    if (_if_err(
            err = _create_file_inode(
                ext2, data, size, mode, blknos.data, blknos.size, &file_ino)))
    {
        goto done;
    }

    /* Initialize the new directory entry */
    {
        /* ent->inode */
        ent->inode = file_ino;

        /* ent->name_len */
        ent->name_len = (uint32_t)strlen(filename);

        /* ent->file_type */
        ent->file_type = EXT2_FT_REG_FILE;

        /* ent->name[] */
        ent->name[0] = '\0';
        _strncat(ent->name, sizeof(ent->name), filename, EXT2_PATH_MAX - 1);

        /* ent->rec_len */
        ent->rec_len =
            _next_mult(sizeof(*ent) - EXT2_PATH_MAX + ent->name_len, 4);
    }

    /* Create new entry for this file in the directory inode */
    if (_if_err(
            err =
                _add_directory_entry(ext2, dir_ino, &dir_inode, filename, ent)))
    {
        goto done;
    }

    err = EXT2_ERR_NONE;

done:

    buf_u32_release(&blknos);

    return err;
}

ext2_err_t ext2_make_dir(ext2_t* ext2, const char* path, uint16_t mode)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    char dirname[EXT2_PATH_MAX];
    char basename[EXT2_PATH_MAX];
    ext2_ino_t dir_ino;
    ext2_inode_t dir_inode;
    ext2_ino_t base_ino;
    ext2_inode_t base_inode;
    ext2_directory_ent_buf_t ent_buf;
    dir_ent_t* ent = &ent_buf.base;

    /* Check parameters */
    if (!_is_valid(ext2) || !path || !mode)
    {
        err = EXT2_ERR_INVALID_PARAMETER;
        goto done;
    }

    /* Reject is not a directory */
    if (!(mode & EXT2_S_IFDIR))
    {
        err = EXT2_ERR_INVALID_PARAMETER;
        goto done;
    }

    /* Split the path */
    if (_if_err(err = _split_full_path(path, dirname, basename)))
    {
        goto done;
    }

    /* Read inode for 'dirname' */
    if (_if_err(err = ext2_path_to_inode(ext2, dirname, &dir_ino, &dir_inode)))
    {
        goto done;
    }

    /* Fail if the directory already exists */
    if (!_if_err(err = ext2_path_to_inode(ext2, path, &base_ino, &base_inode)))
    {
        goto done;
    }

    /* Create the directory inode and its one block */
    if (_if_err(
            err = _create_dir_inode_and_block(ext2, dir_ino, mode, &base_ino)))
    {
        goto done;
    }

    /* Initialize the new directory entry */
    {
        /* ent->inode */
        ent->inode = base_ino;

        /* ent->name_len */
        ent->name_len = (uint32_t)strlen(basename);

        /* ent->file_type */
        ent->file_type = EXT2_FT_DIR;

        /* ent->name[] */
        ent->name[0] = '\0';
        _strncat(ent->name, sizeof(ent->name), basename, EXT2_PATH_MAX - 1);

        /* ent->rec_len */
        ent->rec_len =
            _next_mult(sizeof(*ent) - EXT2_PATH_MAX + ent->name_len, 4);
    }

    /* Include new child directory in parent directory's i_links_count */
    dir_inode.i_links_count++;

    /* Create new entry for this file in the directory inode */
    if (_if_err(
            err =
                _add_directory_entry(ext2, dir_ino, &dir_inode, basename, ent)))
    {
        goto done;
    }

    err = EXT2_ERR_NONE;

done:
    return err;
}

/*
**==============================================================================
**
** ext2_file_t
**
**==============================================================================
*/

struct _ext2_file
{
    ext2_t* ext2;
    ext2_inode_t inode;
    buf_u32_t blknos;
    uint32_t offset;
    bool eof;
};

ext2_file_t* ext2_open_file(
    ext2_t* ext2,
    const char* path,
    ext2_file_mode_t mode)
{
    ext2_file_t* file = NULL;
    ext2_ino_t ino;
    ext2_inode_t inode;
    buf_u32_t blknos = BUF_U32_INITIALIZER;

    /* Reject null parameters */
    if (!ext2 || !path)
        goto done;

    /* Find the inode for this file */
    if (ext2_path_to_inode(ext2, path, &ino, &inode) != EXT2_ERR_NONE)
        goto done;

    /* Load the block numbers for this inode */
    if (_load_block_numbers_from_inode(
            ext2,
            &inode,
            0, /* include_block_blocks */
            &blknos) != EXT2_ERR_NONE)
    {
        goto done;
    }

    /* Allocate and initialize the file object */
    {
        if (!(file = (ext2_file_t*)calloc(1, sizeof(ext2_file_t))))
            goto done;

        file->ext2 = ext2;
        file->inode = inode;
        file->blknos = blknos;
        file->offset = 0;
    }

done:
    if (!file)
        buf_u32_release(&blknos);

    return file;
}

int32_t ext2_read_file(ext2_file_t* file, void* data, uint32_t size)
{
    int32_t nread = -1;
    uint32_t first;
    uint32_t i;
    uint32_t remaining;
    uint8_t* end = (uint8_t*)data;

    /* Check parameters */
    if (!file || !file->ext2 || !data)
        goto done;

    /* The index of the first block to read: ext2->blknos[first] */
    first = file->offset / file->ext2->block_size;

    /* The number of bytes remaining to be read */
    remaining = size;

    /* Read the data block-by-block */
    for (i = first; i < file->blknos.size && remaining > 0 && !file->eof; i++)
    {
        ext2_block_t block;
        uint32_t offset;

        if (ext2_read_block(file->ext2, file->blknos.data[i], &block) !=
            EXT2_ERR_NONE)
        {
            goto done;
        }

        /* The offset of the data within this block */
        offset = file->offset % file->ext2->block_size;

        /* Copy the minimum of these to the caller's buffer:
         *     remaining
         *     block-bytes-remaining
         *     file-bytes-remaining
         */
        {
            uint32_t copy_bytes;

            /* Bytes to copy to user buffer */
            copy_bytes = remaining;

            /* Reduce copy_bytes to bytes remaining in the block? */
            {
                uint32_t block_bytes_remaining =
                    file->ext2->block_size - offset;

                if (block_bytes_remaining < copy_bytes)
                    copy_bytes = block_bytes_remaining;
            }

            /* Reduce copy_bytes to bytes remaining in the file? */
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
            memcpy(end, block.data + offset, copy_bytes);
            remaining -= copy_bytes;
            end += copy_bytes;
            file->offset += copy_bytes;
        }
    }

    /* Calculate number of bytes read */
    nread = size - remaining;

done:
    return nread;
}

int32_t ext2_write_file(ext2_file_t* file, const void* data, uint32_t size)
{
    int32_t nread = -1;
    uint32_t first;
    uint32_t i;
    uint32_t remaining;
    uint8_t* end = (uint8_t*)data;
    uint32_t new_file_size;

    /* Check parameters */
    if (!file || !file->ext2 || !data)
        goto done;

    /* The index of the first block to write: ext2->blknos[first] */
    first = file->offset / file->ext2->block_size;

    /* The number of bytes remaining to be written */
    remaining = size;

    /* Calculate the new size of the file (might be unchanged). */
    {
        uint32_t original_file_size = file->inode.i_size;

        if (original_file_size < file->offset + size)
            new_file_size = file->offset + size;
        else
            new_file_size = original_file_size;
    }

/* 
ATTN: extend the file by first+size 
*/

    /* Write the data block-by-block */
    for (i = first; i < file->blknos.size && remaining > 0 && !file->eof; i++)
    {
        ext2_block_t block;
        uint32_t offset;

        /* Read the block first. */
        if (ext2_read_block(file->ext2, file->blknos.data[i], &block) !=
            EXT2_ERR_NONE)
        {
            goto done;
        }

        /* The offset of the data within this block */
        offset = file->offset % file->ext2->block_size;

        /* Copy the minimum of these into the block.
         *     bytes remaining
         *     block-bytes-remaining
         *     file-bytes-remaining
         */
        {
            uint32_t copy_bytes;

            /* Bytes to copy to user buffer */
            copy_bytes = remaining;

            /* Reduce copy_bytes to bytes remaining in the block? */
            {
                uint32_t block_bytes_remaining =
                    file->ext2->block_size - offset;

                if (block_bytes_remaining < copy_bytes)
                    copy_bytes = block_bytes_remaining;
            }

            /* Reduce copy_bytes to bytes remaining in the file? */
            {
                uint32_t file_bytes_remaining =
                    new_file_size - file->offset;

                if (file_bytes_remaining < copy_bytes)
                {
                    copy_bytes = file_bytes_remaining;
                    file->eof = true;
                }
            }

            /* Copy data to user buffer */
            memcpy(block.data + offset, end, copy_bytes);
            remaining -= copy_bytes;
            end += copy_bytes;
            file->offset += copy_bytes;
        }

        /* Rewrite the block. */
        if (ext2_write_block(file->ext2, file->blknos.data[i], &block) !=
            EXT2_ERR_NONE)
        {
            goto done;
        }
    }

    /* Append remaining data to the file. */
    {
        buf_u32_t blknos = BUF_U32_INITIALIZER;

        /* Write the new raw blocks. */
        if (_write_data(file->ext2, end, remaining, &blknos) != EXT2_ERR_NONE)
            goto done;

#if 0
        /* Append new blocks to the file inode. */
        if (_append_blocks_to_inode(file->ext2, &blknos) != EXT2_ERR_NONE)
            goto done;
#endif
    }

    /* Calculate number of bytes read */
    nread = size - remaining;

done:
    return nread;
}

int ext2_flush_file(ext2_file_t* file)
{
    int rc = -1;

    /* Check parameters */
    if (!file || !file->ext2)
        goto done;

    /* No-op because ext2_t is always flushed */

    rc = 0;

done:
    return rc;
}

int ext2_close_file(ext2_file_t* file)
{
    int rc = -1;

    /* Check parameters */
    if (!file || !file->ext2)
        goto done;

    /* Release the block numbers buffer */
    buf_u32_release(&file->blknos);

    /* Release the file object */
    free(file);

    rc = 0;

done:
    return rc;
}

int ext2_seek_file(ext2_file_t* file, int32_t offset)
{
    int rc = -1;

    if (!file)
        goto done;

    if (offset > file->inode.i_size)
        goto done;

    file->offset = offset;

    rc = 0;

done:
    return rc;
}

int32_t ext2_tell_file(ext2_file_t* file)
{
    return file ? file->offset : -1;
}

int32_t ext2_size_file(ext2_file_t* file)
{
    return file ? file->inode.i_size : -1;
}

ext2_err_t ext2_get_block_numbers(
    ext2_t* ext2,
    const char* path,
    buf_u32_t* blknos)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    ext2_ino_t ino;
    ext2_inode_t inode;

    /* Reject null parameters */
    if (!ext2 || !path || !blknos)
    {
        err = EXT2_ERR_INVALID_PARAMETER;
        goto done;
    }

    /* Find the inode for this file */
    if (ext2_path_to_inode(ext2, path, &ino, &inode) != EXT2_ERR_NONE)
        goto done;

    /* Load the block numbers for this inode */
    if (_if_err(
            err = _load_block_numbers_from_inode(
                ext2,
                &inode,
                1, /* include_block_blocks */
                blknos)))
    {
        goto done;
    }

    err = EXT2_ERR_NONE;

done:

    return err;
}

uint32_t ext2_blkno_to_lba(const ext2_t* ext2, uint32_t blkno)
{
    return BlockOffset(blkno, ext2->block_size) / EXT2_SECTOR_SIZE;
}

ext2_err_t ext2_get_first_blkno(
    const ext2_t* ext2,
    ext2_ino_t ino,
    uint32_t* blkno)
{
    ext2_err_t err = EXT2_ERR_FAILED;
    ext2_inode_t inode;

    /* Initialize output parameter */
    if (blkno)
        *blkno = 0;

    /* Reject null parameters */
    if (!ext2 || !blkno)
    {
        err = EXT2_ERR_INVALID_PARAMETER;
        goto done;
    }

    /* Read the inode into memory */
    if (ext2_read_inode(ext2, ino, &inode) != EXT2_ERR_NONE)
        goto done;

    /* Get the block number from the inode */
    *blkno = inode.i_block[0];

    err = EXT2_ERR_NONE;

done:

    return err;
}

/*
**==============================================================================
**
** filesys_impl_t:
**
**==============================================================================
*/

typedef struct _file_impl
{
    oe_file_t base;
    ext2_file_t* ext2_file;
}
file_impl_t;

static ssize_t _file_read(oe_file_t* file, void *buf, size_t count)
{
    ssize_t ret = -1;
    file_impl_t* file_impl = (file_impl_t*)file;

    if (!file_impl || !file_impl->ext2_file)
        goto done;

    ret = ext2_read_file(file_impl->ext2_file, buf, count);

done:
    return ret;
}

static ssize_t _file_write(oe_file_t* file, const void *buf, size_t count)
{
    ssize_t ret = -1;
    file_impl_t* file_impl = (file_impl_t*)file;

    if (!file_impl || !file_impl->ext2_file)
        goto done;

    ret = ext2_write_file(file_impl->ext2_file, buf, count);

done:
    return ret;
}

static int _file_close(oe_file_t* file)
{
    ssize_t ret = -1;
    file_impl_t* file_impl = (file_impl_t*)file;

    if (!file_impl || !file_impl->ext2_file)
        goto done;

    if (ext2_close_file(file_impl->ext2_file) != EXT2_ERR_NONE)
        goto done;

    memset(file_impl, 0, sizeof(file_impl_t));
    free(file_impl);
    ret = 0;

done:
    return ret;
}

typedef struct _filesys_impl
{
    oe_filesys_t base;
    ext2_t* ext2;
}
filesys_impl_t;

static oe_file_t* _filesys_open_file(
    oe_filesys_t* filesys,
    const char* path, 
    int flags, 
    mode_t mode)
{
    oe_file_t* ret = NULL;
    filesys_impl_t* filesys_impl = (filesys_impl_t*)filesys;
    ext2_file_t* ext2_file = NULL;


    if (!filesys_impl || !filesys_impl->ext2)
        goto done;

    /* ATTN: support create mode. */

    /* Open the EXT2 file. */
    if (!(ext2_file = ext2_open_file(filesys_impl->ext2, path, mode)))
        goto done;

    /* Allocate and initialize the file struct. */
    {
        file_impl_t* file_impl;

        if (!(file_impl = calloc(1, sizeof(file_impl_t))))
            goto done;

        file_impl->base.read = _file_read;
        file_impl->base.write = _file_write;
        file_impl->base.close = _file_close;
        file_impl->ext2_file = ext2_file;
        ext2_file = NULL;

        ret = &file_impl->base;
    }

done:

    if (ext2_file)
        ext2_close_file(ext2_file);

    return ret;
}

static int _filesys_release(oe_filesys_t* filesys)
{
    filesys_impl_t* filesys_impl = (filesys_impl_t*)filesys;
    int ret = -1;

    if (!filesys_impl || !filesys_impl->ext2)
        goto done;

    ext2_delete(filesys_impl->ext2);

    free(filesys_impl);

done:
    return ret;
}

oe_filesys_t* ext2_new_filesys(oe_block_device_t* dev)
{
    oe_filesys_t* ret = NULL;
    ext2_t* ext2 = NULL;

    /* Create the new EXT2 struct. */
    if (ext2_new(dev, &ext2) != EXT2_ERR_NONE)
        goto done;

    /* Create an initalize the filesys struct. */
    {
        filesys_impl_t* filesys_impl = NULL;

        if (!(filesys_impl = calloc(1, sizeof(filesys_impl_t))))
            goto done;

        filesys_impl->base.open_file = _filesys_open_file;
        filesys_impl->base.release = _filesys_release;
        filesys_impl->ext2 = ext2;
        ext2 = NULL;

        ret = &filesys_impl->base;
    }

done:

    if (ext2)
        ext2_delete(ext2);

    return ret;
}
