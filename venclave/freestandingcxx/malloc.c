// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

static unsigned char heap[1024];

void* malloc(unsigned long size)
{
    return heap;
}

void free(void* ptr)
{
}
