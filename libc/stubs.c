// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

#if 1

void __strdup()
{
    assert("__strdup()" == NULL);
    abort();
}

void __strcpy_chk()
{
    assert("__strcpy_chk()" == NULL);
    abort();
}

void __xstat()
{
    assert("__xstat()" == NULL);
    abort();
}

void pthread_atfork()
{
    assert("pthread_atfork()" == NULL);
    abort();
}

void fstat()
{
    assert("fstat()" == NULL);
    abort();
}

void dlopen()
{
    assert("dlopen()" == NULL);
    abort();
}

void dlsym()
{
    assert("dlsym()" == NULL);
    abort();
}

void dlclose()
{
    assert("dlclose()" == NULL);
    abort();
}

void dlerror()
{
    assert("dlerror()" == NULL);
    abort();
}

void __lookup_name()
{
    assert("__lookup_name()" == NULL);
    abort();
}

void pthread_setcancelstate()
{
    assert("pthread_setcancelstate()" == NULL);
    abort();
}

void _pthread_cleanup_push()
{
    assert("_pthread_cleanup_push()" == NULL);
    abort();
}

void _pthread_cleanup_pop()
{
    assert("_pthread_cleanup_pop()" == NULL);
    abort();
}

__thread int __h_error;

int* __h_errno_location()
{
    assert("__h_errno_location()" == NULL);
    abort();
    return &__h_error;
}

#endif
