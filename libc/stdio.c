#include <openenclave/internal/fs.h>
#include <openenclave/internal/fsinternal.h>
#include <stdio.h>
#include <errno.h>

size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream)
{
    return oe_fwrite(ptr, size, nmemb, stream);
}

size_t fread(void* ptr, size_t size, size_t nmemb, FILE* stream)
{
    return oe_fread(ptr, size, nmemb, stream);
}

int fclose(FILE *stream)
{
    return oe_fclose(stream);
}

long ftell(FILE *stream)
{
    return oe_ftell(stream);
}

int fseek(FILE *stream, long offset, int whence)
{
    return oe_fseek(stream, offset, whence);
}

int fflush(FILE *stream)
{
    return oe_fflush(stream);
}

int ferror(FILE *stream)
{
    return oe_ferror(stream);
}

int feof(FILE *stream)
{
    return oe_feof(stream);
}

void clearerr(FILE *stream)
{
    oe_clearerr(stream);
}

