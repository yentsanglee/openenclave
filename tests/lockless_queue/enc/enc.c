/* Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. */

#include <lockless_queue_t.h>
#include <openenclave/internal/tests.h>

#ifdef _MSC_VER
#include <intrin.h>
#endif /* _MSC_VER */

/* thread helpers */
/******************************************************************************/
static void wait_for_barrier(size_t* p_barrier, size_t barrier_size)
{
    size_t barrier_count = 0;
    do
    {
#ifdef _MSC_VER
        barrier_count = _InterlockedCompareExchange64(p_barrier, 0, 0);
#elif defined __GNUC__
        barrier_count = __atomic_load_n(p_barrier, __ATOMIC_ACQUIRE);
#endif /* _MSC_VER or __GNUC__ */
    } while (barrier_count < barrier_size);
} /* wait_for_barrier */

static size_t signal_barrier(size_t* p_barrier)
{
    size_t barrier_count = 0;
#ifdef _MSC_VER
    barrier_count = _InterlockedIncrement64(p_barrier);
#elif defined __GNUC__
    barrier_count = __atomic_add_fetch(p_barrier, 1, __ATOMIC_ACQ_REL);
#endif /* _MSC_VER or __GNUC__ */
    return barrier_count;
} /* signal_barrier */

/* lfds_queue helpers */
/******************************************************************************/
static void lfds_queue_node_init(lfds_queue_node* p_node)
{
    LFDS711_QUEUE_UMM_SET_VALUE_IN_ELEMENT(*p_node, p_node);
} /*lfds_queue_node_init */

static void lfds_queue_push(lfds_queue* queue, lfds_queue_node* p_node)
{
    lfds711_queue_umm_enqueue(queue, p_node);
} /* lfds_queue_push */

static lfds_queue_node* lfds_queue_pop(lfds_queue* p_queue)
{
    lfds_queue_node* p_node = NULL;
    return (1 == lfds711_queue_umm_dequeue(p_queue, &p_node))
               ? LFDS711_QUEUE_UMM_GET_VALUE_FROM_ELEMENT(*p_node)
               : NULL;
} /* lfds_queue_pop */

/* enclave functions */
/******************************************************************************/
void enc_writer_thread(
    oe_lockless_queue* p_queue,
    test_node* p_nodes,
    size_t node_count,
    size_t* p_barrier,
    size_t barrier_size)
{
    signal_barrier(p_barrier);
    wait_for_barrier(p_barrier, barrier_size);

    /* push this thread's nodes onto the queue */
    /* push the nodes onto the queue */
    for (size_t i = 0; i < node_count; ++i)
    {
        oe_lockless_queue_push_back(p_queue, &(p_nodes[i]._node));
    }

    signal_barrier(p_barrier);
} /* enc_writer_thread */

void enc_lfds_writer_thread(
    lfds_queue* p_queue,
    lfds_test_node* p_nodes,
    size_t node_count,
    size_t* p_barrier,
    size_t barrier_size)
{
    signal_barrier(p_barrier);
    wait_for_barrier(p_barrier, barrier_size);

    LFDS_THREAD_INIT;

    /* push this thread's nodes onto the queue */
    /* push the nodes onto the queue */
    for (size_t i = 0; i < node_count; ++i)
    {
        lfds_queue_push(p_queue, &(p_nodes[i]._node));
    }

    signal_barrier(p_barrier);
} /* enc_lfds_writer_thread */

void enc_reader_thread(
    oe_lockless_queue* p_queue,
    size_t node_count,
    size_t* p_barrier,
    size_t barrier_size)
{
    size_t count = 0;

    signal_barrier(p_barrier);
    wait_for_barrier(p_barrier, barrier_size);

    /* pop the nodes off of the queue */
    while (count < node_count)
    {
        oe_lockless_queue_node* p_node = oe_lockless_queue_pop_front(p_queue);
        if (NULL != p_node)
        {
            test_node* p_test_node = (test_node*)p_node;
            ++(p_test_node->count);
            p_test_node->pop_order = count;
            ++count;
        }
    }

    signal_barrier(p_barrier);
} /* enc_reader_thread */

void enc_lfds_reader_thread(
    lfds_queue* p_queue,
    size_t node_count,
    size_t* p_barrier,
    size_t barrier_size)
{
    size_t count = 0;

    signal_barrier(p_barrier);
    wait_for_barrier(p_barrier, barrier_size);

    LFDS_THREAD_INIT;

    /* pop the nodes off of the queue */
    while (count < node_count)
    {
        lfds_queue_node* p_node = lfds_queue_pop(p_queue);
        if (NULL != p_node)
        {
            lfds_test_node* p_test_node = (lfds_test_node*)p_node;
            ++(p_test_node->count);
            p_test_node->pop_order = count;
            ++count;
        }
    }

    signal_barrier(p_barrier);
} /* enc_lfds_reader_thread */

void enc_test_queue_single_threaded(size_t* p_barrier)
{
    oe_lockless_queue queue;
    oe_lockless_queue_node* nodes = (oe_lockless_queue_node*)malloc(
        sizeof(oe_lockless_queue_node) * TEST_COUNT);
    oe_lockless_queue_node* p_node = NULL;

    oe_lockless_queue_init(&queue);

    OE_TEST(NULL == oe_lockless_queue_pop_front(&queue));

    signal_barrier(p_barrier);

    wait_for_barrier(p_barrier, 2);

    for (size_t i = 0; i < TEST_COUNT; ++i)
    {
        oe_lockless_queue_node_init(nodes + i);
    }

    for (size_t i = 0; i < TEST_COUNT; ++i)
    {
        oe_lockless_queue_push_back(&queue, nodes + i);
    }

    for (size_t i = 0; i < TEST_COUNT; ++i)
    {
        p_node = oe_lockless_queue_pop_front(&queue);
        OE_TEST(p_node == nodes + i);
    }

    signal_barrier(p_barrier);
    free(nodes);
} /* enc_test_queue_single_threaded */

void enc_lfds_test_queue_single_threaded(size_t* p_barrier)
{
    lfds_queue queue;
    lfds_queue_node* nodes =
        (lfds_queue_node*)malloc(sizeof(lfds_queue_node) * TEST_COUNT);
    lfds_queue_node dummy;
    lfds_queue_node* p_node = NULL;

    lfds711_queue_umm_init_valid_on_current_logical_core(&queue, &dummy, NULL);

    OE_TEST(NULL == lfds_queue_pop(&queue));

    signal_barrier(p_barrier);

    wait_for_barrier(p_barrier, 2);

    for (size_t i = 0; i < TEST_COUNT; ++i)
    {
        lfds_queue_node_init(nodes + i);
    }

    for (size_t i = 0; i < TEST_COUNT; ++i)
    {
        lfds_queue_push(&queue, nodes + i);
    }

    for (size_t i = 0; i < TEST_COUNT; ++i)
    {
        p_node = lfds_queue_pop(&queue);
        OE_TEST(p_node == nodes + i);
    }

    signal_barrier(p_barrier);
    lfds711_queue_umm_cleanup(&queue, NULL);
    free(nodes);
} /* enc_lfds_test_queue_single_threaded */

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* AllowDebug */
    1024, /* HeapPageCount */
    128,  /* StackPageCount */
    16);  /* TCSCount */
