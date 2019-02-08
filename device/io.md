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

