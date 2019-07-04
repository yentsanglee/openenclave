// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/enclave.h>
#include <openenclave/internal/tests.h>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

static vector<string> _strings;

struct test_exception
{
    int val;

    test_exception(int val_) : val(val_)
    {
    }
};

static bool _object_constructor_called = false;

struct object
{
    object()
    {
        _object_constructor_called = true;
        // cout << "*** object::object()" << endl;
        // printf("*** object::object()\n");
    }

    ~object()
    {
        // cout << "*** object::~object()" << endl;
    }
};

object o;

extern "C" void test_cxx(void)
{
    cout << "*** enclave: " << __FUNCTION__ << endl;

    _strings.push_back(string("red"));
    _strings.push_back(string("green"));
    _strings.push_back(string("blue"));

    /* Test global construction. */
    OE_TEST(_object_constructor_called == true);

    /* Test exceptions */
    {
        bool caught = false;
        const int val = 12345;
        try
        {
            caught = false;
            throw test_exception(val);
        }
        catch (const test_exception& e)
        {
            caught = true;
            OE_TEST(e.val == val);
        }

        OE_TEST(caught);
    }
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* AllowDebug */
    1024, /* HeapPageCount */
    1024, /* StackPageCount */
    8);   /* TCSCount */
