#include "sys/eds_tmr.h"
#include "sys/eds_core.h"

static void
tmr_sentinel_insert(struct eds_object__tmr *tmr, struct eds_object__tmr_node *node)
{
    struct eds_object__list *current;
    struct eds_object__tmr_node *current_tmr;

    for (EDS_CORE__LIST_EACH(current, &tmr->p__active)) {


        current_tmr = EDS_CORE__CONTAINER_OF(current, struct eds_object__tmr_node, p__list);
        if (current_tmr->p__n_rtick > node->p__n_rtick) {
            current_tmr->p__n_rtick -= node->p__n_rtick;
            break;
        }
        node->p__n_rtick -= current_tmr->p__n_rtick;
    }
    eds_core__list_add_before(&node->p__list, current);
}

static void
tmr_sentinel_pend(struct eds_object__tmr *tmr, struct eds_object__tmr_node *node)
{
    eds_core__list_add_after(&node->p__list, &tmr->p__pending);
}

void
eds_tmr__node_init(struct eds_object__tmr_node *tmr, void
(*fn)(struct eds_object__tmr_node*))
{
    eds_core__list_init(&tmr->p__list);
    tmr->p__n_rtick = 0u;
    tmr->p__n_itick = 0u;
    tmr->p__fn = fn;
}

bool
eds_tmr__node_is_running(const struct eds_object__tmr_node *tmr)
{
    return eds_core__list_is_empty(&tmr->p__list) == false;
}

bool
eds_tmr__node_is_periodic(const struct eds_object__tmr_node *tmr)
{
    return tmr->p__n_itick != 0u;
}

void
eds_tmr__start_once(struct eds_object__tmr *self, struct eds_object__tmr_node *node, uint32_t ticks)
{
    ticks++;
    node->p__n_rtick = ticks;
    node->p__n_itick = 0u;
    node->p__state = EDS_OBJECT__TMR_STATE__PENDING;
    tmr_sentinel_pend(self, node);
}

void
eds_tmr__start_periodic(struct eds_object__tmr *self,
    struct eds_object__tmr_node *node,
    uint32_t ticks)
{
    ticks++;
    node->p__n_rtick = ticks;
    node->p__n_itick = ticks;
    node->p__state = EDS_OBJECT__TMR_STATE__PENDING;
    tmr_sentinel_pend(self, node);
}

void
eds_tmr__cancel(struct eds_object__tmr *self, struct eds_object__tmr_node *node)
{
    if (node->p__state == EDS_OBJECT__TMR_STATE__ACTIVE) {
        /* Prevent from modifying the sentinel structure */
        if (eds_core__list_next(&node->p__list) != &self->p__active) {
            struct eds_object__tmr_node *next_tmr;

            next_tmr = EDS_CORE__CONTAINER_OF(eds_core__list_next(&node->p__list),
                struct eds_object__tmr_node,
                p__list);
            next_tmr->p__n_rtick += node->p__n_rtick;
        }
    }
    eds_core__list_remove(&node->p__list);
}

void
eds_tmr__init(struct eds_object__tmr *tmr)
{
    eds_core__list_init(&tmr->p__active);
    eds_core__list_init(&tmr->p__pending);
}

bool
eds_tmr__are_timers_pending(const struct eds_object__tmr *sentinel)
{
    if ((eds_core__list_is_empty(&sentinel->p__active) == false)
        || (eds_core__list_is_empty(&sentinel->p__pending) == false)) {
        return true;
    } else {
        return false;
    }
}

void
eds_tmr__process_timers(struct eds_object__tmr *tmr)
{
    struct eds_object__list elapsed_timers = EDS_CORE__LIST_INITIALIZER(&elapsed_timers)
    ;
    struct eds_object__list *iterator;
    struct eds_object__list *current;

    /* See if we have any pending timers. Move/insert them to sorted active timer list. */
    for (EDS_CORE__LIST_EACH_SAFE(current, iterator, &tmr->p__pending)) {
        struct eds_object__tmr_node *current_node;

        eds_core__list_remove(current);
        current_node = EDS_CORE__CONTAINER_OF(current, struct eds_object__tmr_node, p__list);
        current_node->p__state = EDS_OBJECT__TMR_STATE__ACTIVE;
        tmr_sentinel_insert(tmr, current_node);
    }
    /* See if we have any active timers */
    if (eds_core__list_is_empty(&tmr->p__active) == false) {
        struct eds_object__tmr_node *first_node;
        struct eds_object__list *first;

        first = eds_core__list_next(&tmr->p__active);
        first_node = EDS_CORE__CONTAINER_OF(first, struct eds_object__tmr_node, p__list);
        /* Decrement the first timer relative tick */
        first_node->p__n_rtick--;
        /* Move all zero relative timers to local elapsed_timers list */
        for (EDS_CORE__LIST_EACH_SAFE(current, iterator, &tmr->p__active)) {
            struct eds_object__tmr_node *current_node;

            current_node = EDS_CORE__CONTAINER_OF(current, struct eds_object__tmr_node, p__list);
            if (current_node->p__n_rtick != 0u) {
                break;
            }
            eds_core__list_remove(current);
            eds_core__list_add_after(current, &elapsed_timers);
        }
        /* Execute all elapsed timers callbacks */
        for (EDS_CORE__LIST_EACH_SAFE(current, iterator, &elapsed_timers)) {
            struct eds_object__tmr_node *current_node;

            current_node = EDS_CORE__CONTAINER_OF(current, struct eds_object__tmr_node, p__list);
            if (current_node->p__n_itick != 0u) {
                current_node->p__n_rtick = current_node->p__n_itick;
                eds_core__list_remove(current);
                tmr_sentinel_insert(tmr, current_node);
            } else {
                current_node->p__state = EDS_OBJECT__TMR_STATE__DORMENT;
            }
            current_node->p__fn(current_node);
        }
    }
}

void
eds_tmr__for_each_node(struct eds_object__tmr *self, void (*map)(struct eds_object__tmr_node*, void *), void * arg)
{
    struct eds_object__list *iterator;
    struct eds_object__list *current;

    /* Map the active timers */
    for (EDS_CORE__LIST_EACH_SAFE(current, iterator, &self->p__active)) {
        struct eds_object__tmr_node *current_node;

        current_node = EDS_CORE__CONTAINER_OF(current, struct eds_object__tmr_node, p__list);
        map(current_node, arg);
    }
    /* Map the pending timers */
    for (EDS_CORE__LIST_EACH_SAFE(current, iterator, &self->p__pending)) {
        struct eds_object__tmr_node *current_node;

        current_node = EDS_CORE__CONTAINER_OF(current, struct eds_object__tmr_node, p__list);
        map(current_node, arg);
    }
}
