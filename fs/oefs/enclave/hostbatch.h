// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OEFS_HOSTBATCH_H
#define _OEFS_HOSTBATCH_H

#include <stdint.h>
#include "common.h"

OE_EXTERNC_BEGIN

typedef struct _oefs_host_batch oefs_host_batch_t;

oefs_host_batch_t* oefs_host_batch_new(size_t capacity);

void oefs_host_batch_delete(oefs_host_batch_t* batch);

void* oefs_host_batch_malloc(oefs_host_batch_t* batch, size_t size);

void* oefs_host_batch_calloc(oefs_host_batch_t* batch, size_t size);

char* oefs_host_batch_strdup(oefs_host_batch_t* batch, const char* str);

int oefs_host_batch_free(oefs_host_batch_t* batch);

OE_EXTERNC_END

#endif /* _OEFS_HOSTBATCH_H */
