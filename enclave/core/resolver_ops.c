// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include <openenclave/internal/enclavelibc.h>
#include <openenclave/internal/resolver.h>

static size_t _resolver_table_len = 3;
static oe_resolver_t* _resolver_table[3] = {0}; // At most 3

int oe_register_resolver(int resolver_priority, oe_resolver_t* presolver)

{
    if (resolver_priority > (int)_resolver_table_len)
    {
        oe_errno = OE_EINVAL;
        return -1;
    }

    _resolver_table[resolver_priority] = presolver;
    return 0;
}

int oe_getaddrinfo(
    const char* node,
    const char* service,
    const struct oe_addrinfo* hints,
    struct oe_addrinfo** res)

{
    size_t resolver_idx = 0;
    int rslt = -1;

    for (resolver_idx = 0; resolver_idx < _resolver_table_len; resolver_idx++)
    {
        if (_resolver_table[resolver_idx] != NULL)
        {
            rslt = (*_resolver_table[resolver_idx]->ops->getaddrinfo)(
                _resolver_table[resolver_idx], node, service, hints, res);
            if (rslt == 0)
            {
                return rslt;
            }
        }
    }
    return rslt;
}

void oe_freeaddrinfo(struct oe_addrinfo* res)

{
    if (res != NULL)
    {
        oe_free(res);
    }
}

int oe_getnameinfo(
    const struct oe_sockaddr* sa,
    oe_socklen_t salen,
    char* host,
    oe_socklen_t hostlen,
    char* serv,
    oe_socklen_t servlen,
    int flags)

{
    size_t resolver_idx = 0;
    int rslt = -1;

    for (resolver_idx = 0; resolver_idx < _resolver_table_len; resolver_idx++)
    {
        if (_resolver_table[resolver_idx] != NULL)
        {
            rslt = (*_resolver_table[resolver_idx]->ops->getnameinfo)(
                _resolver_table[resolver_idx],
                sa,
                salen,
                host,
                hostlen,
                serv,
                servlen,
                flags);
            if (rslt == 0)
            {
                return rslt;
            }
        }
    }
    return rslt;
}
