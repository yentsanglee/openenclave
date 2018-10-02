// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/enclave.h>
#include <openenclave/internal/tests.h>
#include <stdio.h>
#include "../args.h"

//#define _LIBCPP_ENABLE_CXX17_REMOVED_RANDOM_SHUFFLE
#define _LIBCPP_DEBUG 0
#include <algorithm>
#include <cassert>
#include <vector>
#include <atomic>
#include <type_traits>
#include <iterator>
#include <memory>
#include <stdexcept>
#include "test_workarounds.h"
#include <__tree>

extern "C" void _exit(int status)
{
    oe_call_host("ocall_exit", (void*)(long)status);
    abort();
}

extern "C" void _Exit(int status)
{
    _exit(status);
    abort();
}

extern "C" void exit(int status)
{
    _exit(status);
    abort();
}

typedef void (*Handler)(int signal);

Handler signal(int signal, Handler)
{
    /* Ignore! */
    return NULL;
}

extern "C" int close(int fd)
{
    OE_TEST("close() panic" == NULL);
    return 0;
}

namespace test1
{
#include "../../3rdparty/libcxx/libcxx/test/libcxx/algorithms/version.pass.cpp"
};

namespace test2
{
using namespace std;
#include "../../3rdparty/libcxx/libcxx/test/libcxx/containers/associative/tree_balance_after_insert.pass.cpp"
};

namespace test3
{
#include "../../3rdparty/libcxx/libcxx/test/libcxx/algorithms/debug_less.pass.cpp"
}

OE_ECALL void test(void* args_)
{
    args_t* args = (args_t*)args_;
    int ret = 0;

    args->ret = 0;

    printf("running: %s\n", "test1");

    test1::main();
    test2::main();
    test3::main();

    if (ret != 0)
        args->ret++;
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* AllowDebug */
    1024, /* HeapPageCount */
    1024, /* StackPageCount */
    2);   /* TCSCount */
