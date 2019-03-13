// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <openenclave/corelibc/ctype.h>
#include <openenclave/corelibc/linux/futex.h>
#include <openenclave/corelibc/stdio.h>
#include <openenclave/corelibc/sys/socket.h>
#include <openenclave/enclave.h>
#include <openenclave/internal/calls.h>
#include <openenclave/internal/cpuid.h>
#include <openenclave/internal/elf.h>
#include <openenclave/internal/globals.h>
#include <openenclave/internal/jump.h>
#include <openenclave/internal/load.h>
#include <openenclave/internal/print.h>
#include <openenclave/internal/time.h>
#include <openenclave/internal/utils.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <syscall.h>
#include <time.h>
#include <unistd.h>
#include "exec_t.h"

extern void oe_register_sgxfs_device(void);

extern long (*oe_continue_execution_hook)(long ret);

void MEMSET(void* s, unsigned char c, size_t n)
{
    uint8_t* p = (uint8_t*)s;

    while (n--)
        *p++ = c;
}

typedef struct _elf_image
{
    elf64_t elf;
    uint64_t image_offset;
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

int __test_elf_header(const elf64_ehdr_t* ehdr)
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
        MEMSET(elf, 0, sizeof(elf64_t));

    if (!data || !size || !elf)
        goto done;

    if (__test_elf_header((elf64_ehdr_t*)data) != 0)
        goto done;

    elf->data = (void*)data;
    elf->size = size;
    elf->magic = ELF_MAGIC;

    rc = 0;

done:

    if (rc != 0)
    {
        free(elf->data);
        MEMSET(elf, 0, sizeof(elf64_t));
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
        if (image->segments)
            free(image->segments);

        MEMSET(image, 0, sizeof(elf_image_t));
    }
}

int __load_elf_image(
    elf_image_t* image,
    const uint8_t* data,
    size_t size,
    uint8_t* exec_base,
    size_t exec_size)
{
    int ret = -1;
    size_t i;
    const elf64_ehdr_t* eh;
    size_t num_segments;

    MEMSET(image, 0, sizeof(elf_image_t));

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
    uint64_t lo = 0xFFFFFFFFFFFFFFFF; /* lowest address of all segments */
    uint64_t hi = 0;                  /* highest address of all segments */
    {
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
        if (lo == 0xFFFFFFFFFFFFFFFF)
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

    /* Save the image offset. */
    image->image_offset = lo;

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
        if (image->image_size > exec_size)
        {
            goto done;
        }

        image->image_base = (uint8_t*)exec_base;

        if (!image->image_base)
        {
            goto done;
        }
    }

    MEMSET(image->image_base, 0, image->image_size);

    /* Clear the image memory */

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
            assert(seg->vaddr >= image->image_offset);
            uint64_t off = seg->vaddr - image->image_offset;
            memcpy(image->image_base + off, segdata, seg->filesz);

            uint8_t* end = image->image_base + off + seg->filesz;
            assert(end < image->image_base + (uint64_t)image->image_size);
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

int __relocate_symbols(elf_image_t* image, uint8_t* exec_base, size_t exec_size)
{
    int ret = -1;
    void* data = NULL;
    size_t size;
    const elf64_rela_t* p;
    size_t n;

    (void)exec_base;
    (void)exec_size;

    if (elf64_load_relocations(&image->elf, &data, &size) != 0)
        goto done;

    p = (const elf64_rela_t*)data;
    n = size / sizeof(elf64_rela_t);

    for (size_t i = 0; i < n; i++, p++)
    {
        if (p->r_offset == 0)
            break;

        if (p->r_offset < image->image_offset)
            goto done;

        uint64_t off = p->r_offset - image->image_offset;
        uint64_t* dest = (uint64_t*)(exec_base + off);
        uint64_t reloc_type = ELF64_R_TYPE(p->r_info);

        /* Relocate the reference */
        if (reloc_type == R_X86_64_RELATIVE)
        {
            if (p->r_addend < (int64_t)image->image_offset)
                goto done;

            int64_t add = p->r_addend - (int64_t)image->image_offset;
            *dest = (uint64_t)(exec_base + add);
#if 0
            printf(
                "Applied relocation: offset=%llu addend==%lld\n",
                p->r_offset,
                p->r_addend);
#endif
        }
        else
        {
#if 0
            printf(
                "Skipped relocation: offset=%llu addend==%lld\n",
                p->r_offset,
                p->r_addend);
#endif
        }
    }

    ret = 0;

done:

    if (data)
        free(data);

    return ret;
}

typedef void (*entry_proc)();

#define SYSCALL_OPCODE 0x050F

static oe_jmpbuf_t _jmp_buf;
static int _exit_status = INT_MAX;

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

static int _syscall_arch_prctl(int code, unsigned long addr)
{
    static const int ARCH_SET_FS = 0x1002;
    static const int ARCH_GET_FS = 0x1003;
    static const int ARCH_SET_GS = 0x1001;
    static const int ARCH_GET_GS = 0x1004;
    static __thread void* _gs_base = NULL;
    static __thread void* _fs_base = NULL;

    (void)addr;

    switch (code)
    {
        case ARCH_SET_GS:
        {
            _gs_base = (void*)addr;
            return 0;
        }
        case ARCH_GET_GS:
        {
            *((void**)addr) = _gs_base;
            return 0;
        }
        case ARCH_SET_FS:
        {
            _fs_base = (void*)addr;
            return 0;
        }
        case ARCH_GET_FS:
        {
            *((void**)addr) = _gs_base;
            return 0;
        }
        default:
        {
            printf("_syscall_arch_prctl() failed\n");
            oe_abort();
            return -1;
        }
    }

    return 0;
}

static long _syscall_set_tid_address(int* tidptr)
{
    static __thread int* _clear_child_tid = NULL;

    if (!tidptr)
        return -1;

    _clear_child_tid = tidptr;

    return (long)tidptr;
}

static long _syscall_exit_group(int status)
{
    _exit_status = status;
    oe_longjmp(&_jmp_buf, 1);

    /* Unrechable. */
    abort();
    return 0;
}

static const char* _syscall_name(long num)
{
    switch (num)
    {
        case SYS_read:
            return "SYS_read";
        case SYS_write:
            return "SYS_write";
        case SYS_open:
            return "SYS_open";
        case SYS_close:
            return "SYS_close";
        case SYS_stat:
            return "SYS_stat";
        case SYS_fstat:
            return "SYS_fstat";
        case SYS_lstat:
            return "SYS_lstat";
        case SYS_poll:
            return "SYS_poll";
        case SYS_lseek:
            return "SYS_lseek";
        case SYS_mmap:
            return "SYS_mmap";
        case SYS_mprotect:
            return "SYS_mprotect";
        case SYS_munmap:
            return "SYS_munmap";
        case SYS_brk:
            return "SYS_brk";
        case SYS_rt_sigaction:
            return "SYS_rt_sigaction";
        case SYS_rt_sigprocmask:
            return "SYS_rt_sigprocmask";
        case SYS_rt_sigreturn:
            return "SYS_rt_sigreturn";
        case SYS_ioctl:
            return "SYS_ioctl";
        case SYS_pread64:
            return "SYS_pread64";
        case SYS_pwrite64:
            return "SYS_pwrite64";
        case SYS_readv:
            return "SYS_readv";
        case SYS_writev:
            return "SYS_writev";
        case SYS_access:
            return "SYS_access";
        case SYS_pipe:
            return "SYS_pipe";
        case SYS_select:
            return "SYS_select";
        case SYS_sched_yield:
            return "SYS_sched_yield";
        case SYS_mremap:
            return "SYS_mremap";
        case SYS_msync:
            return "SYS_msync";
        case SYS_mincore:
            return "SYS_mincore";
        case SYS_madvise:
            return "SYS_madvise";
        case SYS_shmget:
            return "SYS_shmget";
        case SYS_shmat:
            return "SYS_shmat";
        case SYS_shmctl:
            return "SYS_shmctl";
        case SYS_dup:
            return "SYS_dup";
        case SYS_dup2:
            return "SYS_dup2";
        case SYS_pause:
            return "SYS_pause";
        case SYS_nanosleep:
            return "SYS_nanosleep";
        case SYS_getitimer:
            return "SYS_getitimer";
        case SYS_alarm:
            return "SYS_alarm";
        case SYS_setitimer:
            return "SYS_setitimer";
        case SYS_getpid:
            return "SYS_getpid";
        case SYS_sendfile:
            return "SYS_sendfile";
        case SYS_socket:
            return "SYS_socket";
        case SYS_connect:
            return "SYS_connect";
        case SYS_accept:
            return "SYS_accept";
        case SYS_sendto:
            return "SYS_sendto";
        case SYS_recvfrom:
            return "SYS_recvfrom";
        case SYS_sendmsg:
            return "SYS_sendmsg";
        case SYS_recvmsg:
            return "SYS_recvmsg";
        case SYS_shutdown:
            return "SYS_shutdown";
        case SYS_bind:
            return "SYS_bind";
        case SYS_listen:
            return "SYS_listen";
        case SYS_getsockname:
            return "SYS_getsockname";
        case SYS_getpeername:
            return "SYS_getpeername";
        case SYS_socketpair:
            return "SYS_socketpair";
        case SYS_setsockopt:
            return "SYS_setsockopt";
        case SYS_getsockopt:
            return "SYS_getsockopt";
        case SYS_clone:
            return "SYS_clone";
        case SYS_fork:
            return "SYS_fork";
        case SYS_vfork:
            return "SYS_vfork";
        case SYS_execve:
            return "SYS_execve";
        case SYS_exit:
            return "SYS_exit";
        case SYS_wait4:
            return "SYS_wait4";
        case SYS_kill:
            return "SYS_kill";
        case SYS_uname:
            return "SYS_uname";
        case SYS_semget:
            return "SYS_semget";
        case SYS_semop:
            return "SYS_semop";
        case SYS_semctl:
            return "SYS_semctl";
        case SYS_shmdt:
            return "SYS_shmdt";
        case SYS_msgget:
            return "SYS_msgget";
        case SYS_msgsnd:
            return "SYS_msgsnd";
        case SYS_msgrcv:
            return "SYS_msgrcv";
        case SYS_msgctl:
            return "SYS_msgctl";
        case SYS_fcntl:
            return "SYS_fcntl";
        case SYS_flock:
            return "SYS_flock";
        case SYS_fsync:
            return "SYS_fsync";
        case SYS_fdatasync:
            return "SYS_fdatasync";
        case SYS_truncate:
            return "SYS_truncate";
        case SYS_ftruncate:
            return "SYS_ftruncate";
        case SYS_getdents:
            return "SYS_getdents";
        case SYS_getcwd:
            return "SYS_getcwd";
        case SYS_chdir:
            return "SYS_chdir";
        case SYS_fchdir:
            return "SYS_fchdir";
        case SYS_rename:
            return "SYS_rename";
        case SYS_mkdir:
            return "SYS_mkdir";
        case SYS_rmdir:
            return "SYS_rmdir";
        case SYS_creat:
            return "SYS_creat";
        case SYS_link:
            return "SYS_link";
        case SYS_unlink:
            return "SYS_unlink";
        case SYS_symlink:
            return "SYS_symlink";
        case SYS_readlink:
            return "SYS_readlink";
        case SYS_chmod:
            return "SYS_chmod";
        case SYS_fchmod:
            return "SYS_fchmod";
        case SYS_chown:
            return "SYS_chown";
        case SYS_fchown:
            return "SYS_fchown";
        case SYS_lchown:
            return "SYS_lchown";
        case SYS_umask:
            return "SYS_umask";
        case SYS_gettimeofday:
            return "SYS_gettimeofday";
        case SYS_getrlimit:
            return "SYS_getrlimit";
        case SYS_getrusage:
            return "SYS_getrusage";
        case SYS_sysinfo:
            return "SYS_sysinfo";
        case SYS_times:
            return "SYS_times";
        case SYS_ptrace:
            return "SYS_ptrace";
        case SYS_getuid:
            return "SYS_getuid";
        case SYS_syslog:
            return "SYS_syslog";
        case SYS_getgid:
            return "SYS_getgid";
        case SYS_setuid:
            return "SYS_setuid";
        case SYS_setgid:
            return "SYS_setgid";
        case SYS_geteuid:
            return "SYS_geteuid";
        case SYS_getegid:
            return "SYS_getegid";
        case SYS_setpgid:
            return "SYS_setpgid";
        case SYS_getppid:
            return "SYS_getppid";
        case SYS_getpgrp:
            return "SYS_getpgrp";
        case SYS_setsid:
            return "SYS_setsid";
        case SYS_setreuid:
            return "SYS_setreuid";
        case SYS_setregid:
            return "SYS_setregid";
        case SYS_getgroups:
            return "SYS_getgroups";
        case SYS_setgroups:
            return "SYS_setgroups";
        case SYS_setresuid:
            return "SYS_setresuid";
        case SYS_getresuid:
            return "SYS_getresuid";
        case SYS_setresgid:
            return "SYS_setresgid";
        case SYS_getresgid:
            return "SYS_getresgid";
        case SYS_getpgid:
            return "SYS_getpgid";
        case SYS_setfsuid:
            return "SYS_setfsuid";
        case SYS_setfsgid:
            return "SYS_setfsgid";
        case SYS_getsid:
            return "SYS_getsid";
        case SYS_capget:
            return "SYS_capget";
        case SYS_capset:
            return "SYS_capset";
        case SYS_rt_sigpending:
            return "SYS_rt_sigpending";
        case SYS_rt_sigtimedwait:
            return "SYS_rt_sigtimedwait";
        case SYS_rt_sigqueueinfo:
            return "SYS_rt_sigqueueinfo";
        case SYS_rt_sigsuspend:
            return "SYS_rt_sigsuspend";
        case SYS_sigaltstack:
            return "SYS_sigaltstack";
        case SYS_utime:
            return "SYS_utime";
        case SYS_mknod:
            return "SYS_mknod";
        case SYS_uselib:
            return "SYS_uselib";
        case SYS_personality:
            return "SYS_personality";
        case SYS_ustat:
            return "SYS_ustat";
        case SYS_statfs:
            return "SYS_statfs";
        case SYS_fstatfs:
            return "SYS_fstatfs";
        case SYS_sysfs:
            return "SYS_sysfs";
        case SYS_getpriority:
            return "SYS_getpriority";
        case SYS_setpriority:
            return "SYS_setpriority";
        case SYS_sched_setparam:
            return "SYS_sched_setparam";
        case SYS_sched_getparam:
            return "SYS_sched_getparam";
        case SYS_sched_setscheduler:
            return "SYS_sched_setscheduler";
        case SYS_sched_getscheduler:
            return "SYS_sched_getscheduler";
        case SYS_sched_get_priority_max:
            return "SYS_sched_get_priority_max";
        case SYS_sched_get_priority_min:
            return "SYS_sched_get_priority_min";
        case SYS_sched_rr_get_interval:
            return "SYS_sched_rr_get_interval";
        case SYS_mlock:
            return "SYS_mlock";
        case SYS_munlock:
            return "SYS_munlock";
        case SYS_mlockall:
            return "SYS_mlockall";
        case SYS_munlockall:
            return "SYS_munlockall";
        case SYS_vhangup:
            return "SYS_vhangup";
        case SYS_modify_ldt:
            return "SYS_modify_ldt";
        case SYS_pivot_root:
            return "SYS_pivot_root";
        case SYS__sysctl:
            return "SYS__sysctl";
        case SYS_prctl:
            return "SYS_prctl";
        case SYS_arch_prctl:
            return "SYS_arch_prctl";
        case SYS_adjtimex:
            return "SYS_adjtimex";
        case SYS_setrlimit:
            return "SYS_setrlimit";
        case SYS_chroot:
            return "SYS_chroot";
        case SYS_sync:
            return "SYS_sync";
        case SYS_acct:
            return "SYS_acct";
        case SYS_settimeofday:
            return "SYS_settimeofday";
        case SYS_mount:
            return "SYS_mount";
        case SYS_umount2:
            return "SYS_umount2";
        case SYS_swapon:
            return "SYS_swapon";
        case SYS_swapoff:
            return "SYS_swapoff";
        case SYS_reboot:
            return "SYS_reboot";
        case SYS_sethostname:
            return "SYS_sethostname";
        case SYS_setdomainname:
            return "SYS_setdomainname";
        case SYS_iopl:
            return "SYS_iopl";
        case SYS_ioperm:
            return "SYS_ioperm";
        case SYS_create_module:
            return "SYS_create_module";
        case SYS_init_module:
            return "SYS_init_module";
        case SYS_delete_module:
            return "SYS_delete_module";
        case SYS_get_kernel_syms:
            return "SYS_get_kernel_syms";
        case SYS_query_module:
            return "SYS_query_module";
        case SYS_quotactl:
            return "SYS_quotactl";
        case SYS_nfsservctl:
            return "SYS_nfsservctl";
        case SYS_getpmsg:
            return "SYS_getpmsg";
        case SYS_putpmsg:
            return "SYS_putpmsg";
        case SYS_afs_syscall:
            return "SYS_afs_syscall";
        case SYS_tuxcall:
            return "SYS_tuxcall";
        case SYS_security:
            return "SYS_security";
        case SYS_gettid:
            return "SYS_gettid";
        case SYS_readahead:
            return "SYS_readahead";
        case SYS_setxattr:
            return "SYS_setxattr";
        case SYS_lsetxattr:
            return "SYS_lsetxattr";
        case SYS_fsetxattr:
            return "SYS_fsetxattr";
        case SYS_getxattr:
            return "SYS_getxattr";
        case SYS_lgetxattr:
            return "SYS_lgetxattr";
        case SYS_fgetxattr:
            return "SYS_fgetxattr";
        case SYS_listxattr:
            return "SYS_listxattr";
        case SYS_llistxattr:
            return "SYS_llistxattr";
        case SYS_flistxattr:
            return "SYS_flistxattr";
        case SYS_removexattr:
            return "SYS_removexattr";
        case SYS_lremovexattr:
            return "SYS_lremovexattr";
        case SYS_fremovexattr:
            return "SYS_fremovexattr";
        case SYS_tkill:
            return "SYS_tkill";
        case SYS_time:
            return "SYS_time";
        case SYS_futex:
            return "SYS_futex";
        case SYS_sched_setaffinity:
            return "SYS_sched_setaffinity";
        case SYS_sched_getaffinity:
            return "SYS_sched_getaffinity";
        case SYS_set_thread_area:
            return "SYS_set_thread_area";
        case SYS_io_setup:
            return "SYS_io_setup";
        case SYS_io_destroy:
            return "SYS_io_destroy";
        case SYS_io_getevents:
            return "SYS_io_getevents";
        case SYS_io_submit:
            return "SYS_io_submit";
        case SYS_io_cancel:
            return "SYS_io_cancel";
        case SYS_get_thread_area:
            return "SYS_get_thread_area";
        case SYS_lookup_dcookie:
            return "SYS_lookup_dcookie";
        case SYS_epoll_create:
            return "SYS_epoll_create";
        case SYS_epoll_ctl_old:
            return "SYS_epoll_ctl_old";
        case SYS_epoll_wait_old:
            return "SYS_epoll_wait_old";
        case SYS_remap_file_pages:
            return "SYS_remap_file_pages";
        case SYS_getdents64:
            return "SYS_getdents64";
        case SYS_set_tid_address:
            return "SYS_set_tid_address";
        case SYS_restart_syscall:
            return "SYS_restart_syscall";
        case SYS_semtimedop:
            return "SYS_semtimedop";
        case SYS_fadvise64:
            return "SYS_fadvise64";
        case SYS_timer_create:
            return "SYS_timer_create";
        case SYS_timer_settime:
            return "SYS_timer_settime";
        case SYS_timer_gettime:
            return "SYS_timer_gettime";
        case SYS_timer_getoverrun:
            return "SYS_timer_getoverrun";
        case SYS_timer_delete:
            return "SYS_timer_delete";
        case SYS_clock_settime:
            return "SYS_clock_settime";
        case SYS_clock_gettime:
            return "SYS_clock_gettime";
        case SYS_clock_getres:
            return "SYS_clock_getres";
        case SYS_clock_nanosleep:
            return "SYS_clock_nanosleep";
        case SYS_exit_group:
            return "SYS_exit_group";
        case SYS_epoll_wait:
            return "SYS_epoll_wait";
        case SYS_epoll_ctl:
            return "SYS_epoll_ctl";
        case SYS_tgkill:
            return "SYS_tgkill";
        case SYS_utimes:
            return "SYS_utimes";
        case SYS_vserver:
            return "SYS_vserver";
        case SYS_mbind:
            return "SYS_mbind";
        case SYS_set_mempolicy:
            return "SYS_set_mempolicy";
        case SYS_get_mempolicy:
            return "SYS_get_mempolicy";
        case SYS_mq_open:
            return "SYS_mq_open";
        case SYS_mq_unlink:
            return "SYS_mq_unlink";
        case SYS_mq_timedsend:
            return "SYS_mq_timedsend";
        case SYS_mq_timedreceive:
            return "SYS_mq_timedreceive";
        case SYS_mq_notify:
            return "SYS_mq_notify";
        case SYS_mq_getsetattr:
            return "SYS_mq_getsetattr";
        case SYS_kexec_load:
            return "SYS_kexec_load";
        case SYS_waitid:
            return "SYS_waitid";
        case SYS_add_key:
            return "SYS_add_key";
        case SYS_request_key:
            return "SYS_request_key";
        case SYS_keyctl:
            return "SYS_keyctl";
        case SYS_ioprio_set:
            return "SYS_ioprio_set";
        case SYS_ioprio_get:
            return "SYS_ioprio_get";
        case SYS_inotify_init:
            return "SYS_inotify_init";
        case SYS_inotify_add_watch:
            return "SYS_inotify_add_watch";
        case SYS_inotify_rm_watch:
            return "SYS_inotify_rm_watch";
        case SYS_migrate_pages:
            return "SYS_migrate_pages";
        case SYS_openat:
            return "SYS_openat";
        case SYS_mkdirat:
            return "SYS_mkdirat";
        case SYS_mknodat:
            return "SYS_mknodat";
        case SYS_fchownat:
            return "SYS_fchownat";
        case SYS_futimesat:
            return "SYS_futimesat";
        case SYS_newfstatat:
            return "SYS_newfstatat";
        case SYS_unlinkat:
            return "SYS_unlinkat";
        case SYS_renameat:
            return "SYS_renameat";
        case SYS_linkat:
            return "SYS_linkat";
        case SYS_symlinkat:
            return "SYS_symlinkat";
        case SYS_readlinkat:
            return "SYS_readlinkat";
        case SYS_fchmodat:
            return "SYS_fchmodat";
        case SYS_faccessat:
            return "SYS_faccessat";
        case SYS_pselect6:
            return "SYS_pselect6";
        case SYS_ppoll:
            return "SYS_ppoll";
        case SYS_unshare:
            return "SYS_unshare";
        case SYS_set_robust_list:
            return "SYS_set_robust_list";
        case SYS_get_robust_list:
            return "SYS_get_robust_list";
        case SYS_splice:
            return "SYS_splice";
        case SYS_tee:
            return "SYS_tee";
        case SYS_sync_file_range:
            return "SYS_sync_file_range";
        case SYS_vmsplice:
            return "SYS_vmsplice";
        case SYS_move_pages:
            return "SYS_move_pages";
        case SYS_utimensat:
            return "SYS_utimensat";
        case SYS_epoll_pwait:
            return "SYS_epoll_pwait";
        case SYS_signalfd:
            return "SYS_signalfd";
        case SYS_timerfd_create:
            return "SYS_timerfd_create";
        case SYS_eventfd:
            return "SYS_eventfd";
        case SYS_fallocate:
            return "SYS_fallocate";
        case SYS_timerfd_settime:
            return "SYS_timerfd_settime";
        case SYS_timerfd_gettime:
            return "SYS_timerfd_gettime";
        case SYS_accept4:
            return "SYS_accept4";
        case SYS_signalfd4:
            return "SYS_signalfd4";
        case SYS_eventfd2:
            return "SYS_eventfd2";
        case SYS_epoll_create1:
            return "SYS_epoll_create1";
        case SYS_dup3:
            return "SYS_dup3";
        case SYS_pipe2:
            return "SYS_pipe2";
        case SYS_inotify_init1:
            return "SYS_inotify_init1";
        case SYS_preadv:
            return "SYS_preadv";
        case SYS_pwritev:
            return "SYS_pwritev";
        case SYS_rt_tgsigqueueinfo:
            return "SYS_rt_tgsigqueueinfo";
        case SYS_perf_event_open:
            return "SYS_perf_event_open";
        case SYS_recvmmsg:
            return "SYS_recvmmsg";
        case SYS_fanotify_init:
            return "SYS_fanotify_init";
        case SYS_fanotify_mark:
            return "SYS_fanotify_mark";
        case SYS_prlimit64:
            return "SYS_prlimit64";
        case SYS_name_to_handle_at:
            return "SYS_name_to_handle_at";
        case SYS_open_by_handle_at:
            return "SYS_open_by_handle_at";
        case SYS_clock_adjtime:
            return "SYS_clock_adjtime";
        case SYS_syncfs:
            return "SYS_syncfs";
        case SYS_sendmmsg:
            return "SYS_sendmmsg";
        case SYS_setns:
            return "SYS_setns";
        case SYS_getcpu:
            return "SYS_getcpu";
        case SYS_process_vm_readv:
            return "SYS_process_vm_readv";
        case SYS_process_vm_writev:
            return "SYS_process_vm_writev";
        case SYS_kcmp:
            return "SYS_kcmp";
        case SYS_finit_module:
            return "SYS_finit_module";
        case SYS_sched_setattr:
            return "SYS_sched_setattr";
        case SYS_sched_getattr:
            return "SYS_sched_getattr";
        case SYS_renameat2:
            return "SYS_renameat2";
        case SYS_seccomp:
            return "SYS_seccomp";
        case SYS_getrandom:
            return "SYS_getrandom";
        case SYS_memfd_create:
            return "SYS_memfd_create";
        case SYS_kexec_file_load:
            return "SYS_kexec_file_load";
        case SYS_bpf:
            return "SYS_bpf";
        case SYS_execveat:
            return "SYS_execveat";
        case SYS_userfaultfd:
            return "SYS_userfaultfd";
        case SYS_membarrier:
            return "SYS_membarrier";
        case SYS_mlock2:
            return "SYS_mlock2";
        case SYS_copy_file_range:
            return "SYS_copy_file_range";
        case SYS_preadv2:
            return "SYS_preadv2";
        case SYS_pwritev2:
            return "SYS_pwritev2";
        case SYS_pkey_mprotect:
            return "SYS_pkey_mprotect";
        case SYS_pkey_alloc:
            return "SYS_pkey_alloc";
        case SYS_pkey_free:
            return "SYS_pkey_free";
        case SYS_statx:
            return "SYS_statx";
        case SYS_io_pgetevents:
            return "SYS_io_pgetevents";
        case SYS_rseq:
            return "SYS_rseq";
        default:
            return "unknown";
    }
}

long oe_syscall(long number, ...);

static long _dispatch_syscall(syscall_args_t* args)
{
    return oe_syscall(
        args->num,
        args->arg1,
        args->arg2,
        args->arg3,
        args->arg4,
        args->arg5,
        args->arg6);
}

static long _syscall_nanosleep(const struct timespec* req, struct timespec* rem)
{
    long ret = -1;
    uint64_t milliseconds = 0;

    if (rem)
        memset(rem, 0, sizeof(*rem));

    if (!req)
        goto done;

    /* Convert timespec to milliseconds */
    milliseconds += (uint64_t)req->tv_sec * 1000UL;
    milliseconds += (uint64_t)req->tv_nsec / 1000000UL;

    /* Perform OCALL */
    ret = oe_sleep_msec(milliseconds);

done:

    return ret;
}

long handle_syscall(syscall_args_t* args)
{
    switch (args->num)
    {
        case SYS_write:
        {
            return _dispatch_syscall(args);
        }
        case SYS_arch_prctl:
        {
            return _syscall_arch_prctl(
                (int)args->arg1, (unsigned long)args->arg2);
        }
        case SYS_set_tid_address:
        {
            return _syscall_set_tid_address((int*)args->arg1);
        }
        case SYS_exit_group:
        {
            return _syscall_exit_group((int)args->arg1);
        }
        case SYS_ioctl:
        {
            return _dispatch_syscall(args);
        }
        case SYS_writev:
        {
            return _dispatch_syscall(args);
        }
        case SYS_brk:
        {
            /* MUSL does not implement this! */
            return -1;
        }
        case SYS_mmap:
        {
            void* addr = (void*)args->arg1;
            size_t length = (size_t)args->arg2;
            int prot = (int)args->arg3;
            int flags = (int)args->arg4;
            int fd = (int)args->arg5;
            off_t offset = (off_t)args->arg6;
            void* ptr;

            if (addr)
                return -1;

            if (length == 0 || (length % OE_PAGE_SIZE))
                return -1;

            if (prot != (PROT_READ | PROT_WRITE))
                return -1;

            if (flags != (MAP_PRIVATE | MAP_ANONYMOUS))
                return -1;

            if (fd != -1)
                return -1;

            if (offset != 0)
                return -1;

            if (!(ptr = memalign(OE_PAGE_SIZE, length)))
                return -1;

            memset(ptr, 0, length);

            return (long)ptr;
        }
        case SYS_nanosleep:
        {
            const struct timespec* req = (struct timespec*)args->arg1;
            struct timespec* rem = (struct timespec*)args->arg2;
            return _syscall_nanosleep(req, rem);
        }
        case SYS_futex:
        {
            int* uaddr = (int*)args->arg1;
            int futex_op = (int)args->arg2;
            int val = (int)args->arg3;
            struct oe_timespec* timeout = (struct oe_timespec*)args->arg4;
            int* uaddr2 = (int*)args->arg5;
            int val3 = (int)args->arg6;
            return oe_futex(uaddr, futex_op, val, timeout, uaddr2, val3);
        }
        default:
        {
            long ret = _dispatch_syscall(args);

            if (ret == -1 && errno == ENOSYS)
            {
                const char* name = _syscall_name(args->num);
                fprintf(stderr, "*** syscall panic: %s\n", name);
                oe_abort();
            }

            return ret;
        }
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
            syscall_args_t* args = &_args;

            /* Extract syscall arguments. */
            args->num = (long)context->rax;
            args->arg1 = (long)context->rdi;
            args->arg2 = (long)context->rsi;
            args->arg3 = (long)context->rdx;
            args->arg4 = (long)context->r10;
            args->arg5 = (long)context->r8;
            args->arg6 = (long)context->r9;

            context->rip += 2;

            return OE_EXCEPTION_CONTINUE_EXECUTION;
        }
        else
        {
            oe_abort();
        }
    }

    return OE_EXCEPTION_CONTINUE_SEARCH;
}

static long _continue_execution_hook(long ret)
{
    long r;

    OE_UNUSED(ret);

    // printf("_continue_execution_hook().syscall=%ld\n", ret);
    r = handle_syscall(&_args);
    // printf("_continue_execution_hook().syscallret=%ld\n", r);

    return r;
}

#define MAX_ARGC 256

void oe_call_start(
    const void* stack,
    const void* stack_end,
    void (*func)(void));

int __call_start(int argc, const char* argv[], void (*func)(void))
{
    if (argc >= MAX_ARGC)
        return -1;

    typedef struct _args
    {
        int64_t argc;
        const char* argv[MAX_ARGC];
    } args_t;
    __attribute__((aligned(16))) args_t args;

    args.argc = argc;

    for (int i = 0; i < argc; i++)
        args.argv[i] = argv[i];

    const void* stack = &args.argc;

    /* Check for 16-byte alignment of the stack. */
    if (((uint64_t)stack % 16) != 0)
        return -1;

    oe_call_start(stack, __oe_get_stack_end(), func);

    /* Should not return. */
    return 0;
}

int exec(
    const uint8_t* image_base,
    size_t image_size,
    size_t argc,
    const char** argv)
{
    uint8_t* exec_base = (uint8_t*)__oe_get_exec_base();
    size_t exec_size = __oe_get_exec_size();
    elf_image_t image;
    oe_result_t r;

#if 0
    for (size_t i = 0; i < argc; i++)
        printf("argv[%zu]=%s\n", i, argv[i]);
#endif

    (void)image_base;
    (void)image_size;

    oe_continue_execution_hook = _continue_execution_hook;

    r = oe_add_vectored_exception_handler(false, _exception_handler);
    if (r != OE_OK)
    {
        fprintf(stderr, "oe_add_vectored_exception_handler() failed\n");
        abort();
    }

    if (image_size > exec_size)
    {
        fprintf(stderr, "program too big\n");
        abort();
    }

    if (__load_elf_image(
            &image, image_base, image_size, exec_base, exec_size) != 0)
    {
        fprintf(stderr, "__load_elf_image() failed\n");
        abort();
    }

    if (__relocate_symbols(&image, exec_base, exec_size) != 0)
    {
        fprintf(stderr, "__relocate_symbols() failed\n");
        abort();
    }

    if (__test_elf_header((const elf64_ehdr_t*)image.image_base) != 0)
    {
        fprintf(stderr, "__test_elf_header() failed\n");
        abort();
    }

    oe_set_default_socket_devid(OE_DEVID_HOST_SOCKET);
    oe_register_sgxfs_device();

    /* Set up exit handling. */
    if (oe_setjmp(&_jmp_buf) == 1)
    {
        goto done;
    }

    /* Jump to the ELF's entry point. */
    {
        typedef void (*start_proc)(void);
        uint64_t offset = image.entry_rva - image.image_offset;
        start_proc start = (start_proc)(exec_base + offset);

#if 0
        const char* argv[] = {"/bin/program", "arg1", "arg2", NULL};
        const int argc = OE_COUNTOF(argv) - 1;
#endif

        extern void my_start(void);
        __call_start((int)argc, argv, start);
        printf("start failed\n");
        abort();
        return 0;
    }

done:

    _free_elf_image(&image);

    return _exit_status;
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* AllowDebug */
    4096, /* HeapPageCount */
    4096, /* StackPageCount */
    8);   /* TCSCount */
