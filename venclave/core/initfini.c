// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "initfini.h"
#include "panic.h"

static bool _called_init;
static bool _called_constructor;
static bool _called_destructor;

__attribute__((constructor)) static void _constructor(void)
{
    _called_constructor = true;
}

__attribute__((destructor)) static void _destructor(void)
{
    _called_destructor = true;
}

void ve_call_init_functions(void)
{
    void (**fn)(void);
    extern void (*__init_array_start)(void);
    extern void (*__init_array_end)(void);

    for (fn = &__init_array_start; fn < &__init_array_end; fn++)
        (*fn)();

    /* Self-test for destructors. */
    if (!_called_constructor)
        ve_panic("_constructor() not called");

    _called_init = true;
}

void ve_call_fini_functions(void)
{
    if (_called_init)
    {
        void (**fn)(void);
        extern void (*__fini_array_start)(void);
        extern void (*__fini_array_end)(void);

        for (fn = &__fini_array_end - 1; fn >= &__fini_array_start; fn--)
            (*fn)();
    }

    /* Self-test for destructors. */
    if (!_called_destructor)
        ve_panic("_destructor() not called");
}
