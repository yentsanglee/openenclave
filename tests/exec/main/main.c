// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <stdio.h>
#include <stdlib.h>

int main(int argc, const char* argv[])
{
    printf("Hello World!\n");

    printf("argc=%d\n", argc);
#if 0

    for (int i = 0; i < argc; i++)
    {
        printf("argv[%d]=%s\n", i, argv[i]);
    }
#endif
    exit(1);
}
