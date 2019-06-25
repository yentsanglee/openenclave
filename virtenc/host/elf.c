// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "elf.h"
#include <openenclave/internal/trace.h>
#include <stdio.h>

void oe_log(log_level_t level, const char* fmt, ...)
{
    (void)level;
    (void)fmt;
}

static int oe_fopen(FILE** fp, const char* path, const char* mode)
{
    if (!fp)
        return -1;

    if ((*fp = fopen(path, mode)) == NULL)
        return -1;

    return 0;
}

#include "../../host/sgx/elf.c"

int ve_get_elf_info(const char* path, ve_elf_info_t* buf)
{
    int ret = -1;
    elf64_t elf;
    const elf64_ehdr_t* ehdr;
    size_t i;

    if (buf)
        memset(buf, 0, sizeof(ve_elf_info_t));

    if (!path || !buf)
        goto done;

    /* Load the ELF image into memory. */
    if (elf64_load(path, &elf) != 0)
        goto done;

    /* Get a pointer to the elf header. */
    if (!(ehdr = (const elf64_ehdr_t*)elf.data))
        goto done;

    /* Fail if not Intel X86 64-bit */
    if (ehdr->e_machine != EM_X86_64)
        goto done;

    /* Get information from the .tdata and .tbss sections. */
    for (i = 0; i < ehdr->e_shnum; i++)
    {
        const elf64_shdr_t* shdr;
        const char* name;

        if (!(shdr = elf64_get_section_header(&elf, i)))
            goto done;

        if (!(name = elf64_get_string_from_shstrtab(&elf, shdr->sh_name)))
            continue;

        if (strcmp(name, ".tdata") == 0)
        {
            buf->tdata_rva = shdr->sh_addr;
            buf->tdata_size = shdr->sh_size;
            buf->tdata_align = shdr->sh_addralign;
        }
        else if (strcmp(name, ".tbss") == 0)
        {
            buf->tbss_rva = shdr->sh_addr;
            buf->tbss_size = shdr->sh_size;
            buf->tbss_align = shdr->sh_addralign;
        }
    }

    /* Find the relative-virtual address of the __ve_rva. */
    {
        elf64_sym_t sym = {0};

        if (elf64_find_symbol_by_name(&elf, "__ve_self", &sym) != 0)
            goto done;

        printf("SELF.RVA=%lu\n", sym.st_value);
        buf->self_rva = sym.st_value;
    }

    ret = 0;

done:

    if (elf.data)
        elf64_unload(&elf);

    return ret;
}
