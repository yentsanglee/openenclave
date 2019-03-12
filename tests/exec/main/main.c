// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <stdio.h>
#include <stdlib.h>

int main(int argc, const char* argv[])
{
    printf("main(): argc=%d\n", argc);

    for (int i = 0; i < argc; i++)
        printf("argv[%d]=%s\n", i, argv[i]);

    printf("main: done\n");

    return 0;
}
