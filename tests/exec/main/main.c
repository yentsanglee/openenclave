// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <stdio.h>
#include <stdlib.h>

int x = 1;

__attribute__((__constructor__)) void foo(void)
{
    abort();
    x = 99;
}

int main(int argc, const char* argv[])
{
    printf("Hello world!\n");
    exit(x);
}
