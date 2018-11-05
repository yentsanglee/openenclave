// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "ocall.h"
#include "hostblkdev.h"
#include "hostfs.h"

static const fs_guid_t _HOSTBLKDEV_GUID = FS_HOSTBLKDEV_GUID;
static const fs_guid_t _HOSTFS_GUID = FS_HOSTFS_GUID;

void fs_handle_ocall(fs_args_t* args)
{
    if (args)
    {
        if (memcmp(&args->guid, &_HOSTBLKDEV_GUID, sizeof(args->guid)) == 0)
        {
            fs_handle_hostblkdev_ocall((fs_hostblkdev_ocall_args_t*)args);
        }
        else if (memcmp(&args->guid, &_HOSTFS_GUID, sizeof(args->guid)) == 0)
        {
            fs_handle_hostfs_ocall((fs_hostfs_ocall_args_t*)args);
        }
    }
}
