// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OEFS_LIST_H
#define _OEFS_LIST_H

#include "common.h"

OE_EXTERNC_BEGIN

typedef struct _oefs_list_node oefs_list_node_t;

typedef struct _oefs_list oefs_list_t;

struct _oefs_list
{
    oefs_list_node_t* head;
    oefs_list_node_t* tail;
    size_t size;
};

struct _oefs_list_node
{
    oefs_list_node_t* prev;
    oefs_list_node_t* next;
};

OE_INLINE void oefs_list_remove(oefs_list_t* list, oefs_list_node_t* node)
{
    if (node->prev)
        node->prev->next = node->next;
    else
        list->head = node->next;

    if (node->next)
        node->next->prev = node->prev;
    else
        list->tail = node->prev;

    list->size--;
}

OE_INLINE void oefs_list_insert_front(oefs_list_t* list, oefs_list_node_t* node)
{
    if (list->head)
    {
        node->prev = NULL;
        node->next = list->head;
        list->head->prev = node;
        list->head = node;
    }
    else
    {
        node->next = NULL;
        node->prev = NULL;
        list->head = node;
        list->tail = node;
    }

    list->size++;
}

OE_INLINE void oefs_list_insert_back(oefs_list_t* list, oefs_list_node_t* node)
{
    if (list->tail)
    {
        node->next = NULL;
        node->prev = list->tail;
        list->tail->next = node;
        list->tail = node;
    }
    else
    {
        node->next = NULL;
        node->prev = NULL;
        list->head = node;
        list->tail = node;
    }

    list->size++;
}

OE_INLINE void oefs_list_free(oefs_list_t* list)
{
    for (oefs_list_node_t* p = list->head; p;)
    {
        oefs_list_node_t* next = p->next;
        free(p);
        p = next;
    }

    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

OE_EXTERNC_END

#endif /* _OEFS_LIST_H */
