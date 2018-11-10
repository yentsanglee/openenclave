FS (file system)
================

Overview
--------

**FS** is a library for managing enclave file systems. It provides reference
implementations for three file system types.

- **hostfs** - a file system for manipulating unencrypted host files.
- **ramfs** - a ramdisk file system that resides within the enclave.
- **oefs** - an experimental full-disk-encryption (FDE) file system.

Additionally **FS** provides a framework for defining new file system types.

The first step in using a file system is to create an instance of it. For
example, the following snippet creates an instance of the **hostfs** file 
system.

```
fs_t* fs;
fs_create_hostfs(&fs);
```

Next, the file system is mounted as follows.

```
fs_mount(fs, "/mnt/hostfs");
```

Once mounted, file systems are manipulated with the standard POSIX libc
functions. For example:

```
stream = fopen("/mnt/hostfs/myfile", "wb");
...
fwrite(buf, 1, sizeof(buf), stream);
...
fclose(stream);
```

When a file system is no longer needed, it is unmounted as follows.

```
oe_unmount("/mnt/hostfs");
```

Supported libc functions
------------------------

**FS** supports the following standard libc functions.

| Function    |
| ----------- |
| fopen()     |
| fclose()    |
| fread()     |
| fwrite()    |
| ftell()     |
| fseek()     |
| feof()      |
| fflush()    |
| fgetc()     |
| fgets()     |
| fgetwc()    |
| fgetws()    |
| fputc()     |
| fputs()     |
| fgetln()    |
| ferror()    |
| rewind()    |
| vfprintf()  |
| fprintf()   |
| fscanf()    |
| open()      |
| creat()     |
| close()     |
| read()      |
| readv()     |
| write()     |
| writev()    |
| lseek()     |
| stat()      |
| link()      |
| unlink()    |
| rename()    |
| link()      |
| unlink()    |
| access()    |
| mkdir()     |
| chdir()     |
| rmdir()     |
| getcwd()    |
| opendir()   |
| readdir()   |
| readdir_r() |
| closedir()  |

Defining new file system types
------------------------------

New file systems are defined by implementing a function that produces instances
of the the following interface.

```
typedef struct _fs
{
    fs_errno_t (*fs_release)(
        fs_t* fs);

    fs_errno_t (*fs_creat)(
        fs_t* fs, 
        const char* path, 
        uint32_t mode, 
        fs_file_t** file);

    fs_errno_t (*fs_open)(
        fs_t* fs,
        const char* pathname,
        int flags,
        uint32_t mode,
        fs_file_t** file_out);

    fs_errno_t (*fs_lseek)(
        fs_file_t* file,
        ssize_t offset,
        int whence,
        ssize_t* offset_out);

    fs_errno_t (*fs_read)(
        fs_file_t* file, 
        void* data, 
        size_t size, 
        ssize_t* nread);

    fs_errno_t (*fs_write)(
        fs_file_t* file,
        const void* data,
        size_t size,
        ssize_t* nwritten);

    fs_errno_t (*fs_close)(
        fs_file_t* file);

    fs_errno_t (*fs_opendir)(
        fs_t* fs, 
        const char* path, 
        fs_dir_t** dir);

    fs_errno_t (*fs_readdir)(
        fs_dir_t* dir, 
        fs_dirent_t** dirent);

    fs_errno_t (*fs_closedir)(
        fs_dir_t* dir);

    fs_errno_t (*fs_stat)(
        fs_t* fs, 
        const char* path, 
        fs_stat_t* stat);

    fs_errno_t (*fs_link)(
        fs_t* fs, 
        const char* old_path, 
        const char* new_path);

    fs_errno_t (*fs_unlink)(
        fs_t* fs, 
        const char* path);

    fs_errno_t (*fs_rename)(
        fs_t* fs, 
        const char* old_path, 
        const char* new_path);

    fs_errno_t (*fs_truncate)(
        fs_t* fs, 
        const char* path, 
        ssize_t length);

    fs_errno_t (*fs_mkdir)(
        fs_t* fs, 
        const char* path, 
        uint32_t mode);

    fs_errno_t (*fs_rmdir)(
        fs_t* fs, 
        const char* path);
}
fs_t;
```

For example, the following function creates an instance of the **myfs** file
system (parameters are added as needed).

```
int create_myfs(fs_t** fs);
```

As shown earlier, an instance can be created and mounted as follows.

```
fs_t* fs;
create_myfs(&fs);
fs_mount(fs, "myfs");
```

Block devices
-------------

**FS** defines a block device interface and several implementations. File
systems may opt to use these block devices in their implementation. For 
example, **oefs** uses the following chain of block devices.

```
cache_blkdev -> merkle_blkdev -> host_blkdev
```

The block device interface is defined as follows.

```
typedef struct _fs_blkdev
{
    int (*get)(fs_blkdev_t* dev, uint32_t blkno, fs_blk_t* blk);

    int (*put)(fs_blkdev_t* dev, uint32_t blkno, const fs_blk_t* blk);

    int (*begin)(fs_blkdev_t* dev);

    int (*end)(fs_blkdev_t* dev);

    int (*add_ref)(fs_blkdev_t* dev);

    int (*release)(fs_blkdev_t* dev);
}
fs_blkdev_t;
```

Here is a list of existing block device implementations.

- **cache_blkdev** -- Buffer cache block device
- **merkle_blkdev** -- Merkle tree block device
- **crypto_blkdev** -- Length-preserving block encryption device.

