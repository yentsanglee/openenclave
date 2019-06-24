// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_ENCLAVE_REGISTER_H
#define _VE_ENCLAVE_REGISTER_H

void ve_set_fs_register_base(const void* ptr);

void* ve_get_fs_register_base(void);

#endif /* _VE_ENCLAVE_REGISTER_H */
