// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <stdio.h>
#include <stdlib.h>

static int init;

__attribute__((constructor)) void foo()
{
    init = 1;
}

int main(int argc, const char* argv[])
{
    //_call_inits();

    printf("main(): argc=%d\n", argc);

    for (int i = 0; i < argc; i++)
        printf("argv[%d]=%s\n", i, argv[i]);

    printf("main: done\n");
    exit(0);
}
