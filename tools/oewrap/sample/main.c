// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <stdio.h>

int main(int argc, const char* argv[])
{
    printf("argc=%d\n", argc);

    for (int i = 0; i < argc; i++)
    {
        printf("argv[%d]=%s\n", i, argv[i]);
    }

    return 0;
}
