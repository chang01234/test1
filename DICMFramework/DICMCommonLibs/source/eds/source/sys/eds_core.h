/*
 * eds_core.h
 *
 *  Created on: Dec 2, 2021
 *      Author: nenad
 */

#ifndef NEON_KIT_GENERIC_SOURCE_NK_EDS_CORE_H_
#define NEON_KIT_GENERIC_SOURCE_NK_EDS_CORE_H_

#include <stdbool.h>

#include "eds_object.h"

/** @brief      Determines the first dimension of an array.
 *  @param      array An array : type unspecified
 *  @mseffect
 */
#define EDS_CORE__ARRAY_SIZE(array)         (sizeof(array) / sizeof(array[0]))

/**
 * @brief       Cast a member of a structure out to the containing structure.
 * @param       ptr the pointer to the member.
 * @param       type the type of the container struct this is embedded in.
 * @param       member the name of the member within the struct.
 */
#define EDS_CORE__CONTAINER_OF(ptr, type, member)                                               \
    ((type *)((char *)(uintptr_t)(ptr) - offsetof(type, member)))

#define EDS_CORE__ERROR_NONE               0
#define EDS_CORE__ERROR__NO_RESOURCE        0x1
#define EDS_CORE__ERROR__NO_MEMORY          0x2
#define EDS_CORE__ERROR__NO_PERMISSION      0x3
#define EDS_CORE__ERROR__IN_USE             0x4
#define EDS_CORE__ERROR__BAD_STATE          0x5
#define EDS_CORE__ERROR__NO_SPACE           0x6

typedef uint_fast8_t eds_core__error;

#define EDS_CORE__LIST_INITIALIZER(list)                                                        \
    {                                                                                           \
        .p__next = (list),                                                                      \
        .p__prev = (list)                                                                       \
    }

/**
 * @brief      Construct for @a FOR loop to iterate over each element in a list.
 *
 *  @code
 *  struct eds_object__list * current;
 *  struct eds_object__list * sentinel = &g_list_sentinel.list;
 *
 *  for (EDS_CORE__LIST_EACH(current, sentinel)) {
 *      ... do something with @a current (excluding remove)
 *  }
 *  @endcode
 */
#define EDS_CORE__LIST_EACH(current, sentinel)                                                  \
    current = eds_core__list_next(sentinel);                                                    \
    current != (sentinel);                                                                      \
    current = eds_core__list_next(current)

/** @brief      Construct for FOR loop to iterate over each element in list
 *              backwards.
 *
 *  @code
 *  struct eds_object__list * current;
 *  struct eds_object__list * sentinel = &g_list_sentinel.list;
 *
 *  for (EDS_CORE__LIST_BACKWARDS(current, sentinel)) {
 *      ... do something with current (excluding remove)
 *  }
 *  @endcode
 */
#define EDS_CORE__LIST_BACKWARDS(current, sentinel)                                             \
    current = eds_core__list_prev(sentinel);                                                    \
    current != (sentinel);                                                                      \
    current = eds_core__list_prev(current)

/** @brief      Construct for FOR loop to iterate over each element in list
 *              which is safe against element removal.
 *
 *  @code
 *  struct eds_object__list * current;
 *  struct eds_object__list * iterator;
 *  struct eds_object__list * sentinel = &g_list_sentinel.list;
 *
 *  for (nlist_dll_EACH_SAFE(current, iterator, sentinel)) {
 *      ... do something with current (including remove)
 *  }
 *  @endcode
 */
#define EDS_CORE__LIST_EACH_SAFE(current, iterator, sentinel)                                   \
    current = eds_core__list_next(sentinel), iterator = eds_core__list_next(current);           \
    current != (sentinel);                                                                      \
    current = iterator, iterator = eds_core__list_next(iterator)

inline void
eds_core__list_init(struct eds_object__list *self)
{
    self->p__next = self;
    self->p__prev = self;
}

inline struct eds_object__list*
eds_core__list_next(const struct eds_object__list *self)
{
    return self->p__next;
}

inline struct eds_object__list*
eds_core__list_prev(struct eds_object__list *self)
{
    return self->p__prev;
}

inline void
eds_core__list_add_after(struct eds_object__list *self, struct eds_object__list *after)
{
    self->p__next = after->p__next;
    self->p__prev = after;
    after->p__next->p__prev = self;
    after->p__next = self;
}

inline void
eds_core__list_add_before(struct eds_object__list *self, struct eds_object__list *before)
{
    self->p__next = before;
    self->p__prev = before->p__prev;
    before->p__prev->p__next = self;
    before->p__prev = self;
}

inline void
eds_core__list_remove(struct eds_object__list *self)
{
    self->p__next->p__prev = self->p__prev;
    self->p__prev->p__next = self->p__next;
    /* The following `self` initialization code is necessary:
     * - since we want to support calling this function multiple times over the same list node that
     *   was already removed from a list.
     * - since we want to support querying if a list is empty or not.
     */
    self->p__next = self;
    self->p__prev = self;
}

inline bool
eds_core__list_is_empty(const struct eds_object__list *self)
{
    return !!(self->p__next == self);
}

inline bool
eds_core__list_is_singular(const struct eds_object__list *self)
{
    return !!((self->p__next != self) && (self->p__next == self->p__prev));
}

#define EDS_CORE__VECTOR__INITIALIZER(entries)                                                  \
    {                                                                                           \
        .p__entries = (entries),                                                                \
        .p__item_size = sizeof((entries)[0]),                                                   \
        .p__n_entries = 0u,                                                                      \
        .p__n_size = EDS_CORE__ARRAY_SIZE(entries),                                             \
    }

void
eds_core__vector_init(struct eds_object__vector *self,
    void *entries,
    size_t item_size,
    uint32_t n_size);

inline uint32_t
eds_core__vector_n_entries(const struct eds_object__vector *self)
{
    return self->p__n_entries;
}

inline uint32_t
eds_core__vector_n_size(const struct eds_object__vector *self)
{
    return self->p__n_size;
}

inline bool
eds_core__vector_is_full(const struct eds_object__vector *self)
{
    return (self->p__n_entries == self->p__n_size);
}

inline void*
eds_core__vector_peek(const struct eds_object__vector *self, uint32_t index)
{
    return (char*) self->p__entries + (self->p__item_size * index);
}

void
eds_core__vector_insert(struct eds_object__vector *self, uint32_t index, const void *entry);

void
eds_core__vector_remove(struct eds_object__vector *self, uint32_t index);

void
eds_core__queue_init(struct eds_object__queue *self, uint32_t n_entries, void **storage);

void
eds_core__queue_term(struct eds_object__queue *self);

void
eds_core__queue_put_fifo(struct eds_object__queue *self, void *item);

void
eds_core__queue_put_lifo(struct eds_object__queue *self, void *item);

void*
eds_core__queue_get(struct eds_object__queue *self);

void*
eds_core__queue_head(const struct eds_object__queue *self);

void*
eds_core__queue_tail(const struct eds_object__queue *self);

inline uint32_t
eds_core__queue_n_entries(const struct eds_object__queue *self)
{
    return self->p__n_entries;
}

inline uint32_t
eds_core__queue_n_free(const struct eds_object__queue *self)
{
    return self->p__n_free;
}

inline bool
eds_core__queue_is_empty(const struct eds_object__queue *self)
{
    return self->p__n_free == self->p__n_entries;
}

inline bool
eds_core__queue_is_full(const struct eds_object__queue *self)
{
    return self->p__n_free == 0u;
}

void
eds_core__tasker_node_init(struct eds_object__tasker_node *self, uint_fast8_t prio);

void
eds_core__tasker_run(struct eds_object__tasker *self, struct eds_object__tasker_node *task);

void
eds_core__tasker_pending_sleep(struct eds_object__tasker *self,
    struct eds_object__tasker_node *task);

void
eds_core__tasker_init(struct eds_object__tasker *self);

void
eds_core__tasker_term(struct eds_object__tasker *self);

struct eds_object__tasker_node*
eds_core__tasker_highest(const struct eds_object__tasker *self);

bool
eds_core__tasker_is_running(const struct eds_object__tasker *self);

#endif /* NEON_KIT_GENERIC_SOURCE_NK_EDS_CORE_H_ */
