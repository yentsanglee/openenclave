#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <pthread.h>

static char _cwd[PATH_MAX] = "/";
static pthread_mutex_t _lock = PTHREAD_MUTEX_INITIALIZER;

char *getcwd(char *buf, size_t size)
{
    char* ret = NULL;
    char tmp[PATH_MAX];

    if (!buf)
    {
        buf = tmp;
        size = PATH_MAX;
    }
    else if (!size)
    {
        errno = EINVAL;
        goto done;
    }

    pthread_mutex_lock(&_lock);
    {
        if (strlen(_cwd) + 1 > size)
        {
            errno = ERANGE;
            pthread_mutex_unlock(&_lock);
            goto done;
        }

        strcpy(buf, _cwd);
    }
    pthread_mutex_unlock(&_lock);

    if (tmp == buf)
        ret = strdup(buf);
    else
        ret = buf;

done:
    return ret;
}

int chdir(const char* path)
{
    return -1;
}
