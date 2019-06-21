Using the Open Enclave I/O subsystem
====================================

Synopsis
--------

This document explains how to use the **Open Enclave I/O subsystem**, which
encompasses file I/O and socket I/O. This subsystem provides enclaves with
access to files and sockets.

Overview
--------

The **Open Enclave I/O subsystem** exposes I/O features through ordinary system
header files. These headers fall roughly into three categories.

    - C headers, (such as stdio.h, stdlib.h)
    - POSIX headers (such as socket.h, dirent.h, fnctl.h, stat.h)
    - Linux headers (such as mount.h)

**Open Enclave** provides a partial implementation of these headers and many
others. These headers are part of [**musl libc**](https://www.musl-libc.org),
which **Open Enclave** redistributes for use within enclaves under the name
**oelibc**. The I/O subsystem expands **oelibc** to support many additional
functions.

For the most part, developers just use standard headers to build enclaves;
however, a few additional functions are needed to select which I/O features
are needed. This is purely a security measure. By default, enclaves have no
access to files or sockets. The next section describes how to opt-in to
various I/O features.

Opting in
---------

An enclave application must first opt in to the I/O features it wishes to
use. Opting in is a matter of (1) linking the desired module libraries and
(2) calling functions to load the modules. I/O features are packaged as
static libraries. This release provides the following modules.

    - **liboehostfs** -- access to non-secure host files and directories.
    - **liboehostsock** -- access to non-secure sockets.
    - **libhostresolver** -- access to network information.

After linking modules, the enclave loads modules by calling one of the
following.

    - **oe_load_module_host_file_system()**
    - **oe_load_module_host_socket_interface()**
    - **oe_load_module_host_resolver()**

A socket example
----------------

This section shows how to create an enclave that hosts an echo server, which
echos back requests received on a socket.
