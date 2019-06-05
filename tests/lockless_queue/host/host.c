/* Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. */

#include <liblfds711.h>
#include <lockless_queue_u.h>
#include <openenclave/internal/error.h>
#include <openenclave/internal/tests.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* thread help */
/******************************************************************************/
#ifdef _MSC_VER
/* #include <intrin.h> */
#include <Windows.h>
#elif defined __GNUC__
#include <pthread.h>
#endif /* _MSC_VER or __GNUC__ */

#ifdef _MSC_VER

#define THREAD_RETURN_TYPE DWORD WINAPI
#define THREAD_ARG_TYPE LPVOID
#define THREAD_RETURN_VAL 0
#define THREAD_TYPE HANDLE

typedef DWORD (*thread_op_t)(THREAD_ARG_TYPE);

int thread_create(THREAD_TYPE* thread, thread_op_t op, THREAD_ARG_TYPE arg)
{
    *thread = CreateThread(NULL, 0, op, arg, 0, NULL);
    return NULL == *thread;
} /* thread_create */

void thread_join(THREAD_TYPE thread)
{
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
} /* thread_join */

#elif defined __GNUC__

#define THREAD_RETURN_TYPE void*
#define THREAD_ARG_TYPE void*
#define THREAD_RETURN_VAL NULL
#define THREAD_TYPE pthread_t

typedef THREAD_RETURN_TYPE (*thread_op_t)(THREAD_ARG_TYPE);

int thread_create(THREAD_TYPE* thread, thread_op_t op, THREAD_ARG_TYPE arg)
{
    return pthread_create(thread, NULL, op, arg);
} /* thread_create */

void thread_join(THREAD_TYPE thread)
{
    pthread_join(thread, NULL);
} /* thread_join */

#endif /* _MSC_VER or __GNUC__ */

/* barrier help */
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

/* timer help */
/******************************************************************************/
#if _MSC_VER

typedef LARGE_INTEGER oe_timer_t;

static void get_time(oe_timer_t* p_time)
{
    QueryPerformanceCounter(p_time);
} /* get_time */

static oe_timer_t elapsed(oe_timer_t start, oe_timer_t stop)
{
    LARGE_INTEGER rval;
    rval.QuadPart = stop.QuadPart - start.QuadPart;
    return rval;
} /* elapsed */

static double timespan_to_sec(oe_timer_t ts)
{
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    return (double)ts.QuadPart / (double)freq.QuadPart;
} /* timespec_to_sec */

#elif defined __GNUC__

static long NS_PER_SEC = 1000000000;

typedef struct timespec oe_timer_t;

static void get_time(oe_timer_t* p_time)
{
    clock_gettime(CLOCK_REALTIME, p_time);
} /*get_time */

static oe_timer_t elapsed(oe_timer_t start, oe_timer_t stop)
{
    oe_timer_t out = {0, 0};
    if (start.tv_nsec > stop.tv_nsec)
    {
        out.tv_sec = stop.tv_sec - (start.tv_sec + 1);
        out.tv_nsec = NS_PER_SEC + stop.tv_nsec - start.tv_nsec;
    }
    else
    {
        out.tv_sec = stop.tv_sec - start.tv_sec;
        out.tv_nsec = stop.tv_nsec - start.tv_nsec;
    }
    return out;
} /* elapsed */

static double timespan_to_sec(oe_timer_t ts)
{
    return (double)ts.tv_nsec / (double)NS_PER_SEC + (double)ts.tv_sec;
} /* timespec_to_sec */

#endif /* _MSC_VER or __GNUC__ */

/* oe_lockless_queue helpers */
/******************************************************************************/
static void test_node_init(test_node* p_node)
{
    oe_lockless_queue_node_init(&(p_node->_node));
    p_node->count = 0;
    p_node->pop_order = 0;
} /* test_node_init */

typedef struct _thread_data
{
    oe_lockless_queue* p_queue;
    test_node* p_nodes;
    size_t node_count;
    size_t* p_barrier;
    size_t barrier_size;
    THREAD_TYPE thread;
    oe_enclave_t* enclave;
} thread_data;

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

static void lfds_test_node_init(lfds_test_node* p_node)
{
    lfds_queue_node_init(&(p_node->_node));
    p_node->count = 0;
    p_node->pop_order = 0;
} /* lfds_test_node_init */

typedef struct _lfds_thread_data
{
    lfds_queue* p_queue;
    lfds_test_node* p_nodes;
    size_t node_count;
    size_t* p_barrier;
    size_t barrier_size;
    THREAD_TYPE thread;
    oe_enclave_t* enclave;
} lfds_thread_data;

/* thread functions */
/******************************************************************************/
THREAD_RETURN_TYPE host_writer_thread(THREAD_ARG_TYPE _data)
{
    thread_data* data = (thread_data*)_data;

    /* printf("    host_writer_thread - started\n"); */

    signal_barrier(data->p_barrier);
    wait_for_barrier(data->p_barrier, data->barrier_size);

    /* push this thread's nodes onto the queue */
    for (size_t i = 0; i < data->node_count; ++i)
    {
        oe_lockless_queue_push_back(data->p_queue, &(data->p_nodes[i]._node));
    }

    signal_barrier(data->p_barrier);

    /* printf("    host_writer_thread finished\n"); */
    return THREAD_RETURN_VAL;
} /* host_writer_thread */

THREAD_RETURN_TYPE lfds_host_writer_thread(THREAD_ARG_TYPE _data)
{
    lfds_thread_data* data = (lfds_thread_data*)_data;

    /* printf("    lfds_host_writer_thread - started\n"); */

    signal_barrier(data->p_barrier);
    wait_for_barrier(data->p_barrier, data->barrier_size);

    LFDS_THREAD_INIT;

    /* push this thread's nodes onto the queue */
    for (size_t i = 0; i < data->node_count; ++i)
    {
        lfds_queue_push(data->p_queue, &(data->p_nodes[i]._node));
    }

    signal_barrier(data->p_barrier);

    /* printf("    lfds_host_writer_thread finished\n"); */
    return THREAD_RETURN_VAL;
} /* lfds_host_writer_thread */

THREAD_RETURN_TYPE host_reader_thread(THREAD_ARG_TYPE _data)
{
    size_t node_count = 0;
    thread_data* data = (thread_data*)_data;
    /* printf("    host_reader_thread - started\n"); */

    signal_barrier(data->p_barrier);
    wait_for_barrier(data->p_barrier, data->barrier_size);

    /* pop the nodes off of the queue */
    while (node_count < data->node_count)
    {
        oe_lockless_queue_node* p_node =
            oe_lockless_queue_pop_front(data->p_queue);
        if (NULL != p_node)
        {
            test_node* p_test_node = (test_node*)p_node;
            ++(p_test_node->count);
            p_test_node->pop_order = node_count;
            ++node_count;
        }
    }

    signal_barrier(data->p_barrier);

    /* printf("    host_reader_thread - finished\n"); */
    return THREAD_RETURN_VAL;
} /* host_reader_thread */

THREAD_RETURN_TYPE lfds_host_reader_thread(THREAD_ARG_TYPE _data)
{
    size_t node_count = 0;
    lfds_thread_data* data = (lfds_thread_data*)_data;
    /* printf("    lfds_host_reader_thread - started\n"); */

    signal_barrier(data->p_barrier);
    wait_for_barrier(data->p_barrier, data->barrier_size);

    LFDS_THREAD_INIT;

    /* pop the nodes off of the queue */
    while (node_count < data->node_count)
    {
        lfds_queue_node* p_node = lfds_queue_pop(data->p_queue);
        if (NULL != p_node)
        {
            lfds_test_node* p_test_node = (lfds_test_node*)p_node;
            ++(p_test_node->count);
            p_test_node->pop_order = node_count;
            ++node_count;
        }
    }

    signal_barrier(data->p_barrier);

    /* printf("    lfds_host_reader_thread - finished\n"); */
    return THREAD_RETURN_VAL;
} /* lfds_host_reader_thread */

THREAD_RETURN_TYPE enc_test_queue_single_threaded_wrapper(THREAD_ARG_TYPE _data)
{
    thread_data* data = (thread_data*)_data;
    /* printf("    enc_test_queue_single_threaded_thread - started\n"); */

    OE_TEST(
        OE_OK ==
        enc_test_queue_single_threaded(data->enclave, data->p_barrier));

    /* printf("    enc_test_queue_single_threaded_thread - finished\n"); */
    return THREAD_RETURN_VAL;
} /* enc_test_queue_single_threaded_wrapper */

THREAD_RETURN_TYPE enc_lfds_test_queue_single_threaded_wrapper(
    THREAD_ARG_TYPE _data)
{
    lfds_thread_data* data = (lfds_thread_data*)_data;
    /* printf("    enc_lfds_test_queue_single_threaded_thread - started\n"); */

    OE_TEST(
        OE_OK ==
        enc_lfds_test_queue_single_threaded(data->enclave, data->p_barrier));

    /* printf("    enc_lfds_test_queue_single_threaded_thread - finished\n"); */
    return THREAD_RETURN_VAL;
} /* enc_lfds_test_queue_single_threaded_wrapper */

THREAD_RETURN_TYPE enc_writer_thread_wrapper(THREAD_ARG_TYPE _data)
{
    thread_data* data = (thread_data*)_data;
    /* printf("    enc_writer_thread - started\n"); */

    OE_TEST(
        OE_OK == enc_writer_thread(
                     data->enclave,
                     data->p_queue,
                     data->p_nodes,
                     data->node_count,
                     data->p_barrier,
                     data->barrier_size));

    /* printf("    enc_writer_thread - finished\n"); */
    return THREAD_RETURN_VAL;
} /* enc_writer_thread_wrapper */

THREAD_RETURN_TYPE enc_lfds_writer_thread_wrapper(THREAD_ARG_TYPE _data)
{
    lfds_thread_data* data = (lfds_thread_data*)_data;
    /* printf("    enc_lfds_writer_thread - started\n"); */

    OE_TEST(
        OE_OK == enc_lfds_writer_thread(
                     data->enclave,
                     data->p_queue,
                     data->p_nodes,
                     data->node_count,
                     data->p_barrier,
                     data->barrier_size));

    /* printf("    enc_lfds_writer_thread - finished\n"); */
    return THREAD_RETURN_VAL;
} /* enc_lfds_writer_thread_wrapper */

THREAD_RETURN_TYPE enc_reader_thread_wrapper(THREAD_ARG_TYPE _data)
{
    thread_data* data = (thread_data*)_data;
    /* printf("    enc_reader_thread started\n"); */

    OE_TEST(
        OE_OK == enc_reader_thread(
                     data->enclave,
                     data->p_queue,
                     data->node_count,
                     data->p_barrier,
                     data->barrier_size));

    /* printf("    enc_reader_thread finished\n"); */
    return THREAD_RETURN_VAL;
} /* enc_reader_thread_wrapper */

THREAD_RETURN_TYPE enc_lfds_reader_thread_wrapper(THREAD_ARG_TYPE _data)
{
    lfds_thread_data* data = (lfds_thread_data*)_data;
    /* printf("    enc_lfds_reader_thread started\n"); */

    OE_TEST(
        OE_OK == enc_lfds_reader_thread(
                     data->enclave,
                     data->p_queue,
                     data->node_count,
                     data->p_barrier,
                     data->barrier_size));

    /* printf("    enc_lfds_reader_thread finished\n"); */
    return THREAD_RETURN_VAL;
} /* enc_lfds_reader_thread_wrapper */

/* tests */
/******************************************************************************/
/*static*/ void host_queue_test()
{
    oe_lockless_queue queue;
    oe_lockless_queue_node* nodes = (oe_lockless_queue_node*)malloc(
        sizeof(oe_lockless_queue_node) * TEST_COUNT);
    oe_lockless_queue_node* p_node = NULL;
    oe_timer_t start, stop, timespan;

    /* printf("<host_queue_test>\n"); */

    oe_lockless_queue_init(&queue);

    OE_TEST(NULL == oe_lockless_queue_pop_front(&queue));

    get_time(&start);

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

    get_time(&stop);
    timespan = elapsed(start, stop);
    printf("    lockless_queue: %.8lf\n", timespan_to_sec(timespan));

    OE_TEST(NULL == oe_lockless_queue_pop_front(&queue));

    free(nodes);
    /* printf("</host_queue_test>\n"); */
} /* host_queue_test */

/*static*/ void host_lfds_queue_test()
{
    lfds_queue queue;
    lfds_queue_node* nodes =
        (lfds_queue_node*)malloc(sizeof(lfds_queue_node) * TEST_COUNT);
    lfds_queue_node* p_node = NULL;
    lfds_queue_node dummy; /* required */
    oe_timer_t start, stop, timespan;

    /* printf("<host_lfds_queue_test>\n"); */

    lfds711_queue_umm_init_valid_on_current_logical_core(&queue, &dummy, NULL);

    OE_TEST(NULL == lfds_queue_pop(&queue));

    get_time(&start);

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

    get_time(&stop);
    timespan = elapsed(start, stop);
    printf("    lfds_queue:     %.8lf\n", timespan_to_sec(timespan));

    OE_TEST(NULL == lfds_queue_pop(&queue));

    lfds711_queue_umm_cleanup(&queue, NULL);
    free(nodes);
    /* printf("</host_lfds_queue_test>\n"); */
} /* host_lfds_queue_test */

/*static*/ void host_queue_mw_sr_test()
{
    size_t barrier = 0;
    thread_data threads[THREAD_COUNT];
    test_node* nodes =
        (test_node*)malloc(sizeof(test_node) * THREAD_COUNT * TEST_COUNT);
    thread_data reader_thread;
    oe_lockless_queue queue;
    oe_timer_t start, stop, timespan;
    /* printf("<host_queue_mw_sr_test>\n"); */

    oe_lockless_queue_init(&queue);
    for (size_t i = 0; i < THREAD_COUNT * TEST_COUNT; ++i)
    {
        test_node_init(nodes + i);
    }

    reader_thread.p_queue = &queue;
    reader_thread.p_barrier = &barrier;
    reader_thread.barrier_size = THREAD_COUNT + 2;
    reader_thread.node_count = THREAD_COUNT * TEST_COUNT;
    OE_TEST(
        0 == thread_create(
                 &(reader_thread.thread), host_reader_thread, &reader_thread));

    for (size_t i = 0; i < THREAD_COUNT; ++i)
    {
        threads[i].p_barrier = &barrier;
        threads[i].barrier_size = THREAD_COUNT + 2;
        threads[i].p_nodes = nodes + i * TEST_COUNT;
        threads[i].node_count = TEST_COUNT;
        threads[i].p_queue = &queue;
        threads[i].enclave = NULL;
        OE_TEST(
            0 == thread_create(
                     &(threads[i].thread), host_writer_thread, threads + i));
    }

    wait_for_barrier(&barrier, THREAD_COUNT + 1);

    get_time(&start);

    signal_barrier(&barrier);

    wait_for_barrier(&barrier, THREAD_COUNT * 2 + 3);

    get_time(&stop);
    timespan = elapsed(start, stop);
    printf("    lockless_queue: %.8lf\n", timespan_to_sec(timespan));

    thread_join(reader_thread.thread);
    for (size_t i = 0; i < THREAD_COUNT; ++i)
    {
        size_t last_pop_order = 0;
        thread_join(threads[i].thread);
        OE_TEST(1 == nodes[TEST_COUNT * i].count);
        last_pop_order = nodes[TEST_COUNT * i].pop_order;
        for (size_t j = 1; j < TEST_COUNT; ++j)
        {
            OE_TEST(1 == nodes[TEST_COUNT * i + j].count);
            OE_TEST(last_pop_order <= nodes[TEST_COUNT * i + j].pop_order);
            last_pop_order = nodes[TEST_COUNT * i + j].pop_order;
        }
    }
    OE_TEST(NULL == oe_lockless_queue_pop_front(&queue));

    free(nodes);
    /* printf("</host_queue_mw_sr_test>\n"); */
} /* host_queue_mw_sr_test */

/*static*/ void host_lfds_queue_mw_sr_test()
{
    size_t barrier = 0;
    lfds_thread_data threads[THREAD_COUNT];
    lfds_test_node* nodes = (lfds_test_node*)malloc(
        sizeof(lfds_test_node) * THREAD_COUNT * TEST_COUNT);
    lfds_thread_data reader_thread;
    lfds_queue queue;
    oe_timer_t start, stop, timespan;
    lfds_queue_node dummy;
    /* printf("<lfds_host_queue_mw_sr_test>\n"); */

    lfds711_queue_umm_init_valid_on_current_logical_core(&queue, &dummy, NULL);
    for (size_t i = 0; i < THREAD_COUNT * TEST_COUNT; ++i)
    {
        lfds_test_node_init(nodes + i);
    }

    reader_thread.p_queue = &queue;
    reader_thread.p_barrier = &barrier;
    reader_thread.barrier_size = THREAD_COUNT + 2;
    reader_thread.node_count = THREAD_COUNT * TEST_COUNT;
    OE_TEST(
        0 ==
        thread_create(
            &(reader_thread.thread), lfds_host_reader_thread, &reader_thread));

    for (size_t i = 0; i < THREAD_COUNT; ++i)
    {
        threads[i].p_barrier = &barrier;
        threads[i].barrier_size = THREAD_COUNT + 2;
        threads[i].p_nodes = nodes + i * TEST_COUNT;
        threads[i].node_count = TEST_COUNT;
        threads[i].p_queue = &queue;
        threads[i].enclave = NULL;
        OE_TEST(
            0 ==
            thread_create(
                &(threads[i].thread), lfds_host_writer_thread, threads + i));
    }

    wait_for_barrier(&barrier, THREAD_COUNT + 1);

    get_time(&start);

    signal_barrier(&barrier);

    wait_for_barrier(&barrier, THREAD_COUNT * 2 + 3);

    get_time(&stop);
    timespan = elapsed(start, stop);
    printf("    lfds_queue:     %.8lf\n", timespan_to_sec(timespan));

    thread_join(reader_thread.thread);
    for (size_t i = 0; i < THREAD_COUNT; ++i)
    {
        size_t last_pop_order = 0;
        thread_join(threads[i].thread);
        OE_TEST(1 == nodes[TEST_COUNT * i].count);
        last_pop_order = nodes[TEST_COUNT * i].pop_order;
        for (size_t j = 1; j < TEST_COUNT; ++j)
        {
            OE_TEST(1 == nodes[TEST_COUNT * i + j].count);
            OE_TEST(last_pop_order <= nodes[TEST_COUNT * i + j].pop_order);
            last_pop_order = nodes[TEST_COUNT * i + j].pop_order;
        }
    }

    OE_TEST(NULL == lfds_queue_pop(&queue));

    lfds711_queue_umm_cleanup(&queue, NULL);
    free(nodes);
    /* printf("</lfds_host_queue_mw_sr_test>\n"); */
} /* lfds_host_queue_mw_sr_test */

/*static*/ void enc_queue_test(oe_enclave_t* enclave)
{
    size_t barrier = 0;
    thread_data enc_thread;
    oe_timer_t start, stop, timespan;
    /* printf("<enc_queue_test>\n"); */

    enc_thread.enclave = enclave;
    enc_thread.p_barrier = &barrier;

    OE_TEST(
        0 == thread_create(
                 &(enc_thread.thread),
                 enc_test_queue_single_threaded_wrapper,
                 &enc_thread));

    wait_for_barrier(&barrier, 1);

    get_time(&start);

    signal_barrier(&barrier);

    wait_for_barrier(&barrier, 3);

    get_time(&stop);
    timespan = elapsed(start, stop);
    printf("    lockless_queue: %.8lf\n", timespan_to_sec(timespan));

    thread_join(enc_thread.thread);

    /* printf("</enc_queue_test>\n"); */
} /* enc_queue_test */

/*static*/ void enc_lfds_queue_test(oe_enclave_t* enclave)
{
    size_t barrier = 0;
    lfds_thread_data enc_thread;
    oe_timer_t start, stop, timespan;
    /* printf("<enc_lfds_queue_test>\n"); */

    enc_thread.enclave = enclave;
    enc_thread.p_barrier = &barrier;

    OE_TEST(
        0 == thread_create(
                 &(enc_thread.thread),
                 enc_lfds_test_queue_single_threaded_wrapper,
                 &enc_thread));

    wait_for_barrier(&barrier, 1);

    get_time(&start);

    signal_barrier(&barrier);

    wait_for_barrier(&barrier, 3);

    get_time(&stop);
    timespan = elapsed(start, stop);
    printf("    lfds_queue:     %.8lf\n", timespan_to_sec(timespan));

    thread_join(enc_thread.thread);

    /* printf("</enc_lfds_queue_test>\n"); */
} /* enc_lfds_queue_test */

/*static*/ void enc_queue_mw_sr_test(oe_enclave_t* enclave)
{
    size_t barrier = 0;
    thread_data threads[THREAD_COUNT];
    test_node* nodes =
        (test_node*)malloc(sizeof(test_node) * THREAD_COUNT * TEST_COUNT);
    oe_lockless_queue queue;
    thread_data reader_thread;
    oe_timer_t start, stop, timespan;
    /* printf("<enc_queue_mw_sr_test>\n"); */

    oe_lockless_queue_init(&queue);
    for (size_t i = 0; i < THREAD_COUNT * TEST_COUNT; ++i)
    {
        test_node_init(nodes + i);
    }

    reader_thread.enclave = enclave;
    reader_thread.p_queue = &queue;
    reader_thread.node_count = TEST_COUNT * THREAD_COUNT;
    reader_thread.p_barrier = &barrier;
    reader_thread.barrier_size = THREAD_COUNT + 2;

    OE_TEST(
        0 == thread_create(
                 &(reader_thread.thread),
                 enc_reader_thread_wrapper,
                 &reader_thread));

    for (size_t i = 0; i < THREAD_COUNT; ++i)
    {
        threads[i].p_barrier = &barrier;
        threads[i].barrier_size = THREAD_COUNT + 2;
        threads[i].p_nodes = nodes + i * TEST_COUNT;
        threads[i].node_count = TEST_COUNT;
        threads[i].p_queue = &queue;
        threads[i].enclave = enclave;
        OE_TEST(
            0 ==
            thread_create(
                &(threads[i].thread), enc_writer_thread_wrapper, threads + i));
    }

    wait_for_barrier(&barrier, THREAD_COUNT + 1);

    get_time(&start);

    signal_barrier(&barrier);

    wait_for_barrier(&barrier, THREAD_COUNT * 2 + 3);

    get_time(&stop);
    timespan = elapsed(start, stop);
    printf("    lockless_queue: %.8lf\n", timespan_to_sec(timespan));

    thread_join(reader_thread.thread);
    for (size_t i = 0; i < THREAD_COUNT; ++i)
    {
        size_t last_pop_order = 0;
        thread_join(threads[i].thread);
        OE_TEST(1 == nodes[TEST_COUNT * i].count);
        last_pop_order = nodes[TEST_COUNT * i].pop_order;
        for (size_t j = 1; j < TEST_COUNT; ++j)
        {
            OE_TEST(1 == nodes[TEST_COUNT * i + j].count);
            OE_TEST(last_pop_order <= nodes[TEST_COUNT * i + j].pop_order);
            last_pop_order = nodes[TEST_COUNT * i + j].pop_order;
        }
    }
    OE_TEST(NULL == oe_lockless_queue_pop_front(&queue));

    free(nodes);
    /* printf("</enc_queue_mw_sr_test>\n"); */
} /*enc_queue_mw_sr_test */

/*static*/ void enc_lfds_queue_mw_sr_test(oe_enclave_t* enclave)
{
    size_t barrier = 0;
    lfds_queue_node dummy;
    lfds_thread_data threads[THREAD_COUNT];
    lfds_test_node* nodes = (lfds_test_node*)malloc(
        sizeof(lfds_test_node) * THREAD_COUNT * TEST_COUNT);
    lfds_queue queue;
    lfds_thread_data reader_thread;
    oe_timer_t start, stop, timespan;
    /* printf("<enc_lfds_queue_mw_sr_test>\n"); */

    lfds711_queue_umm_init_valid_on_current_logical_core(&queue, &dummy, NULL);
    for (size_t i = 0; i < THREAD_COUNT * TEST_COUNT; ++i)
    {
        lfds_test_node_init(nodes + i);
    }

    reader_thread.enclave = enclave;
    reader_thread.p_queue = &queue;
    reader_thread.node_count = TEST_COUNT * THREAD_COUNT;
    reader_thread.p_barrier = &barrier;
    reader_thread.barrier_size = THREAD_COUNT + 2;

    OE_TEST(
        0 == thread_create(
                 &(reader_thread.thread),
                 enc_lfds_reader_thread_wrapper,
                 &reader_thread));

    for (size_t i = 0; i < THREAD_COUNT; ++i)
    {
        threads[i].p_barrier = &barrier;
        threads[i].barrier_size = THREAD_COUNT + 2;
        threads[i].p_nodes = nodes + i * TEST_COUNT;
        threads[i].node_count = TEST_COUNT;
        threads[i].p_queue = &queue;
        threads[i].enclave = enclave;
        OE_TEST(
            0 == thread_create(
                     &(threads[i].thread),
                     enc_lfds_writer_thread_wrapper,
                     threads + i));
    }

    wait_for_barrier(&barrier, THREAD_COUNT + 1);

    get_time(&start);

    signal_barrier(&barrier);

    wait_for_barrier(&barrier, THREAD_COUNT * 2 + 3);

    get_time(&stop);
    timespan = elapsed(start, stop);
    printf("    lfds_queue:     %.8lf\n", timespan_to_sec(timespan));

    thread_join(reader_thread.thread);
    for (size_t i = 0; i < THREAD_COUNT; ++i)
    {
        size_t last_pop_order = 0;
        thread_join(threads[i].thread);
        OE_TEST(1 == nodes[TEST_COUNT * i].count);
        last_pop_order = nodes[TEST_COUNT * i].pop_order;
        for (size_t j = 1; j < TEST_COUNT; ++j)
        {
            OE_TEST(1 == nodes[TEST_COUNT * i + j].count);
            OE_TEST(last_pop_order <= nodes[TEST_COUNT * i + j].pop_order);
            last_pop_order = nodes[TEST_COUNT * i + j].pop_order;
        }
    }
    OE_TEST(NULL == lfds_queue_pop(&queue));

    lfds711_queue_umm_cleanup(&queue, NULL);

    free(nodes);
    /* printf("</enc_lfds_queue_mw_sr_test>\n"); */
} /*enc_lfds_queue_mw_sr_test */

/*static*/ void host_enq_enc_deq_test(oe_enclave_t* enclave)
{
    oe_lockless_queue queue;
    test_node* nodes = (test_node*)malloc(sizeof(test_node) * TEST_COUNT);
    size_t barrier = 0;
    thread_data enc_thread;
    oe_timer_t start, stop, timespan;
    /* printf("<host_enq_enc_deq_test>\n"); */

    oe_lockless_queue_init(&queue);

    OE_TEST(NULL == oe_lockless_queue_pop_front(&queue));

    enc_thread.enclave = enclave;
    enc_thread.p_queue = &queue;
    enc_thread.node_count = TEST_COUNT;
    enc_thread.p_barrier = &barrier;
    enc_thread.barrier_size = 2;

    OE_TEST(
        0 == thread_create(
                 &(enc_thread.thread), enc_reader_thread_wrapper, &enc_thread));

    wait_for_barrier(&barrier, 1);

    get_time(&start);

    for (size_t i = 0; i < TEST_COUNT; ++i)
    {
        test_node_init(nodes + i);
    }

    for (size_t i = 0; i < TEST_COUNT; ++i)
    {
        oe_lockless_queue_push_back(&queue, &(nodes[i]._node));
    }

    signal_barrier(&barrier);

    wait_for_barrier(&barrier, 3);

    get_time(&stop);
    timespan = elapsed(start, stop);
    printf("    lockless_queue: %.8lf\n", timespan_to_sec(timespan));

    thread_join(enc_thread.thread);

    for (size_t i = 0; i < TEST_COUNT; ++i)
    {
        OE_TEST(1 == nodes[i].count);
        OE_TEST(i == nodes[i].pop_order);
    }

    free(nodes);
    /* printf("<host_enq_enc_deq_test>\n"); */
} /* host_enq_enc_deq_test */

/*static*/ void lfds_host_enq_enc_deq_test(oe_enclave_t* enclave)
{
    lfds_queue queue;
    lfds_queue_node dummy;
    lfds_test_node* nodes =
        (lfds_test_node*)malloc(sizeof(lfds_test_node) * TEST_COUNT);
    size_t barrier = 0;
    lfds_thread_data enc_thread;
    oe_timer_t start, stop, timespan;
    /* printf("<lfds_host_enq_enc_deq_test>\n"); */

    lfds711_queue_umm_init_valid_on_current_logical_core(&queue, &dummy, NULL);

    OE_TEST(NULL == lfds_queue_pop(&queue));

    enc_thread.enclave = enclave;
    enc_thread.p_queue = &queue;
    enc_thread.node_count = TEST_COUNT;
    enc_thread.p_barrier = &barrier;
    enc_thread.barrier_size = 2;

    OE_TEST(
        0 ==
        thread_create(
            &(enc_thread.thread), enc_lfds_reader_thread_wrapper, &enc_thread));

    wait_for_barrier(&barrier, 1);

    get_time(&start);

    for (size_t i = 0; i < TEST_COUNT; ++i)
    {
        lfds_test_node_init(nodes + i);
    }

    for (size_t i = 0; i < TEST_COUNT; ++i)
    {
        lfds_queue_push(&queue, &(nodes[i]._node));
    }

    signal_barrier(&barrier);

    wait_for_barrier(&barrier, 3);

    get_time(&stop);
    timespan = elapsed(start, stop);
    printf("    lfds_queue:     %.8lf\n", timespan_to_sec(timespan));

    thread_join(enc_thread.thread);

    for (size_t i = 0; i < TEST_COUNT; ++i)
    {
        OE_TEST(1 == nodes[i].count);
        OE_TEST(i == nodes[i].pop_order);
    }

    lfds711_queue_umm_cleanup(&queue, NULL);
    free(nodes);
    /* printf("<lfds_host_enq_enc_deq_test>\n"); */
} /* lfds_host_enq_enc_deq_test */

/*static*/ void host_enq_enc_deq_mw_sr_test(oe_enclave_t* enclave)
{
    size_t barrier = 0;
    thread_data threads[THREAD_COUNT];
    test_node* nodes =
        (test_node*)malloc(sizeof(test_node) * THREAD_COUNT * TEST_COUNT);
    oe_lockless_queue queue;
    thread_data reader_thread;
    oe_timer_t start, stop, timespan;
    /* printf("<host_enq_enc_deq_mw_sr_test>\n"); */

    oe_lockless_queue_init(&queue);
    for (size_t i = 0; i < THREAD_COUNT * TEST_COUNT; ++i)
    {
        test_node_init(nodes + i);
    }

    reader_thread.enclave = enclave;
    reader_thread.p_queue = &queue;
    reader_thread.node_count = TEST_COUNT * THREAD_COUNT;
    reader_thread.p_barrier = &barrier;
    reader_thread.barrier_size = THREAD_COUNT + 2;

    OE_TEST(
        0 == thread_create(
                 &(reader_thread.thread),
                 enc_reader_thread_wrapper,
                 &reader_thread));

    for (size_t i = 0; i < THREAD_COUNT; ++i)
    {
        threads[i].p_barrier = &barrier;
        threads[i].barrier_size = THREAD_COUNT + 2;
        threads[i].p_nodes = nodes + i * TEST_COUNT;
        threads[i].node_count = TEST_COUNT;
        threads[i].p_queue = &queue;
        threads[i].enclave = NULL;
        OE_TEST(
            0 == thread_create(
                     &(threads[i].thread), host_writer_thread, threads + i));
    }

    wait_for_barrier(&barrier, THREAD_COUNT + 1);

    get_time(&start);

    signal_barrier(&barrier);

    wait_for_barrier(&barrier, THREAD_COUNT * 2 + 3);

    get_time(&stop);
    timespan = elapsed(start, stop);
    printf("    lockless_queue: %.8lf\n", timespan_to_sec(timespan));

    thread_join(reader_thread.thread);
    for (size_t i = 0; i < THREAD_COUNT; ++i)
    {
        size_t last_pop_order = 0;
        thread_join(threads[i].thread);
        OE_TEST(1 == nodes[TEST_COUNT * i].count);
        last_pop_order = nodes[TEST_COUNT * i].pop_order;
        for (size_t j = 1; j < TEST_COUNT; ++j)
        {
            OE_TEST(1 == nodes[TEST_COUNT * i + j].count);
            OE_TEST(last_pop_order <= nodes[TEST_COUNT * i + j].pop_order);
            last_pop_order = nodes[TEST_COUNT * i + j].pop_order;
        }
    }
    OE_TEST(NULL == oe_lockless_queue_pop_front(&queue));

    free(nodes);
    /* printf("</host_enq_enc_deq_mw_sr_test>\n"); */
} /* host_enq_enc_deq_mw_sr_test */

/*static*/ void lfds_host_enq_enc_deq_mw_sr_test(oe_enclave_t* enclave)
{
    size_t barrier = 0;
    lfds_thread_data threads[THREAD_COUNT];
    lfds_test_node* nodes = (lfds_test_node*)malloc(
        sizeof(lfds_test_node) * THREAD_COUNT * TEST_COUNT);
    lfds_queue queue;
    lfds_queue_node dummy;
    lfds_thread_data reader_thread;
    oe_timer_t start, stop, timespan;
    /* printf("<lfds_host_enq_enc_deq_mw_sr_test>\n"); */

    lfds711_queue_umm_init_valid_on_current_logical_core(&queue, &dummy, NULL);
    for (size_t i = 0; i < THREAD_COUNT * TEST_COUNT; ++i)
    {
        lfds_test_node_init(nodes + i);
    }

    reader_thread.enclave = enclave;
    reader_thread.p_queue = &queue;
    reader_thread.node_count = TEST_COUNT * THREAD_COUNT;
    reader_thread.p_barrier = &barrier;
    reader_thread.barrier_size = THREAD_COUNT + 2;

    OE_TEST(
        0 == thread_create(
                 &(reader_thread.thread),
                 enc_lfds_reader_thread_wrapper,
                 &reader_thread));

    for (size_t i = 0; i < THREAD_COUNT; ++i)
    {
        threads[i].p_barrier = &barrier;
        threads[i].barrier_size = THREAD_COUNT + 2;
        threads[i].p_nodes = nodes + i * TEST_COUNT;
        threads[i].node_count = TEST_COUNT;
        threads[i].p_queue = &queue;
        threads[i].enclave = NULL;
        OE_TEST(
            0 ==
            thread_create(
                &(threads[i].thread), lfds_host_writer_thread, threads + i));
    }

    wait_for_barrier(&barrier, THREAD_COUNT + 1);

    get_time(&start);

    signal_barrier(&barrier);

    wait_for_barrier(&barrier, THREAD_COUNT * 2 + 3);

    get_time(&stop);
    timespan = elapsed(start, stop);
    printf("    lfds_queue:     %.8lf\n", timespan_to_sec(timespan));

    thread_join(reader_thread.thread);
    for (size_t i = 0; i < THREAD_COUNT; ++i)
    {
        size_t last_pop_order = 0;
        thread_join(threads[i].thread);
        OE_TEST(1 == nodes[TEST_COUNT * i].count);
        last_pop_order = nodes[TEST_COUNT * i].pop_order;
        for (size_t j = 1; j < TEST_COUNT; ++j)
        {
            OE_TEST(1 == nodes[TEST_COUNT * i + j].count);
            OE_TEST(last_pop_order <= nodes[TEST_COUNT * i + j].pop_order);
            last_pop_order = nodes[TEST_COUNT * i + j].pop_order;
        }
    }
    OE_TEST(NULL == lfds_queue_pop(&queue));

    lfds711_queue_umm_cleanup(&queue, NULL);
    free(nodes);
    /* printf("</lfds_host_enq_enc_deq_mw_sr_test>\n"); */
} /* lfds_host_enq_enc_deq_mw_sr_test */

/*static*/ void enc_enq_host_deq_test(oe_enclave_t* enclave)
{
    test_node* nodes = (test_node*)malloc(sizeof(test_node) * TEST_COUNT);
    oe_lockless_queue queue;
    test_node* p_node = NULL;
    size_t barrier = 0;
    thread_data enc_thread;
    oe_timer_t start, stop, timespan;
    /* printf("<enc_enq_host_deq_test>\n"); */

    oe_lockless_queue_init(&queue);
    for (size_t i = 0; i < TEST_COUNT; ++i)
    {
        test_node_init(nodes + i);
    }

    enc_thread.p_barrier = &barrier;
    enc_thread.barrier_size = 2;
    enc_thread.p_nodes = nodes;
    enc_thread.node_count = TEST_COUNT;
    enc_thread.p_queue = &queue;
    enc_thread.enclave = enclave;
    OE_TEST(
        0 == thread_create(
                 &(enc_thread.thread), enc_writer_thread_wrapper, &enc_thread));

    wait_for_barrier(&barrier, 1);

    get_time(&start);

    signal_barrier(&barrier);

    wait_for_barrier(&barrier, 3);

    for (size_t i = 0; i < TEST_COUNT; ++i)
    {
        p_node = (test_node*)oe_lockless_queue_pop_front(&queue);
        OE_TEST(p_node != NULL);
        OE_TEST(p_node == nodes + i);
    }

    get_time(&stop);
    timespan = elapsed(start, stop);
    printf("    lockless_queue: %.8lf\n", timespan_to_sec(timespan));

    thread_join(enc_thread.thread);

    OE_TEST(NULL == oe_lockless_queue_pop_front(&queue));

    free(nodes);
    /* printf("</enc_enq_host_deq_test>\n"); */
} /* enc_enq_host_deq_test */

/*static*/ void lfds_enc_enq_host_deq_test(oe_enclave_t* enclave)
{
    lfds_test_node* nodes =
        (lfds_test_node*)malloc(sizeof(lfds_test_node) * TEST_COUNT);
    lfds_queue queue;
    lfds_queue_node dummy;
    lfds_test_node* p_node = NULL;
    size_t barrier = 0;
    lfds_thread_data enc_thread;
    oe_timer_t start, stop, timespan;
    /* printf("<lfds_enc_enq_host_deq_test>\n"); */

    lfds711_queue_umm_init_valid_on_current_logical_core(&queue, &dummy, NULL);
    for (size_t i = 0; i < TEST_COUNT; ++i)
    {
        lfds_test_node_init(nodes + i);
    }

    enc_thread.p_barrier = &barrier;
    enc_thread.barrier_size = 2;
    enc_thread.p_nodes = nodes;
    enc_thread.node_count = TEST_COUNT;
    enc_thread.p_queue = &queue;
    enc_thread.enclave = enclave;
    OE_TEST(
        0 ==
        thread_create(
            &(enc_thread.thread), enc_lfds_writer_thread_wrapper, &enc_thread));

    wait_for_barrier(&barrier, 1);

    get_time(&start);

    signal_barrier(&barrier);

    wait_for_barrier(&barrier, 3);

    for (size_t i = 0; i < TEST_COUNT; ++i)
    {
        p_node = (lfds_test_node*)lfds_queue_pop(&queue);
        OE_TEST(p_node != NULL);
        OE_TEST(p_node == nodes + i);
    }

    get_time(&stop);
    timespan = elapsed(start, stop);
    printf("    lfds_queue:     %.8lf\n", timespan_to_sec(timespan));

    thread_join(enc_thread.thread);

    OE_TEST(NULL == lfds_queue_pop(&queue));

    /* printf("</lfds_enc_enq_host_deq_test>\n"); */
    lfds711_queue_umm_cleanup(&queue, NULL);
    free(nodes);
} /* lfds_enc_enq_host_deq_test */

void enc_enq_host_deq_mw_sr_test(oe_enclave_t* enclave)
{
    size_t barrier = 0;
    thread_data threads[THREAD_COUNT];
    test_node* nodes =
        (test_node*)malloc(sizeof(test_node) * THREAD_COUNT * TEST_COUNT);
    thread_data reader_thread;
    oe_lockless_queue queue;
    oe_timer_t start, stop, timespan;
    /* printf("<enc_enq_host_deq_mw_sr_test>\n"); */

    oe_lockless_queue_init(&queue);
    for (size_t i = 0; i < THREAD_COUNT * TEST_COUNT; ++i)
    {
        test_node_init(nodes + i);
    }

    reader_thread.p_queue = &queue;
    reader_thread.p_barrier = &barrier;
    reader_thread.barrier_size = THREAD_COUNT + 2;
    reader_thread.node_count = TEST_COUNT * THREAD_COUNT;
    OE_TEST(
        0 == thread_create(
                 &(reader_thread.thread), host_reader_thread, &reader_thread));

    for (size_t i = 0; i < THREAD_COUNT; ++i)
    {
        threads[i].p_barrier = &barrier;
        threads[i].barrier_size = THREAD_COUNT + 2;
        threads[i].p_nodes = nodes + i * TEST_COUNT;
        threads[i].node_count = TEST_COUNT;
        threads[i].p_queue = &queue;
        threads[i].enclave = enclave;
        OE_TEST(
            0 ==
            thread_create(
                &(threads[i].thread), enc_writer_thread_wrapper, threads + i));
    }

    wait_for_barrier(&barrier, THREAD_COUNT + 1);

    get_time(&start);

    signal_barrier(&barrier);

    wait_for_barrier(&barrier, THREAD_COUNT * 2 + 3);

    get_time(&stop);
    timespan = elapsed(start, stop);
    printf("    lockless_queue: %.8lf\n", timespan_to_sec(timespan));

    thread_join(reader_thread.thread);
    for (size_t i = 0; i < THREAD_COUNT; ++i)
    {
        size_t last_pop_order = 0;
        thread_join(threads[i].thread);
        OE_TEST(1 == nodes[TEST_COUNT * i].count);
        last_pop_order = nodes[TEST_COUNT * i].pop_order;
        for (size_t j = 1; j < TEST_COUNT; ++j)
        {
            OE_TEST(1 == nodes[TEST_COUNT * i + j].count);
            OE_TEST(last_pop_order <= nodes[TEST_COUNT * i + j].pop_order);
            last_pop_order = nodes[TEST_COUNT * i + j].pop_order;
        }
    }
    OE_TEST(NULL == oe_lockless_queue_pop_front(&queue));

    free(nodes);
    /* printf("</enc_enq_host_deq_mw_sr_test>\n"); */
} /* enc_enq_host_deq_mw_sr_test */

void lfds_enc_enq_host_deq_mw_sr_test(oe_enclave_t* enclave)
{
    size_t barrier = 0;
    lfds_thread_data threads[THREAD_COUNT];
    lfds_test_node* nodes = (lfds_test_node*)malloc(
        sizeof(lfds_test_node) * THREAD_COUNT * TEST_COUNT);
    lfds_thread_data reader_thread;
    lfds_queue queue;
    lfds_queue_node dummy;
    oe_timer_t start, stop, timespan;
    /* printf("<lfds_enc_enq_host_deq_mw_sr_test>\n"); */

    lfds711_queue_umm_init_valid_on_current_logical_core(&queue, &dummy, NULL);
    for (size_t i = 0; i < THREAD_COUNT * TEST_COUNT; ++i)
    {
        lfds_test_node_init(nodes + i);
    }

    reader_thread.p_queue = &queue;
    reader_thread.p_barrier = &barrier;
    reader_thread.barrier_size = THREAD_COUNT + 2;
    reader_thread.node_count = TEST_COUNT * THREAD_COUNT;
    OE_TEST(
        0 ==
        thread_create(
            &(reader_thread.thread), lfds_host_reader_thread, &reader_thread));

    for (size_t i = 0; i < THREAD_COUNT; ++i)
    {
        threads[i].p_barrier = &barrier;
        threads[i].barrier_size = THREAD_COUNT + 2;
        threads[i].p_nodes = nodes + i * TEST_COUNT;
        threads[i].node_count = TEST_COUNT;
        threads[i].p_queue = &queue;
        threads[i].enclave = enclave;
        OE_TEST(
            0 == thread_create(
                     &(threads[i].thread),
                     enc_lfds_writer_thread_wrapper,
                     threads + i));
    }

    wait_for_barrier(&barrier, THREAD_COUNT + 1);

    get_time(&start);

    signal_barrier(&barrier);

    wait_for_barrier(&barrier, THREAD_COUNT * 2 + 3);

    get_time(&stop);
    timespan = elapsed(start, stop);
    printf("    lfds_queue:     %.8lf\n", timespan_to_sec(timespan));

    thread_join(reader_thread.thread);
    for (size_t i = 0; i < THREAD_COUNT; ++i)
    {
        size_t last_pop_order = 0;
        thread_join(threads[i].thread);
        OE_TEST(1 == nodes[TEST_COUNT * i].count);
        last_pop_order = nodes[TEST_COUNT * i].pop_order;
        for (size_t j = 1; j < TEST_COUNT; ++j)
        {
            OE_TEST(1 == nodes[TEST_COUNT * i + j].count);
            OE_TEST(last_pop_order <= nodes[TEST_COUNT * i + j].pop_order);
            last_pop_order = nodes[TEST_COUNT * i + j].pop_order;
        }
    }
    OE_TEST(NULL == lfds_queue_pop(&queue));

    /* printf("</lfds_enc_enq_host_deq_mw_sr_test>\n"); */
    lfds711_queue_umm_cleanup(&queue, NULL);
    free(nodes);
} /* lfds_enc_enq_host_deq_mw_sr_test */

#if (0)
#endif

int main(int argc, const char** argv)
{
    oe_result_t result = OE_OK;
    const uint32_t flags = oe_get_create_flags();
    oe_enclave_t* enclave;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s ENCLAVE\n", argv[0]);
        exit(1);
    }

    /* these tests are executed within the host */
    printf("host only, single threaded:\n");
    host_queue_test();
    host_lfds_queue_test();
    printf("\nhost only, multiple concurrent writers, single reader:\n");
    host_queue_mw_sr_test();
    host_lfds_queue_mw_sr_test();

    result = oe_create_lockless_queue_enclave(
        argv[1], OE_ENCLAVE_TYPE_SGX, flags, NULL, 0, &enclave);
    if (OE_OK != result)
    {
        oe_put_err("oe_create_lockless_queue_enclave(): result=%u", result);
    }

    /* these tests are executed within the enclave */
    printf("\nenc only, single threaded:\n");
    enc_queue_test(enclave);
    enc_lfds_queue_test(enclave);
    printf("\nenc only, multiple concurrent writers, single reader:\n");
    enc_queue_mw_sr_test(enclave);
    enc_lfds_queue_mw_sr_test(enclave);

    /* these tests enqueue nodes from the host and dequeue nodes from the
     * enclave */
    printf("\nhost enq - enc deq, single threaded:\n");
    host_enq_enc_deq_test(enclave);
    lfds_host_enq_enc_deq_test(enclave);
    printf("\nhost enq - enc deq, multiple concurrent writers, single "
           "reader:\n");
    host_enq_enc_deq_mw_sr_test(enclave);
    lfds_host_enq_enc_deq_mw_sr_test(enclave);

    /* these tests enqueue nodes from the enclave and dequeue nodes from the
     * host */
    printf("\nenc enq - host deq, single threaded:\n");
    enc_enq_host_deq_test(enclave);
    lfds_enc_enq_host_deq_test(enclave);
    printf(
        "\nenc enq - host deq, multiple concurrent writers, single reader:\n");
    enc_enq_host_deq_mw_sr_test(enclave);
    lfds_enc_enq_host_deq_mw_sr_test(enclave);

    oe_terminate_enclave(enclave);

    return 0;
} /* main */
