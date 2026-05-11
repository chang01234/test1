
#include "sys/eds_smp.h"
#include "sys/eds_evt.h"
#include "sys/eds_core.h"
#include "eds_port.h"
#include "eds_trace.h"

#define SM__ACTION__HANDLED_OR_IGNORED(action)                                                  \
    ((action) < EDS__SM__ACTION__SUPER)

#define SM__ACTION__HANDLED_IGNORED_OR_SUPER(action)                                            \
    ((action) <= EDS__SM__ACTION__SUPER)

#if (EDS_CONFIG__SMP__ENABLE_HSM != 0)
struct hsm_path
{
    eds_object__smp_state *buff[EDS_CONFIG__SMP__HSM_LEVELS];
    uint_fast8_t index;
};
#endif

const struct eds_object__evt g__smp_events[] =
{
    [EDS__SM_EVENT__INIT] = EDS_EVT__INITIALIZER(EDS__SM_EVENT__INIT, 0),
    [EDS__SM_EVENT__ENTER] = EDS_EVT__INITIALIZER(EDS__SM_EVENT__ENTER, 0),
    [EDS__SM_EVENT__EXIT] = EDS_EVT__INITIALIZER(EDS__SM_EVENT__EXIT, 0),
    [EDS__SM_EVENT__SUPER] = EDS_EVT__INITIALIZER(EDS__SM_EVENT__SUPER, 0),
};

#if (EDS_CONFIG__SMP__ENABLE_HSM != 0)
static eds__error
hsm_get_state_super(struct eds_object__smp * sm, eds__sm_state * state)
{
    eds__sm_action action;

    action = state(sm, sm->p__workspace, &g__smp_events[EDS__SM_EVENT__SUPER]);
    if (action != EDS__SM__ACTION__SUPER) {
        return EDS__ERROR_MALFORMED_SM;
    } else {
        return EDS__ERROR_NONE;
    }
}
#endif /* (EDS_CONFIG__SMP__ENABLE_HSM != 0) */

#if (EDS_CONFIG__SMP__ENABLE_HSM != 0)
static eds__error
hsm_path_enter(struct eds_object__smp * sm, const struct hsm_path * entry)
{
    uint_fast8_t index = entry->index;

    while (index--) {
        eds__sm_action action;

        EDS_TRACE__INFO(EDS_TRACE__SOURCE_EDS_SMP, "(%p) enter: state = (%p)", sm, entry->buff[index]);
        action = entry->buff[index](sm, sm->p__workspace, &g__smp_events[EDS__SM_EVENT__ENTER]);
        if (!SM__ACTION__HANDLED_IGNORED_OR_SUPER(action)) {
            EDS_TRACE__EXIT(
                EDS_TRACE__SOURCE_EDS_SMP,
                EDS__ERROR_MALFORMED_SM,
                "(%p) enter: state (%p) bad action %u",
                sm,
                entry->buff[index],
                action);
            return EDS__ERROR_SM_BAD_ENTER;
        }
    }
    return EDS__ERROR_NONE;
}
#endif /* (EDS_CONFIG__SMP__ENABLE_HSM != 0) */

#if (EDS_CONFIG__SMP__ENABLE_HSM != 0)
static eds__error
hsm_path_exit(struct eds_object__smp * sm, const struct hsm_path * exit)
{
    for (uint_fast8_t count = 0u; count < exit->index; count++) {
        eds__sm_action action;

        EDS_TRACE__INFO(EDS_TRACE__SOURCE_EDS_SMP, "(%p) exit: state = (%p)", sm, exit->buff[count]);
        action = exit->buff[count](sm, sm->p__workspace, &g__smp_events[EDS__SM_EVENT__EXIT]);
        if (!SM__ACTION__HANDLED_IGNORED_OR_SUPER(action)) {
            EDS_TRACE__EXIT(
                EDS_TRACE__SOURCE_EDS_SMP,
                EDS__ERROR_MALFORMED_SM,
                "(%p) exit: state (%p) bad action %u",
                sm,
                exit->buff[count],
                action);
            return EDS__ERROR_SM_BAD_EXIT;
        }
    }
    return EDS__ERROR_NONE;
}

static eds__error
hsm_path_build(struct eds_object__smp * sm, struct hsm_path * entry, struct hsm_path * exit)
{
    eds__error error;

    entry->index = 0;
    exit->index--;

/*--  path: a) source ?== destination  ---------------------------------------*/
    if (exit->buff[exit->index] == entry->buff[entry->index]) {
        entry->index++;
        exit->index++;
        return EDS__ERROR_NONE;
    }
/*--  path: b) source ?== super(destination)  --------------------------------*/
    error = hsm_get_state_super(sm, entry->buff[entry->index++]);
    if (error) {
        return EDS__ERROR_MALFORMED_SM;
    }
    entry->buff[entry->index] = sm->p__state;

    if (exit->buff[exit->index] == entry->buff[entry->index]) {
        return EDS__ERROR_NONE;
    }
/*--  path: c) super(source) ?== super(destination)  -------------------------*/
    error = hsm_get_state_super(sm, exit->buff[exit->index++]);
    if (error) {
        return EDS__ERROR_MALFORMED_SM;
    }
    exit->buff[exit->index] = sm->p__state;

    if (exit->buff[exit->index] == entry->buff[entry->index]) {
        return EDS__ERROR_NONE;
    }
/*--  path: d) super(source) ?== destination  --------------------------------*/
    entry->index--;

    if (exit->buff[exit->index] == entry->buff[entry->index]) {
        return EDS__ERROR_NONE;
    }
/*--  path: e) source ?== ...super(super(destination))  ----------------------*/
    exit->index--;
    entry->index++;

    while (entry->buff[entry->index] != &eds__sm_top_state) {
        error = hsm_get_state_super(sm, entry->buff[entry->index++]);
        if (error) {
            return EDS__ERROR_MALFORMED_SM;
        }
        entry->buff[entry->index] = sm->p__state;
        if (exit->buff[exit->index] == entry->buff[entry->index]) {
            return EDS__ERROR_NONE;
        }
    }
/*--  path: f) super(source) ?== ...super(super(destination))  ---------------*/
    exit->index++;

    while (entry->index != 1u) {
        if (exit->buff[exit->index] == entry->buff[entry->index]) {
            return EDS__ERROR_NONE;
        }
        entry->index--;
    }
/*--  path: g) ...super(super(source)) ?== ...super(super(destination))  -----*/

    for (;;) {
        error = hsm_get_state_super(sm, exit->buff[exit->index++]);
        if (error) {
            return EDS__ERROR_MALFORMED_SM;
        }
        exit->buff[exit->index] = sm->p__state;
        entry->index = 0u;

        do {
            entry->index++;
            if (exit->buff[exit->index] == entry->buff[entry->index]) {
                return EDS__ERROR_NONE;
            }
        } while (entry->buff[entry->index] != &eds__sm_top_state);
    }
    return EDS__ERROR_NONE;
}
#endif /* (EDS_CONFIG__SMP__ENABLE_HSM != 0) */

void
eds_smp__init(struct eds_object__smp * sm, eds_object__smp_state * initial_state, void * workspace)
{
    sm->p__state = initial_state;
    sm->p__workspace = workspace;
}

eds__error
eds_smp__dispatch(struct eds_object__smp *sm, const struct eds_object__evt *event)
{
#if (EDS_CONFIG__SMP__ENABLE_HSM == 0)
    eds__sm_action action;
    eds__sm_state * current_state;

    current_state = sm->p__state;

    action = current_state(sm, sm->p__workspace, event);

    while (action == EDS__SM__ACTION__TRANSIT) {
        action = current_state(sm, sm->p__workspace, &g__smp_events[EDS__SM_EVENT__EXIT]);
        if (!SM__ACTION__HANDLED_OR_IGNORED(action)) {
            return EDS_CORE__ERROR__BAD_STATE;
        }
        current_state = sm->p__state;
        action = current_state(sm, sm->p__workspace, &g__smp_events[EDS__SM_EVENT__ENTER]);
        if (!SM__ACTION__HANDLED_OR_IGNORED(action)) {
            return EDS_CORE__ERROR__BAD_STATE;
        }
        action = current_state(sm, sm->p__workspace, &g__smp_events[EDS__SM_EVENT__INIT]);
    }
    if (!SM__ACTION__HANDLED_OR_IGNORED(action)) {
        return EDS_CORE__ERROR__BAD_STATE;
    }
    sm->p__state = current_state;
    return EDS_CORE__ERROR_NONE;
#else
    eds__sm_action action;
    eds__sm_state * current_state;
    struct hsm_path entry;
    struct hsm_path exit;

    exit.index = 0u;
    current_state = sm->p__state;

    EDS_TRACE__INFO(EDS_TRACE__SOURCE_EDS_SMP, "proc: sm, workspace, event_id = (%p, %p, %u)", sm, sm->p__workspace, event->p__id);
    do {
        EDS_TRACE__INFO(EDS_TRACE__SOURCE_EDS_SMP, "(%p) handle: state = (%p)", sm, sm->p__state);
        exit.buff[exit.index++] = sm->p__state;
        action = sm->p__state(sm, sm->p__workspace, event);
    } while (action == EDS__SM__ACTION__SUPER);

    while (action == EDS__SM__ACTION__TRANSIT) {
        entry.buff[0] = sm->p__state;
        EDS_TRACE__INFO(EDS_TRACE__SOURCE_EDS_SMP, "(%p) transit: %p to %p", sm, exit.buff[0], entry.buff[0]);
        hsm_path_build(sm, &entry, &exit);
        hsm_path_exit(sm, &exit);
        hsm_path_enter(sm, &entry);
        EDS_TRACE__INFO(EDS_TRACE__SOURCE_EDS_SMP, "(%p) init: state = (%p)", sm, entry.buff[0]);
        action = entry.buff[0](sm, sm->p__workspace, &g__smp_events[EDS__SM_EVENT__INIT]);
        exit.buff[0] = entry.buff[0];
        exit.index = 1u;
        current_state = entry.buff[0];
    }
    sm->p__state = current_state;

    return EDS__ERROR_NONE;
#endif /* !(EDS_CONFIG__SMP__ENABLE_HSM == 0) */
}

extern inline eds__sm_action
eds_smp__action_handled(struct eds_object__smp *sm);

extern inline eds__sm_action
eds_smp__action_ignored(struct eds_object__smp *sm);

extern inline eds__sm_action
eds_smp__action_super(struct eds_object__smp *sm, eds_object__smp_state * super_state);

extern inline eds__sm_action
eds_smp__action_transit(struct eds_object__smp *sm, eds_object__smp_state * next_state);

extern inline void*
eds_smp__workspace(const struct eds_object__smp *sm);