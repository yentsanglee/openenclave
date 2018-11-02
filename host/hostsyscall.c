#include <openenclave/host.h>
#include <openenclave/internal/calls.h>
#include <openenclave/internal/hostsyscall.h>
#include "ocalls.h"

void oe_handle_host_syscall(oe_enclave_t* enclave, uint64_t arg)
{
    oe_host_syscall_args_t* args = (oe_host_syscall_args_t*)arg;

    if (args)
    {
    }
}
