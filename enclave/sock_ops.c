// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "device.h"

int oe_socket(int domain, int type, int protocol)

{
#if 0
    switch (domain)
    {
    case AF_ENCLAVE:

        break;

    }
#endif
}

int oe_connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen)

{
    int rtnval = -1;

    if (sockfd >= file_descriptor_table_len)
    {
        oe_errno = EBADF;
        return -1;
    }
    __oe_device_t* pdevice = __oe_file_descriptor_table + sockfd;
    if (!VALID_DEVICE_TYPE(pdevice->device_type))
    {
        oe_errno = EINVAL;
        return -1;
    }

    if (pdevice->pops.conntect == NULL)
    {
        oe_errno = EINVAL;
        return -1;
    }

    rtnval = (*pdevice->pops.connect)(pdevice, addr, addrlen, &_errno);
    if (rtnval < 0)
    {
        return -1;
    }

    return rtnval;
}

int oe_accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen)

{
    accept_Result rslt = {};
    int newfd = -1;
    oe_device_t* newdev = NULL;
    oe_device_t* olddev = oe_fd_to_devie(sockfd);

    if (sockfd >= file_descriptor_table_len)
    {
        oe_errno = EBADF;
        return -1;
    }
    __oe_device_t* pdevice = __oe_file_descriptor_table + sockfd;
    if (!VALID_DEVICE_TYPE(pdevice->device_type))
    {
        oe_errno = EINVAL;
        return -1;
    }

    if (pdevice->pops.conntect == NULL)
    {
        oe_errno = EINVAL;
        return -1;
    }
    rslt = ocall_accept(pdevice, addr, addrlen);

    if (rslt.error != OE_OK)
    {
        set_errno(rslt.errno);
        return -1;
    }

    // Create new file descriptor
}

int oe_listen(int sockfd, int backlog)

{
}

int oe_getsockname(int sockfd, struct sockaddr* addr, socklen_t* addrlen)

{
}

int oe_getsockopt(
    int sockfd,
    int level,
    int optname,
    void* optval,
    socklen_t* optlen)

{
}

int oe_setsockopt(
    int sockfd,
    int level,
    int optname,
    const void* optval,
    socklen_t optlen)

{
}

oe_socket_error_t ocall_bind(
    intptr_t a_hSocket,
    [ in, size = a_nNameLen ] const void* a_Name,
    socklen_t a_nNameLen);

oe_socket_error_t ocall_closesocket(intptr_t a_hSocket);

oe_socket_error_t ocall_connect(
    intptr_t a_hSocket,
    [ in, size = a_nAddrLen ] const void* a_Addr,
    socklen_t a_nAddrLen);

int ocall_getaddrinfo(
    [ in, string ] const char* a_NodeName,
    [ in, string ] const char* a_ServiceName,
    int64_t a_Flags,
    int64_t a_Family,
    int64_t a_SockType,
    int64_t a_Protocol,
    [ out, size = len ] addrinfo_Buffer* buf,
    socklen_t len,
    [out] socklen_t* length_needed);

gethostname_Result ocall_gethostname(void);

getnameinfo_Result ocall_getnameinfo(
    [ in, size = a_AddrLen ] const void* a_Addr,
    socklen_t a_AddrLen,
    uint64_t a_Flags);

GetSockName_Result ocall_getpeername(intptr_t a_hSocket, socklen_t a_nNameLen);

GetSockName_Result ocall_getsockname(intptr_t a_hSocket, socklen_t a_nNameLen);

getsockopt_Result ocall_getsockopt(
    intptr_t a_hSocket,
    int64_t a_nLevel,
    socklen_t a_nOptName,
    socklen_t a_nOptLen);

ioctlsocket_Result ocall_ioctlsocket(
    intptr_t a_hSocket,
    int64_t a_nCommand,
    uint64_t a_uInputValue);

oe_socket_error_t ocall_listen(intptr_t a_hSocket, socklen_t a_nMaxConnections);

ssize_t ocall_recv(
    intptr_t s,
    [ out, size = len ] void* buf,
    size_t len,
    int64_t flags,
    [out] oe_socket_error_t* error);

select_Result ocall_select(
    int64_t a_nFds,
    oe_fd_set_internal a_ReadFds,
    oe_fd_set_internal a_WriteFds,
    oe_fd_set_internal a_ExceptFds,
    struct timeval a_Timeval);

send_Result ocall_send(
    intptr_t a_hSocket,
    [ in, size = a_nMessageLen ] const void* a_Message,
    size_t a_nMessageLen,
    int64_t a_Flags);

oe_socket_error_t ocall_setsockopt(
    intptr_t a_hSocket,
    int64_t a_nLevel,
    int64_t a_nOptName,
    [ in, size = a_nOptLen ] const void* a_OptVal,
    socklen_t a_nOptLen);

oe_socket_error_t ocall_shutdown(intptr_t a_hSocket, oe_shutdown_how_t a_How);

socket_Result ocall_socket(
    oe_socket_address_family_t a_AddressFamily,
    oe_socket_type_t a_Type,
    int64_t a_Protocol);
