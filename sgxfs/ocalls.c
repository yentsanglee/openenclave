// clang-format off
#include "common.h"
#include "../3rdparty/linux-sgx/linux-sgx/sdk/protected_fs/sgx_tprotected_fs/sgx_tprotected_fs_t.h"
// clang-format on

sgx_status_t SGX_CDECL u_sgxprotectedfs_exclusive_file_open(
    void** retval,
    const char* filename,
    uint8_t read_only,
    int64_t* file_size,
    int32_t* error_code)
{
    /* TODO: */
    return SGX_ERROR_UNEXPECTED;
}

sgx_status_t SGX_CDECL
u_sgxprotectedfs_check_if_file_exists(uint8_t* retval, const char* filename)
{
    /* TODO: */
    return SGX_ERROR_UNEXPECTED;
}

sgx_status_t SGX_CDECL u_sgxprotectedfs_fread_node(
    int32_t* retval,
    void* f,
    uint64_t node_number,
    uint8_t* buffer,
    uint32_t node_size)
{
    /* TODO: */
    return SGX_ERROR_UNEXPECTED;
}

sgx_status_t SGX_CDECL u_sgxprotectedfs_fwrite_node(
    int32_t* retval,
    void* f,
    uint64_t node_number,
    uint8_t* buffer,
    uint32_t node_size)
{
    /* TODO: */
    return SGX_ERROR_UNEXPECTED;
}

sgx_status_t SGX_CDECL u_sgxprotectedfs_fclose(int32_t* retval, void* f)
{
    /* TODO: */
    return SGX_ERROR_UNEXPECTED;
}

sgx_status_t SGX_CDECL u_sgxprotectedfs_fflush(uint8_t* retval, void* f)
{
    /* TODO: */
    return SGX_ERROR_UNEXPECTED;
}

sgx_status_t SGX_CDECL
u_sgxprotectedfs_remove(int32_t* retval, const char* filename)
{
    /* TODO: */
    return SGX_ERROR_UNEXPECTED;
}

sgx_status_t SGX_CDECL
u_sgxprotectedfs_recovery_file_open(void** retval, const char* filename)
{
    /* TODO: */
    return SGX_ERROR_UNEXPECTED;
}

sgx_status_t SGX_CDECL u_sgxprotectedfs_fwrite_recovery_node(
    uint8_t* retval,
    void* f,
    uint8_t* data,
    uint32_t data_length)
{
    /* TODO: */
    return SGX_ERROR_UNEXPECTED;
}

sgx_status_t SGX_CDECL u_sgxprotectedfs_do_file_recovery(
    int32_t* retval,
    const char* filename,
    const char* recovery_filename,
    uint32_t node_size)
{
    /* TODO: */
    return SGX_ERROR_UNEXPECTED;
}
