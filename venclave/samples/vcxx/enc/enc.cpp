// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/enclave.h>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

static vector<string> _strings;

extern "C" void test_cxx(void)
{
    cout << "*** enclave: " << __FUNCTION__ << endl;

    _strings.push_back(string("red"));
    _strings.push_back(string("green"));
    _strings.push_back(string("blue"));
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* AllowDebug */
    1024, /* HeapPageCount */
    1024, /* StackPageCount */
    8);   /* TCSCount */
