/*
 * eds_equeue.h
 *
 *  Created on: Dec 5, 2021
 *      Author: nenad
 */

#ifndef NEON_KIT_GENERIC_SOURCE_EDS_EQUEUE_H_
#define NEON_KIT_GENERIC_SOURCE_EDS_EQUEUE_H_

#include "eds_object.h"
#include "sys/eds_core.h"

#include <stdint.h>

size_t
eds_equeue__calculate_storage_size(size_t n_entries);

void
eds_equeue__init(struct eds_object__equeue *self,
    uint32_t n_entries,
    struct eds_object__evt **storage);

void
eds_equeue__term(struct eds_object__equeue *self);

inline void
eds_equeue__push_back(struct eds_object__equeue *self, const struct eds_object__evt *event)
{
    /* NOTE:
     * Cast away pointer const-ness at this point. Event queue always works with const pointers,
     * but the underlaying generic queue implementation assumes that pointers are non-const. In
     * order to avoid defining a totally new const queue implementation it is more practical to just
     * do the pointer casting in particular cases.
     */
    eds_core__queue_put_fifo(&self->queue, (struct eds_object__evt *) (uintptr_t) event);
}

inline void
eds_equeue__push_front(struct eds_object__equeue *self, const struct eds_object__evt *event)
{
    /*
     * NOTE:
     * The same note applies here as well, as in previous function.
     */
    eds_core__queue_put_lifo(&self->queue, (struct eds_object__evt *) (uintptr_t) event);
}

inline const struct eds_object__evt*
eds_equeue__pop(struct eds_object__equeue *self)
{
    return eds_core__queue_get(&self->queue);
}

inline bool
eds_equeue__is_empty(const struct eds_object__equeue *self)
{
    return eds_core__queue_is_empty(&self->queue);
}

inline bool
eds_equeue__is_full(const struct eds_object__equeue *self)
{
    return eds_core__queue_is_full(&self->queue);
}

#endif /* NEON_KIT_GENERIC_SOURCE_EDS_EQUEUE_H_ */
