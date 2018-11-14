// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _ENCLIBC_PTHREAD_H
#define _ENCLIBC_PTHREAD_H

#include "bits/common.h"
#include "time.h"

// clang-format off
#define ENCLIBC_PTHREAD_MUTEX_INITIALIZER {{0}}
#define ENCLIBC_PTHREAD_RWLOCK_INITIALIZER {{0}}
#define ENCLIBC_PTHREAD_COND_INITIALIZER {{0}}
#define ENCLIBC_ONCE_INIT 0
// clang-format on

typedef uint64_t enclibc_pthread_t;

typedef uint32_t enclibc_pthread_once_t;

typedef volatile uint32_t enclibc_pthread_spinlock_t;

typedef uint32_t enclibc_pthread_key_t;

typedef struct _oe_pthread_attr
{
    uint64_t __private[7];
} enclibc_pthread_attr_t;

typedef struct _oe_pthread_mutexattr
{
    uint32_t __private;
} enclibc_pthread_mutexattr_t;

typedef struct _oe_pthread_mutex
{
    uint64_t __private[4];
} enclibc_pthread_mutex_t;

typedef struct _oe_pthread_condattr
{
    uint32_t __private;
} enclibc_pthread_condattr_t;

typedef struct _oe_pthread_cond
{
    uint64_t __private[4];
} enclibc_pthread_cond_t;

typedef struct _oe_pthread_rwlockattr
{
    uint32_t __private[2];
} enclibc_pthread_rwlockattr_t;

typedef struct _oe_pthread_rwlock
{
    uint64_t __private[5];
} enclibc_pthread_rwlock_t;

enclibc_pthread_t enclibc_pthread_self(void);

int enclibc_pthread_equal(enclibc_pthread_t thread1, enclibc_pthread_t thread2);

int enclibc_pthread_create(
    enclibc_pthread_t* thread,
    const enclibc_pthread_attr_t* attr,
    void* (*start_routine)(void*),
    void* arg);

int enclibc_pthread_join(enclibc_pthread_t thread, void** retval);

int enclibc_pthread_detach(enclibc_pthread_t thread);

int enclibc_pthread_once(enclibc_pthread_once_t* once, void (*func)(void));

int enclibc_pthread_spin_init(
    enclibc_pthread_spinlock_t* spinlock,
    int pshared);

int enclibc_pthread_spin_lock(enclibc_pthread_spinlock_t* spinlock);

int enclibc_pthread_spin_unlock(enclibc_pthread_spinlock_t* spinlock);

int enclibc_pthread_spin_destroy(enclibc_pthread_spinlock_t* spinlock);

int enclibc_pthread_mutexattr_init(enclibc_pthread_mutexattr_t* attr);

int enclibc_pthread_mutexattr_settype(
    enclibc_pthread_mutexattr_t* attr,
    int type);

int enclibc_pthread_mutexattr_destroy(enclibc_pthread_mutexattr_t* attr);

int enclibc_pthread_mutex_init(
    enclibc_pthread_mutex_t* m,
    const enclibc_pthread_mutexattr_t* attr);

int enclibc_pthread_mutex_lock(enclibc_pthread_mutex_t* m);

int enclibc_pthread_mutex_trylock(enclibc_pthread_mutex_t* m);

int enclibc_pthread_mutex_unlock(enclibc_pthread_mutex_t* m);

int enclibc_pthread_mutex_destroy(enclibc_pthread_mutex_t* m);

int enclibc_pthread_rwlock_init(
    enclibc_pthread_rwlock_t* rwlock,
    const enclibc_pthread_rwlockattr_t* attr);

int enclibc_pthread_rwlock_rdlock(enclibc_pthread_rwlock_t* rwlock);

int enclibc_pthread_rwlock_wrlock(enclibc_pthread_rwlock_t* rwlock);

int enclibc_pthread_rwlock_unlock(enclibc_pthread_rwlock_t* rwlock);

int enclibc_pthread_rwlock_destroy(enclibc_pthread_rwlock_t* rwlock);

int enclibc_pthread_cond_init(
    enclibc_pthread_cond_t* cond,
    const enclibc_pthread_condattr_t* attr);

int enclibc_pthread_cond_wait(
    enclibc_pthread_cond_t* cond,
    enclibc_pthread_mutex_t* mutex);

int enclibc_pthread_cond_timedwait(
    enclibc_pthread_cond_t* cond,
    enclibc_pthread_mutex_t* mutex,
    const struct enclibc_timespec* ts);

int enclibc_pthread_cond_signal(enclibc_pthread_cond_t* cond);

int enclibc_pthread_cond_broadcast(enclibc_pthread_cond_t* cond);

int enclibc_pthread_cond_destroy(enclibc_pthread_cond_t* cond);

int enclibc_pthread_key_create(
    enclibc_pthread_key_t* key,
    void (*destructor)(void* value));

int enclibc_pthread_key_delete(enclibc_pthread_key_t key);

int enclibc_pthread_setspecific(enclibc_pthread_key_t key, const void* value);

void* enclibc_pthread_getspecific(enclibc_pthread_key_t key);

#if defined(ENCLIBC_NEED_STDC_NAMES)

#include "bits/pthread.h"

#endif /* defined(ENCLIBC_NEED_STDC_NAMES) */

#endif /* _ENCLIBC_PTHREAD_H */
