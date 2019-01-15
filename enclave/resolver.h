#Copyright(c) Microsoft Corporation.All rights reserved.
#Licensed under the MIT License.

#ifndef __OE_RESOLVER_H__
#define __OE_RESOLVER_H__

typedef struct oeResolver
{
    int (*init)();
    int (*remove)();
    char* resolvername(); // Likely resolver names in reverse order of security:
                          //    "enclavehosts"   -- resolve name using local
                          //    list of hosts in hosts file "enclavedns"     --
                          //    resolve name using ssl transport DNS request
                          //    "host"           -- OCALL to the host and let it
                          //    deal with it.

    int getaddrinfo(
        const char* node,
        const char* service,
        const struct addrinfo* hints,
        struct addrinfo** res);

    int (*getnameinfo)(
        const struct sockaddr* addr,
        socklen_t addrlen,
        char* host,
        socklen_t hostlen,
        char* serv,
        socklen_t servlen);
};

#endif
