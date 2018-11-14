// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define PTHREAD_MUTEX_INITIALIZER ENCLIBC_PTHREAD_MUTEX_INITIALIZER
#define PTHREAD_RWLOCK_INITIALIZER ENCLIBC_PTHREAD_RWLOCK_INITIALIZER
#define PTHREAD_COND_INITIALIZER ENCLIBC_PTHREAD_COND_INITIALIZER
#define PTHREAD_ONCE_INIT ENCLIBC_PTHREAD_ONCE_INIT

typedef enclibc_pthread_t pthread_t;

typedef enclibc_pthread_once_t pthread_once_t;

typedef enclibc_pthread_spinlock_t pthread_spinlock_t;

typedef enclibc_pthread_key_t pthread_key_t;

typedef enclibc_pthread_attr_t pthread_attr_t;

typedef enclibc_pthread_mutexattr_t pthread_mutexattr_t;

typedef enclibc_pthread_mutex_t pthread_mutex_t;

typedef enclibc_pthread_condattr_t pthread_condattr_t;

typedef enclibc_pthread_cond_t pthread_cond_t;

typedef enclibc_pthread_rwlockattr_t pthread_rwlockattr_t;

typedef enclibc_pthread_rwlock_t pthread_rwlock_t;

ENCLIBC_INLINE
pthread_t pthread_self()
{
    return (pthread_t)enclibc_pthread_self();
}

ENCLIBC_INLINE
int pthread_equal(pthread_t thread1, pthread_t thread2)
{
    return enclibc_pthread_equal((enclibc_pthread_t)thread1, (enclibc_pthread_t)thread2);
}

ENCLIBC_INLINE
int pthread_create(
    pthread_t* thread,
    const pthread_attr_t* attr,
    void* (*start_routine)(void*),
    void* arg)
{
    return enclibc_pthread_create(
        (enclibc_pthread_t*)thread,
        (const enclibc_pthread_attr_t*)attr,
        start_routine,
        arg);
}

ENCLIBC_INLINE
int pthread_join(pthread_t thread, void** retval)
{
    return enclibc_pthread_join((enclibc_pthread_t)thread, retval);
}

ENCLIBC_INLINE
int pthread_detach(pthread_t thread)
{
    return enclibc_pthread_detach((enclibc_pthread_t)thread);
}

ENCLIBC_INLINE
int pthread_once(pthread_once_t* once, void (*func)(void))
{
    return enclibc_pthread_once((enclibc_pthread_once_t*)once, func);
}

ENCLIBC_INLINE
int pthread_spin_init(pthread_spinlock_t* spinlock, int pshared)
{
    return enclibc_pthread_spin_init((enclibc_pthread_spinlock_t*)spinlock, pshared);
}

ENCLIBC_INLINE
int pthread_spin_lock(pthread_spinlock_t* spinlock)
{
    return enclibc_pthread_spin_lock((enclibc_pthread_spinlock_t*)spinlock);
}

ENCLIBC_INLINE
int pthread_spin_unlock(pthread_spinlock_t* spinlock)
{
    return enclibc_pthread_spin_unlock((enclibc_pthread_spinlock_t*)spinlock);
}

ENCLIBC_INLINE
int pthread_spin_destroy(pthread_spinlock_t* spinlock)
{
    return enclibc_pthread_spin_destroy((enclibc_pthread_spinlock_t*)spinlock);
}

ENCLIBC_INLINE
int pthread_mutexattr_init(pthread_mutexattr_t* attr)
{
    return enclibc_pthread_mutexattr_init((enclibc_pthread_mutexattr_t*)attr);
}

ENCLIBC_INLINE
int pthread_mutexattr_settype(pthread_mutexattr_t* attr, int type)
{
    return enclibc_pthread_mutexattr_settype((enclibc_pthread_mutexattr_t*)attr, type);
}

ENCLIBC_INLINE
int pthread_mutexattr_destroy(pthread_mutexattr_t* attr)
{
    return enclibc_pthread_mutexattr_destroy((enclibc_pthread_mutexattr_t*)attr);
}

ENCLIBC_INLINE
int pthread_mutex_init(pthread_mutex_t* m, const pthread_mutexattr_t* attr)
{
    return enclibc_pthread_mutex_init(
        (enclibc_pthread_mutex_t*)m, (const enclibc_pthread_mutexattr_t*)attr);
}

ENCLIBC_INLINE
int pthread_mutex_lock(pthread_mutex_t* m)
{
    return enclibc_pthread_mutex_lock((enclibc_pthread_mutex_t*)m);
}

ENCLIBC_INLINE
int pthread_mutex_trylock(pthread_mutex_t* m)
{
    return enclibc_pthread_mutex_trylock((enclibc_pthread_mutex_t*)m);
}

ENCLIBC_INLINE
int pthread_mutex_unlock(pthread_mutex_t* m)
{
    return enclibc_pthread_mutex_unlock((enclibc_pthread_mutex_t*)m);
}

ENCLIBC_INLINE
int pthread_mutex_destroy(pthread_mutex_t* m)
{
    return enclibc_pthread_mutex_destroy((enclibc_pthread_mutex_t*)m);
}

ENCLIBC_INLINE
int pthread_rwlock_init(
    pthread_rwlock_t* rwlock,
    const pthread_rwlockattr_t* attr)
{
    return enclibc_pthread_rwlock_init(
        (enclibc_pthread_rwlock_t*)rwlock, (enclibc_pthread_rwlockattr_t*)attr);
}

ENCLIBC_INLINE
int pthread_rwlock_rdlock(pthread_rwlock_t* rwlock)
{
    return enclibc_pthread_rwlock_rdlock((enclibc_pthread_rwlock_t*)rwlock);
}

ENCLIBC_INLINE
int pthread_rwlock_wrlock(pthread_rwlock_t* rwlock)
{
    return enclibc_pthread_rwlock_wrlock((enclibc_pthread_rwlock_t*)rwlock);
}

ENCLIBC_INLINE
int pthread_rwlock_unlock(pthread_rwlock_t* rwlock)
{
    return enclibc_pthread_rwlock_unlock((enclibc_pthread_rwlock_t*)rwlock);
}

ENCLIBC_INLINE
int pthread_rwlock_destroy(pthread_rwlock_t* rwlock)
{
    return enclibc_pthread_rwlock_destroy((enclibc_pthread_rwlock_t*)rwlock);
}

ENCLIBC_INLINE
int pthread_cond_init(pthread_cond_t* cond, const pthread_condattr_t* attr)
{
    return enclibc_pthread_cond_init(
        (enclibc_pthread_cond_t*)cond, (const enclibc_pthread_condattr_t*)attr);
}

ENCLIBC_INLINE
int pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex)
{
    return enclibc_pthread_cond_wait(
        (enclibc_pthread_cond_t*)cond, (enclibc_pthread_mutex_t*)mutex);
}

ENCLIBC_INLINE
int pthread_cond_timedwait(
    pthread_cond_t* cond,
    pthread_mutex_t* mutex,
    const struct timespec* ts)
{
    return enclibc_pthread_cond_timedwait(
        (enclibc_pthread_cond_t*)cond,
        (enclibc_pthread_mutex_t*)mutex,
        (const struct enclibc_timespec*)ts);
}

ENCLIBC_INLINE
int pthread_cond_signal(pthread_cond_t* cond)
{
    return enclibc_pthread_cond_signal((enclibc_pthread_cond_t*)cond);
}

ENCLIBC_INLINE
int pthread_cond_broadcast(pthread_cond_t* cond)
{
    return enclibc_pthread_cond_broadcast((enclibc_pthread_cond_t*)cond);
}

ENCLIBC_INLINE
int pthread_cond_destroy(pthread_cond_t* cond)
{
    return enclibc_pthread_cond_destroy((enclibc_pthread_cond_t*)cond);
}

ENCLIBC_INLINE
int pthread_key_create(pthread_key_t* key, void (*destructor)(void* value))
{
    return enclibc_pthread_key_create((enclibc_pthread_key_t*)key, destructor);
}

ENCLIBC_INLINE
int pthread_key_delete(pthread_key_t key)
{
    return enclibc_pthread_key_delete((enclibc_pthread_key_t)key);
}

ENCLIBC_INLINE
int pthread_setspecific(pthread_key_t key, const void* value)
{
    return enclibc_pthread_setspecific((enclibc_pthread_key_t)key, value);
}

ENCLIBC_INLINE
void* pthread_getspecific(pthread_key_t key)
{
    return enclibc_pthread_getspecific((enclibc_pthread_key_t)key);
}
