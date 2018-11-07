#include <stdio.h>
#include "kernel32.h"
#include "oelibc.h"

int write_stderr(const char* data, size_t size)
{
    HANDLE handle;
    DWORD n;

    if (!(handle = GetStdHandle(STD_ERROR_HANDLE)))
        return -1;

    if (!WriteFile(handle, data, (DWORD)size, &n, NULL))
        return -1;

    return 0;
}
