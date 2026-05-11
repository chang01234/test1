/*
 * eds_event.h
 *
 *  Created on: Dec 5, 2021
 *      Author: nenad
 */

#ifndef NEON_KIT_GENERIC_SOURCE_EDS_EVENT_H_
#define NEON_KIT_GENERIC_SOURCE_EDS_EVENT_H_

#include "eds_object.h"
#include "sys/eds_core.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define EDS_EVT__INITIALIZER(event_id, event_data_size)                                         \
        {                                                                                       \
            .p__id = (event_id),                                                                \
            .p__size = (event_data_size),                                                       \
        }

size_t
eds_evt__calculate_bundle_size(size_t event_data_size);

eds_core__error
eds_evt__allocate(uint32_t event_id, size_t event_data_size, struct eds_object__evt **event);

void
eds_evt__deallocate(const struct eds_object__evt *event);

void
eds_evt__null(const struct eds_object__evt * event);

void
eds_evt__init(struct eds_object__evt *event,
    uint32_t event_id,
    size_t event_data_size,
    struct eds_object__mem *mem);

void
eds_evt__term(struct eds_object__evt *d_event);

inline struct eds_object__evt*
eds_evt__to_dynamic(const struct eds_object__evt *event)
{
    if (event->p__mem != NULL) {
        /*
         * At this point we know that event is dynamically allocated, therefore, we are safe to
         * discard pointer const-ness.
         */
        return (struct eds_object__evt*) (uintptr_t) event;
    } else {
        return NULL;
    }
}

inline void
eds_evt__ref_up(const struct eds_object__evt *event)
{
    struct eds_object__evt *d_event;

    d_event = eds_evt__to_dynamic(event);
    if ((d_event != NULL) && (d_event->p__ref_count < UINT32_MAX)) {
        d_event->p__ref_count++;
    }
}

inline void
eds_evt__ref_down(const struct eds_object__evt *event)
{
    struct eds_object__evt *d_event;

    d_event = eds_evt__to_dynamic(event);
    if ((d_event != NULL) && (d_event->p__ref_count > 0u)){
        d_event->p__ref_count--;
    }
}

inline bool
eds_evt__is_in_use(const struct eds_object__evt *event)
{
    return (event->p__ref_count != 0u);
}

inline bool
eds_evt__is_dynamic(const struct eds_object__evt *event)
{
    return (event->p__mem != NULL);
}

inline struct eds_object__mem*
eds_evt__mem(const struct eds_object__evt *event)
{
    return event->p__mem;
}

#endif /* NEON_KIT_GENERIC_SOURCE_EDS_EVENT_H_ */
