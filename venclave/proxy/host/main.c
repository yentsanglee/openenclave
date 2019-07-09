// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <assert.h>
#include <limits.h>
#include <openenclave/host.h>
#include <unistd.h>
#include "../../common/call.h"
#include "../../common/msg.h"
#include "proxy_u.h"

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

static void _close_standard_devices(void)
{
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

static int _derive_enclave_path(const char* host_path, char path[PATH_MAX])
{
    int ret = -1;
    char* p;

    if (strlen(host_path) >= PATH_MAX)
        goto done;

    strcpy(path, host_path);

    if (strlen(path) < 4)
        goto done;

    p = path + strlen(path) - sizeof("host") + 1;

    if (strcmp(p, "host") != 0)
        goto done;

    strcpy(p, "enc");

    ret = 0;

done:
    return ret;
}

static int _handle_egetkey_request(
    oe_enclave_t* enclave,
    int fd,
    const void* data,
    size_t size)
{
    int ret = -1;
    ve_egetkey_request_t req;
    ve_egetkey_response_t rsp;

    if (size != sizeof(req))
        goto done;

    memcpy(&req, data, sizeof(req));
    memset(&rsp, 0, sizeof(rsp));

    if (egetkey_ecall(
            enclave,
            &rsp.ret,
            &req.request,
            sizeof(req.request),
            &rsp.key,
            sizeof(rsp.key)) != OE_OK)
    {
        goto done;
    }

    if (ve_msg_send(fd, VE_MSG_EGETKEY, &rsp, sizeof(rsp)) != 0)
        goto done;

    ret = 0;

done:
    return ret;
}

static int _handle_ereport_request(
    oe_enclave_t* enclave,
    int fd,
    const void* data,
    size_t size)
{
    int ret = -1;
    ve_ereport_request_t req;
    ve_ereport_response_t rsp;

    if (size != sizeof(req))
        goto done;

    memcpy(&req, data, sizeof(req));
    memset(&rsp, 0, sizeof(rsp));

    if (ereport_ecall(
            enclave,
            &rsp.result,
            &req.target_info,
            sizeof(req.target_info),
            &req.report_data,
            sizeof(req.report_data),
            &rsp.report,
            sizeof(rsp.report)) != OE_OK)
    {
        goto done;
    }

    if (ve_msg_send(fd, VE_MSG_EREPORT, &rsp, sizeof(rsp)) != 0)
        goto done;

    ret = 0;

done:
    return ret;
}

static int _handle_messages(oe_enclave_t* enclave, int fd)
{
    int ret = -1;
    ve_msg_type_t type;
    void* data = NULL;
    size_t size;

    if (fd < 0)
        goto done;

    for (;;)
    {
        if (ve_msg_recv_any(fd, &type, &data, &size) != 0)
            goto done;

        switch (type)
        {
            case VE_MSG_TERMINATE:
            {
                /* Send back response. */
                if (ve_msg_send(fd, type, NULL, 0) != 0)
                    goto done;

                close(fd);

                /* Set the process exit status and exit this function. */
                ret = 0;
                goto done;
            }
            case VE_MSG_EGETKEY:
            {
                if (_handle_egetkey_request(enclave, fd, data, size) != 0)
                    goto done;

                break;
            }
            case VE_MSG_EREPORT:
            {
                if (_handle_ereport_request(enclave, fd, data, size) != 0)
                    goto done;

                break;
            }
        }

        free(data);
        data = NULL;
    }

done:

    if (data)
        free(data);

    return ret;
}

int main(int argc, const char* argv[])
{
    arg0 = argv[0];
    int sock;
    char path[PATH_MAX];
    oe_result_t result;
    oe_enclave_t* enclave = NULL;
    int exit_status = 0;

    /* Check command line arguments. */
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s socket\n", argv[0]);
        return 1;
    }

    /* Convert the socket argument to integer. */
    {
        char* end = NULL;
        unsigned long x = strtoul(argv[1], &end, 10);

        if (end == NULL || *end != '\0' || x > INT_MAX)
            err("bad sock argument: %s", argv[1]);

        sock = (int)x;
    }

    /* Form the name of the enclave from argv[0]. */
    if (_derive_enclave_path(argv[0], path) != 0)
        err("failed to drive the enclave path");

    /* Verify that the enclave exists. */
    if (access(path, F_OK) != 0)
        err("enclave file not found: %s", path);

    /* Create the enclave. */
    if ((result = oe_create_proxy_enclave(
             path,
             OE_ENCLAVE_TYPE_SGX,
             OE_ENCLAVE_FLAG_DEBUG,
             NULL,
             0,
             &enclave)) != OE_OK)
    {
        err("oe_create_enclave() failed: %s\n", oe_result_str(result));
    }

#if 0
    /* Call into the enclave. */
    if ((result = dummy_ecall(enclave)) != OE_OK)
        err("dummy_ecall() failed: %s\n", oe_result_str(result));
#endif

    /* Block while waiting to read the socket. */
    if (sock != INT_MAX)
    {
        /* ATTN: capture exit status here. */
        if ((exit_status = _handle_messages(enclave, sock)) == -1)
            err("vproxyhost: termination error");
    }

    /* Terminate the enclave. */
    if ((result = oe_terminate_enclave(enclave)) != OE_OK)
        err("oe_terminate_enclave() failed: %s\n", oe_result_str(result));

    _close_standard_devices();

    return exit_status;
}
