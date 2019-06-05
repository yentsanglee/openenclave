/* Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. */

#ifndef _SPINLOCK_QUEUE_H_
#define _SPINLOCK_QUEUE_H_

#include <openenclave/internal/defs.h>


OE_EXTERNC_BEGIN

#ifdef _MSC_VER
typedef long oe_spinlock_t;
#elif defined __GNUC__
typedef char oe_spinlock_t;
#endif /* _MSC_VER or __GNUC__ */


/**
 * @struct _oe_spinlock_queue_node
 *
 * @brief The basic structure for a spinlock queue node.
 *
 * This structure is the basic node used with struct _oe_spinlock queue.
 *
 * @note This should be initialized with oe_spinlock_queue_node_init() before
 *       use.
 *
 * @see _oe_spinlock_queue
 * @see oe_spinlock_queue_node_init()
 */
typedef struct _oe_spinlock_queue_node
{
    /**
     * @internal
     */
    struct _oe_spinlock_queue_node* p_link;
} oe_spinlock_queue_node;

/**
 * @function oe_spinlock_queue_node_init
 *
 * @brief Initializes an _oe_spinlock_queue_node.
 *
 * Prepares a node for use with _oe_spinlock_queue.
 *
 * @param p_node An uninitialized _oe_spinlock_queue_node.
 *
 * @pre p_node is non-NULL and points to an unitialized node.
 * @post p_node is prepared to pass to oe_spinlock_queue_push_back().
 */
void oe_spinlock_queue_node_init(oe_spinlock_queue_node* p_node);

/**
 * @struct _oe_spinlock_queue
 *
 * @brief The structure for managing a multi-producer, multi-consumer, FIFO
 *        queue data structure that is multi-thread stable.
 *
 * This structure is the basic control data type for a thread-safe spinlock FIFO
 * queue.  This data structure allows any number of threads to call
 * oe_spinlock_queue_push_back() and any number of threads to call
 * oe_spinlock_queue_pop_front() concurrently without the use of any mutex while
 * maintaining a consistent and stable state.
 *
 * @note This should be initialized with oe_spinlock_queue_init() before use.
 *
 * @see oe_spinlock_queue_node_init()
 * @see oe_spinlock_queue_push_front()
 * @see oe_spinlock_queue_pop_back()
 */
typedef struct _oe_spinlock_queue
{
    /**
     * @internal
     */
    oe_spinlock_queue_node* p_tail;
    /**
     * @internal
     */
    oe_spinlock_queue_node* p_head;
    /**
     * @internal
     */
    oe_spinlock_t lock;
} oe_spinlock_queue;

/**
 * @function oe_spinlock_queue_init
 *
 * @brief Initializes an _oe_spinlock_queue_node.
 *
 * Prepares an _oe_spinlock_queue for use.
 *
 * @param p_queue An uninitialized _oe_spinlock_queue.
 *
 * @pre p_queue is non-NULL and points to an unitialized queue.
 * @post p_queue is prepared to use with oe_spinlock_queue_push_back() and
 * oe_spinlock_queue_pop_front().
 */
void oe_spinlock_queue_init(oe_spinlock_queue* p_queue);

/**
 * @function oe_spinlock_queue_push_back
 *
 * @brief Appends an _oe_spinlock_queue node to the tail end of an
 *        _oe_spinlock_queue.
 *
 * @param p_queue The _oe_spinlock_queue to append the node.
 * @param p_node The _oe_spinlock_queue_node to append to the queue.
 *
 * @pre p_queue is non-NULL and points to an initialized queue and p_node is
 *      non-NULL and points to an initialized node.
 * @post p_node has been appended to the end of p_queue.
 */
void oe_spinlock_queue_push_back(
    oe_spinlock_queue* p_queue,
    oe_spinlock_queue_node* p_node);

/**
 * @function oe_spinlock_queue_pop_front
 *
 * @brief Attempts to remove and return an _oe_spinlock_queue_node from the head
 *        of an _oe_spinlock_queue.
 *
 * @param p_queue The _oe_spinlock_queue to remove a node from.
 * @return A pointer to an _oe_spinlock_queue_node if there was at least one in
 *         the queue or NULL if there was not a node in the queue.
 * @pre p_queue is non-NULL and points to an initialized queue and p_node is
 *      non-NULL and points to an initialized node.
 */
oe_spinlock_queue_node* oe_spinlock_queue_pop_front(oe_spinlock_queue* p_queue);

OE_EXTERNC_END

#endif /* _SPINLOCK_QUEUE_H_ */
