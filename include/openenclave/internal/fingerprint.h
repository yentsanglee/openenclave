// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OE_INTERNAL_FINGERPRINT
#define _OE_INTERNAL_FINGERPRINT

typedef struct _oe_fingerprint
{
    /* The size of the file. */
    unsigned long size;

    /* The SHA-256 hash of the file. */
    unsigned char hash[32];
} oe_fingerprint_t;

int oe_compute_fingerprint(const char* path, oe_fingerprint_t* fingerprint);

#endif /* _OE_INTERNAL_FINGERPRINT */
