#include "socket.h"
#include "string.h"

#define CMSG_DATA(cmsg) ((unsigned char*)(((struct ve_cmsghdr*)(cmsg)) + 1))

#define CMSG_ALIGN(len) \
    (((len) + sizeof(size_t) - 1) & (size_t) ~(sizeof(size_t) - 1))

#define CMSG_SPACE(len) \
    (CMSG_ALIGN(len) + CMSG_ALIGN(sizeof(struct ve_cmsghdr)))

#define CMSG_LEN(len) (CMSG_ALIGN(sizeof(struct ve_cmsghdr)) + (len))

int ve_recv_fd(int sock)
{
    int ret = -1;
    struct ve_iovec iov[1];
    struct ve_msghdr msg;
    const uint32_t MAGIC = 0x5E2DFD00;
    uint32_t magic = 0;
    OE_ALIGNED(sizeof(uint64_t)) char cmsg_buf[CMSG_SPACE(sizeof(int))];
    struct ve_cmsghdr* cmsg = (struct ve_cmsghdr*)cmsg_buf;
    const size_t cmsg_len = CMSG_LEN(sizeof(int));

    if (sock < 0)
        goto done;

    iov[0].iov_base = &magic;
    iov[0].iov_len = sizeof(magic);

    ve_memset(&msg, 0, sizeof(msg));
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = cmsg;
    msg.msg_controllen = cmsg_len;

    if (ve_recvmsg(sock, &msg, 0) != sizeof(magic))
        goto done;

    if (ve_memcmp(&magic, &MAGIC, sizeof(magic)) != 0)
        goto done;

    ve_memcpy(&ret, CMSG_DATA(cmsg), sizeof(ret));

done:
    return ret;
}
