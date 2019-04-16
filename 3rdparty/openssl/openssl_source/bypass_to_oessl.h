/*
 * Copyright (C) 2011-2017 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
 
#ifndef _BYPASS_TO_SGXSSL_
#define _BYPASS_TO_SGXSSL_

#ifdef _WIN32
#define __declspec(dllimport) 

#ifndef _USE_32BIT_TIME_T
    #define _stat64i32	oe__stat64i32
#endif /* _USE_32BIT_TIME_T */

/*fileapi.h*/
#define FindClose oe_FindClose
#define FindFirstFileA      oe_FindFirstFileA
#define FindFirstFileW      oe_FindFirstFileW
#define FindNextFileA       oe_FindNextFileA
#define FindNextFileW       oe_FindNextFileW
#define GetFileType oe_GetFileType
#define WriteFile oe_WriteFile


/*stddef.h*/
#define _errno       oe__errno
/*stdio.h*/
#define _vsnprintf    oe__vsnprintf
#define _snprintf     oe__snprintf
#define _vsnwprintf   oe__vsnwprintf
#define fclose        oe_fclose
#define feof    oe_feof
#define ferror oe_ferror
#define fflush        oe_fflush
#define fgets oe_fgets
#define _fileno oe__fileno
#define fopen oe_fopen
#define fputs oe_fputs
#define fread   oe_fread
#define fseek oe_fseek
#define ftell oe_ftell
#define fwrite  oe_fwrite
#define vfprintf      oe_vfprintf
#define fprintf oe_fprintf
#define printf oe_printf
#define sscanf        oe_sscanf

/*stdlib.h*/
#define _exit        oe__exit
//#define getenv       oe_getenv

/*string.h*/
#define strerror_s   oe_strerror_s
#define _strdup      oe__strdup
#define _stricmp        oe__stricmp
#define _strnicmp       oe__strnicmp
#define strcat          oe_strcat
#define strcpy          oe_strcpy
#define _wassert        oe__wassert 
#define wcscpy		oe_wcscpy

/*conio.h*/
#define _getch  oe__getch

/*processthreadsapi.h*/
#define GetCurrentThreadId        oe_GetCurrentThreadId
#define TlsAlloc oe_TlsAlloc
#define TlsGetValue oe_TlsGetValue
#define TlsSetValue oe_TlsSetValue
#define TlsFree oe_TlsFree

/*synchapi.h*/
#define EnterCriticalSection oe_EnterCriticalSection
#define LeaveCriticalSection oe_LeaveCriticalSection
#define InitializeCriticalSectionAndSpinCount oe_InitializeCriticalSectionAndSpinCount
#define DeleteCriticalSection oe_DeleteCriticalSection


/*WinSock2.h*/
#define WSAGetLastError    oe_WSAGetLastError
#define closesocket        oe_closesocket
#define recv       oe_recv
#define send       oe_send
#define WSASetLastError oe_WSASetLastError

/*WinUser.h*/
#define GetProcessWindowStation     oe_GetProcessWindowStation
#define GetUserObjectInformationW   oe_GetUserObjectInformationW
#define MessageBoxA oe_MessageBoxA
#define MessageBoxW oe_MessageBoxW
#define GetDesktopWindow    oe_GetDesktopWindow

/*WinBase.h*/
#define DeregisterEventSource oe_DeregisterEventSource
#define RegisterEventSourceA oe_RegisterEventSourceA
#define RegisterEventSourceW oe_RegisterEventSourceW
#define ReportEventA oe_ReportEventA
#define ReportEventW oe_ReportEventW
#define QueryPerformanceCounter oe_QueryPerformanceCounter
#define GetCurrentProcessId oe_GetCurrentProcessId
#define BCryptGenRandom oe_BCryptGenRandom
#define OutputDebugStringW oe_OutputDebugStringW
#define GetEnvironmentVariableW oe_GetEnvironmentVariableW

/*errhandlingapi.h*/
#define GetLastError oe_GetLastError
#define SetLastError oe_SetLastError


/*errno.h*/
#define _errno   oe__errno

/*io.h*/
#define _setmode oe__setmode

/*libloaderapi.h*/
#define GetModuleHandleA       oe_GetModuleHandleA
#define GetModuleHandleW       oe_GetModuleHandleW
#define GetProcAddress oe_GetProcAddress

/*processenv.h*/
#define GetStdHandle     oe_GetStdHandle

/*signal.h*/
#define signal oe_signal
#define raise oe_raise

/*stringapiset.h*/
#define MultiByteToWideChar oe_MultiByteToWideChar
#define WideCharToMultiByte oe_WideCharToMultiByte
/*sys/timeb.h**/
#define _ftime64        oe__ftime64

/*sysinfoapi.h*/
#define GetVersion oe_GetVersion
#define GetSystemTimeAsFileTime oe_GetSystemTimeAsFileTime

/*time.h*/
#define _time64        oe__time64
#define _gmtime64      oe__gmtime64
#define _gmtime64_s    oe__gmtime64_s
#define _localtime64   oe__localtime64
#define _getsystime    oe_getsystime

/*wincon.h*/
#define FlushConsoleInputBuffer oe_FlushConsoleInputBuffer

#else //_WIN32

//#define mmap     oe_mmap
//#define munmap   oe_munmap
#define mprotect oe_mprotect
#define mlock    oe_mlock
#define madvise  oe_madvise

/*
#define fopen64 oe_fopen64
#define fopen oe_fopen
#define wfopen oe_wfopen
#define fclose oe_fclose
#define ferror oe_ferror
#define feof oe_feof
#define fflush oe_fflush
#define ftell oe_ftell
#define fseek oe_fseek
#define fread oe_fread
#define fwrite oe_fwrite
#define fgets oe_fgets
#define fputs oe_fputs
#define fileno oe_fileno
#define __fprintf_chk oe_fprintf
*/

#if defined(SGXSDK_INT_VERSION) && (SGXSDK_INT_VERSION > 18)
	#define _longjmp longjmp
	#define _setjmp setjmp
#endif

#define pipe oe_pipe
#define __read_alias oe_read
#define write oe_write
#define close oe_close


//#define sysconf oe_sysconf

#define getsockname oe_getsockname
#define getsockopt oe_getsockopt
#define setsockopt oe_setsockopt
#define socket oe_socket
#define bind oe_bind
#define listen oe_listen
#define connect oe_connect
#define accept oe_accept
#define getaddrinfo oe_getaddrinfo
#define freeaddrinfo oe_freeaddrinfo
//#define gethostbyname oe_gethostbyname
#define getnameinfo oe_getnameinfo
#define ioctl oe_ioctl

char * oe___builtin___strcat_chk(char *dest, const char *src, unsigned int dest_size);
char * oe___builtin___strcpy_chk(char *dest, const char *src, unsigned int dest_size);


#define __builtin___strcpy_chk __strcpy_chk
#define __builtin___strcat_chk __strcat_chk
#define __memcpy_chk memcpy

#define time oe_time
#define gmtime_r oe_gmtime_r
//#define gettimeofday oe_gettimeofday

//openssl 1.1.1 new APIs
//
#define getpid  oe_getpid
#define stat    oe_stat
#define syscall        oe_syscall
#define pthread_atfork oe_pthread_atfork
#define opendir        oe_opendir
#define readdir        oe_readdir
#define closedir       oe_closedir
#define OPENSSL_issetugid oe_OPENSSL_issetugid
//#define clock_gettime oe_clock_gettime


#define pthread_rwlock_init    oe_pthread_rwlock_init
#define pthread_rwlock_rdlock  oe_pthread_rwlock_rdlock
#define pthread_rwlock_wrlock  oe_pthread_rwlock_wrlock
#define pthread_rwlock_unlock  oe_pthread_rwlock_unlock
#define pthread_rwlock_destroy oe_pthread_rwlock_destroy
#define pthread_once           oe_pthread_once
#define pthread_key_create     oe_pthread_key_create
#define pthread_setspecific    oe_pthread_setspecific
#define pthread_getspecific    oe_pthread_getspecific
#define pthread_key_delete oe_pthread_key_delete
#define pthread_self oe_pthread_self
#define pthread_equal oe_pthread_equal

//#define __ctype_b_loc oe___ctype_b_loc
//#define __ctype_tolower_loc oe___ctype_tolower_loc

#define gai_strerror oe_gai_strerror

#define getcontext  oe_getcontext
#define setcontext  oe_setcontext
#define makecontext oe_makecontext
//#define getenv     	oe_getenv
#define secure_getenv getenv
#define atexit oe_atexit
//#define sscanf oe_sscanf

#include <sys/cdefs.h>

#undef __REDIRECT
#define __REDIRECT(name, proto, alias) name proto 
#undef __REDIRECT_NTH
#define __REDIRECT_NTH(name, proto, alias) name proto 
#undef __REDIRECT_NTHNL
#define __REDIRECT_NTHNL(name, proto, alias) name proto 

#endif //_WIN32

#endif // _BYPASS_TO_SGXSSL_

