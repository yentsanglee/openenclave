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

OE will support three interfaces.

- **The IOT interface** (**<stdio.h>** and **<socket.h>**).
- **The low-level I/O interface** (**open()**, **read**(), etc.).
- **The stream-based file I/O interface** (**fopen()**, **fread**(), etc.).

The low-level interface is needed to support legacy applications that depend 
on that interface (such as **OpenSSL**). No extra effort is needed to support 
the stream-based functions, since MUSL implements these based on the low-level 
I/O functions.

