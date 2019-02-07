The Open Enclave I/O system
===========================

Overview:
---------

This document describes the **Open Enclave** I/O system, which provides a 
framework for building devices to support various I/O styles. By default, OE 
will support the following device types.

- An unencrypted host file system.
- An encrypted host file system.
- A secure hardware-based file system.
- A host socket manager.
- An enclave-to-enclave socket manager.

Interfaces:
-----------

OE will support two interfaces.

- **The IOT interface** (**<stdio.h>** and **<socket.h>**).
- **The low-level I/O interface** (**open()**, **read**(), etc.).
- **The stream-based file I/O interface** (**fopen()**, **fread**(), etc.).

The latter interface is required to support legacy applications that depend on 
the low-level functions (such as **OpenSSL**). No extra effort is needed to
support the stream-based interface, since MUSL implements this based on the
low-level I/O functions.

