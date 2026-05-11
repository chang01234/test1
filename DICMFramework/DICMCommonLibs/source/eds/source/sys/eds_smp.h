/*
 * eds_sm.h
 *
 *  Created on: Dec 5, 2021
 *      Author: nenad
 */

#ifndef NEON_KIT_GENERIC_SOURCE_EDS_SM_H_
#define NEON_KIT_GENERIC_SOURCE_EDS_SM_H_

#include "eds_object.h"
#include "sys/eds_core.h"

#define EDS__SM__ACTION__HANDLED            0
#define EDS__SM__ACTION__IGNORED            1
#define EDS__SM__ACTION__SUPER              2
#define EDS__SM__ACTION__PUSHED_BACK        3
#define EDS__SM__ACTION__TRANSIT            4

extern const struct eds_object__evt g__smp_events[];

void
eds_smp__init(struct eds_object__smp *sm, eds_object__smp_state *initial_state, void *workspace);

eds__error
eds_smp__dispatch(struct eds_object__smp *sm, const struct eds_object__evt *event);

inline eds__sm_action
eds_smp__action_handled(struct eds_object__smp *sm)
{
    (void)sm;

    return EDS__SM__ACTION__HANDLED;
}

inline eds__sm_action
eds_smp__action_ignored(struct eds_object__smp *sm)
{
    (void)sm;

    return EDS__SM__ACTION__IGNORED;
}

inline eds__sm_action
eds_smp__action_super(struct eds_object__smp *sm, eds_object__smp_state * super_state)
{
    sm->p__state = super_state;

    return EDS__SM__ACTION__SUPER;
}

inline eds__sm_action
eds_smp__action_transit(struct eds_object__smp *sm, eds_object__smp_state * next_state)
{
    sm->p__state = next_state;
    return EDS__SM__ACTION__TRANSIT;
}

inline void*
eds_smp__workspace(const struct eds_object__smp *sm)
{
    return sm->p__workspace;
}

#endif /* NEON_KIT_GENERIC_SOURCE_EDS_SM_H_ */
