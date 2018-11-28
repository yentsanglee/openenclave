#include <errno.h>
#include <openenclave/fs.h>
#include <openenclave/internal/fs.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

size_t musl_fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream);

size_t musl_fread(void* ptr, size_t size, size_t nmemb, FILE* stream);

int musl_fclose(FILE* stream);

long musl_ftell(FILE* stream);

int musl_fseek(FILE* stream, long offset, int whence);

int musl_fflush(FILE* stream);

int musl_ferror(FILE* stream);

int musl_feof(FILE* stream);

void musl_clearerr(FILE* stream);

int musl_fputc(int c, FILE* stream);

int musl_putc(int c, FILE* stream);

int musl_fgetc(FILE* stream);

int musl_getc(FILE* stream);

FILE* musl_fopen(const char* path, const char* mode);

char* musl_fgets(char* s, int size, FILE* stream);

/*
**==============================================================================
**
** OE I/O functions.
**
**==============================================================================
*/

static oe_fs_t* _default_fs = NULL;

bool oe_fs_set_default(oe_fs_t* fs)
{
    if (!oe_fs_is_valid(fs))
        return false;

    _default_fs = fs;
    return true;
}

oe_fs_t* oe_fs_get_default(void)
{
    return _default_fs;
}

int oe_release(oe_fs_t* fs)
{
    if (!oe_fs_is_valid(fs))
        return -1;

    return oe_fs_ft(fs)->fs_release(fs);
}

FILE* oe_fopen(oe_fs_t* fs, const char* path, const char* mode, ...)
{
    FILE* ret;
    va_list ap;

    if (!oe_fs_is_valid(fs))
        return NULL;

    va_start(ap, mode);
    ret = oe_fs_ft(fs)->fs_fopen(fs, path, mode, ap);
    va_end(ap);

    return ret;
}

DIR* oe_opendir(oe_fs_t* fs, const char* name)
{
    if (!oe_fs_is_valid(fs))
        return NULL;

    return oe_fs_ft(fs)->fs_opendir(fs, name);
}

int oe_stat(oe_fs_t* fs, const char* path, struct stat* stat)
{
    if (!oe_fs_is_valid(fs))
        return -1;

    return oe_fs_ft(fs)->fs_stat(fs, path, stat);
}

int oe_remove(oe_fs_t* fs, const char* path)
{
    if (!oe_fs_is_valid(fs))
        return -1;

    return oe_fs_ft(fs)->fs_remove(fs, path);
}

int oe_rename(oe_fs_t* fs, const char* old_path, const char* new_path)
{
    if (!oe_fs_is_valid(fs))
        return -1;

    return oe_fs_ft(fs)->fs_rename(fs, old_path, new_path);
}

int oe_mkdir(oe_fs_t* fs, const char* path, unsigned int mode)
{
    if (!oe_fs_is_valid(fs))
        return -1;

    return oe_fs_ft(fs)->fs_mkdir(fs, path, mode);
}

int oe_rmdir(oe_fs_t* fs, const char* path)
{
    if (!oe_fs_is_valid(fs))
        return -1;

    return oe_fs_ft(fs)->fs_rmdir(fs, path);
}

int oe_fclose(FILE* file)
{
    if (!file)
    {
        errno = EINVAL;
        return -1;
    }

    return file->f_fclose(file);
}

size_t oe_fread(void* ptr, size_t size, size_t nmemb, FILE* file)
{
    if (!file)
    {
        errno = EINVAL;
        return -1;
    }

    return file->f_fread(ptr, size, nmemb, file);
}

size_t oe_fwrite(const void* ptr, size_t size, size_t nmemb, FILE* file)
{
    if (!file)
    {
        errno = EINVAL;
        return -1;
    }

    return file->f_fwrite(ptr, size, nmemb, file);
}

int64_t oe_ftell(FILE* file)
{
    if (!file)
    {
        errno = EINVAL;
        return -1;
    }

    return file->f_ftell(file);
}

int oe_fseek(FILE* file, int64_t offset, int whence)
{
    if (!file)
    {
        errno = EINVAL;
        return -1;
    }

    return file->f_fseek(file, offset, whence);
}

int oe_fflush(FILE* file)
{
    if (!file)
    {
        errno = EINVAL;
        return -1;
    }

    return file->f_fflush(file);
}

int oe_ferror(FILE* file)
{
    if (!file)
    {
        errno = EINVAL;
        return -1;
    }

    return file->f_ferror(file);
}

int oe_feof(FILE* file)
{
    if (!file)
    {
        errno = EINVAL;
        return -1;
    }

    return file->f_feof(file);
}

void oe_clearerr(FILE* file)
{
    if (file)
        file->f_clearerr(file);
}

struct dirent* oe_readdir(DIR* dir)
{
    if (!dir)
    {
        errno = EINVAL;
        return NULL;
    }

    return dir->d_readdir(dir);
}

int oe_closedir(DIR* dir)
{
    if (!dir)
    {
        errno = EINVAL;
        return -1;
    }

    return dir->d_closedir(dir);
}

/*
**==============================================================================
**
** Standard I/O functions:
**
**==============================================================================
*/

FILE* fopen(const char* path, const char* mode)
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

int fgetpos(FILE* stream, fpos_t* pos)
{
    long off;

    if (!stream || !pos)
    {
        errno = EINVAL;
        return -1;
    }

    if ((off = ftell(stream)) < 0)
        return -1;

    *((long long*)pos) = off;

    return 0;
}

int fseek(FILE* stream, long offset, int whence)
{
    if (stream && stream->magic == OE_FILE_MAGIC)
        return stream->f_fseek(stream, offset, whence);

    return musl_fseek(stream, offset, whence);
}

int fsetpos(FILE* stream, const fpos_t* pos)
{
    if (!stream || !pos)
    {
        errno = EINVAL;
        return -1;
    }

    if (fseek(stream, *(const long long*)pos, SEEK_SET) != 0)
        return -1;

    return 0;
}

void rewind(FILE* stream)
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

    return oe_opendir(fs, name);
}

int readdir_r(DIR* dir, struct dirent* entry, struct dirent** result)
{
    struct dirent* tmp;

    if (!dir || !entry)
    {
        errno = EINVAL;
        return -1;
    }

    if (!(tmp = dir->d_readdir(dir)))
        return -1;

    memcpy(entry, tmp, sizeof(struct dirent));

    if (result)
        *result = entry;

    return 0;
}

struct dirent* readdir(DIR* dir)
{
    if (!dir)
    {
        errno = EINVAL;
        return NULL;
    }

    return dir->d_readdir(dir);
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

int stat(const char* path, struct stat* buf)
{
    oe_fs_t* fs = oe_fs_get_default();

    if (!fs)
    {
        errno = EINVAL;
        return -1;
    }

    return oe_stat(fs, path, buf);
}

int remove(const char* pathname)
{
    oe_fs_t* fs = oe_fs_get_default();

    if (!fs)
    {
        errno = EINVAL;
        return -1;
    }

    return oe_remove(fs, pathname);
}

int rename(const char* oldpath, const char* newpath)
{
    oe_fs_t* fs = oe_fs_get_default();

    if (!fs)
    {
        errno = EINVAL;
        return -1;
    }

    return oe_rename(fs, oldpath, newpath);
}

int mkdir(const char* pathname, mode_t mode)
{
    oe_fs_t* fs = oe_fs_get_default();

    if (!fs)
    {
        errno = EINVAL;
        return -1;
    }

    return oe_mkdir(fs, pathname, mode);
}

int rmdir(const char* pathname)
{
    oe_fs_t* fs = oe_fs_get_default();

    if (!fs)
    {
        errno = EINVAL;
        return -1;
    }

    return oe_rmdir(fs, pathname);
}

int access(const char* pathname, int mode)
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

    /* TODO: resolve R_OK, W_OK, and X_OK (need uid/gid) */
    if (mode == F_OK || mode == R_OK || mode == F_OK)
    {
        return 0;
    }

    errno = EINVAL;
    return -1;
}

int fputc(int c, FILE* stream)
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

int putc(int c, FILE* stream)
{
    if (stream && stream->magic == OE_FILE_MAGIC)
    {
        char ch = (char)c;

        if (fwrite(&ch, 1, 1, stream) != 1)
            return EOF;

        return c;
    }

    return musl_putc(c, stream);
}

int fgetc(FILE* stream)
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

int getc(FILE* stream)
{
    if (stream && stream->magic == OE_FILE_MAGIC)
    {
        char ch;

        if (fread(&ch, 1, 1, stream) != 1)
            return EOF;

        return ch;
    }

    return musl_getc(stream);
}

int fputs(const char* s, FILE* stream)
{
    if (!s)
    {
        errno = EINVAL;
        return -1;
    }

    size_t len = strlen(s);

    if (fwrite(s, 1, len, stream) == len)
        return 0;

    return EOF;
}

char* fgets(char* s, int size, FILE* stream)
{
    size_t i;

    if (!s || !size || !stream)
    {
        errno = EINVAL;
        return NULL;
    }

    for (i = 0; i < size - 1; i++)
    {
        int c = fgetc(stream);

        if (c == EOF)
            break;

        s[i] = c;
    }

    s[i] = '\0';

    return s;
}
