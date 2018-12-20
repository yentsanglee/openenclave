// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifdef weak_alias
#undef weak_alias
#endif

#define weak_alias(old, new) \
    extern __typeof(old) new __attribute__((weak, alias(#old)))
