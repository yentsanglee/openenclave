#ifndef _oe_oefs_h
#define _oe_oefs_h

#include <openenclave/internal/defs.h>
#include <openenclave/internal/types.h>
#include "blockdevice.h"

#define OEFS_PATH_MAX 256

#define OEFS_BLOCK_SIZE 512

#define OEFS_BITS_PER_BLOCK (OEFS_BLOCK_SIZE * 8)

#define OEFS_SUPER_BLOCK_MAGIC 0x0EF55FE0

#define OEFS_INODE_MAGIC 0x0120DD021

/* The minimum number of blocks in a file system. */
#define OEFS_MIN_BLOCKS OEFS_BLOCK_SIZE

#define OEFS_ROOT_INODE_BLKNO 1

/* oefs_dir_entry_t.d_type */
#define OEFS_DT_UNKNOWN 0
#define OEFS_DT_FIFO 1 /* unused */
#define OEFS_DT_CHR 2 /* unused */
#define OEFS_DT_DIR 4
#define OEFS_DT_BLK 6 /* unused */
#define OEFS_DT_REG 8
#define OEFS_DT_LNK 10 /* unused */
#define OEFS_DT_SOCK 12 /* unused */
#define OEFS_DT_WHT 14 /* unused */

typedef struct _oefs_super_block
{
    /* Magic number: OEFS_SUPER_BLOCK_MAGIC. */
    uint32_t s_magic;

    /* Total blocks in the file system. */
    uint32_t s_num_blocks;

    /* The number of free blocks. */
    uint32_t s_free_blocks;

    /* Reserved. */
    uint8_t s_reserved[500];
}
oefs_super_block_t;

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

    /* Total number of 512-byte blocks in this file. */
    uint32_t i_total_blocks;

    /* The next blknos block. */
    uint32_t i_next;

    /* Reserved. */
    uint32_t i_reserved[6];

    /* Blocks comprising this file. */
    uint32_t i_blocks[112];

} oefs_inode_t;

OE_STATIC_ASSERT(sizeof(oefs_inode_t) == OEFS_BLOCK_SIZE);

typedef struct _oefs_bnode
{
    /* The next blknos block. */
    uint32_t b_next;

    /* Blocks comprising this file. */
    uint32_t b_blocks[127];

} oefs_bnode_t;

OE_STATIC_ASSERT(sizeof(oefs_bnode_t) == OEFS_BLOCK_SIZE);

typedef struct _oefs_dir_entry
{
    /* Inode of this entry. */
    uint32_t d_inode;

    /* Type of this entry (file, directory, etc.). */
    uint32_t d_type;

    /* Name of this file. */
    char d_name[OEFS_PATH_MAX];

} oefs_dir_entry_t;

OE_STATIC_ASSERT(sizeof(oefs_dir_entry_t) == 264);

typedef enum _oefs_result
{
    OEFS_OK,
    OEFS_BAD_PARAMETER,
    OEFS_FAILED,
}
oefs_result_t;

// Initialize the blocks of a new file system; num_blocks must be a multiple
// OEFS_BITS_PER_BLOCK (4096).
oefs_result_t oefs_initialize(oe_block_device_t* dev, size_t num_blocks);

oefs_result_t oefs_compute_size(size_t num_blocks, size_t* total_bytes);

typedef struct _oefs
{
    oe_block_device_t* dev;
    
    /* Super block. */
    oefs_super_block_t sb;

    /* Bitmap of allocated blocks. */
    uint8_t* bitmap;
    size_t bitmap_size;
}
oefs_t;

oefs_result_t oefs_open(oefs_t* oefs, oe_block_device_t* dev);

#endif /* _oe_oefs_h */
