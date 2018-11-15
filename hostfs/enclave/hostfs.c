#define _GNU_SOURCE
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "../common/hostfsargs.h"
#include "../common/hostfs.h"
#include "hostbatch.h"
#include <openenclave/internal/calls.h>

#define BATCH_SIZE 4096

typedef oe_hostfs_args_t args_t;

static oe_host_batch_t* _get_host_batch(void)
{
    static oe_host_batch_t* batch;
    static pthread_spinlock_t lock;

    if (batch == NULL)
    {
        pthread_spin_lock(&lock);

        if (batch == NULL)
            batch = oe_host_batch_new(BATCH_SIZE);

        pthread_spin_unlock(&lock);
    }

    return batch;
}

typedef struct _file
{
    oe_file_t base;
    void* host_file;
}
file_t;

static int32_t _f_fclose(oe_file_t* base)
{
    int32_t ret = -1;
    file_t* file = (file_t*)base;
    oe_host_batch_t* batch = _get_host_batch();
    args_t* args = NULL;

    if (!file || !file->host_file || !batch)
        goto done;

    /* Input */
    {
        if (!(args = oe_host_batch_calloc(batch, sizeof(args_t))))
            goto done;

        args->u.fclose.ret = -1;
        args->op = OE_HOSTFS_OP_FCLOSE;
        args->u.fclose.file = file->host_file;
    }
    
    /* Call */
    {
        if (oe_ocall(OE_OCALL_HOSTFS, (uint64_t)args, NULL) != OE_OK)
            goto done;
    }

    /* Output */
    {
        if ((ret = args->u.fclose.ret) != 0)
            goto done;
    }
    
    free(file);

done:

    if (args)
        oe_host_batch_free(batch);

    return ret;
}

static size_t _f_fread(void* ptr, size_t size, size_t nmemb, oe_file_t* base)
{
    size_t ret = 0;
    file_t* file = (file_t*)base;
    oe_host_batch_t* batch = _get_host_batch();
    args_t* args = NULL;

    if (!ptr || !file || !file->host_file)
        goto done;

    /* Input */
    {
        size_t n = size + nmemb;

        if (!(args = oe_host_batch_calloc(batch, sizeof(args_t) + n)))
            goto done;

        memset(ptr, 0, n);
        args->u.fread.ret = -1;
        args->op = OE_HOSTFS_OP_FREAD;
        args->u.fread.size = size;
        args->u.fread.nmemb = nmemb;
        args->u.fread.file = file->host_file;
    }
    
    /* Call */
    {
        if (oe_ocall(OE_OCALL_HOSTFS, (uint64_t)args, NULL) != OE_OK)
            goto done;
    }

    /* Output */
    {
        if ((ret = args->u.fread.ret) > 0)
            memcpy(ptr, args->buf, ret);
    }

done:

    if (args)
        oe_host_batch_free(batch);

    return ret;
}

static size_t _f_fwrite(
    const void* ptr, size_t size, size_t nmemb, oe_file_t* base)
{
    size_t ret = 0;
    file_t* file = (file_t*)base;
    oe_host_batch_t* batch = _get_host_batch();
    args_t* args = NULL;

    if (!ptr || !file || !file->host_file || !batch)
        goto done;

    /* Input */
    {
        size_t n = size + nmemb;

        if (!(args = oe_host_batch_calloc(batch, sizeof(args_t) + n)))
            goto done;

        args->u.fwrite.ret = -1;
        args->op = OE_HOSTFS_OP_FWRITE;
        args->u.fwrite.size = size;
        args->u.fwrite.nmemb = nmemb;
        args->u.fwrite.file = file->host_file;
        memcpy(args->buf, ptr, n);
    }
    
    /* Call */
    {
        if (oe_ocall(OE_OCALL_HOSTFS, (uint64_t)args, NULL) != OE_OK)
            goto done;
    }

    /* Output */
    {
        ret = args->u.fwrite.ret;
    }

done:

    if (args)
        oe_host_batch_free(batch);

    return ret;
}

static int64_t _f_ftell(oe_file_t* base)
{
    int64_t ret = -1;
    file_t* file = (file_t*)base;
    oe_host_batch_t* batch = _get_host_batch();
    args_t* args = NULL;

    if (!file || !file->host_file || !batch)
        goto done;

    /* Input */
    {
        if (!(args = oe_host_batch_calloc(batch, sizeof(args_t))))
            goto done;

        args->u.ftell.ret = -1;
        args->op = OE_HOSTFS_OP_FTELL;
        args->u.ftell.file = file->host_file;
    }
    
    /* Call */
    {
        if (oe_ocall(OE_OCALL_HOSTFS, (uint64_t)args, NULL) != OE_OK)
            goto done;
    }

    /* Output */
    {
        ret = args->u.ftell.ret;
    }

done:

    if (args)
        oe_host_batch_free(batch);

    return ret;
}

static int32_t _f_fseek(oe_file_t* base, int64_t offset, int whence)
{
    int64_t ret = -1;
    file_t* file = (file_t*)base;
    oe_host_batch_t* batch = _get_host_batch();
    args_t* args = NULL;

    if (!file || !file->host_file || !batch)
        goto done;

    /* Input */
    {
        if (!(args = oe_host_batch_calloc(batch, sizeof(args_t))))
            goto done;

        args->u.fseek.ret = -1;
        args->op = OE_HOSTFS_OP_FSEEK;
        args->u.fseek.file = file->host_file;
        args->u.fseek.offset = offset;
        args->u.fseek.whence = whence;
    }
    
    /* Call */
    {
        if (oe_ocall(OE_OCALL_HOSTFS, (uint64_t)args, NULL) != OE_OK)
            goto done;
    }

    /* Output */
    {
        ret = args->u.fseek.ret;
    }

done:

    if (args)
        oe_host_batch_free(batch);

    return ret;
}

static int32_t _f_fflush(oe_file_t* base)
{
    int64_t ret = -1;
    file_t* file = (file_t*)base;
    oe_host_batch_t* batch = _get_host_batch();
    args_t* args = NULL;

    if (!file || !file->host_file || !batch)
        goto done;

    /* Input */
    {
        if (!(args = oe_host_batch_calloc(batch, sizeof(args_t))))
            goto done;

        args->u.fflush.ret = -1;
        args->op = OE_HOSTFS_OP_FFLUSH;
        args->u.fflush.file = file->host_file;
    }
    
    /* Call */
    {
        if (oe_ocall(OE_OCALL_HOSTFS, (uint64_t)args, NULL) != OE_OK)
            goto done;
    }

    /* Output */
    {
        ret = args->u.fflush.ret;
    }

done:

    if (args)
        oe_host_batch_free(batch);

    return ret;
}

static int32_t _f_ferror(oe_file_t* base)
{
    int64_t ret = -1;
    file_t* file = (file_t*)base;
    oe_host_batch_t* batch = _get_host_batch();
    args_t* args = NULL;

    if (!file || !file->host_file || !batch)
        goto done;

    /* Input */
    {
        if (!(args = oe_host_batch_calloc(batch, sizeof(args_t))))
            goto done;

        args->u.ferror.ret = -1;
        args->op = OE_HOSTFS_OP_FERROR;
        args->u.ferror.file = file->host_file;
    }
    
    /* Call */
    {
        if (oe_ocall(OE_OCALL_HOSTFS, (uint64_t)args, NULL) != OE_OK)
            goto done;
    }

    /* Output */
    {
        ret = args->u.ferror.ret;
    }

done:

    if (args)
        oe_host_batch_free(batch);

    return ret;
}

static int32_t _f_feof(oe_file_t* base)
{
    int64_t ret = -1;
    file_t* file = (file_t*)base;
    oe_host_batch_t* batch = _get_host_batch();
    args_t* args = NULL;

    if (!file || !file->host_file || !batch)
        goto done;

    /* Input */
    {
        if (!(args = oe_host_batch_calloc(batch, sizeof(args_t))))
            goto done;

        args->u.feof.ret = -1;
        args->op = OE_HOSTFS_OP_FEOF;
        args->u.feof.file = file->host_file;
    }
    
    /* Call */
    {
        if (oe_ocall(OE_OCALL_HOSTFS, (uint64_t)args, NULL) != OE_OK)
            goto done;
    }

    /* Output */
    {
        ret = args->u.feof.ret;
    }

done:

    if (args)
        oe_host_batch_free(batch);

    return ret;
}

static int32_t _f_clearerr(oe_file_t* base)
{
    int64_t ret = -1;
    file_t* file = (file_t*)base;
    oe_host_batch_t* batch = _get_host_batch();
    args_t* args = NULL;

    if (!file || !file->host_file || !batch)
        goto done;

    /* Input */
    {
        if (!(args = oe_host_batch_calloc(batch, sizeof(args_t))))
            goto done;

        args->u.clearerr.ret = -1;
        args->op = OE_HOSTFS_OP_CLEARERR;
        args->u.clearerr.file = file->host_file;
    }
    
    /* Call */
    {
        if (oe_ocall(OE_OCALL_HOSTFS, (uint64_t)args, NULL) != OE_OK)
            goto done;
    }

    /* Output */
    {
        ret = args->u.clearerr.ret;
    }

done:

    if (args)
        oe_host_batch_free(batch);

    return ret;
}

static oe_file_t* _fs_fopen(
    oe_fs_t* fs,
    const char* path,
    const char* mode,
    const void* args_)
{
    oe_file_t* ret = NULL;
    file_t* file = NULL;
    oe_host_batch_t* batch = _get_host_batch();
    args_t* args = NULL;

    if (!path || !mode || !batch)
        goto done;

    /* Input */
    {
        if (!(args = oe_host_batch_calloc(batch, sizeof(args_t))))
            goto done;

        args->op = OE_HOSTFS_OP_FOPEN;
        args->u.fopen.ret = NULL;
        strlcpy(args->u.fopen.path, path, sizeof(args->u.fopen.path));
        strlcpy(args->u.fopen.mode, mode, sizeof(args->u.fopen.mode));
    }
    
    /* Call */
    {
        if (oe_ocall(OE_OCALL_HOSTFS, (uint64_t)args, NULL) != OE_OK)
            goto done;

        if (args->u.fopen.ret == NULL)
            goto done;
    }
    
    /* Output */
    {
        if (!(file = calloc(1, sizeof(file_t))))
            goto done;

        file->base.f_fclose = _f_fclose;
        file->base.f_fread = _f_fread;
        file->base.f_fwrite = _f_fwrite;
        file->base.f_ftell = _f_ftell;
        file->base.f_fseek = _f_fseek;
        file->base.f_fflush = _f_fflush;
        file->base.f_ferror = _f_ferror;
        file->base.f_feof = _f_feof;
        file->base.f_clearerr = _f_clearerr;
        file->host_file = args->u.fopen.ret;
    }

    ret = &file->base;
    file = NULL;

done:

    if (file)
        free(file);

    if (args)
        oe_host_batch_free(batch);

    return ret;
}

oe_fs_t oe_hostfs =
{
    .fs_fopen = _fs_fopen
};
