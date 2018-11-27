fs (file system)
================

This directory contains the Open Enclave file system implementations. These
include:

    (*) sgxfs - Intel's protected file system.
    (*) hostfs - unsecure host file system.
    (*) muxfs - multiplexed file system.
    (*) oefs - OE file system (with integrity, authentication, and encryption).

Overview:
=========

Open Enclave provides APIs for utilizing one or more of its file systems. The
**hostfs** and **sgxfs** file systems are defined globally. The following
snippet declares an external reference to these.

```
#include <openenclave/fs.h>

extern oe_fs_t oe_sgxfs;
extern oe_fs_t oe_hostfs;
```

File systems can be used to open files and directories. For example:

```
FILE* stream = oe_fopen(&oe_sgxfs, "/myfile", "w");

DIR* dir = oe_opendir(&oe_hostfs, "/mydir");
```

Once opened, these files and directories are manipulated with the standard
C functions as shown in the following example.

```
oe_fwrite("hello", 1, 5, stream);
```


Supported Functions:
====================

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
| scanf()       | *             | stdin         |
| snprintf()    | Y             | string        |
| sprintf()     | Y             | string        |
| sscanf()      | Y             | string        |
| vfprintf()    |               | stream        |
| vfscanf()     |               | stream        |
| vprintf()     | Y             | stdout        |
| vscanf()      | *             | stdin         |
| vsnprintf()   | Y             | string        |
| vsprintf()    | Y             | string        |
| vsscanf()     | Y             | string        |
| fgetc()       | Y             | stream        |
| fgets()       | Y             | stream        |
| fputc()       | Y             | stream        |
| fputs()       | Y             | stream        |
| getc()        | Y             | stream        |
| getchar()     | *             | stdin         |
| gets()        | *             | stdin         |
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
