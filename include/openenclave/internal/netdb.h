// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef __NETDB_H__
#define __NETDB_H__
#include <openenclave/bits/socket.h>

OE_EXTERNC_BEGIN

struct oe_hostent
{
    char* h_name;       /* Official name of host.  */
    char** h_aliases;   /* Alias list.  */
    int h_addrtype;     /* Host address type.  */
    int h_length;       /* Length of address.  */
    char** h_addr_list; /* List of addresses from name server.  */
};

void oe_sethostent(int __stay_open);
void oe_end_hostent(void);
struct oe_hostent* getoe_hostent(void);

struct oe_hostent* gethostbyaddr(
    const void* __addr,
    oe_socklen_t __len,
    int __type);

struct oe_hostent* gethostbyname(const char* __name);

struct oe_netent
{
    char* n_name;     /* Official name of network.  */
    char** n_aliases; /* Alias list.  */
    int n_addrtype;   /* Net address type.  */
    uint32_t n_net;   /* Network number.  */
};

void oe_setnetent(int __stay_open);
void oe_endnetent(void);
struct oe_netent* oe_getnetent(void);
struct oe_netent* oe_getnetbyaddr(uint32_t __net, int __type);
struct oe_netent* oe_getnetbyname(const char* __name);

int oe_getnetent_r(
    struct oe_netent* __restrict __result_buf,
    char* __restrict __buf,
    size_t __buflen,
    struct oe_netent** __restrict __result,
    int* __restrict __h_errnop);

int oe_getnetbyaddr_r(
    uint32_t __net,
    int __type,
    struct oe_netent* __restrict __result_buf,
    char* __restrict __buf,
    size_t __buflen,
    struct oe_netent** __restrict __result,
    int* __restrict __h_errnop);

int oe_getnetbyname_r(
    const char* __restrict __name,
    struct oe_netent* __restrict __result_buf,
    char* __restrict __buf,
    size_t __buflen,
    struct oe_netent** __restrict __result,
    int* __restrict __h_errnop);

struct oe_servent
{
    char* s_name;     /* Official service name.  */
    char** s_aliases; /* Alias list.  */
    int s_port;       /* Port number.  */
    char* s_proto;    /* Protocol to use.  */
};

void setservent(int __stay_open);
void endservent(void);
struct oe_servent* getservent(void);
struct oe_servent* getservbyname(const char* __name, const char* __proto);
struct oe_servent* getservbyport(int __port, const char* __proto);

int getservent_r(
    struct oe_servent* __restrict __result_buf,
    char* __restrict __buf,
    size_t __buflen,
    struct oe_servent** __restrict __result);

int getservbyname_r(
    const char* __restrict __name,
    const char* __restrict __proto,
    struct oe_servent* __restrict __result_buf,
    char* __restrict __buf,
    size_t __buflen,
    struct oe_servent** __restrict __result);

int getservbyport_r(
    int __port,
    const char* __restrict __proto,
    struct oe_servent* __restrict __result_buf,
    char* __restrict __buf,
    size_t __buflen,
    struct oe_servent** __restrict __result);

/* Description of data base entry for a single service.  */
struct oe_protoent
{
    char* p_name;     /* Official protocol name.  */
    char** p_aliases; /* Alias list.  */
    int p_proto;      /* Protocol number.  */
};

void setprotoent(int __stay_open);
void endprotoent(void);
struct oe_protoent* getprotoent(void);
struct oe_protoent* getprotobyname(const char* __name);
struct oe_protoent* getprotobynumber(int __proto);

int oe_getprotoent_r(
    struct oe_protoent* __restrict __result_buf,
    char* __restrict __buf,
    size_t __buflen,
    struct oe_protoent** __restrict __result);

int oe_getprotobyname_r(
    const char* __restrict __name,
    struct oe_protoent* __restrict __result_buf,
    char* __restrict __buf,
    size_t __buflen,
    struct oe_protoent** __restrict __result);

int oe_getprotobynumber_r(
    int __proto,
    struct oe_protoent* __restrict __result_buf,
    char* __restrict __buf,
    size_t __buflen,
    struct oe_protoent** __restrict __result);

int oe_setnetgrent(const char* __netgroup);
void oe_endnetgrent(void);
int oe_getnetgrent(
    char** __restrict __hostp,
    char** __restrict __userp,
    char** __restrict __domainp);

int oe_innetgr(
    const char* __netgroup,
    const char* __host,
    const char* __user,
    const char* __domain);
int oe_getnetgrent_r(
    char** __restrict __hostp,
    char** __restrict __userp,
    char** __restrict __domainp,
    char* __restrict __buffer,
    size_t __buflen);

/* Extension from POSIX.1:2001.  */
struct oe_addrinfo
{
    int ai_flags;                /* Input flags.  */
    int ai_family;               /* Protocol family for socket.  */
    int ai_socktype;             /* Socket type.  */
    int ai_protocol;             /* Protocol for socket.  */
    oe_socklen_t ai_addrlen;     /* Length of socket address.  */
    struct oe_sockaddr* ai_addr; /* Socket address for socket.  */
    char* ai_canonname;          /* Canonical name for service location.  */
    struct oe_addrinfo* ai_next; /* Pointer to next in list.  */
};

// Lookup mode.
#define GAI_WAIT 0
#define GAI_NOWAIT 1
#endif

/* Possible values for `ai_flags' field in `addrinfo' structure.  */
#define OE_AI_PASSIVE 0x0001     /* Socket address is intended for `bind'.  */
#define OE_AI_CANONNAME 0x0002   /* Request for canonical name.  */
#define OE_AI_NUMERICHOST 0x0004 /* Don't use name resolution.  */
#define OE_AI_V4MAPPED 0x0008    /* IPv4 mapped addresses are acceptable.  */
#define OE_AI_ALL 0x0010         /* Return IPv4 mapped and IPv6 addresses.  */
#define OE_AI_ADDRCONFIG                                                       \
    0x0020 /* Use configuration of this host to choose returned address type.. \
            */
#define OE_AI_IDN                                                         \
    0x0040                    /* IDN encode input (assuming it is encoded \
               in the current locale's character set)                 \
               before looking it up. */
#define OE_AI_CANONIDN 0x0080 /* Translate canonical name from IDN format. */
#define OE_AI_IDN_ALLOW_UNASSIGNED            \
    0x0100 /* Don't reject unassigned Unicode \
code points.  */
#define OE_AI_IDN_USE_STD3_ASCII_RULES                            \
    0x0200                       /* Validate strings according to \
            STD3 rules.  */
#define OE_AI_NUMERICSERV 0x0400 /* Don't use name resolution.  */

// Error values for `getaddrinfo' function.
#define OE_EAI_BADFLAGS -1      /* Invalid value for `ai_flags' field.  */
#define OE_EAI_NONAME -2        /* NAME or SERVICE is unknown.  */
#define OE_EAI_AGAIN -3         /* Temporary failure in name resolution.  */
#define OE_EAI_FAIL -4          /* Non-recoverable failure in name res.  */
#define OE_EAI_FAMILY -6        /* `ai_family' not supported.  */
#define OE_EAI_SOCKTYPE -7      /* `ai_socktype' not supported.  */
#define OE_EAI_SERVICE -8       /* SERVICE not supported for `ai_socktype'.  */
#define OE_EAI_MEMORY -10       /* Memory allocation failure.  */
#define OE_EAI_SYSTEM -11       /* System error returned in `errno'.  */
#define OE_EAI_OVERFLOW -12     /* Argument buffer overflow.  */
#define OE_EAI_NODATA -5        /* No address associated with NAME.  */
#define OE_EAI_ADDRFAMILY -9    /* Address family for NAME not supported.  */
#define OE_EAI_INPROGRESS -100  /* Processing request in progress.  */
#define OE_EAI_CANCELED -101    /* Request canceled.  */
#define OE_EAI_NOTCANCELED -102 /* Request not canceled.  */
#define OE_EAI_ALLDONE -103     /* All requests done.  */
#define OE_EAI_INTR -104        /* Interrupted by a signal.  */
#define OE_EAI_IDN_ENCODE -105  /* IDN encoding failed.  */

#define NI_MAXHOST 1025
#define NI_MAXSERV 32

#define NI_NUMERICHOST 1 /* Don't try to look up hostname.  */
#define NI_NUMERICSERV 2 /* Don't convert port number to name.  */
#define NI_NOFQDN 4      /* Only return nodename portion.  */
#define NI_NAMEREQD 8    /* Don't return numeric addresses.  */
#define NI_DGRAM 16      /* Look up UDP service rather than TCP.  */
#ifdef __USE_GNU
#define NI_IDN 32 /* Convert name from IDN format.  */
#define NI_IDN_ALLOW_UNASSIGNED           \
    64 /* Don't reject unassigned Unicode \
code points.  */
#define NI_IDN_USE_STD3_ASCII_RULES      \
    128 /* Validate strings according to \
STD3 rules.  */

int oe_getaddrinfo(
    const char* __restrict __name,
    const char* __restrict __service,
    const struct oe_addrinfo* __restrict __req,
    struct oe_addrinfo** __restrict __pai);

void oe_freeaddrinfo(struct oe_addrinfo* __ai);

const char* oe_gai_strerror(int __ecode);

int oe_getnameinfo(
    const struct oe_sockaddr* __restrict __sa,
    oe_socklen_t __salen,
    char* __restrict __host,
    oe_socklen_t __hostlen,
    char* __restrict __serv,
    oe_socklen_t __servlen,
    int __flags);

OE_EXTERNC_END

#endif /* netinet/netdb.h */
