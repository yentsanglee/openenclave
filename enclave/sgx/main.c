// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#if defined(OE_BUILD_VENCLAVE)

int main(void)
{
    extern const void* oe_link_enclave(void);
    extern int oe_main(void);

    oe_link_enclave();
    return oe_main();
}

#endif /* defined(OE_BUILD_VENCLAVE) */
