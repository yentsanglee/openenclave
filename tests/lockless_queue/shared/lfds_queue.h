#ifndef _LFDS_QUEUE_H_
#define _LFDS_QUEUE_H_

#include <liblfds711.h>

typedef struct lfds711_queue_umm_element lfds_queue_node;
typedef struct lfds711_queue_umm_state lfds_queue;

typedef struct _lfds_test_node{
    lfds_queue_node _node;
    size_t count;
    size_t pop_order;
} lfds_test_node;

#define LFDS_THREAD_INIT LFDS711_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE

#endif /* _LFDS_QUEUE_H_ */
