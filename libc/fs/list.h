#ifndef _FS_LIST_H
#define _FS_LIST_H

#include "common.h"

typedef struct _fs_list_node fs_list_node_t;

typedef struct _fs_list fs_list_t;

struct _fs_list
{
    fs_list_node_t* head;
    fs_list_node_t* tail;
    size_t count;
};

struct _fs_list_node
{
    fs_list_node_t* next;
    fs_list_node_t* prev;
};

FS_INLINE void fs_list_remove(fs_list_t* list, fs_list_node_t* node)
{
    if (node->prev)
        node->prev->next = node->next;
    else
        list->head = node->next;

    if (node->next)
        node->next->prev = node->prev;
    else
        list->tail = node->prev;

    list->count--;
}

FS_INLINE void fs_list_insert_front(fs_list_t* list, fs_list_node_t* node)
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

    list->count++;
}

#endif /* _FS_LIST_H */
