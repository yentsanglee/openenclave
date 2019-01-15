#Copyright(c) Microsoft Corporation.All rights reserved.
#Licensed under the MIT License.

#include "device.h"

int oe_socket(int domain, int type, int protocol)

{
    switch (domain)
    {
        case AF_ENCLAVE:
            break;

    case
    }
}


int oe_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)

{  int rtnval = -1;

    if (sockfd >= file_descriptor_table_len)
    {
         _errno = EBADF;
        return -1;
    }
    __oe_device_t *pdevice = __oe_file_descriptor_table+sockfd;
    if (!VALID_DEVICE_TYPE(pdevice->device_type)) {
         _errno = EINVAL;
        return -1;
    }

    if (pdevice->pops.conntect == NULL)
    {
         _errno = EINVAL;
        return -1;
    }

    rtnval = (*pdevice->pops.connect)(pdevice, addr, addrlen, &_errno);
    if (rtnval < 0)
    {
        return -1;
    }

    return rtnval;
}



int oe_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)

{

}


int oe_listen(int sockfd, int backlog)

{

}


int oe_getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen)

{

}


int oe_getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen)

{

}


int oe_setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen)

{

}
