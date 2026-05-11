#ifndef NEON_KIT_EDS_SOURCE_EDS_TMR_H_
#define NEON_KIT_EDS_SOURCE_EDS_TMR_H_

#include "eds_object.h"
#include <stdbool.h>

void
eds_tmr__node_init(struct eds_object__tmr_node *tmr, void
(*fn)(struct eds_object__tmr_node*));

bool
eds_tmr__node_is_running(const struct eds_object__tmr_node *tmr);

bool
eds_tmr__node_is_periodic(const struct eds_object__tmr_node *tmr);

void
eds_tmr__start_once(struct eds_object__tmr *self, struct eds_object__tmr_node *node, uint32_t tick);

void
eds_tmr__start_periodic(struct eds_object__tmr *self,
    struct eds_object__tmr_node *node,
    uint32_t ticks);

void
eds_tmr__cancel(struct eds_object__tmr *self, struct eds_object__tmr_node *node);

void
eds_tmr__init(struct eds_object__tmr *self);

bool
eds_tmr__are_timers_pending(const struct eds_object__tmr *self);

void
eds_tmr__process_timers(struct eds_object__tmr *self);

void
eds_tmr__for_each_node(struct eds_object__tmr *self, void (*map)(struct eds_object__tmr_node*, void *), void * arg);

#endif /* NEON_KIT_EDS_SOURCE_EDS_TMR_H_ */
