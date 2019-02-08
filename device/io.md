The Open Enclave I/O system
===========================

Overview:
---------

This document describes the **Open Enclave** I/O system, which provides a 
framework for building devices and a set of predefined devices. OE supports
two types of devices.

- File-system devices
- Socket devices

Examples of **file-system devices** include:

- The non-secure **host file system** device (hostfs).
- The Intel **SGX protected file system** device (sgxfs).
- The ARM **secure hardware file system** device (shwfs).
- The **Open Enclave File System** device (oefs).

Examples of the **socket devices** include:

- The non-secure **host sockets** device (hsock).
- The secure **enclave-to-enclave sockets** device (e2esock).

Open Enclave provides three programming interfaces to meet the needs of
different applications.

- The POSIX interface, which includes:
    - Stream-based file I/O (**fopen()**, **fwrite()**, etc.).
    - Low-level file I/O (**open()**, **write()**).
    - POSIX/BSD socket I/O (**socket()**, **recv()**).
- The device-oriented interface, which addresses objects by id rather than path.
  Examples include:
    - **int oe_device_open(int devid, const char* path, int flags, mode_t mode);**


Interfaces:
-----------

OE supports three interfaces.

- **The low-level interface** (**open()**, **read**(), etc.).
- **The stream-based file interface** (**fopen()**, **fread**(), etc.).
- **The IOT interface** (**<stdio.h>** and **<socket.h>**).

Each interface supports a different use case. For example, both the low-level
and stream-based interfaces are used by **OpenSSL**. The IOT interface is used
by IOT team.

The low-level interface:
------------------------

The stream-based interface:
---------------------------

The IOT Interface:
------------------

