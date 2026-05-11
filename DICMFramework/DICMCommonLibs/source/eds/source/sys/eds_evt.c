#include "eds_evt.h"

#include "sys/eds_mem.h"
#include "eds_port.h"

#include <assert.h>
#include <stddef.h>

size_t
eds_evt__calculate_bundle_size(size_t event_data_size)
{
    return sizeof(struct eds_object__evt) + eds_port__align_up(event_data_size);
}

eds_core__error
eds_evt__allocate(uint32_t event_id,
    size_t event_data_size,
    struct eds_object__evt ** event)
{
    struct eds_object__mem * mem;
    struct eds_object__evt * l_event;
    size_t event_size;

    event_size = eds_evt__calculate_bundle_size(event_data_size);
    mem = eds_mem__find(&mem__instances, event_size);
    if (mem == NULL) {
        return EDS_CORE__ERROR__NO_RESOURCE;
    }
    l_event = eds_mem__allocate_from(mem, event_size);
    if (l_event == NULL) {
        return EDS_CORE__ERROR__NO_MEMORY;
    }
    eds_evt__init(l_event, event_id, event_data_size, mem);
    *event = l_event;
    return EDS_CORE__ERROR_NONE;
}

void
eds_evt__deallocate(const struct eds_object__evt * event)
{
    struct eds_object__evt * d_event;

    d_event = eds_evt__to_dynamic(event);
    if ((d_event != NULL) && (d_event->p__ref_count == 0u)) {
        struct eds_object__mem * mem;

        mem = eds_evt__mem(d_event);
        eds_evt__term(d_event);
        eds_mem__deallocate_to(mem, d_event);
    }
}

void
eds_evt__null(const struct eds_object__evt * event)
{
    struct eds_object__evt * d_event;

    d_event = eds_evt__to_dynamic(event);
    if (d_event != NULL) {
        d_event->p__id = EDS__EVENT__NULL;
        d_event->p__size = 0u;
    }
}

void
eds_evt__init(struct eds_object__evt *event,
    uint32_t event_id,
    size_t event_data_size,
    struct eds_object__mem *mem)
{
    event->p__id = event_id;
    event->p__size = event_data_size;
    event->p__mem = mem;
    event->p__ref_count = 0u;
}

void
eds_evt__term(struct eds_object__evt *d_event)
{
    d_event->p__id = EDS__EVENT__NULL;
    d_event->p__size = 0u;
    d_event->p__mem = NULL;
    d_event->p__ref_count = 0u;
}

extern inline struct eds_object__evt*
eds_evt__to_dynamic(const struct eds_object__evt *event);

extern inline void
eds_evt__ref_up(const struct eds_object__evt *event);

extern inline void
eds_evt__ref_down(const struct eds_object__evt *event);

extern inline bool
eds_evt__is_in_use(const struct eds_object__evt *event);

extern inline bool
eds_evt__is_dynamic(const struct eds_object__evt *event);

extern inline struct eds_object__mem*
eds_evt__mem(const struct eds_object__evt *event);
