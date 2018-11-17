#include <errno.h>
#include <openenclave/internal/fs.h>
#include <openenclave/internal/fsinternal.h>
#include <stdio.h>

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
