// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/bits/socket.h>
#include <openenclave/internal/device.h>

intptr_t oe_socket(int domain, int type, int protocol)

{
    int sd = -1;
    oe_device_t psock = NULL;

    switch (domain)
    {
        case OE_AF_ENCLAVE: // Temprory until we dicde how to indicate enclave
                            // sockets
            psock =
                oe_device_alloc(OE_DEVICE_ENCLAVE_SOCKET, "enclave_socket", 0);
            break;

        default:
            psock = oe_device_alloc(OE_DEVICE_HOST_SOCKET, "host_socket", 0);
            break;
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

    if ((*psock->ops.socket->socket)(psock, domain, type, protocol) < 0)
    {
        oe_release_fd(sd);
        return -1;
    }
    return sd;
}

int oe_connect(
    oe_sockfd_t sockfd,
    const struct oe_sockaddr* addr,
    socklen_t addrlen)

{
    int rtnval = -1;
    oe_device_t psock = oe_get_fd_device(sockfd);
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

int oe_accept(oe_sockfd_t sockfd, struct oe_sockaddr* addr, socklen_t* addrlen)

{
    oe_sockfd_t newfd = -1;
    newfd = oe_clone_fd(sockfd);
    oe_device_t psock = oe_get_fd_device(newfd);

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
    oe_device_t psock = oe_get_fd_device(sockfd);

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
    oe_device_t psock = oe_get_fd_device(sockfd);

    if (!psock)
    {
        // Log error here
        return -1; // erno is already set
    }

    if (psock->ops.socket->recvmsg == NULL)
    {
        oe_errno = OE_EINVAL;
        return -1;
    }

    // The action routine sets errno
    return (*psock->ops.socket->recvmsg)(psock, buf, len, flags);
}

ssize_t oe_recv(oe_sockfd_t sockfd, void* buf, size_t len)

{
    oe_device_t psock = oe_get_fd_device(sockfd);

    if (!psock)
    {
        // Log error here
        return -1; // erno is already set
    }

    if (psock->ops.socket->recvmsg == NULL)
    {
        oe_errno = OE_EINVAL;
        return -1;
    }

    // The action routine sets errno
    return (*psock->ops.socket->recvmsg)(psock, buf, len, 0);
}

ssize_t oe_sendmsg(oe_sockfd_t sockfd, const void* buf, size_t len, int flags)

{
    oe_device_t psock = oe_get_fd_device(sockfd);

    if (!psock)
    {
        // Log error here
        return -1; // erno is already set
    }

    if (psock->ops.socket->sendmsg == NULL)
    {
        oe_errno = OE_EINVAL;
        return -1;
    }

    // The action routine sets errno
    return (*psock->ops.socket->sendmsg)(psock, buf, len, flags);
}

ssize_t oe_send(oe_sockfd_t sockfd, const void* buf, size_t len)

{
    oe_device_t psock = oe_get_fd_device(sockfd);

    if (!psock)
    {
        // Log error here
        return -1; // erno is already set
    }

    if (psock->ops.socket->sendmsg == NULL)
    {
        oe_errno = OE_EINVAL;
        return -1;
    }

    // The action routine sets errno
    return (*psock->ops.socket->sendmsg)(psock, buf, len, 0);
}

int oe_shutdown(oe_sockfd_t sockfd, int how)

{
    oe_device_t psock = oe_get_fd_device(sockfd);

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
    socklen_t* addrlen)

{
    oe_device_t psock = oe_get_fd_device(sockfd);

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
    socklen_t* optlen)

{
    oe_device_t psock = oe_get_fd_device(sockfd);

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
    oe_socklen_t sockfd,
    int level,
    int optname,
    const void* optval,
    socklen_t optlen)

{
    oe_device_t psock = oe_get_fd_device(sockfd);

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

int oe_bind(oe_sockfd_t sockfd, const oe_sockaddr* name, socklen_t namelen)

{
    oe_device_t psock = oe_get_fd_device(sockfd);

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
