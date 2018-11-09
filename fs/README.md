FS (file system)
================

Overview
--------

**FS** is a library for managing enclave file systems. It provides reference
implementations for three file systems.

- **hostfs** - a file system for manipulating unencrypted host files.
- **oefs** - an experimental full-disk-encryption (FDE) file system.
- **ramfs** - a ramdisk file system that resides within the enclave.

Additionally **FS** provides a framework for registering other file systems.

To use a file system, it must first be mounted into the enclave directory 
hierarchy. For example.

```
oe_mount("hostfs", "/", "/mnt/hostfs");
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
|:-----------:|
| vfprintf()  |
| fprintf()   |
| fscanf()    |
| ----------- |
| open()      |
| creat()     |
| close()     |
| read()      |
| readv()     |
| write()     |
| writev()    |
| lseek()     |
| ----------- |
| stat()      |
| link()      |
| unlink()    |
| rename()    |
| link()      |
| unlink()    |
| access()    |
| ----------- |
| mkdir()     |
| chdir()     |
| rmdir()     |
| getcwd()    |
| ----------- |
| opendir()   |
| readdir()   |
| readdir_r() |
| closedir()  |

Defining, registering, and mounting a new file system
-----------------------------------------------------

New file systems are defined by implementing the following interface.

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

Once this interface is implemented, one may write a callback function for 
mounting the interface into the directory hierarchy. For example, here is the 
an example.

```
int fs_mount_myfs(
    const char* type, 
    const char* source, 
    const char* target, 
    va_list ap)
{
    if (!source || !target)
        return -1;

    /* Create and initialize fs_t structure here */
    ...

    if (fs_bind(fs, target) != 0)
        return -1;

    return 0;
}
```

Next the file system may be registered with **FS**.

```
fs_register("myfs", fs_mount_myfs);
```

Finally the file system can be mounted.

```
fs_mount("myfs", source_path, target_path);
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

