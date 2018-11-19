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

int32_t oe_release(oe_fs_t* fs)
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

int32_t oe_fclose(FILE* file)
{
    if (!file || !file->f_fclose)
        return -1;

    return file->f_fclose(file);
}

size_t oe_fread(void* ptr, size_t size, size_t nmemb, FILE* file)
{
    if (!file || !file->f_fread)
        return -1;

    return file->f_fread(ptr, size, nmemb, file);
}

size_t oe_fwrite(const void* ptr, size_t size, size_t nmemb, FILE* file)
{
    if (!file || !file->f_fwrite)
        return -1;

    return file->f_fwrite(ptr, size, nmemb, file);
}

int64_t oe_ftell(FILE* file)
{
    if (!file || !file->f_ftell)
        return -1;

    return file->f_ftell(file);
}

int32_t oe_fseek(FILE* file, int64_t offset, int whence)
{
    if (!file || !file->f_fseek)
        return -1;

    return file->f_fseek(file, offset, whence);
}

int32_t oe_fflush(FILE* file)
{
    if (!file || !file->f_fflush)
        return -1;

    return file->f_fflush(file);
}

int32_t oe_ferror(FILE* file)
{
    if (!file || !file->f_ferror)
        return -1;

    return file->f_ferror(file);
}

int32_t oe_feof(FILE* file)
{
    if (!file || !file->f_feof)
        return -1;

    return file->f_feof(file);
}

void oe_clearerr(FILE* file)
{
    if (!file || !file->f_clearerr)
        return;

    file->f_clearerr(file);
}

DIR* oe_opendir(oe_fs_t* fs, const char* name, const void* args)
{
    if (!fs || !fs->fs_opendir)
        return NULL;

    return fs->fs_opendir(fs, name, args);
}

int32_t oe_readdir(DIR* dir, struct dirent* entry, struct dirent** result)
{
    if (!dir || !dir->d_readdir)
        return -1;

    return dir->d_readdir(dir, entry, result);
}

int32_t oe_closedir(DIR* dir)
{
    if (!dir || !dir->d_closedir)
        return -1;

    return dir->d_closedir(dir);
}

int32_t oe_stat(oe_fs_t* fs, const char* path, struct stat* stat)
{
    if (!fs || !fs->fs_stat)
        return -1;

    return fs->fs_stat(fs, path, stat);
}

int32_t oe_unlink(oe_fs_t* fs, const char* path)
{
    if (!fs || !fs->fs_unlink)
        return -1;

    return fs->fs_unlink(fs, path);
}

int32_t oe_rename(oe_fs_t* fs, const char* old_path, const char* new_path)
{
    if (!fs || !fs->fs_rename)
        return -1;

    return fs->fs_rename(fs, old_path, new_path);
}

int32_t oe_mkdir(oe_fs_t* fs, const char* path, unsigned int mode)
{
    if (!fs || !fs->fs_mkdir)
        return -1;

    return fs->fs_mkdir(fs, path, mode);
}

int32_t oe_rmdir(oe_fs_t* fs, const char* path)
{
    if (!fs || !fs->fs_rmdir)
        return -1;

    return fs->fs_rmdir(fs, path);
}

int oe_access(oe_fs_t* fs, const char* path, int mode)
{
    struct stat buf;

    if (oe_stat(fs, path, &buf) != 0)
        return -1;

    if (mode == F_OK)
        return 0;

    /* TODO: resolve R_OK, W_OK, and X_OK (need uid/gid) */

    return -1;
}

/*
**==============================================================================
**
** Standard I/O functions:
**
**==============================================================================
*/

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

int access(const char *pathname, int mode)
{
    oe_fs_t* fs = oe_fs_get_default();

    if (!fs)
    {
        errno = EINVAL;
        return -1;
    }

    return oe_access(fs, pathname, mode);
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

size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream)
{
    if (stream && stream->magic == OE_FILE_MAGIC)
        return oe_fwrite(ptr, size, nmemb, stream);

    return musl_fwrite(ptr, size, nmemb, stream);
}

size_t fread(void* ptr, size_t size, size_t nmemb, FILE* stream)
{
    if (stream && stream->magic == OE_FILE_MAGIC)
        return oe_fread(ptr, size, nmemb, stream);

    return musl_fread(ptr, size, nmemb, stream);
}

int fclose(FILE* stream)
{
    if (stream && stream->magic == OE_FILE_MAGIC)
        return oe_fclose(stream);

    return musl_fclose(stream);
}

long ftell(FILE* stream)
{
    if (stream && stream->magic == OE_FILE_MAGIC)
        return oe_ftell(stream);

    return musl_ftell(stream);
}

int fseek(FILE* stream, long offset, int whence)
{
    if (stream && stream->magic == OE_FILE_MAGIC)
        return oe_fseek(stream, offset, whence);

    return musl_fseek(stream, offset, whence);
}

void rewind(FILE *stream)
{
    fseek(stream, 0L, SEEK_SET);
}

int fflush(FILE* stream)
{
    if (stream && stream->magic == OE_FILE_MAGIC)
        return oe_fflush(stream);

    return musl_fflush(stream);
}

int ferror(FILE* stream)
{
    if (stream && stream->magic == OE_FILE_MAGIC)
        return oe_ferror(stream);

    return musl_ferror(stream);
}

int feof(FILE* stream)
{
    if (stream && stream->magic == OE_FILE_MAGIC)
        return oe_feof(stream);

    return musl_feof(stream);
}

void clearerr(FILE* stream)
{
    if (stream && stream->magic == OE_FILE_MAGIC)
        oe_clearerr(stream);

    musl_clearerr(stream);
}

int readdir_r(DIR* dir, struct dirent* entry, struct dirent** result)
{
    return oe_readdir(dir, entry, result);
}

int closedir(DIR* dir)
{
    return oe_closedir(dir);
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

FILE *fopen(const char *path, const char *mode)
{
    oe_fs_t* fs = oe_fs_get_default();

    if (fs)
        return oe_fopen(fs, path, mode, NULL);

    return musl_fopen(path, mode);
}
