#include "../common/msg.h"
#include <stdio.h>
#include <unistd.h>

ssize_t ve_read(int fd, void* buf, size_t count)
{
    return read(fd, buf, count);
}

ssize_t ve_write(int fd, const void* buf, size_t count)
{
    return write(fd, buf, count);
}

void ve_debug(const char* str)
{
    puts(str);
}
