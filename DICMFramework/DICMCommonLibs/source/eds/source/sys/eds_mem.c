/*
 * eds_mem.c
 *
 *  Created on: Dec 5, 2021
 *      Author: nenad
 */

#include "sys/eds_mem.h"
#include "eds_object.h"
#include "sys/eds_core.h"
#include "eds.h"

static struct eds_object__mem mem__instances_storage[EDS__DEFAULT_MEM_ENTRIES];
struct eds_object__vector mem__instances = EDS_CORE__VECTOR__INITIALIZER(mem__instances_storage);

void
eds_mem__init(struct eds_object__mem * mem,
    eds__mem_alloc_fn * alloc,
    eds__mem_dealloc_fn * dealloc,
    void * context,
    size_t max_size)
{
    mem->p__alloc = alloc;
    mem->p__dealloc = dealloc;
    mem->p__context = context;
    mem->p__max_size = max_size;
}

extern inline void *
eds_mem__context(const struct eds_object__mem * mem);

extern inline size_t
eds_mem__max_size(const struct eds_object__mem * mem);

extern inline eds__mem_alloc_fn *
eds_mem__alloc_fn(const struct eds_object__mem * mem);

extern inline eds__mem_dealloc_fn *
eds_mem__dealloc_fn(const struct eds_object__mem * mem);

struct eds_object__mem *
eds_mem__find(const struct eds_object__vector * vector, size_t size)
{
    uint32_t entries = eds_core__vector_n_entries(vector);
    for (uint32_t i = 0u; i < entries; i++) {
        struct eds_object__mem *mem;

        mem = eds_core__vector_peek(vector, i);
        if (mem->p__max_size >= size) {
            return mem;
        }
    }
    return NULL;
}

void
eds_mem__add(struct eds_object__vector * vector, const struct eds_object__mem * mem)
{
    uint32_t index;

    index = 0u;
    for (; index < eds_core__vector_n_entries(vector); index++) {
        struct eds_object__mem *current;

        current = eds_core__vector_peek(vector, index);
        if (current->p__max_size > mem->p__max_size) {
            break;
        }
    }
    eds_core__vector_insert(vector, index, mem);
}

uint32_t
eds_mem__instance_count(const struct eds_object__vector * vector)
{
    return eds_core__vector_n_entries(vector);
}

extern inline void *
eds_mem__allocate_from(struct eds_object__mem * mem, size_t size);

extern inline void
eds_mem__deallocate_to(struct eds_object__mem * mem, void * memory);
