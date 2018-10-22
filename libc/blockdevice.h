#ifndef _oe_libc_blockdevice_h
#define _oe_libc_blockdevice_h

#include <stdint.h>
#include <openenclave/enclave.h>

typedef struct _oe_block_device oe_block_device_t;

struct _oe_block_device
{
    int (*close)(oe_block_device_t* dev);

    int (*get)(oe_block_device_t* dev, uint32_t blkno, void* data);

    int (*put)(oe_block_device_t* dev, uint32_t blkno, const void* data);
};

/* Ask host to open a block device. */
oe_result_t oe_open_block_device(
    const char* device_name,
    oe_block_device_t** block_device);

#endif /* _oe_libc_blockdevice_h */
