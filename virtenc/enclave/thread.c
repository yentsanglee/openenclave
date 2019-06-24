// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "thread.h"
#include "hexdump.h"
#include "print.h"
#include "syscall.h"

void ve_dump_thread(thread_t* thread)
{
    thread_blocks_t* block = thread->blocks;

    if (block)
    {
        ve_print("=== DUMP THREAD (%p):\n", thread);
        ve_put("TLS:\n");
        ve_hexdump(&block->tls, sizeof(block->tls));
        ve_put("THREAD:\n");
        ve_print("self=%p\n", thread->base.self);
        ve_print("dtv=%lx\n", thread->base.dtv);
        ve_print("sysinfo=%lx\n", thread->base.sysinfo);
        ve_print("canary=%lx\n", thread->base.canary);
        ve_print("canary2=%lx\n", thread->base.canary2);
        ve_hexdump(&block->thread, sizeof(block->thread));
    }
}

#define VE_ARCH_GET_FS 0x1003

thread_t* ve_thread_self(void)
{
    thread_t* thread = NULL;
    ve_syscall2(OE_SYS_arch_prctl, VE_ARCH_GET_FS, (long)&thread);
    return thread;
}
