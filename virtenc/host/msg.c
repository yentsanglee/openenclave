#include "../common/msg.h"
#include <stdio.h>
#include <unistd.h>

ssize_t ve_read(int fd, void* buf, size_t count)
{
    ssize_t ret = read(fd, buf, count);
    printf("host:read: %zu\n", ret);
    return ret;
}

ssize_t ve_write(int fd, const void* buf, size_t count)
{
    ssize_t ret = write(fd, buf, count);
    printf("host:wrote: %zu\n", ret);
    return ret;
}
