#include <string.h>
#include <stdlib.h>
#include "kernel32.h"

void* malloc(size_t size)
{
    return LocalAlloc(0, size);
}

void free(void* ptr)
{
    LocalFree(ptr);
}

void* realloc(void* ptr, size_t size)
{
    return LocalReAlloc(ptr, size, 0);
}

void* calloc(size_t nmemb, size_t size)
{
    void* ptr = malloc(nmemb * size);

    if (ptr)
        memset(ptr, 0, nmemb * size);

    return ptr;
}
