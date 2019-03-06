// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/corelibc/string.h>
#include <openenclave/edger8r/enclave.h>
#include <openenclave/enclave.h>
#include <openenclave/internal/calls.h>
#include <openenclave/internal/fault.h>
#include <openenclave/internal/globals.h>
#include <openenclave/internal/sgxtypes.h>
#include <openenclave/internal/tests.h>
#include <memory>
#include "ocall_t.h"

uint64_t enc_test2(uint64_t val)
{
    return val;
}

void enc_test4()
{
    unsigned char buffer[32];
    memset(buffer, 0xAA, sizeof(buffer));

    /* Call into host with enclave memory */
    if (OE_OK != host_func2(buffer))
    {
        oe_abort();
    }
}

static bool g_destructor_called = false;

class thread_data
{
  public:
    /*ctor*/ thread_data(char* str) : m_str(str)
    {
        // empty
    }

    /*dtor*/ ~thread_data()
    {
        g_destructor_called = true;
        if (m_str)
        {
            oe_host_free(m_str);
            m_str = nullptr;
        }
    }

    char* get_str() const
    {
        return m_str;
    }

  private:
    char* m_str;
};

thread_local std::unique_ptr<thread_data> t_data;

int enc_set_tsd(char* str)
{
    int rval = -1;
    if (!t_data)
    {
        t_data.reset(new thread_data(str));
        rval = 0;
    }
    return rval;
}

char* enc_get_tsd()
{
    return t_data ? t_data->get_str() : nullptr;
}

bool was_destructor_called()
{
    return g_destructor_called;
}

uint64_t enc_test_my_ocall()
{
    uint64_t ret_val;
    oe_result_t result = host_my_ocall(&ret_val, MY_OCALL_SEED);
    OE_TEST(OE_OK == result);

    /* Test low-level OCALL of illegal function number */
    {
        oe_result_t result = oe_ocall(0xffff, 0, NULL);
        OE_TEST(OE_NOT_FOUND == result);
    }

    return ret_val;
}

void enc_test_reentrancy()
{
    oe_result_t result = host_test_reentrancy();
    OE_TEST(OE_OK == result);
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* AllowDebug */
    1024, /* HeapPageCount */
    128,  /* StackPageCount */
    16);  /* TCSCount */
