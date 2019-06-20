// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

void ve_call_init_functions(void)
{
    void (**fn)(void);
    extern void (*__init_array_start)(void);
    extern void (*__init_array_end)(void);

    for (fn = &__init_array_start; fn < &__init_array_end; fn++)
        (*fn)();
}

void __libc_csu_init(void)
{
    ve_call_init_functions();
}
