#ifndef _OE_IO_H
#define _OE_IO_H

#include <openenclave/internal/fs.h>
#include <stdio.h>

extern oe_fs_t oe_hostfs;
extern oe_fs_t oe_sgxfs;
extern oe_fs_t oe_shwfs;

#define OE_FILE_INSECURE (&oe_hostfs)
#define OE_FILE_SECURE_HARDWARE (&oe_shwfs)
#define OE_FILE_SECURE_ENCRYPTION (&oe_sgxfs)

#ifdef OE_USE_OPTEE
#define OE_FILE_SECURE_BEST_EFFORT OE_FILE_SECURE_HARDWARE
#else
#define OE_FILE_SECURE_BEST_EFFORT OE_FILE_SECURE_ENCRYPTION
#endif

#ifdef OE_SECURE_POSIX_FILE_API
#define OE_FILE_DEFAULT OE_FILE_SECURE_BEST_EFFORT
#else
#define OE_FILE_DEFAULT OE_FILE_INSECURE
#endif

#if defined(OE_USE_FILE_DEFAULT)
#define fopen(...) oe_fopen(OE_FILE_DEFAULT, __VA_ARGS__)
#define opendir(...) oe_opendir(OE_FILE_DEFAULT, __VA_ARGS__)
#define stat(...) oe_stat(OE_FILE_DEFAULT, __VA_ARGS__)
#define remove(...) oe_remove(OE_FILE_DEFAULT, __VA_ARGS__)
#define rename(...) oe_rename(OE_FILE_DEFAULT, __VA_ARGS__)
#define mkdir(...) oe_mkdir(OE_FILE_DEFAULT, __VA_ARGS__)
#define rmdir(...) oe_rmdir(OE_FILE_DEFAULT, __VA_ARGS__)
#endif

#if defined(OE_USE_OPTEE)
#ifndef SEEK_SET
#define SEEK_SET TEE_DATA_SEEK_SET
#endif
#ifndef SEEK_END
#define SEEK_END TEE_DATA_SEEK_END
#endif
#endif /* OE_USE_OPTEE */

#endif /* _OE_IO_H */
