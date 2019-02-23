// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OE_INTERNAL_MMAN_H
#define _OE_INTERNAL_MMAN_H

#include <openenclave/bits/result.h>
#include <openenclave/internal/defs.h>
#include <openenclave/internal/types.h>

OE_EXTERNC_BEGIN

#define OE_PROT_NONE 0
#define OE_PROT_READ 1
#define OE_PROT_WRITE 2
#define OE_PROT_EXEC 4

#define OE_MAP_SHARED 1
#define OE_MAP_PRIVATE 2
#define OE_MAP_FIXED 16
#define OE_MAP_ANONYMOUS 32

#define OE_MREMAP_MAYMOVE 1

#define OE_HEAP_ERROR_SIZE 256

/* Virtual Address Descriptor */
typedef struct _oe_vad
{
    /* Pointer to next oe_vad_t on linked list */
    struct _oe_vad* next;

    /* Pointer to previous oe_vad_t on linked list */
    struct _oe_vad* prev;

    /* Address of this memory region */
    uintptr_t addr;

    /* Size of this memory region in bytes */
    uint32_t size;

    /* Protection flags for this region OE_PROT_???? */
    uint16_t prot;

    /* Mapping flags for this region: OE_MAP_???? */
    uint16_t flags;
} oe_vad_t;

OE_STATIC_ASSERT(sizeof(oe_vad_t) == 32);

#define OE_HEAP_MAGIC 0xcc8e1732ebd80b0b

#define OE_HEAP_ERR_SIZE 256

/* Heap Code coverage */
typedef enum _OE_HeapCoverage
{
    OE_HEAP_COVERAGE_0,
    OE_HEAP_COVERAGE_1,
    OE_HEAP_COVERAGE_2,
    OE_HEAP_COVERAGE_3,
    OE_HEAP_COVERAGE_4,
    OE_HEAP_COVERAGE_5,
    OE_HEAP_COVERAGE_6,
    OE_HEAP_COVERAGE_7,
    OE_HEAP_COVERAGE_8,
    OE_HEAP_COVERAGE_9,
    OE_HEAP_COVERAGE_10,
    OE_HEAP_COVERAGE_11,
    OE_HEAP_COVERAGE_12,
    OE_HEAP_COVERAGE_13,
    OE_HEAP_COVERAGE_14,
    OE_HEAP_COVERAGE_15,
    OE_HEAP_COVERAGE_16,
    OE_HEAP_COVERAGE_17,
    OE_HEAP_COVERAGE_18,
    OE_HEAP_COVERAGE_N,
} oe_mman_coverage_t;

/* oe_mman_t data structures and fields */
typedef struct _oe_mman
{
    /* Magic number (OE_HEAP_MAGIC) */
    uint64_t magic;

    /* True if oe_mman_init() has been called */
    bool initialized;

    /* Base of heap (aligned on page boundary) */
    uintptr_t base;

    /* Size of heap (a multiple of OE_PAGE_SIZE) */
    size_t size;

    /* Start of heap (immediately aft4er VADs array) */
    uintptr_t start;

    /* End of heap (points to first page after end of heap) */
    uintptr_t end;

    /* Current break value: top of break memory partition (grows positively) */
    uintptr_t brk;

    /* Current map value: top of mapped memory partition (grows negatively) */
    uintptr_t map;

    /* The next available oe_vad_t in the VADs array */
    oe_vad_t* next_vad;

    /* The end of the VADs array */
    oe_vad_t* end_vad;

    /* The oe_vad_t free list (singly linked) */
    oe_vad_t* free_vads;

    /* Linked list of VADs (sorted by address and doubly linked) */
    oe_vad_t* vad_list;

    /* Whether sanity checks are enabled: see OE_HeapEnableSanityChecks() */
    bool sanity;

    /* Whether to scrub memory when it is unmapped (fill with 0xDD) */
    bool scrub;

    /* Heap locking */
    uint64_t lock[8];

    /* Error string */
    char err[OE_HEAP_ERROR_SIZE];

    /* Code coverage array */
    bool coverage[OE_HEAP_COVERAGE_N];

} oe_mman_t;

oe_result_t oe_mman_init(oe_mman_t* heap, uintptr_t base, size_t size);

void* oe_mman_map(
    oe_mman_t* heap,
    void* addr,
    size_t length,
    int prot,
    int flags);

void* oe_mman_remap(
    oe_mman_t* heap,
    void* addr,
    size_t old_size,
    size_t new_size,
    int flags);

oe_result_t oe_mman_unmap(oe_mman_t* heap, void* address, size_t size);

void oe_mman_dump(const oe_mman_t* h, bool full);

void* oe_mman_sbrk(oe_mman_t* heap, ptrdiff_t increment);

oe_result_t oe_mman_brk(oe_mman_t* heap, uintptr_t addr);

void oe_mman_set_sanity(oe_mman_t* heap, bool sanity);

bool oe_mman_is_sane(oe_mman_t* heap);

#if 0
void* OE_Map(void* addr, size_t length, int prot, int flags);

void* OE_Remap(void* addr, size_t old_size, size_t new_size, int flags);

int OE_Unmap(void* address, size_t size);

void* OE_Sbrk(ptrdiff_t increment);

int OE_Brk(uintptr_t addr);
#endif

OE_EXTERNC_END

#endif /* _OE_INTERNAL_MMAN_H */
