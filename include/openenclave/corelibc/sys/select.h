// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OE_SYS_SELECT_H
#define _OE_SYS_SELECT_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>
#include <openenclave/corelibc/time.h>
#include <sys/types.h>
#include <unistd.h>

OE_EXTERNC_BEGIN

typedef struct _oe_fd_set oe_fd_set;

int oe_select(
    int nfds,
    fd_set* readfds,
    fd_set* writefds,
    fd_set* exceptfds,
    struct timeval* timeout);

void OE_FD_CLR(int fd, oe_fd_set* set);

int OE_FD_ISSET(int fd, oe_fd_set* set);

void OE_FD_SET(int fd, oe_fd_set* set);

void OE_FD_ZERO(oe_fd_set* set);

#if defined(OE_NEED_STDC_NAMES)

#endif /* defined(OE_NEED_STDC_NAMES) */

OE_EXTERNC_END

#endif /* _OE_SYS_SELECT_H */
