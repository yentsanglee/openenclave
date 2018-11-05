// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _FS_GUID_H
#define _FS_GUID_H

#include "common.h"

#define FS_GUID_STRING_SIZE 37

typedef struct _fs_guid
{
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint8_t data4[8];
} fs_guid_t;


void fs_format_guid(
    char buf[FS_GUID_STRING_SIZE],
    const fs_guid_t* guid);

#endif /* _FS_GUID_H */
