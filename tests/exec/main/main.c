// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <stdio.h>
#include <string.h>

char buf[1024] = "hello";

int main()
{
    return 99;
}

int foo()
{
    return (int)strlen(buf);
}
