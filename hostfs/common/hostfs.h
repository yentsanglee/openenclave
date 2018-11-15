#ifndef _OE_HOSTFS_H
#define _OE_HOSTFS_H

#include <openenclave/internal/fs.h>

OE_EXTERNC_BEGIN

extern oe_fs_t hostfs;

void oe_install_hostfs(void);

OE_EXTERNC_END

#endif /* _OE_HOSTFS_H */
