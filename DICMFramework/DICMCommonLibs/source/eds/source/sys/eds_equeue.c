/*
 * eds_equeue.c
 *
 *  Created on: Dec 5, 2021
 *      Author: nenad
 */

#include "sys/eds_equeue.h"
#include "eds_port.h"

size_t
eds_equeue__calculate_storage_size(size_t n_entries)
{
    return eds_port__align_up(sizeof(struct eds_object__evt*)) * n_entries;
}

void
eds_equeue__init(struct eds_object__equeue *self,
    uint32_t n_entries,
    struct eds_object__evt **storage)
{
    eds_core__queue_init(&self->queue, n_entries, (void**)storage);
}

void
eds_equeue__term(struct eds_object__equeue *self)
{
    eds_core__queue_term(&self->queue);
}

extern inline void
eds_equeue__push_back(struct eds_object__equeue *self, const struct eds_object__evt *event);

extern inline void
eds_equeue__push_front(struct eds_object__equeue *self, const struct eds_object__evt *event);

extern inline const struct eds_object__evt*
eds_equeue__pop(struct eds_object__equeue *self);

extern inline bool
eds_equeue__is_empty(const struct eds_object__equeue *self);

extern inline bool
eds_equeue__is_full(const struct eds_object__equeue *self);
