#include <openenclave/internal/fs.h>
#include <openenclave/internal/fsinternal.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

size_t musl_fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream);

size_t musl_fread(void* ptr, size_t size, size_t nmemb, FILE* stream);

int musl_fclose(FILE *stream);

long musl_ftell(FILE *stream);

int musl_fseek(FILE *stream, long offset, int whence);

int musl_fflush(FILE *stream);

int musl_ferror(FILE *stream);

int musl_feof(FILE *stream);

void musl_clearerr(FILE *stream);

int musl_fputc(int c, FILE *stream);

int musl_fgetc(FILE *stream);

FILE *musl_fopen(const char *path, const char *mode);

/*
**==============================================================================
**
** OE I/O functions.
**
**==============================================================================
*/

oe_fs_t* __oe_default_fs;

void oe_fs_set_default(oe_fs_t* fs)
{
    __oe_default_fs = fs;
}

oe_fs_t* oe_fs_get_default(void)
{
    return __oe_default_fs;
}

int oe_release(oe_fs_t* fs)
{
    if (!fs || !fs->fs_release)
        return -1;

    return fs->fs_release(fs);
}

FILE* oe_fopen(
    oe_fs_t* fs,
    const char* path,
    const char* mode,
    const void* args)
{
    if (!fs || !fs->fs_fopen)
        return NULL;

    return fs->fs_fopen(fs, path, mode, args);
}

DIR* oe_opendir(oe_fs_t* fs, const char* name, const void* args)
{
    if (!fs || !fs->fs_opendir)
        return NULL;

    return fs->fs_opendir(fs, name, args);
}

int oe_stat(oe_fs_t* fs, const char* path, struct stat* stat)
{
    if (!fs || !fs->fs_stat)
        return -1;

    return fs->fs_stat(fs, path, stat);
}

int oe_unlink(oe_fs_t* fs, const char* path)
{
    if (!fs || !fs->fs_unlink)
        return -1;

    return fs->fs_unlink(fs, path);
}

int oe_rename(oe_fs_t* fs, const char* old_path, const char* new_path)
{
    if (!fs || !fs->fs_rename)
        return -1;

    return fs->fs_rename(fs, old_path, new_path);
}

int oe_mkdir(oe_fs_t* fs, const char* path, unsigned int mode)
{
    if (!fs || !fs->fs_mkdir)
        return -1;

    return fs->fs_mkdir(fs, path, mode);
}

int oe_rmdir(oe_fs_t* fs, const char* path)
{
    if (!fs || !fs->fs_rmdir)
        return -1;

    return fs->fs_rmdir(fs, path);
}

/*
**==============================================================================
**
** Standard I/O functions:
**
**==============================================================================
*/

FILE *fopen(const char *path, const char *mode)
{
    oe_fs_t* fs = oe_fs_get_default();

    if (fs)
        return oe_fopen(fs, path, mode, NULL);

    return musl_fopen(path, mode);
}

int fclose(FILE* stream)
{
    if (stream && stream->magic == OE_FILE_MAGIC)
        return stream->f_fclose(stream);

    return musl_fclose(stream);
}

size_t fread(void* ptr, size_t size, size_t nmemb, FILE* stream)
{
    if (stream && stream->magic == OE_FILE_MAGIC)
        return stream->f_fread(ptr, size, nmemb, stream);

    return musl_fread(ptr, size, nmemb, stream);
}

size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream)
{
    if (stream && stream->magic == OE_FILE_MAGIC)
        return stream->f_fwrite(ptr, size, nmemb, stream);

    return musl_fwrite(ptr, size, nmemb, stream);
}

long ftell(FILE* stream)
{
    if (stream && stream->magic == OE_FILE_MAGIC)
        return stream->f_ftell(stream);

    return musl_ftell(stream);
}

int fseek(FILE* stream, long offset, int whence)
{
    if (stream && stream->magic == OE_FILE_MAGIC)
        return stream->f_fseek(stream, offset, whence);

    return musl_fseek(stream, offset, whence);
}

void rewind(FILE *stream)
{
    fseek(stream, 0L, SEEK_SET);
}

int fflush(FILE* stream)
{
    if (stream && stream->magic == OE_FILE_MAGIC)
        return stream->f_fflush(stream);

    return musl_fflush(stream);
}

int ferror(FILE* stream)
{
    if (stream && stream->magic == OE_FILE_MAGIC)
        return stream->f_ferror(stream);

    return musl_ferror(stream);
}

int feof(FILE* stream)
{
    if (stream && stream->magic == OE_FILE_MAGIC)
        return stream->f_feof(stream);

    return musl_feof(stream);
}

void clearerr(FILE* stream)
{
    if (stream && stream->magic == OE_FILE_MAGIC)
        stream->f_clearerr(stream);

    musl_clearerr(stream);
}

DIR* opendir(const char* name)
{
    oe_fs_t* fs = oe_fs_get_default();

    if (!fs)
    {
        errno = EINVAL;
        return NULL;
    }

    return oe_opendir(fs, name, NULL);
}

int readdir_r(DIR* dir, struct dirent* entry, struct dirent** result)
{
    if (!dir)
    {
        errno = EINVAL;
        return -1;
    }

    return dir->d_readdir(dir, entry, result);
}

int closedir(DIR* dir)
{
    if (!dir)
    {
        errno = EINVAL;
        return -1;
    }

    return dir->d_closedir(dir);
}

int stat(const char *path, struct stat *buf)
{
    oe_fs_t* fs = oe_fs_get_default();

    if (!fs)
    {
        errno = EINVAL;
        return -1;
    }

    return oe_stat(fs, path, buf);
}

int unlink(const char *pathname)
{
    oe_fs_t* fs = oe_fs_get_default();

    if (!fs)
    {
        errno = EINVAL;
        return -1;
    }

    return oe_unlink(fs, pathname);
}

int rename(const char *oldpath, const char *newpath)
{
    oe_fs_t* fs = oe_fs_get_default();

    if (!fs)
    {
        errno = EINVAL;
        return -1;
    }

    return oe_rename(fs, oldpath, newpath);
}

int mkdir(const char *pathname, mode_t mode)
{
    oe_fs_t* fs = oe_fs_get_default();

    if (!fs)
    {
        errno = EINVAL;
        return -1;
    }

    return oe_mkdir(fs, pathname, mode);
}

int rmdir(const char *pathname)
{
    oe_fs_t* fs = oe_fs_get_default();

    if (!fs)
    {
        errno = EINVAL;
        return -1;
    }

    return oe_rmdir(fs, pathname);
}

int access(const char *pathname, int mode)
{
    oe_fs_t* fs = oe_fs_get_default();
    struct stat buf;

    if (!fs)
    {
        errno = EINVAL;
        return -1;
    }

    if (oe_stat(fs, pathname, &buf) != 0)
        return -1;

    if (mode == F_OK)
        return 0;

    /* TODO: resolve R_OK, W_OK, and X_OK (need uid/gid) */

    errno = EINVAL;
    return -1;
}

int fputc(int c, FILE *stream)
{
    if (stream && stream->magic == OE_FILE_MAGIC)
    {
        char ch = (char)c;

        if (fwrite(&ch, 1, 1, stream) != 1)
            return EOF;

        return c;
    }

    return musl_fputc(c, stream);
}

int fgetc(FILE *stream)
{
    if (stream && stream->magic == OE_FILE_MAGIC)
    {
        char ch;

        if (fread(&ch, 1, 1, stream) != 1)
            return EOF;

        return ch;
    }

    return musl_fgetc(stream);
}
