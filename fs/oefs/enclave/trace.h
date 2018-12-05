#ifndef _OE_OEFS_TRACE_H
#define _OE_OEFS_TRACE_H

#include <stdio.h>

#define TRACE printf("GOTO: %s(%u): %s()\n", __FILE__, __LINE__, __FUNCTION__)

// clang-format off
#if defined(ENABLE_TRACE_GOTO)
# define GOTO(LABEL) \
    do \
    { \
        printf("GOTO: %s(%u): %s()\n", __FILE__, __LINE__, __FUNCTION__); \
        goto LABEL; \
    } \
    while (0)
#else
# define GOTO(LABEL) goto LABEL;
#endif
// clang-format on

#endif /* _OE_OEFS_TRACE_H */
