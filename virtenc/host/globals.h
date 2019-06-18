#ifndef _VE_HOST_GLOBALS_H
#define _VE_HOST_GLOBALS_H

#include <stddef.h>

#define MAX_THREADS 1024

typedef struct _thread_info
{
    int sock;
    uint32_t tcs;
} thread_info_t;

typedef struct _globals
{
    int sock;
    thread_info_t threads[MAX_THREADS];
    size_t num_threads;
} globals_t;

extern globals_t globals;

#endif /* _VE_HOST_GLOBALS_H */
