#ifndef _SYS_TIME_H
#define _SYS_TIME_H

#include "../__common.h"

struct timezone 
{
    int tz_minuteswest;
    int tz_dsttime;
};

int gettimeofday(struct timeval *tv, struct timezone *tz);

#endif /* _SYS_TIME_H */
