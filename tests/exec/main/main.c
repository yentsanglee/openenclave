// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <stdio.h>
#include <stdlib.h>

static int initialized = 0;

__attribute__((constructor)) void constructor(void)
{
    initialized = 1;
}

int main(int argc, const char* argv[])
{
    printf("main(): argc=%d\n", argc);

    printf("initialized=%d\n", initialized);

    for (int i = 0; i < argc; i++)
        printf("argv[%d]=%s\n", i, argv[i]);

    printf("main: done\n");
    exit(0);
}
