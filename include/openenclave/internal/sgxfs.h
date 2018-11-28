#ifndef _OE_SGXFS_H
#define _OE_SGXFS_H

#include <openenclave/fs.h>

OE_EXTERNC_BEGIN

extern oe_fs_t oe_sgxfs;

void oe_install_sgxfs(void);

OE_EXTERNC_END

#endif /* _OE_SGXFS_H */
