#ifndef _OE_LIBC_KERNEL32_H
#define _OE_LIBC_KERNEL32_H

typedef unsigned long long HANDLE;
typedef unsigned int DWORD;
typedef const void* LPCVOID;
typedef unsigned int* LPDWORD;
typedef int BOOL;
typedef void* LPOVERLAPPED;
typedef unsigned int UINT;

#define STD_INPUT_HANDLE ((DWORD)-10)
#define STD_OUTPUT_HANDLE ((DWORD)-11)
#define STD_ERROR_HANDLE ((DWORD)-12)

HANDLE GetStdHandle(DWORD nStdHandle);

BOOL WriteFile(
    HANDLE hFile,
    LPCVOID lpBuffer,
    DWORD nNumberOfBytesToWrite,
    LPDWORD lpNumberOfBytesWritten,
    LPOVERLAPPED lpOverlapped);

void ExitProcess(UINT uExitCode);

#endif /* _OE_LIBC_KERNEL32_H */
