#include <dirent.h>
#include <openenclave/internal/hostfs.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../common/hostfsargs.h"

void (*oe_handle_hostfs_ocall_callback)(void*);

static void _handle_hostfs_ocall(void* args_)
{
    oe_hostfs_args_t* args = (oe_hostfs_args_t*)args_;

    if (!args)
        return;

    switch (args->op)
    {
        case OE_HOSTFS_OP_NONE:
        {
            break;
        }
        case OE_HOSTFS_OP_FOPEN:
        {
            args->u.fopen.ret = fopen(args->u.fopen.path, args->u.fopen.mode);
            break;
        }
        case OE_HOSTFS_OP_FCLOSE:
        {
            args->u.fclose.ret = fclose(args->u.fclose.file);
            break;
        }
        case OE_HOSTFS_OP_FREAD:
        {
            args->u.fread.ret = fread(
                args->u.fread.ptr,
                args->u.fread.size,
                args->u.fread.nmemb,
                args->u.fread.file);
            break;
        }
        case OE_HOSTFS_OP_FWRITE:
        {
            args->u.fwrite.ret = fwrite(
                args->u.fwrite.ptr,
                args->u.fwrite.size,
                args->u.fwrite.nmemb,
                args->u.fwrite.file);
            break;
        }
        case OE_HOSTFS_OP_FTELL:
        {
            args->u.ftell.ret = ftell(args->u.ftell.file);
            break;
        }
        case OE_HOSTFS_OP_FSEEK:
        {
            args->u.fseek.ret = fseek(
                args->u.fseek.file, args->u.fseek.offset, args->u.fseek.whence);
            break;
        }
        case OE_HOSTFS_OP_FFLUSH:
        {
            args->u.fflush.ret = fflush(args->u.fflush.file);
            break;
        }
        case OE_HOSTFS_OP_FERROR:
        {
            args->u.ferror.ret = ferror(args->u.ferror.file);
            break;
        }
        case OE_HOSTFS_OP_FEOF:
        {
            args->u.feof.ret = feof(args->u.feof.file);
            break;
        }
        case OE_HOSTFS_OP_CLEARERR:
        {
            clearerr(args->u.clearerr.file);
            break;
        }
        case OE_HOSTFS_OP_OPENDIR:
        {
            args->u.opendir.ret = opendir(args->u.opendir.name);
            break;
        }
        case OE_HOSTFS_OP_READDIR:
        {
            struct dirent entry;
            struct dirent* result = NULL;
            args->u.readdir.ret =
                readdir_r(args->u.readdir.dir, &entry, &result);
            args->u.readdir.result = NULL;

            if (args->u.readdir.ret == 0 && result)
            {
                args->u.readdir.entry.d_ino = result->d_ino;
                args->u.readdir.entry.d_off = result->d_off;
                args->u.readdir.entry.d_reclen = result->d_reclen;
                args->u.readdir.entry.d_type = result->d_type;

                *args->u.readdir.entry.d_name = '\0';

                strncat(
                    args->u.readdir.entry.d_name,
                    result->d_name,
                    sizeof(args->u.readdir.entry.d_name) - 1);

                args->u.readdir.result = &args->u.readdir.entry;
            }
            else
            {
                memset(
                    &args->u.readdir.entry, 0, sizeof(args->u.readdir.entry));
            }
            break;
        }
        case OE_HOSTFS_OP_CLOSEDIR:
        {
            args->u.closedir.ret = closedir(args->u.closedir.dir);
            break;
        }
        case OE_HOSTFS_OP_STAT:
        {
            struct stat buf;

            if ((args->u.stat.ret = stat(args->u.stat.path, &buf)) == 0)
            {
                args->u.stat.buf.st_dev = buf.st_dev;
                args->u.stat.buf.st_ino = buf.st_ino;
                args->u.stat.buf.st_mode = buf.st_mode;
                args->u.stat.buf.st_nlink = buf.st_nlink;
                args->u.stat.buf.st_uid = buf.st_uid;
                args->u.stat.buf.st_gid = buf.st_gid;
                args->u.stat.buf.st_rdev = buf.st_rdev;
                args->u.stat.buf.st_size = buf.st_size;
                args->u.stat.buf.st_blksize = buf.st_blksize;
                args->u.stat.buf.st_blocks = buf.st_blocks;
            }
            else
            {
                memset(&args->u.stat.buf, 0, sizeof(args->u.stat.buf));
            }
            break;
        }
        case OE_HOSTFS_OP_REMOVE:
        {
            args->u.remove.ret = remove(args->u.remove.path);
            break;
        }
        case OE_HOSTFS_OP_RENAME:
        {
            args->u.rename.ret =
                rename(args->u.rename.old_path, args->u.rename.new_path);
            break;
        }
        case OE_HOSTFS_OP_MKDIR:
        {
            args->u.mkdir.ret = mkdir(args->u.mkdir.path, args->u.mkdir.mode);
            break;
        }
        case OE_HOSTFS_OP_RMDIR:
        {
            args->u.rmdir.ret = rmdir(args->u.rmdir.path);
            break;
        }
    }
}

void oe_install_hostfs(void)
{
    oe_handle_hostfs_ocall_callback = _handle_hostfs_ocall;
}
