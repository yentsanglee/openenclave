fs (file system)
================

This directory contains the Open Enclave file system implementations. These
include:

- SGXFS - Intel's protected file system.
- HOSTFS - unsecure host file system.
- MUXFS - multiplexed file system.
- OEFS - OE file system.

Overview:
=========

Open Enclave provides APIs for utilizing file systems implementations. The 
**HOSTFS** and **SGXFS** file systems are defined globally as **oe_hostfs**
and **oe_sgxfs** respectively.

File systems are used to open files and directories. For example:

```
/* Open an SGXFS file. */
FILE* stream = oe_fopen(&oe_sgxfs, "/myfile", "w");

/* Open a HOSTFS file. */
DIR* dir = oe_opendir(&oe_hostfs, "/mydir");
```

Once opened, files and directories are manipulated with the standard C 
functions as follows.

```
fwrite("hello", 1, 5, stream);

while ((ent = readdir(dir)))
{
    ...
}
```

Binding a path to a file system:
================================

All path-oriented file and directory objects must be bound to a file system
before they are manipulated. For example, **oe_fopen()** binds a path to the 
file system given by the first argument. The following example binds "/myfile" 
to the SGXFS file system.

```
FILE* stream = oe_fopen(&oe_sgxfs, "/myfile", "w");
```

Once bound, the **stream** can be passed to standard C I/O functions.

There are four ways to bind a path to a file system.

(1) By using the oe-prefixed functions, such as **oe_fopen()**, 
    **oe_opendir()**, **oe_mkdir()**, and **oe_stat()**.

(2) By using macros that redirect path-oriented functions to a particular file
    system. For example:

    ```
    #define fopen(...) oe_fopen(OE_FILE_SECURE_BEST_EFFORT, __VA_ARGS__)
    ```

    This method requires using a custom **<stdio.h>** header file.

(3) By using the **oe_fs_set_default()** function to set the default file
    system used by the standard C I/O functions. For example:

    ```
    oe_fs_set_default(&oe_hostfs);
    stream = fopen("/myfile", "r");
    ```

(4) By setting the multiplexing file system (**MUXFS**) as the default and
    using paths to direct to one or more file systems. For example:

    ```
    oe_fs_set_default(&oe_muxfs);

    /* Open a HOSTFS file. */
    fopen("/hostfs/myfile", "r");

    /* Open an SGXFS file. */
    fopen("/sgxfs/myfile", "r");
    ```

The first method uses **explicit binding** and is the most secure. The other
methods employ **implicit binding** which makes it easier to port existing 
applications without having to retrofit all the path-oriented functions with
special oe-prefixed functions.

Supported Functions:
====================

This directory in conjunction with **MUSL libc** supports the following 
**stdio.h** functions from the **C99** standard.

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

*stdin is not supported in enclaves.
