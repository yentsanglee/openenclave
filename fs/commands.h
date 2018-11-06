// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _FS_COMMANDS_H
#define _FS_COMMANDS_H

#include "common.h"
#include "strarr.h"

int fs_lsr(const char* root, fs_strarr_t* paths);

#endif /* _FS_COMMANDS_H */
