/* Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. */

#include <openenclave/internal/spinlock_queue.h>
#include <stdlib.h>

#if _MSC_VER
#include <intrin.h>

#define OE_SPINLOCK_LOCK(p_lock) \
    while (_bittestandset(p_lock, 0)) continue

#define OE_SPINLOCK_UNLOCK(p_lock) _bittestandreset(p_lock, 0)

#elif defined __GNUC__

#define OE_SPINLOCK_LOCK(p_lock) \
    while (__atomic_test_and_set(p_lock, __ATOMIC_ACQ_REL)) continue

#define OE_SPINLOCK_UNLOCK(p_lock) __atomic_clear(p_lock, __ATOMIC_RELEASE)

#endif /* _MSC_VER or __GNUC__ */



/* functions for oe_spinlock_queue_node */
/*---------------------------------------------------------------------------*/
void oe_spinlock_queue_node_init(oe_spinlock_queue_node* p_node)
{
    p_node->p_link = NULL;
} /* init_oe_spinlock_queue_node */

/* functions for oe_spinlock_queue */
/*---------------------------------------------------------------------------*/
void oe_spinlock_queue_init(oe_spinlock_queue* p_queue)
{
    p_queue->lock = 0;
    p_queue->p_head = p_queue->p_tail = NULL;
} /* init_oe_spinlock_queue */

void oe_spinlock_queue_push_back(
    oe_spinlock_queue* p_queue,
    oe_spinlock_queue_node* p_node)
{
    OE_SPINLOCK_LOCK(&(p_queue->lock));
    if (p_queue->p_tail)
    {
        p_queue->p_tail->p_link = p_node;
        p_queue->p_tail = p_node;
    }
    else
    {
        p_queue->p_head = p_queue->p_tail = p_node;
    }
    OE_SPINLOCK_UNLOCK(&(p_queue->lock));
} /* oe_spinlock_queue_push */

oe_spinlock_queue_node* oe_spinlock_queue_pop_front(oe_spinlock_queue* p_queue)
{
    oe_spinlock_queue_node* p_node = NULL;
    OE_SPINLOCK_LOCK(&(p_queue->lock));
    if (NULL != (p_node = p_queue->p_head) &&
        NULL == (p_queue->p_head = p_node->p_link))
    {
        p_queue->p_tail = NULL;
    }
    OE_SPINLOCK_UNLOCK(&(p_queue->lock));
    return p_node;
} /* oe_spinlock_queue_pop */
