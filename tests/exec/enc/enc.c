// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <assert.h>
#include <openenclave/enclave.h>
#include <openenclave/internal/calls.h>
#include <openenclave/internal/cpuid.h>
#include <openenclave/internal/elf.h>
#include <openenclave/internal/globals.h>
#include <openenclave/internal/jump.h>
#include <openenclave/internal/load.h>
#include <openenclave/internal/print.h>
#include <openenclave/internal/utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syscall.h>
#include "exec_t.h"

extern void (*oe_continue_execution_hook)(void);

typedef struct _elf_image
{
    elf64_t elf;
    uint8_t* image_base;
    size_t image_size;
    //    const elf64_ehdr_t* ehdr;
    uint64_t entry_rva;
    uint64_t text_rva;

    uint64_t tdata_rva;
    uint64_t tdata_size;
    uint64_t tdata_align;

    uint64_t tbss_rva;
    uint64_t tbss_size;
    uint64_t tbss_align;

    oe_elf_segment_t* segments;
    size_t num_segments;
} elf_image_t;

static int _test_elf_header(const elf64_ehdr_t* ehdr)
{
    if (!ehdr)
        return -1;

    if (ehdr->e_ident[EI_MAG0] != 0x7f)
        return -1;

    if (ehdr->e_ident[EI_MAG1] != 'E')
        return -1;

    if (ehdr->e_ident[EI_MAG2] != 'L')
        return -1;

    if (ehdr->e_ident[EI_MAG3] != 'F')
        return -1;

    if (ehdr->e_ident[EI_CLASS] != ELFCLASS64)
        return -1;

    if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB)
        return -1;

    if (ehdr->e_machine != EM_X86_64)
        return -1;

    if (ehdr->e_ehsize != sizeof(elf64_ehdr_t))
        return -1;

    if (ehdr->e_phentsize != sizeof(elf64_phdr_t))
        return -1;

    if (ehdr->e_shentsize != sizeof(elf64_shdr_t))
        return -1;

    /* If there is no section header table, then the index should be 0. */
    if (ehdr->e_shnum == 0 && ehdr->e_shstrndx != 0)
        return -1;

    /* If there is a section header table, then the index shouldn't overrun. */
    if (ehdr->e_shnum > 0 && ehdr->e_shstrndx >= ehdr->e_shnum)
        return -1;

    return 0;
}

static int _elf64_init(elf64_t* elf, const void* data, size_t size)
{
    int rc = -1;

    if (elf)
        memset(elf, 0, sizeof(elf64_t));

    if (!data || !size || !elf)
        goto done;

    if (_test_elf_header((elf64_ehdr_t*)data) != 0)
        goto done;

    elf->data = (void*)data;
    elf->size = size;
    elf->magic = ELF_MAGIC;

    rc = 0;

done:

    if (rc != 0)
    {
        free(elf->data);
        memset(elf, 0, sizeof(elf64_t));
    }

    return rc;
}

static int _qsort_compare_segments(const void* s1, const void* s2)
{
    const oe_elf_segment_t* seg1 = (const oe_elf_segment_t*)s1;
    const oe_elf_segment_t* seg2 = (const oe_elf_segment_t*)s2;

    return (int)(seg1->vaddr - seg2->vaddr);
}

static void _free_elf_image(elf_image_t* image)
{
    if (image)
    {
        if (image->image_base)
            free(image->image_base);

        if (image->segments)
            free(image->segments);

        memset(image, 0, sizeof(elf_image_t));
    }
}

static int _load_elf_image(elf_image_t* image, const uint8_t* data, size_t size)
{
    int ret = -1;
    size_t i;
    const elf64_ehdr_t* eh;
    size_t num_segments;

    memset(image, 0, sizeof(elf_image_t));

    if (_elf64_init(&image->elf, data, size) != 0)
        goto done;

    eh = (elf64_ehdr_t*)image->elf.data;

    /* Fail if not Intel X86 64-bit */
    if (eh->e_machine != EM_X86_64)
        goto done;

    /* Fail if image is relocatable */
    if (eh->e_type == ET_REL)
        goto done;

    /* Save the header. */
    image->entry_rva = eh->e_entry;

    /* Find the addresses of various segments. */
    for (i = 0; i < eh->e_shnum; i++)
    {
        const elf64_shdr_t* sh;
        const char* name;

        if (!(sh = elf64_get_section_header(&image->elf, i)))
            goto done;

        if ((name = elf64_get_string_from_shstrtab(&image->elf, sh->sh_name)))
        {
            if (strcmp(name, ".text") == 0)
            {
                image->text_rva = sh->sh_addr;
            }
            else if (strcmp(name, ".tdata") == 0)
            {
                image->tdata_rva = sh->sh_addr;
                image->tdata_size = sh->sh_size;
                image->tdata_align = sh->sh_addralign;
            }
            else if (strcmp(name, ".tbss") == 0)
            {
                image->tbss_rva = sh->sh_addr;
                image->tbss_size = sh->sh_size;
                image->tbss_align = sh->sh_addralign;
            }
        }
    }

    /* Fail if required sections not found */
    // if (!image->text_rva || !image->tdata_rva || !image->tbss_rva)
    if (!image->text_rva)
        goto done;

    /* Find out the image size and number of segments to be loaded */
    {
        uint64_t lo = 0xFFFFFFFFFFFFFFFF; /* lowest address of all segments */
        uint64_t hi = 0;                  /* highest address of all segments */

        for (i = 0; i < eh->e_phnum; i++)
        {
            const elf64_phdr_t* ph;

            if (!(ph = elf64_get_program_header(&image->elf, i)))
                goto done;

            if (ph->p_filesz > ph->p_memsz)
                goto done;

            switch (ph->p_type)
            {
                case PT_LOAD:
                {
                    if (lo > ph->p_vaddr)
                        lo = ph->p_vaddr;

                    if (hi < ph->p_vaddr + ph->p_memsz)
                        hi = ph->p_vaddr + ph->p_memsz;

                    image->num_segments++;
                    break;
                }
                default:
                {
                    break;
                }
            }
        }

        /* Fail if LO not found */
        if (lo != 0)
            goto done;

        /* Fail if HI not found */
        if (hi == 0)
            goto done;

        /* Fail if no segment found */
        if (image->num_segments == 0)
            goto done;

        /* Calculate the full size of the image (rounded up to the page size) */
        image->image_size = oe_round_up_to_page_size(hi - lo);
    }

    /* allocate segments */
    {
        image->segments = (oe_elf_segment_t*)calloc(
            image->num_segments, sizeof(oe_elf_segment_t));

        if (!image->segments)
        {
            goto done;
        }
    }

    /* Allocate image on a page boundary */
    {
        image->image_base = (uint8_t*)memalign(OE_PAGE_SIZE, image->image_size);
        if (!image->image_base)
        {
            goto done;
        }
    }

    /* Clear the image memory */
    memset(image->image_base, 0, image->image_size);

    /* Add all loadable program segments to SEGMENTS array */
    for (i = 0, num_segments = 0; i < eh->e_phnum; i++)
    {
        const elf64_phdr_t* ph = elf64_get_program_header(&image->elf, i);
        oe_elf_segment_t* seg = &image->segments[num_segments];
        void* segdata;

        assert(ph);
        assert(ph->p_filesz <= ph->p_memsz);

        if (ph->p_type == PT_TLS)
        {
            if (image->tdata_rva != ph->p_vaddr)
            {
                if (image->tdata_rva != 0)
                {
                    goto done;
                }
            }

            if (image->tdata_size != ph->p_filesz)
            {
                goto done;
            }

            continue;
        }

        /* Skip non-loadable program segments */
        if (ph->p_type != PT_LOAD)
            continue;

        /* Set oe_elf_segment_t.memsz */
        seg->memsz = ph->p_memsz;

        /* Set oe_elf_segment_t.filesz, IS THIS FIELD NEEDED??? */
        seg->filesz = ph->p_filesz;

        /* Set oe_elf_segment_t.offset. IS THIS FIELD NEEDED??? */
        seg->offset = ph->p_offset;

        /* Set oe_elf_segment_t.vaddr */
        seg->vaddr = ph->p_vaddr;

        /* Set oe_elf_segment_t.flags */
        {
            if (ph->p_flags & PF_R)
                seg->flags |= OE_SEGMENT_FLAG_READ;

            if (ph->p_flags & PF_W)
                seg->flags |= OE_SEGMENT_FLAG_WRITE;

            if (ph->p_flags & PF_X)
                seg->flags |= OE_SEGMENT_FLAG_EXEC;
        }

        /* Set oe_elf_segment_t.filedata  IS THE FIELD NEEDED??? */
        seg->filedata = (unsigned char*)image->elf.data + seg->offset;

        /* Should we fail if elf64_get_segment failed??? */
        segdata = elf64_get_segment(&image->elf, i);
        if (segdata)
        {
            /* copy the segment to image */
            memcpy(image->image_base + seg->vaddr, segdata, seg->filesz);
        }

        num_segments++;
    }

    assert(num_segments == image->num_segments);

    /* sort the segment by vaddr. */
    qsort(
        image->segments,
        num_segments,
        sizeof(oe_elf_segment_t),
        _qsort_compare_segments);

    /* validate segments are valid */
    for (i = 0; i < image->num_segments - 1; i++)
    {
        const oe_elf_segment_t* seg = &image->segments[i];
        const oe_elf_segment_t* seg_next = &image->segments[i + 1];
        uint64_t rounded_vaddr = oe_round_down_to_page_size(seg_next->vaddr);

        if ((seg->vaddr + seg->memsz) > rounded_vaddr)
            goto done;
    }

    ret = 0;

done:

    if (ret != 0)
        _free_elf_image(image);

    return ret;
}

int _add_segment_pages(
    uint8_t* image_base,
    size_t image_size,
    oe_elf_segment_t* segment,
    uint8_t* exec_base,
    size_t exec_size)
{
    int result = -1;
    uint64_t page_rva;
    uint64_t segment_end;
    const uint8_t* image_end = image_base + image_size;
    const uint8_t* exec_end = exec_base + exec_size;

    page_rva = oe_round_down_to_page_size(segment->vaddr);
    segment_end = segment->vaddr + segment->memsz;

    for (; page_rva < segment_end; page_rva += OE_PAGE_SIZE)
    {
        uint8_t* dest = exec_base + page_rva;
        const uint8_t* src = image_base + page_rva;

        if (!(dest >= exec_base && dest <= exec_end + OE_PAGE_SIZE))
            goto done;

        if (!(src >= image_base && src <= image_end + OE_PAGE_SIZE))
            goto done;

        memcpy(dest, src, OE_PAGE_SIZE);
    }

    result = 0;

done:
    return result;
}

static int _add_pages(elf_image_t* image, uint8_t* exec_base, size_t exec_size)
{
    int ret = -1;
    size_t i;

    for (i = 0; i < image->num_segments; i++)
    {
        if (_add_segment_pages(
                image->image_base,
                image->image_size,
                &image->segments[i],
                exec_base,
                exec_size) != 0)
        {
            goto done;
        }
    }

    ret = 0;

done:
    return ret;
}

typedef void (*entry_proc)();

#define SYSCALL_OPCODE 0x050F

typedef struct _syscall_args
{
    long num;
    long arg1;
    long arg2;
    long arg3;
    long arg4;
    long arg5;
    long arg6;
} syscall_args_t;

static syscall_args_t _args;

long handle_syscall(void)
{
    if (_args.num == SYS_write)
    {
        oe_host_write(1, (const char*)_args.arg2, (size_t)_args.arg3);
        return 0;
    }

    return -1;
}

static uint64_t _exception_handler(oe_exception_record_t* exception)
{
    oe_context_t* context = exception->context;

    if (exception->code == OE_EXCEPTION_ILLEGAL_INSTRUCTION)
    {
        uint64_t opcode = *((uint16_t*)context->rip);

        if (opcode == SYSCALL_OPCODE)
        {
            /* Extract the syscall arguments. */
            _args.num = (long)context->rax;
            _args.arg1 = (long)context->rdi;
            _args.arg2 = (long)context->rsi;
            _args.arg3 = (long)context->rdx;
            _args.arg4 = (long)context->r10;
            _args.arg5 = (long)context->r8;
            _args.arg6 = (long)context->r9;

#if 0
            assert(_args.num == 99);
            assert(_args.arg1 == 10);
            assert(_args.arg2 == 20);
            assert(_args.arg3 == 30);
            assert(_args.arg4 == 40);
            assert(_args.arg5 == 50);
            assert(_args.arg6 == 60);
#endif

            context->rip += 2;

            return OE_EXCEPTION_CONTINUE_EXECUTION;
        }
    }

    return OE_EXCEPTION_CONTINUE_SEARCH;
}

static void _continue_execution_hook(void)
{
    handle_syscall();
}

void test_exec(const uint8_t* image_base, size_t image_size)
{
    uint8_t* exec_base = (uint8_t*)__oe_get_exec_base();
    size_t exec_size = __oe_get_exec_size();
    elf_image_t image;
    entry_proc entry;
    oe_result_t r;

    oe_continue_execution_hook = _continue_execution_hook;

    r = oe_add_vectored_exception_handler(false, _exception_handler);
    if (r != OE_OK)
    {
        fprintf(stderr, "oe_add_vectored_exception_handler() failed\n");
        abort();
    }

    // asm volatile("syscall");

    if (image_size > exec_size)
    {
        fprintf(stderr, "program too big\n");
        abort();
    }

    if (_load_elf_image(&image, image_base, image_size) != 0)
    {
        fprintf(stderr, "_load_elf_image() failed\n");
        abort();
    }

    if (_add_pages(&image, exec_base, exec_size) != 0)
    {
        fprintf(stderr, "_add_pages() failed\n");
        abort();
    }

    entry = (entry_proc)(exec_base + image.entry_rva);

    printf("<<<<<<<<<<<\n");
    entry();
    printf(">>>>>>>>>>>\n");

    _free_elf_image(&image);
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* AllowDebug */
    4096, /* HeapPageCount */
    4096, /* StackPageCount */
    8);   /* TCSCount */
