#define _GNU_SOURCE

// clang-format off
#include "common.h"
#include "linux-sgx/sdk/protected_fs/sgx_tprotected_fs/sgx_tprotected_fs_t.h"
// clang-format on

#include "sgx_error.h"
#include "../common/protectedfs.h"
#include <openenclave/enclave.h>
#include <openenclave/internal/calls.h>
#include <string.h>
#include <stdio.h>

typedef oe_protectedfs_args_t args_t;

static bool _copy_path(char dest[SECURE_FILE_MAX_PATH], const char* src)
{
    return strlcpy(dest, src, SECURE_FILE_MAX_PATH) < SECURE_FILE_MAX_PATH;
}

sgx_status_t SGX_CDECL u_sgxprotectedfs_exclusive_file_open(
    void** retval,
    const char* filename,
    uint8_t read_only,
    int64_t* file_size,
    int32_t* error_code)
{
    sgx_status_t err = 0;
    args_t* args = NULL;

    if (!retval || !filename || !file_size || !error_code)
    {
        err = SGX_ERROR_INVALID_PARAMETER;
        goto done;
    }

    if (!(args = oe_host_calloc(1, sizeof(args_t))))
    {
        err = SGX_ERROR_OUT_OF_MEMORY;
        goto done;
    }

    args->op = oe_protectedfs_op_exclusive_file_open;

    if (!_copy_path(args->u.exclusive_file_open.filename, filename))
    {
        err = SGX_ERROR_INVALID_PARAMETER;
        goto done;
    }

    args->u.exclusive_file_open.read_only = read_only;

    if (oe_ocall(OE_OCALL_PROTECTEDFS, (uint64_t)args, NULL) != OE_OK)
    {
        err = SGX_ERROR_UNEXPECTED;
        goto done;
    }

    *retval = args->u.exclusive_file_open.retval;
    *file_size = args->u.exclusive_file_open.file_size;
    *error_code = args->u.exclusive_file_open.error_code;

done:

    if (args)
        oe_host_free(args);

    return err;
}

sgx_status_t SGX_CDECL
u_sgxprotectedfs_check_if_file_exists(uint8_t* retval, const char* filename)
{
    sgx_status_t err = 0;
    args_t* args = NULL;

    if (!retval || !filename)
    {
        err = SGX_ERROR_INVALID_PARAMETER;
        goto done;
    }

    if (!(args = oe_host_calloc(1, sizeof(args_t))))
    {
        err = SGX_ERROR_OUT_OF_MEMORY;
        goto done;
    }

    args->op = oe_protectedfs_op_check_if_file_exists;

    if (!_copy_path(args->u.check_if_file_exists.filename, filename))
    {
        err = SGX_ERROR_INVALID_PARAMETER;
        goto done;
    }

    if (oe_ocall(OE_OCALL_PROTECTEDFS, (uint64_t)args, NULL) != OE_OK)
    {
        err = SGX_ERROR_UNEXPECTED;
        goto done;
    }

    *retval = args->u.check_if_file_exists.retval;

done:

    if (args)
        oe_host_free(args);

    return err;
}

sgx_status_t SGX_CDECL u_sgxprotectedfs_fread_node(
    int32_t* retval,
    void* file,
    uint64_t node_number,
    uint8_t* buffer,
    uint32_t node_size)
{
    sgx_status_t err = 0;
    args_t* args = NULL;

    if (!retval || !file || !buffer || !node_size)
    {
        err = SGX_ERROR_INVALID_PARAMETER;
        goto done;
    }

    if (!(args = oe_host_calloc(1, sizeof(args_t) + node_size + 1)))
    {
        err = SGX_ERROR_OUT_OF_MEMORY;
        goto done;
    }

    args->op = oe_protectedfs_op_fread_node;
    args->u.fread_node.file = file;
    args->u.fread_node.node_number = node_number;
    args->u.fread_node.buffer = args->buffer;
    args->u.fread_node.node_size = node_size;

    if (oe_ocall(OE_OCALL_PROTECTEDFS, (uint64_t)args, NULL) != OE_OK)
    {
        err = SGX_ERROR_UNEXPECTED;
        goto done;
    }

    memcpy(buffer, args->buffer, node_size);
    *retval = args->u.fread_node.retval;

done:

    if (args)
        oe_host_free(args);

    return err;
}

sgx_status_t SGX_CDECL u_sgxprotectedfs_fwrite_node(
    int32_t* retval,
    void* file,
    uint64_t node_number,
    uint8_t* buffer,
    uint32_t node_size)
{
    sgx_status_t err = 0;
    args_t* args = NULL;

    if (!retval || !file || !buffer || !node_size)
    {
        err = SGX_ERROR_INVALID_PARAMETER;
        goto done;
    }

    if (!(args = oe_host_calloc(1, sizeof(args_t) + node_size + 1)))
    {
        err = SGX_ERROR_OUT_OF_MEMORY;
        goto done;
    }

    args->op = oe_protectedfs_op_fwrite_node;
    args->u.fwrite_node.file = file;
    args->u.fwrite_node.node_number = node_number;
    memcpy(args->buffer, buffer, node_size);
    args->u.fwrite_node.buffer = args->buffer;
    args->u.fwrite_node.node_size = node_size;

    if (oe_ocall(OE_OCALL_PROTECTEDFS, (uint64_t)args, NULL) != OE_OK)
    {
        err = SGX_ERROR_UNEXPECTED;
        goto done;
    }

    *retval = args->u.fwrite_node.retval;

done:

    if (args)
        oe_host_free(args);

    return err;
}

sgx_status_t SGX_CDECL u_sgxprotectedfs_fclose(int32_t* retval, void* file)
{
    sgx_status_t err = 0;
    args_t* args = NULL;

    if (!retval || !file)
    {
        err = SGX_ERROR_INVALID_PARAMETER;
        goto done;
    }

    if (!(args = oe_host_calloc(1, sizeof(args_t))))
    {
        err = SGX_ERROR_OUT_OF_MEMORY;
        goto done;
    }

    args->op = oe_protectedfs_op_fclose;
    args->u.fclose.file = file;

    if (oe_ocall(OE_OCALL_PROTECTEDFS, (uint64_t)args, NULL) != OE_OK)
    {
        err = SGX_ERROR_UNEXPECTED;
        goto done;
    }

    *retval = args->u.fclose.retval;

done:

    if (args)
        oe_host_free(args);

    return err;
}

sgx_status_t SGX_CDECL u_sgxprotectedfs_fflush(uint8_t* retval, void* file)
{
    sgx_status_t err = 0;
    args_t* args = NULL;

    if (!retval || !file)
    {
        err = SGX_ERROR_INVALID_PARAMETER;
        goto done;
    }

    if (!(args = oe_host_calloc(1, sizeof(args_t))))
    {
        err = SGX_ERROR_OUT_OF_MEMORY;
        goto done;
    }

    args->op = oe_protectedfs_op_fflush;
    args->u.fflush.file = file;

    if (oe_ocall(OE_OCALL_PROTECTEDFS, (uint64_t)args, NULL) != OE_OK)
    {
        err = SGX_ERROR_UNEXPECTED;
        goto done;
    }

    *retval = args->u.fflush.retval;

done:

    if (args)
        oe_host_free(args);

    return err;
}

sgx_status_t SGX_CDECL
u_sgxprotectedfs_remove(int32_t* retval, const char* filename)
{
    sgx_status_t err = 0;
    args_t* args = NULL;

    if (!retval || !filename)
    {
        err = SGX_ERROR_INVALID_PARAMETER;
        goto done;
    }

    if (!(args = oe_host_calloc(1, sizeof(args_t))))
    {
        err = SGX_ERROR_OUT_OF_MEMORY;
        goto done;
    }

    args->op = oe_protectedfs_op_remove;

    if (!_copy_path(args->u.remove.filename, filename))
    {
        err = SGX_ERROR_INVALID_PARAMETER;
        goto done;
    }

    if (oe_ocall(OE_OCALL_PROTECTEDFS, (uint64_t)args, NULL) != OE_OK)
    {
        err = SGX_ERROR_UNEXPECTED;
        goto done;
    }

    *retval = args->u.remove.retval;

done:

    if (args)
        oe_host_free(args);

    return err;
}

sgx_status_t SGX_CDECL
u_sgxprotectedfs_recovery_file_open(void** retval, const char* filename)
{
    sgx_status_t err = 0;
    args_t* args = NULL;

    if (!retval || !filename)
    {
        err = SGX_ERROR_INVALID_PARAMETER;
        goto done;
    }

    if (!(args = oe_host_calloc(1, sizeof(args_t))))
    {
        err = SGX_ERROR_OUT_OF_MEMORY;
        goto done;
    }

    args->op = oe_protectedfs_op_recovery_file_open;

    if (!_copy_path(args->u.recovery_file_open.filename, filename))
    {
        err = SGX_ERROR_INVALID_PARAMETER;
        goto done;
    }

    if (oe_ocall(OE_OCALL_PROTECTEDFS, (uint64_t)args, NULL) != OE_OK)
    {
        err = SGX_ERROR_UNEXPECTED;
        goto done;
    }

    *retval = args->u.recovery_file_open.retval;

done:

    if (args)
        oe_host_free(args);

    return err;
}

sgx_status_t SGX_CDECL u_sgxprotectedfs_fwrite_recovery_node(
    uint8_t* retval,
    void* file,
    uint8_t* data,
    uint32_t data_length)
{
    sgx_status_t err = 0;
    args_t* args = NULL;

    if (!retval || !file || !data || !data_length)
    {
        err = SGX_ERROR_INVALID_PARAMETER;
        goto done;
    }

    if (!(args = oe_host_calloc(1, sizeof(args_t) + data_length + 1)))
    {
        err = SGX_ERROR_OUT_OF_MEMORY;
        goto done;
    }

    args->op = oe_protectedfs_op_fwrite_recovery_node;
    args->u.fwrite_recovery_node.file = file;
    memcpy(args->buffer, data, data_length);
    args->u.fwrite_recovery_node.data = args->buffer;
    args->u.fwrite_recovery_node.data_length = data_length;

    if (oe_ocall(OE_OCALL_PROTECTEDFS, (uint64_t)args, NULL) != OE_OK)
    {
        err = SGX_ERROR_UNEXPECTED;
        goto done;
    }

    *retval = args->u.fwrite_recovery_node.retval;

done:

    if (args)
        oe_host_free(args);

    return err;
}

sgx_status_t SGX_CDECL u_sgxprotectedfs_do_file_recovery(
    int32_t* retval,
    const char* filename,
    const char* recovery_filename,
    uint32_t node_size)
{
    sgx_status_t err = 0;
    args_t* args = NULL;

    if (!retval || !filename || !recovery_filename || !node_size)
    {
        err = SGX_ERROR_INVALID_PARAMETER;
        goto done;
    }

    if (!(args = oe_host_calloc(1, sizeof(args_t))))
    {
        err = SGX_ERROR_OUT_OF_MEMORY;
        goto done;
    }

    if (!_copy_path(args->u.do_file_recovery.filename, filename))
    {
        err = SGX_ERROR_INVALID_PARAMETER;
        goto done;
    }

    if (!_copy_path(
            args->u.do_file_recovery.recovery_filename, recovery_filename))
    {
        err = SGX_ERROR_INVALID_PARAMETER;
        goto done;
    }

    args->op = oe_protectedfs_op_do_file_recovery;
    args->u.do_file_recovery.node_size = node_size;

    if (oe_ocall(OE_OCALL_PROTECTEDFS, (uint64_t)args, NULL) != OE_OK)
    {
        err = SGX_ERROR_UNEXPECTED;
        goto done;
    }

    *retval = args->u.do_file_recovery.retval;

done:

    if (args)
        oe_host_free(args);

    return err;
}
