/*
 *  This file is auto generated by oeedger8r. DO NOT EDIT.
 */
#ifndef TESTENCLAVE_ARGS_H
#define TESTENCLAVE_ARGS_H

#include <stdint.h>
#include <stdlib.h> /* for wchar_t */ 

#include <openenclave/bits/result.h>

typedef struct _test_args_t {
	int _retval;
    oe_result_t _result;
 } test_args_t;

/* trusted function ids */
enum {
    fcn_id_test = 0,
    fcn_id_trusted_call_id_max = OE_ENUM_MAX
};


/* untrusted function ids */
enum {
    fcn_id_untrusted_call_max = OE_ENUM_MAX
};


#endif // TESTENCLAVE_ARGS_H