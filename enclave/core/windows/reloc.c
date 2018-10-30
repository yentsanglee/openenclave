// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/internal/globals.h>

/*
 * Based relocation format.
 *
 * N.B. Including "Windows.h" in Enclave core causes all kinds of 
 *      compiler error. Since we only care about base-reloc 
 *      information, define it here.
 *      The structure image_base_reloc_t must layout exactly as
 *      IMAGE_BASE_RELOCATION, and IMAGE_REL_BASED_DIR64 must match
 *      the value in Winnt.h.
 */
typedef struct _image_base_reloc_t
{
    uint32_t reloc_block_rva;
    uint32_t reloc_block_size;
    uint16_t reloc_data[0];
} image_base_reloc_t;

#define IMAGE_REL_BASED_DIR64	10

/*
 *  _reloc_bias is used to calculate relocation difference *BEFORE*
 *  relocation.
 *
 *  Since _reloc_bias hasn't been relocated, it contains the original value.
 *  Therefore, before relocation,
 *      relocation_diff = (uint64_t)&_reloc_bias - _reloc_bias;
 *
 *  because &_reloc_bias is pc-relative and will be the post-relocation
 *  value.
 */
static volatile uint64_t _reloc_bias = (uint64_t)&_reloc_bias;

static uint64_t _next_reloc_block(uint64_t reloc_addr)
{
    const image_base_reloc_t* reloc = (const image_base_reloc_t*)reloc_addr;
    return (reloc_addr + reloc->reloc_block_size);
}

static bool _relocate_one_block(
    uint64_t image_base,
    uint64_t reloc_block_addr,
    uint64_t reloc_diff)
{
    image_base_reloc_t* reloc_block = (image_base_reloc_t*)reloc_block_addr;
    uint16_t* reloc_data = reloc_block->reloc_data;
    uint16_t* reloc_end = (uint16_t*)_next_reloc_block(reloc_block_addr);
    uint64_t block_base_addr = image_base + reloc_block->reloc_block_rva;
    bool result = true;

    for (; reloc_data < reloc_end; reloc_data++)
    {
        uint64_t fixup_addr = block_base_addr + (*reloc_data & 0xfff);

        /* All relocation must be 64-bit relocation */
        if (IMAGE_REL_BASED_DIR64 != (*reloc_data >> 12))
        {
            result = false;
            break;
        }

        *(uint64_t __unaligned*)fixup_addr += reloc_diff;
    }
    return result;
}

/*
**==============================================================================
**
** oe_apply_relocations()
**
**     Apply relocations from PE Enclave image.
**
**==============================================================================
*/

bool oe_apply_relocations(void)
{
    uint64_t image_base = (uint64_t)__oe_get_enclave_base();
    uint64_t reloc_addr = (uint64_t)__oe_get_reloc_base();
    uint64_t reloc_end = reloc_addr + __oe_get_reloc_size();
    uint64_t reloc_diff = (uint64_t)&_reloc_bias - _reloc_bias;
    bool result = true;

    while (reloc_addr < reloc_end)
    {
        result = _relocate_one_block(image_base, reloc_addr, reloc_diff);
        if (!result)
        {
            break;
        }
        reloc_addr = _next_reloc_block(reloc_addr);
    }

    /*
     *  _reloc_bias must be relocated after relocation. Therefore it must
     *  be equal to its address.
     */

    return result && ((uint64_t)&_reloc_bias == _reloc_bias);
}
