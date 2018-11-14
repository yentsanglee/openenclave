#ifndef _OE_PROTECTEDFS_HOST_PROTECTEDFS_H
#define _OE_PROTECTEDFS_HOST_PROTECTEDFS_H

#define SECURE_FILE_MAX_PATH 1024

#include <stddef.h>
#include <stdint.h>
#include "../../fs/fs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _oe_sgxfs_op {
    oe_sgxfs_op_none,
    oe_sgxfs_op_exclusive_file_open,
    oe_sgxfs_op_check_if_file_exists,
    oe_sgxfs_op_fread_node,
    oe_sgxfs_op_fwrite_node,
    oe_sgxfs_op_fclose,
    oe_sgxfs_op_fflush,
    oe_sgxfs_op_remove,
    oe_sgxfs_op_recovery_file_open,
    oe_sgxfs_op_fwrite_recovery_node,
    oe_sgxfs_op_do_file_recovery,
} oe_sgxfs_op_t;

typedef struct _oe_sgxfs_args
{
    oe_sgxfs_op_t op;
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
} oe_sgxfs_args_t;

int sgxfs_new(fs_t** fs_out);

int fs_mount_sgxfs(const char* target);

#ifdef __cplusplus
}
#endif

#endif /* _OE_PROTECTEDFS_COMMON_PROTECTEDFS_H */
