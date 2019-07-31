// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _PLATFORM_WINDOWS_H
#define _PLATFORM_WINDOWS_H

#pragma warning(disable : 4005)

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>

#include <direct.h>
#include <openenclave/corelibc/errno.h>
// clang-format off
#include <winsock2.h>
#include <windows.h>
// clang-format on
//
#include <stdio.h>

typedef SOCKET socket_t;
typedef int socklen_t;
typedef unsigned short in_port_t;
typedef int length_t;
typedef void pthread_attr_t;

struct errno_tab_entry
{
    DWORD winerr;
    int error_no;
};

static struct errno_tab_entry errno2winsockerr[] = {
    {WSAEINTR, OE_EINTR},
    {WSAEBADF, OE_EBADF},
    {WSAEACCES, OE_EACCES},
    {WSAEFAULT, OE_EFAULT},
    {WSAEINVAL, OE_EINVAL},
    {WSAEMFILE, OE_EMFILE},
    {WSAEWOULDBLOCK, OE_EWOULDBLOCK},
    {WSAEINPROGRESS, OE_EINPROGRESS},
    {WSAEALREADY, OE_EALREADY},
    {WSAENOTSOCK, OE_ENOTSOCK},
    {WSAEDESTADDRREQ, OE_EDESTADDRREQ},
    {WSAEMSGSIZE, OE_EMSGSIZE},
    {WSAEPROTOTYPE, OE_EPROTOTYPE},
    {WSAENOPROTOOPT, OE_ENOPROTOOPT},
    {WSAEPROTONOSUPPORT, OE_EPROTONOSUPPORT},
    {WSAESOCKTNOSUPPORT, OE_ESOCKTNOSUPPORT},
    {WSAEOPNOTSUPP, OE_EOPNOTSUPP},
    {WSAEPFNOSUPPORT, OE_EPFNOSUPPORT},
    {WSAEAFNOSUPPORT, OE_EAFNOSUPPORT},
    {WSAEADDRINUSE, OE_EADDRINUSE},
    {WSAEADDRNOTAVAIL, OE_EADDRNOTAVAIL},
    {WSAENETDOWN, OE_ENETDOWN},
    {WSAENETUNREACH, OE_ENETUNREACH},
    {WSAENETRESET, OE_ENETRESET},
    {WSAECONNABORTED, OE_ECONNABORTED},
    {WSAECONNRESET, OE_ECONNRESET},
    {WSAENOBUFS, OE_ENOBUFS},
    {WSAEISCONN, OE_EISCONN},
    {WSAENOTCONN, OE_ENOTCONN},
    {WSAESHUTDOWN, OE_ESHUTDOWN},
    {WSAETOOMANYREFS, OE_ETOOMANYREFS},
    {WSAETIMEDOUT, OE_ETIMEDOUT},
    {WSAECONNREFUSED, OE_ECONNREFUSED},
    {WSAELOOP, OE_ELOOP},
    {WSAENAMETOOLONG, OE_ENAMETOOLONG},
    {WSAEHOSTDOWN, OE_EHOSTDOWN},
    {WSAEHOSTUNREACH, OE_EHOSTUNREACH},
    {WSAENOTEMPTY, OE_ENOTEMPTY},
    {WSAEUSERS, OE_EUSERS},
    {WSAEDQUOT, OE_EDQUOT},
    {WSAESTALE, OE_ESTALE},
    {WSAEREMOTE, OE_EREMOTE},
    {WSAEDISCON, 199},
    {WSAEPROCLIM, 200},
    {WSASYSNOTREADY, 201}, // Made up number but close to adjacent
    {WSAVERNOTSUPPORTED, 202},
    {WSANOTINITIALISED, 203},
    {0, 0}};

OE_INLINE int _winsockerr_to_errno(DWORD winsockerr)
{
    struct errno_tab_entry* pent = errno2winsockerr;

    do
    {
        if (pent->winerr == winsockerr)
        {
            return pent->error_no;
        }
        pent++;

    } while (pent->winerr != 0);

    return OE_EINVAL;
}

OE_INLINE int sleep(unsigned int seconds)
{
    Sleep(seconds * 1000);
    return 0;
}

OE_INLINE void sock_startup(void)
{
    static WSADATA wsadata = {0};
    WSAStartup(MAKEWORD(2, 2), &wsadata);
}

OE_INLINE void sock_cleanup(void)
{
    WSACleanup();
}

OE_INLINE int sock_set_blocking(socket_t sock, bool blocking)
{
    unsigned long flag = blocking ? 0 : 1;

    if (ioctlsocket(sock, FIONBIO, &flag) != 0)
        return -1;

    return 0;
}

OE_INLINE ssize_t
sock_send(socket_t sockfd, const void* buf, size_t len, int flags)
{
    return send(sockfd, (const char*)buf, (int)len, flags);
}

OE_INLINE ssize_t sock_recv(socket_t sockfd, void* buf, size_t len, int flags)
{
    ssize_t ret = recv(sockfd, (char*)buf, (int)len, flags);
    if (ret < 0)
      _set_errno(_winsockerr_to_errno(WSAGetLastError()));

    return ret;
}

OE_INLINE int sock_close(socket_t sock)
{
    return closesocket(sock);
}

OE_INLINE int sock_select(
    socket_t nfds,
    fd_set* readfds,
    fd_set* writefds,
    fd_set* exceptfds,
    struct timeval* timeout)
{
    OE_UNUSED(nfds);
    return select(0, readfds, writefds, exceptfds, timeout);
}

OE_INLINE int get_error(void)
{
    return WSAGetLastError();
}

typedef struct _thread
{
    HANDLE __impl;
} thread_t;

typedef struct _thread_proc_param
{
    void* (*start_routine)(void*);
    void* arg;
} thread_proc_param_t;

static DWORD _thread_proc(void* param_)
{
    thread_proc_param_t* param = (thread_proc_param_t*)param_;

    (*param->start_routine)(param->arg);

    free(param);

    return 0;
}

OE_INLINE int thread_create(
    thread_t* thread,
    void* (*start_routine)(void*),
    void* arg)
{
    HANDLE handle;
    thread_proc_param_t* param;

    if (!(param = (thread_proc_param_t*)calloc(1, sizeof(thread_proc_param_t))))
        return -1;

    param->start_routine = start_routine;
    param->arg = arg;

    handle = CreateThread(NULL, 0, _thread_proc, param, 0, NULL);

    if (handle == INVALID_HANDLE_VALUE)
        return -1;

    thread->__impl = handle;
    return 0;
}

OE_INLINE int thread_join(thread_t thread)
{
    if (WaitForSingleObject(thread.__impl, 60000) == WAIT_OBJECT_0)
        return 0;

    return -1;
}

OE_INLINE void sleep_msec(uint32_t msec)
{
    Sleep(msec);
}

OE_INLINE bool test_would_block()
{
    return WSAGetLastError() == WSAEWOULDBLOCK;
}


// Allocates char* string which follows the expected rules for
// enclaves. Paths in the format
// <driveletter>:\<item>\<item> -> /<driveletter>/<item>/item>
// <driveletter>:/<item>/<item> -> /<driveletter>/<item>/item>
// paths without drive letter are detected and the drive added
// /<item>/<item> -> /<current driveletter>/<item>/item>
// relative paths are translated to absolute with drive letter
// returns null if the string is illegal
//
// The string  must be freed
// ATTN: we don't handle paths which start with the "\\?\" thing. don't really
// think we need them
//
OE_INLINE char* win_path_to_posix(const char* path)
{
    size_t required_size = 0;
    size_t current_dir_len = 0;
    char* current_dir = NULL;
    char* enclave_path = NULL;

    if (!path)
    {
        return NULL;
    }
    // Relative or incomplete path?

    // absolute path with drive letter.
    // we do not handle device type paths ("CON:) or double-letter paths in case
    // of really large numbers of disks (>26). If you have those, mount on
    // windows
    //
    if (isalpha(path[0]) && path[1] == ':')
    {
        // Abosolute path is drive letter
        required_size = strlen(path) + 1;
    }
    else if (path[0] == '/' || path[0] == '\\')
    {
        required_size = strlen(path) + 3; // Add a drive letter to the path
    }
    else
    {
        current_dir = _getcwd(NULL, 32767);
        current_dir_len = strlen(current_dir);

        if (isalpha(*current_dir) && (current_dir[1] == ':'))
        {
            // This is expected. We convert drive: to /drive.

            char drive_letter = *current_dir;
            *current_dir = '/';
            current_dir[1] = drive_letter;
        }
        // relative path. If the path starts with "." or ".." we accomodate
        required_size = strlen(path) + current_dir_len + 1;
    }

    enclave_path = (char*)calloc(1, required_size);

    const char* psrc = path;
    const char* plimit = path + strlen(path);
    char* pdst = enclave_path;

    if (isalpha(*psrc) && psrc[1] == ':')
    {
        *pdst++ = '/';
        *pdst++ = *psrc;
        psrc += 2;
    }
    else if (*psrc == '/')
    {
        *pdst++ = '/';
        *pdst++ = _getdrive() + 'a';
    }
    else if (*psrc == '.')
    {
        memcpy(pdst, current_dir, current_dir_len);
        if (psrc[1] == '/' || psrc[1] == '\\')
        {
            pdst += current_dir_len;
            psrc++;
        }
        else if (psrc[1] == '.' && (psrc[2] == '/' || psrc[2] == '\\'))
        {
            char* rstr = strrchr(
                current_dir, '\\'); // getcwd always returns at least '\'
            pdst += current_dir_len - (rstr - current_dir);
            // When we shortend the curdir by 1 slash, we perform the ".."
            // operation we could leave it in here, but at least sometimes this
            // will allow a path that would otherwise be too long
            psrc += 2;
        }
        else
        {
            // It is an incomplete which starts with a file which starts with .
            // so we dont increment psrc at all
            pdst += current_dir_len;
            *pdst = '/';
        }
    }
    else
    {
        // Still a relative path
        memcpy(pdst, current_dir, current_dir_len);
        pdst += current_dir_len;
        *pdst++ = '/';
    }

    // Since we have to translater slashes, use a loop rather than memcpy
    while (psrc < plimit)
    {
        if (*psrc == '\\')
        {
            *pdst = '/';
        }
        else
        {
            *pdst = *psrc;
        }
        psrc++;
        pdst++;
    }
    *pdst = '\0';

    if (current_dir)
    {
        free(current_dir);
    }
    return enclave_path;
}

// Allocates WCHAR* string which follows the expected rules for
// enclaves comminication with the host file system API. Paths in the format
// /<driveletter>/<item>/<item>  become <driveletter>:/<item>/<item>
//
// The resulting string, especially with a relative path, will probably contain
// mixed slashes. We beleive Windows handles this.
//
// Adds the string "post" to the resulting string end
//
// The string  must be freed
OE_INLINE WCHAR* posix_path_to_win(const char* path, const char* post)
{
    size_t required_size = 0;
    size_t current_dir_len = 0;
    char* current_dir = NULL;
    int pathlen = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0);
    size_t postlen = MultiByteToWideChar(CP_UTF8, 0, post, -1, NULL, 0);
    if (post)
    {
        postlen = MultiByteToWideChar(CP_UTF8, 0, post, -1, NULL, 0);
    }

    WCHAR* wpath = NULL;

    if (path[0] == '/')
    {
        if (isalpha(path[1]) && path[2] == '/')
        {
            wpath =
                (WCHAR*)(calloc((pathlen + postlen + 1) * sizeof(WCHAR), 1));
            MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, (int)pathlen);
            if (postlen)
            {
                MultiByteToWideChar(
                    CP_UTF8, 0, post, -1, wpath + pathlen - 1, (int)postlen);
            }
            WCHAR drive_letter = wpath[1];
            wpath[0] = drive_letter;
            wpath[1] = ':';
        }
        else
        {
            // Absolute path needs drive letter
            wpath =
                (WCHAR*)(calloc((pathlen + postlen + 3) * sizeof(WCHAR), 1));
            MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath + 2, (int)pathlen);
            if (postlen)
            {
                MultiByteToWideChar(
                    CP_UTF8, 0, post, -1, wpath + pathlen - 1, (int)postlen);
            }
            WCHAR drive_letter = _getdrive() + 'A';
            wpath[0] = drive_letter;
            wpath[1] = ':';
        }
    }
    else
    {
        // Relative path
        WCHAR* current_dir = _wgetcwd(NULL, 32767);
        if (!current_dir)
        {
            _set_errno(ENOMEM);
            return NULL;
        }
        size_t current_dir_len = wcslen(current_dir);

        wpath = (WCHAR*)(calloc(
            (pathlen + current_dir_len + postlen + 1) * sizeof(WCHAR), 1));
        memcpy(wpath, current_dir, current_dir_len);
        wpath[current_dir_len] = '/';
        MultiByteToWideChar(
            CP_UTF8, 0, path, -1, wpath + current_dir_len, pathlen);
        if (postlen)
        {
            MultiByteToWideChar(
                CP_UTF8,
                0,
                path,
                -1,
                wpath + current_dir_len + pathlen - 1,
                (int)postlen);
        }

        free(current_dir);
    }
    return wpath;
}


#endif /* _PLATFORM_WINDOWS_H */
