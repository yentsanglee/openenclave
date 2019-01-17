// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef __OE_ENCLAVE_DEVICE_H__
#define __OE_ENCLAVE_DEVICE_H__ 1

namespace OE_DEVICE

    enum oe_device_type {
        OE_DEV_NONE = -1,         // This entry is invalid
        OE_DEV_SECURE_FILESYSTEM, // This entry describes a file in the enclaves
                                  // secure file system
        OE_DEV_HOST_FILESYSTEM,   // This entry describes a file in the hosts's
                                  // file system
        OE_DEV_HOST_SOCKET,       // This entry describes an internet socket
        OE_DEV_ENCLAVE_SOCKET     // This entry describes an enclave to enclave
                              // communication channel not available to the host
    };

typedef struct oe_device_ops
{
    int (*init)();
    int (*create)();
    int (*remove)();

    int (*read)();
    int (*write)();
    int (*ioctl)();
    int (*close)();

    union {
        struct
        {
            int (*mount)();
            int (*umount)();
            int (*open)();
            int (*seek)();
            int (*stat)();
            int (*fcntl)();
        } FileDevice;

        struct SocketDevice
        {
            int (*socket)(
                int domain,
                int type,
                int protocol); // We mainly check these args to be sure
                               // we can do what they are asking. Otherwise,
                               // the effect of the args is implementation
                               // specific

            int (*connect)(
                int sockfd,
                const struct sockaddr* addr,
                socklen_t addrlen);
            int (
                *accept)(int sockfd, struct sockaddr* addr, socklen_t* addrlen);
            int (*bind)(
                int sockfd,
                const struct sockaddr* addr,
                socklen_t addrlen);
            int (*listen)(int sockfd, int backlog);
            int (*getsockopt)(
                int sockfd,
                int level,
                int optname,
                void* optval,
                socklen_t* optlen);
            int (*setsockopt)(
                int sockfd,
                int level,
                int optname,
                const void* optval,
                socklen_t optlen);
        } SocketDevice;
    };
};

typdef struct oe_device_entry
{
    int32_t length;
    enum oe_device_type;
    char* devicename;
    uint32_t flags; //

    oe_device_ops* pops;

} * oe_device_t;

extern size_t file_descriptor_table_len;
extern oe_device_t* _oe_file_descriptor_table;

int oe_device_init(); // Overridable function to set up device structures
int oe_allocate_fd();
void oe_release_fd(int fd);

oe_device_t* oe_device_alloc(
    int device_id,
    char* device_name,
    int private_size); // Allocate a device of sizeof(struct
int oe_device_addref(int device_id);
int oe_device_release(int device_id);

oe_device_t oe_set_fd_device() oe_device_t oe_get_fd_device()
}
;      // OE_DEVICE
#endif // __OE_ENCLAVE_DEVICE_H__
