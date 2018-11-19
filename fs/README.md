FS
==

This directory contains the Open Enclave file system implementations. These
include:

    (*) sgxfs - Intel's protected file system.
    (*) hostfs - unsecure host file system.

This directory in conjunction with MUSL libc supports the following stdio.h
functions from the C99 standard.

| Function      | Supported     | Source/target |
| ----------------------------- | ------------- |
| remove()      | Y             | path          |
| rename()      | Y             | path          |
| tmpfile()     |               | stream        |
| tmpnam()      |               | stream        |
| fclose()      | Y             | stream        |
| fflush()      | Y             | stream        |
| fopen()       | Y             | stream        |
| freopen()     |               | stream        |
| setbuf()      |               | stream        |
| setvbuf()     |               | stream        |
| fprintf()     |               | stream        |
| fscanf()      |               | stream        |
| printf()      | Y             | stdout        |
| scanf()       |               | stdin         |
| snprintf()    | Y             | string        |
| sprintf()     | Y             | string        |
| sscanf()      | Y             | string        |
| vfprintf()    |               | stream        |
| vfscanf()     |               | stream        |
| vprintf()     | Y             | stdout        |
| vscanf()      |               | stdin         |
| vsnprintf()   | Y             | string        |
| vsprintf()    | Y             | string        |
| vsscanf()     | Y             | string        |
| fgetc()       | Y             | stream        |
| fgets()       | Y             | stream        |
| fputc()       | Y             | stream        |
| fputs()       | Y             | stream        |
| getc()        | Y             | stream        |
| getchar()     |               | stdin         |
| gets()        |               | stdin         |
| putc()        | Y             | stream        |
| putchar()     | Y             | stdout        |
| puts()        | Y             | stdout        |
| ungetc()      |               | stream        |
| fread()       | Y             | stream        |
| fwrite()      | Y             | stream        |
| fgetpos()     | Y             | stream        |
| fseek()       | Y             | stream        |
| fsetpos()     | Y             | stream        |
| ftell()       | Y             | stream        |
| rewind()      | Y             | stream        |
| clearerr()    | Y             | stream        |
| feof()        | Y             | stream        |
| ferror()      | Y             | stream        |
| perror()      | Y             | stream        |

Note: reading from stdin is not supported in enclaves.
