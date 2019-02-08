// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/bits/device.h>
#include <openenclave/internal/fs.h>
#include <stdio.h>

OE_FILE* oe_device_fopen(int device_id, const char* path, const char* mode)
{
    oe_set_thread_default_device(device_id);
    OE_FILE* ret = (OE_FILE*)fopen(path, mode);
    oe_clear_thread_default_device();

    return ret;
}
