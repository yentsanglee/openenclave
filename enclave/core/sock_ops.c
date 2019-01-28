// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/internal/device.h>
#include <openenclave/internal/socket.h>

intptr_t oe_socket(int domain, int type, int protocol)

{
    int sd = -1;
    oe_device_t* psock = NULL;
    oe_device_t* pdevice = NULL;

    switch (domain)
    {
        case OE_AF_ENCLAVE: // Temprory until we dicde how to indicate enclave
                            // sockets
            pdevice = oe_get_devid_device(OE_DEVICE_ENCLAVE_SOCKET);
            break;

        default:
            pdevice = oe_get_devid_device(OE_DEVICE_HOST_SOCKET);
            break;
    }
    if ((psock = (*pdevice->ops.socket->socket)(
             pdevice, domain, type, protocol)) == NULL)
    {
        oe_release_fd(sd);
        return -1;
    }
    sd = oe_allocate_fd();
    if (sd < 0)
    {
        // Log error here
        return -1; // errno is already set
    }
    psock = oe_set_fd_device(sd, psock);
    if (!psock)
    {
        // Log error here
        return -1; // erno is already set
    }

    return sd;
}

int oe_connect(
    oe_sockfd_t sockfd,
    const struct oe_sockaddr* addr,
    oe_socklen_t addrlen)

{
    int rtnval = -1;
    oe_device_t* psock = oe_get_fd_device(sockfd);
    if (!psock)
    {
        // Log error here
        return -1; // erno is already set
    }

    if (psock->ops.socket->connect == NULL)
    {
        oe_errno = OE_EINVAL;
        return -1;
    }

    rtnval = (*psock->ops.socket->connect)(psock, addr, addrlen);
    if (rtnval < 0)
    {
        return -1;
    }

    return rtnval;
}

int oe_accept(
    oe_sockfd_t sockfd,
    struct oe_sockaddr* addr,
    oe_socklen_t* addrlen)

{
    oe_sockfd_t newfd = -1;
    newfd = oe_clone_fd(sockfd);
    oe_device_t* psock = oe_get_fd_device(newfd);

    if (!psock)
    {
        // Log error here
        return -1; // erno is already set
    }

    if (psock->ops.socket->accept == NULL)
    {
        oe_errno = OE_EINVAL;
        return -1;
    }

    // Create new file descriptor

    if ((*psock->ops.socket->accept)(psock, addr, addrlen) < 0)
    {
        oe_release_fd(newfd);
        return -1;
    }
    return newfd;
}

int oe_listen(oe_sockfd_t sockfd, int backlog)

{
    oe_device_t* psock = oe_get_fd_device(sockfd);

    if (!psock)
    {
        // Log error here
        return -1; // erno is already set
    }

    if (psock->ops.socket->listen == NULL)
    {
        oe_errno = OE_EINVAL;
        return -1;
    }

    // The action routine sets errno
    return (*psock->ops.socket->listen)(psock, backlog);
}

ssize_t oe_recvmsg(oe_sockfd_t sockfd, void* buf, size_t len, int flags)

{
    oe_device_t* psock = oe_get_fd_device(sockfd);

    if (!psock)
    {
        // Log error here
        return -1; // erno is already set
    }

    if (psock->ops.socket->recv == NULL)
    {
        oe_errno = OE_EINVAL;
        return -1;
    }

    // The action routine sets errno
    return (*psock->ops.socket->recv)(psock, buf, len, flags);
}

ssize_t oe_send(oe_sockfd_t sockfd, const void* buf, size_t len, int flags)

{
    oe_device_t* psock = oe_get_fd_device(sockfd);

    if (!psock)
    {
        // Log error here
        return -1; // erno is already set
    }

    if (psock->ops.socket->send == NULL)
    {
        oe_errno = OE_EINVAL;
        return -1;
    }

    // The action routine sets errno
    return (*psock->ops.socket->send)(psock, buf, len, flags);
}

int oe_shutdown(oe_sockfd_t sockfd, int how)

{
    oe_device_t* psock = oe_get_fd_device(sockfd);

    if (!psock)
    {
        // Log error here
        return -1; // erno is already set
    }

    if (psock->ops.socket->shutdown == NULL)
    {
        oe_errno = OE_EINVAL;
        return -1;
    }

    // The action routine sets errno
    return (*psock->ops.socket->shutdown)(psock, how);
}

int oe_getsockname(
    oe_sockfd_t sockfd,
    struct oe_sockaddr* addr,
    oe_socklen_t* addrlen)

{
    oe_device_t* psock = oe_get_fd_device(sockfd);

    if (!psock)
    {
        // Log error here
        return -1; // erno is already set
    }

    if (psock->ops.socket->getsockname == NULL)
    {
        oe_errno = OE_EINVAL;
        return -1;
    }

    // The action routine sets errno
    return (*psock->ops.socket->getsockname)(psock, addr, addrlen);
}

int oe_getsockopt(
    oe_sockfd_t sockfd,
    int level,
    int optname,
    void* optval,
    oe_socklen_t* optlen)

{
    oe_device_t* psock = oe_get_fd_device(sockfd);

    if (!psock)
    {
        // Log error here
        return -1; // erno is already set
    }

    if (psock->ops.socket->getsockopt == NULL)
    {
        oe_errno = OE_EINVAL;
        return -1;
    }

    // The action routine sets errno
    return (*psock->ops.socket->getsockopt)(
        psock, level, optname, optval, optlen);
}

int oe_setsockopt(
    oe_sockfd_t sockfd,
    int level,
    int optname,
    const void* optval,
    oe_socklen_t optlen)

{
    oe_device_t* psock = oe_get_fd_device(sockfd);

    if (!psock)
    {
        // Log error here
        return -1; // erno is already set
    }

    if (psock->ops.socket->setsockopt == NULL)
    {
        oe_errno = OE_EINVAL;
        return -1;
    }

    // The action routine sets errno
    return (*psock->ops.socket->setsockopt)(
        psock, level, optname, optval, optlen);
}

int oe_bind(oe_sockfd_t sockfd, const oe_sockaddr* name, oe_socklen_t namelen)

{
    oe_device_t* psock = oe_get_fd_device(sockfd);

    if (!psock)
    {
        // Log error here
        return -1; // erno is already set
    }

    if (psock->ops.socket->bind == NULL)
    {
        oe_errno = OE_EINVAL;
        return -1;
    }

    // The action routine sets errno
    return (*psock->ops.socket->bind)(psock, name, namelen);
}
