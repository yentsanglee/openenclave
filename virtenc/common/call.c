// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

const char* ve_func_name(ve_func_t func)
{
    switch (func)
    {
        case VE_FUNC_RET:
            return "RET";
        case VE_FUNC_ERR:
            return "ERR";
        case VE_FUNC_INIT:
            return "INIT";
        case VE_FUNC_ADD_THREAD:
            return "ADD_THREAD";
        case VE_FUNC_TERMINATE_THREAD:
            return "TERMINATE_THREAD";
        case VE_FUNC_TERMINATE:
            return "TERMINATE";
        case VE_FUNC_PING:
            return "PING";
        case VE_FUNC_MALLOC:
            return "MALLOC";
        case VE_FUNC_CALLOC:
            return "CALLOC";
        case VE_FUNC_REALLOC:
            return "REALLOC";
        case VE_FUNC_MEMALIGN:
            return "MEMALIGN";
        case VE_FUNC_FREE:
            return "FREE";
    }

    return "UNKNOWN";
}
