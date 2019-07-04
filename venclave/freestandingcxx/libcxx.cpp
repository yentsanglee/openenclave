// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

extern int main(void);

extern "C" void __libc_csu_fini(void)
{
}

extern "C" void __libc_csu_init(void)
{
}

extern "C" void __libc_start_main(void)
{
    main();
}

extern "C" void __stack_chk_fail(void)
{
}
