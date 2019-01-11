#ifndef _UNISTD_H
#define _UNISTD_H

#include "__common.h"

int close(int fd);

ssize_t read(int fd, void *buf, size_t count);

pid_t getpid(void);

uid_t getuid(void);

uid_t geteuid(void);

gid_t getgid(void);

gid_t getegid(void);

ssize_t write(int fd, const void *buf, size_t count);

off_t lseek(int fd, off_t offset, int whence);

#endif /* _UNISTD_H */
