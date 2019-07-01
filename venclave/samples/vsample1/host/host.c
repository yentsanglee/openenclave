#include <openenclave/host.h>
#include <unistd.h>
#include "vsample1_u.h"

const char* arg0;

OE_PRINTF_FORMAT(1, 2)
void err(const char* fmt, ...)
{
    va_list ap;

    fprintf(stderr, "%s: error: ", arg0);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    exit(1);
}

uint64_t test_ocall(uint64_t x)
{
    printf("*** test_ocall()\n");
    return x;
}

static void _close_standard_devices(void)
{
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

int main(int argc, const char* argv[])
{
    arg0 = argv[0];
    oe_result_t result;
    oe_enclave_t* enclave = NULL;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s program\n", argv[0]);
        return 1;
    }

    if ((result = oe_create_vsample1_enclave(
             argv[1],
             OE_ENCLAVE_TYPE_SGX,
             OE_ENCLAVE_FLAG_DEBUG,
             NULL,
             0,
             &enclave)) != OE_OK)
    {
        err("oe_create_enclave() failed: %s\n", oe_result_str(result));
    }

    /* call test_ecall() */
    {
        uint64_t retval = 0;

        if ((result = test_ecall(enclave, &retval, 12345)) != OE_OK)
            err("test1() failed: %s\n", oe_result_str(result));

        if (retval != 12345)
            err("test1() failed: expected retval=12345");

        printf("retval=%ld\n", retval);
    }

    {
        char buf[100] = {'\0'};
        int retval;

        if ((result = test1(enclave, &retval, "hello", buf)) != OE_OK)
            err("test1() failed: %s\n", oe_result_str(result));

        if (strcmp(buf, "hello") != 0)
            err("test1() failed: expected 'hello' string");
    }

    if ((result = oe_terminate_enclave(enclave)) != OE_OK)
        err("oe_terminate_enclave() failed: %s\n", oe_result_str(result));

    _close_standard_devices();

    return 0;
}
