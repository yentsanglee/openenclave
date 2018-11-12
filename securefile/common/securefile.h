#ifndef _OE_SECUREFILE_H
#define _OE_SECUREFILE_H

#define SECURE_FILE_MAX_PATH 1024

#include <stddef.h>
#include <stdint.h>

typedef enum _sgx_secure_file_op {
    sgx_secure_file_op_none,
    sgx_secure_file_op_exclusive_file_open,
    sgx_secure_file_op_check_if_file_exists,
    sgx_secure_file_op_fread_node,
    sgx_secure_file_op_fwrite_node,
    sgx_secure_file_op_fclose,
    sgx_secure_file_op_fflush,
    sgx_secure_file_op_remove,
    sgx_secure_file_op_recovery_file_open,
    sgx_secure_file_op_fwrite_recovery_node,
    sgx_secure_file_op_do_file_recovery,
} sgx_secure_file_op_t;

typedef struct _sgx_secure_file_args
{
    sgx_secure_file_op_t op;
    union {
        struct
        {
            void* retval;
            char filename[SECURE_FILE_MAX_PATH];
            uint8_t read_only;
            int64_t file_size;
            int32_t error_code;
        } exclusive_file_open;
        struct
        {
            uint8_t retval;
            char filename[SECURE_FILE_MAX_PATH];
        } check_if_file_exists;
        struct
        {
            int32_t retval;
            void* file;
            uint64_t node_number;
            uint8_t* buffer;
            uint32_t node_size;
        } fread_node;
        struct
        {
            int32_t retval;
            void* file;
            uint64_t node_number;
            uint8_t* buffer;
            uint32_t node_size;
        } fwrite_node;
        struct
        {
            int32_t retval;
            void* file;
        } fclose;
        struct
        {
            int32_t retval;
            void* file;
        } fflush;
        struct
        {
            int32_t retval;
            char filename[SECURE_FILE_MAX_PATH];
        } remove;
        struct
        {
            void* retval;
            char filename[SECURE_FILE_MAX_PATH];
        } recovery_file_open;
        struct
        {
            uint8_t retval;
            void* file;
            uint8_t* data;
            uint32_t data_length;
        } fwrite_recovery_node;
        struct
        {
            int32_t retval;
            char filename[SECURE_FILE_MAX_PATH];
            char recovery_filename[SECURE_FILE_MAX_PATH];
            uint32_t node_size;
        } do_file_recovery;
    } u;
    uint8_t buffer[];
} sgx_secure_file_args_t;

#endif /* _OE_SECUREFILE_H */
