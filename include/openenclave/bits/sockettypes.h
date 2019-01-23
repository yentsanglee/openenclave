/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* Licensed under the MIT License. */
#pragma once

typedef enum oe_socket_error_t
{
    OE_OK = 0,
#if defined(WIN32)
    OE_SOCKET_ENOMEM = 8,
    OE_SOCKET_EACCES = 10013,
    OE_SOCKET_EFAULT = 10014,
    OE_SOCKET_EINVAL = 10022,
    OE_SOCKET_EMFILE = 10024,
    OE_SOCKET_EAGAIN = 10035, // = WSAEWOULDBLOCK
    OE_SOCKET_EINPROGRESS = 10036,
    OE_SOCKET_ENOPROTOOPT = 10042,
    OE_SOCKET_EPROTONOSUPPORT = 10043,
    OE_SOCKET_EAFNOSUPPORT = 10047,
    OE_SOCKET_ENETDOWN = 10050,
    OE_SOCKET_ECONNABORTED = 10053,
    OE_SOCKET_ECONNRESET = 10054,
    OE_SOCKET_ENOBUFS = 10055,
    OE_SOCKET_SYSNOTREADY = 10091,
    OE_SOCKET_ENOTRECOVERABLE = 11003
#else
    OE_SOCKET_ENOMEM = 8,
    OE_SOCKET_EACCES = 13,
    OE_SOCKET_EFAULT = 14,
    OE_SOCKET_EINVAL = 22,
    OE_SOCKET_EMFILE = 24,
    OE_SOCKET_EAGAIN = 11,
    OE_SOCKET_EINPROGRESS = 115,
    OE_SOCKET_ENOPROTOOPT = 92,
    OE_SOCKET_EPROTONOSUPPORT = 93,
    OE_SOCKET_EAFNOSUPPORT = 97,
    OE_SOCKET_ENETDOWN = 100,
    OE_SOCKET_ECONNABORTED = 103,
    OE_SOCKET_ECONNRESET = 104,
    OE_SOCKET_ENOBUFS = 105,
    OE_SOCKET_SYSNOTREADY = OE_SOCKET_EFAULT,
    OE_SOCKET_ENOTRECOVERABLE = 131
#endif
} oe_socket_error_t;

typedef enum oe_socket_address_family_t
{
    OE_AF_INET = 2,
    OE_AF_ENCLAVE = 50,
#ifdef LINUX
    OE_AF_INET6 = 10,
#else
    OE_AF_INET6 = 23,
#endif
} oe_socket_address_family_t;

typedef enum oe_socket_type_t
{
    OE_SOCK_STREAM = 1,
} oe_socket_type_t;

typedef enum oe_shutdown_how_t
{
    OE_SHUT_RD = 0,   // WSA = OE_SD_RECEIVE
    OE_SHUT_WR = 1,   // WSA = OE_SHUT_WR
    OE_SHUT_RDWR = 2, // WSA = OE_SHUT_RDWR
} oe_shutdown_how_t;
