// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _FS_HOSTBATCH_H
#define _FS_HOSTBATCH_H

#include <stdint.h>

typedef struct _fs_host_batch fs_host_batch_t;

fs_host_batch_t* fs_host_batch_new(size_t capacity);

void fs_host_batch_delete(fs_host_batch_t* batch);

void* fs_host_batch_alloc(fs_host_batch_t* batch, size_t size);

int fs_host_batch_free(fs_host_batch_t* batch);

#endif /* _FS_HOSTBATCH_H */
