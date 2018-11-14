// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <pthread.h>
#include <assert.h>
#include <errno.h>
#include <openenclave/enclave.h>
#include <openenclave/internal/enclavelibc.h>
#include <openenclave/internal/thread.h>

enclibc_pthread_t enclibc_pthread_self()
{
    return (enclibc_pthread_t)oe_thread_self();
}

int enclibc_pthread_equal(enclibc_pthread_t thread1, enclibc_pthread_t thread2)
{
    return (int)oe_thread_equal((oe_thread_t)thread1, (oe_thread_t)thread2);
}

int enclibc_pthread_create(
    enclibc_pthread_t* thread,
    const enclibc_pthread_attr_t* attr,
    void* (*start_routine)(void*),
    void* arg)
{
    assert("enclibc_pthread_create(): panic" == NULL);
    return -1;
}

int enclibc_pthread_join(enclibc_pthread_t thread, void** retval)
{
    assert("pthread_join(): panic" == NULL);
    return -1;
}

int enclibc_pthread_detach(enclibc_pthread_t thread)
{
    assert("pthread_detach(): panic" == NULL);
    return -1;
}

/* Map an oe_result_t to a POSIX error number */
ENCLIBC_INLINE int _to_errno(oe_result_t result)
{
    switch (result)
    {
        case OE_OK:
            return 0;
        case OE_INVALID_PARAMETER:
            return EINVAL;
        case OE_BUSY:
            return EBUSY;
        case OE_NOT_OWNER:
            return EPERM;
        case OE_OUT_OF_MEMORY:
            return ENOMEM;
        default:
            return EINVAL; /* unreachable */
    }
}

/*
**==============================================================================
**
** enclibc_pthread_once_t
**
**==============================================================================
*/

int enclibc_pthread_once(enclibc_pthread_once_t* once, void (*func)(void))
{
    return _to_errno(oe_once((oe_once_t*)once, func));
}

/*
**==============================================================================
**
** enclibc_pthread_spinlock_t
**
**==============================================================================
*/

int enclibc_pthread_spin_init(enclibc_pthread_spinlock_t* spinlock, int pshared)
{
    return _to_errno(oe_spin_init((oe_spinlock_t*)spinlock));
}

int enclibc_pthread_spin_lock(enclibc_pthread_spinlock_t* spinlock)
{
    return _to_errno(oe_spin_lock((oe_spinlock_t*)spinlock));
}

int enclibc_pthread_spin_unlock(enclibc_pthread_spinlock_t* spinlock)
{
    return _to_errno(oe_spin_unlock((oe_spinlock_t*)spinlock));
}

int enclibc_pthread_spin_destroy(enclibc_pthread_spinlock_t* spinlock)
{
    return _to_errno(oe_spin_destroy((oe_spinlock_t*)spinlock));
}

/*
**==============================================================================
**
** enclibc_pthread_mutex_t
**
**==============================================================================
*/

int enclibc_pthread_mutexattr_init(enclibc_pthread_mutexattr_t* attr)
{
    return 0;
}

int enclibc_pthread_mutexattr_settype(enclibc_pthread_mutexattr_t* attr, int type)
{
    return 0;
}

int enclibc_pthread_mutexattr_destroy(enclibc_pthread_mutexattr_t* attr)
{
    return 0;
}

int enclibc_pthread_mutex_init(
    enclibc_pthread_mutex_t* m,
    const enclibc_pthread_mutexattr_t* attr)
{
    return _to_errno(oe_mutex_init((oe_mutex_t*)m));
}

int enclibc_pthread_mutex_lock(enclibc_pthread_mutex_t* m)
{
    return _to_errno(oe_mutex_lock((oe_mutex_t*)m));
}

int enclibc_pthread_mutex_trylock(enclibc_pthread_mutex_t* m)
{
    return _to_errno(oe_mutex_trylock((oe_mutex_t*)m));
}

int enclibc_pthread_mutex_unlock(enclibc_pthread_mutex_t* m)
{
    return _to_errno(oe_mutex_unlock((oe_mutex_t*)m));
}

int enclibc_pthread_mutex_destroy(enclibc_pthread_mutex_t* m)
{
    return _to_errno(oe_mutex_destroy((oe_mutex_t*)m));
}

/*
**==============================================================================
**
** enclibc_pthread_rwlock_t
**
**==============================================================================
*/

int enclibc_pthread_rwlock_init(
    enclibc_pthread_rwlock_t* rwlock,
    const enclibc_pthread_rwlockattr_t* attr)
{
    return _to_errno(oe_rwlock_init((oe_rwlock_t*)rwlock));
}

int enclibc_pthread_rwlock_rdlock(enclibc_pthread_rwlock_t* rwlock)
{
    return _to_errno(oe_rwlock_rdlock((oe_rwlock_t*)rwlock));
}

int enclibc_pthread_rwlock_wrlock(enclibc_pthread_rwlock_t* rwlock)
{
    return _to_errno(oe_rwlock_wrlock((oe_rwlock_t*)rwlock));
}

int enclibc_pthread_rwlock_unlock(enclibc_pthread_rwlock_t* rwlock)
{
    return _to_errno(oe_rwlock_unlock((oe_rwlock_t*)rwlock));
}

int enclibc_pthread_rwlock_destroy(enclibc_pthread_rwlock_t* rwlock)
{
    return _to_errno(oe_rwlock_destroy((oe_rwlock_t*)rwlock));
}

/*
**==============================================================================
**
** enclibc_pthread_cond_t
**
**==============================================================================
*/

int enclibc_pthread_cond_init(
    enclibc_pthread_cond_t* cond,
    const enclibc_pthread_condattr_t* attr)
{
    return _to_errno(oe_cond_init((oe_cond_t*)cond));
}

int enclibc_pthread_cond_wait(enclibc_pthread_cond_t* cond, enclibc_pthread_mutex_t* mutex)
{
    return _to_errno(oe_cond_wait((oe_cond_t*)cond, (oe_mutex_t*)mutex));
}

int enclibc_pthread_cond_timedwait(
    enclibc_pthread_cond_t* cond,
    enclibc_pthread_mutex_t* mutex,
    const struct enclibc_timespec* ts)
{
    assert("pthread_cond_timedwait(): panic" == NULL);
    return -1;
}

int enclibc_pthread_cond_signal(enclibc_pthread_cond_t* cond)
{
    return _to_errno(oe_cond_signal((oe_cond_t*)cond));
}

int enclibc_pthread_cond_broadcast(enclibc_pthread_cond_t* cond)
{
    return _to_errno(oe_cond_broadcast((oe_cond_t*)cond));
}

int enclibc_pthread_cond_destroy(enclibc_pthread_cond_t* cond)
{
    return _to_errno(oe_cond_destroy((oe_cond_t*)cond));
}

/*
**==============================================================================
**
** enclibc_pthread_key_t (thread specific data)
**
**==============================================================================
*/

int enclibc_pthread_key_create(
    enclibc_pthread_key_t* key,
    void (*destructor)(void* value))
{
    return _to_errno(oe_thread_key_create((oe_thread_key_t*)key, destructor));
}

int enclibc_pthread_key_delete(enclibc_pthread_key_t key)
{
    return _to_errno(oe_thread_key_delete((oe_thread_key_t)key));
}

int enclibc_pthread_setspecific(enclibc_pthread_key_t key, const void* value)
{
    return _to_errno(oe_thread_setspecific((oe_thread_key_t)key, value));
}

void* enclibc_pthread_getspecific(enclibc_pthread_key_t key)
{
    return oe_thread_getspecific((oe_thread_key_t)key);
}
