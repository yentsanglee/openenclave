// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

extern int main(void);

void __libc_csu_fini(void)
{
}

void __libc_csu_init(void)
{
}

void __libc_start_main(void)
{
    main();
}
