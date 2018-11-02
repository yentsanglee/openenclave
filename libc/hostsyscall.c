#include <openenclave/enclave.h>
#include <openenclave/internal/calls.h>
#include <openenclave/internal/hostsyscall.h>
#include <openenclave/internal/raise.h>

oe_result_t oe_host_syscall(oe_host_syscall_args_t* args)
{
    oe_result_t result = OE_OK;

    if (!args)
        OE_RAISE(OE_UNEXPECTED);

    OE_CHECK(oe_ocall(OE_OCALL_HOST_SYSCALL, (uint64_t)args, NULL));

    result = OE_OK;

done:
    return result;
}
