The Open Enclave I/O system
===========================

1 Overview
==========

This document describes the **Open Enclave** I/O system, which provides a
framework for building devices and a set of predefined devices. OE supports
two types of devices.

- File-system devices
- Socket devices

Examples of **file-system devices** include:

- The non-secure **host file system** device (hostfs).
- The Intel **protected file system** device (sgxfs).
- The ARM **secure hardware file system** device (shwfs).
- The **Open Enclave File System** device (oefs).

Examples of the **socket devices** include:

- The non-secure **host sockets** device.
- The secure **enclave sockets** device.
- The OPTEE **secure-sockets** device.

Open Enclave provides three programming interfaces to meet the needs of
different applications.

- The device-oriented interface, which addresses devices by id rather than by
    path (for files) or domain (for sockets). Examples include:
    - int oe_device_open(int devid, const char* path, int flags, mode_t mode);
    - int oe_device_rmdir(int devid, const char* pathname);
    - int oe_device_socket(int devid, int domain, int type, int protocol);
- The POSIX interface, which includes:
    - Stream-based file I/O (**fopen()**, **fwrite()**, etc.).
    - Low-level file I/O (**open()**, **write()**).
    - POSIX/BSD socket I/O (**socket()**, **recv()**).
- The IOT interface, which includes customized, macro-driven headers, including:
    - **<stdio.h>**
    - **<socket.h>**

Each of these interfaces builds on the one above it in the list. The next
section discusses these in detail.

2 Interfaces
============

This section describes the three interfaces introduced in the previous section.

2.1 The device-oriented interface
---------------------------------

The device-oriented interface addresses devices by a **device id** rather than
by path (for files) or domain (for sockets). This interface defines functions
similar to their POSIX counterparts, except for the use of of an extra
**devid** parameter. For example, consider the POSIX **open()** function.

```
int open(const char *pathname, int flags, mode_t mode);
```

The device-oriented form of this method is defined as follows.

```
int oe_open(int devid, const char *pathname, int flags, mode_t mode);
```

The **devid** parameter is the integer ID of the device that will perform
the operation. **Open Enclave** shall support the following device ids.

- **OE_DEVID_HOST_FILE_SYSTEM** (non-secure host file system).
- **OE_DEVID_PROTECTED_FILE_SYSTEM** (Intel's protected file system).
- **OE_DEVID_SECURE_HARDWARE_FILE_SYSTEM** (OPTEE secure hardware file system).
- **OE_DEVID_OPEN_ENCLAVE_FILE_SYSTEM** (Open enclave whole disk file system).
- **OE_DEVID_HOST_SOCKETS** (non-secure host sockets).
- **OE_DEVID_ENCLAVE_SOCKETS** (secure enclave-to-enclave sockets).
- **OE_DEVID_SECURE_SOCKETS** (OPTEE secure sockets).

The following example uses the device interface to create an unencrypted file
on the host file system.

```
    int fd;

    fd = oe_device_open(
        OE_DEVID_HOST_FILE_SYSTEM,
        "/tmp/myfile",
        OE_O_CREAT | OE_O_TRUNC | OE_O_WRONLY,
        0644);

    oe_write(fd, "hello", 5);

    oe_close(fd);
```

The oe-prefixed functions that have POSIX signatures (**oe_write()**,
**oe_close**(), etc.) allow enclaves to perform I/O without a C runtime.
The following section discusses the corresponding POSIX functions defined
by the C runtime.

The device interface also provides functions for opening stream-oriented
files. For example:

```
    OE_FILE* stream;

    stream = oe_device_fopen(OE_DEVID_HOST_FILE_SYSTEM, "/tmp/myfile", "r");

    fwrite("hello", 1, 5, stream);

    fclose(stream);
```

The **fwrite** and **fclose** functions are part of the POSIX interface,
discussed in the next chapter.
