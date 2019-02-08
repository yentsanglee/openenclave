/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* Licensed under the MIT License. */

#ifndef _IOT_COMMON_H
#define _IOT_COMMON_H

#ifdef __cplusplus
#define IOT_EXTERN_C_BEGIN \
    extern "C"                \
    {
#define IOT_EXTERN_C_END }
#else
#define IOT_EXTERN_C_BEGIN
#define IOT_EXTERN_C_END
#endif

#define IOT_INLINE static inline

#endif /* _IOT_COMMON_H */
