/*
 * eds_epa.h
 *
 *  Created on: Dec 5, 2021
 *      Author: nenad
 */

#ifndef NEON_KIT_GENERIC_SOURCE_EDS_EPA_H_
#define NEON_KIT_GENERIC_SOURCE_EDS_EPA_H_

#include <stdbool.h>

#include "eds_object.h"
#include "sys/eds_core.h"

struct eds_port__critical;

inline void
eds_epa__designate(struct eds_object__epa *epa, struct eds_object__epn *epn)
{
    epa->p__epn = epn;
}

inline struct eds_object__epn *
eds_epa__designation(const struct eds_object__epa *epa)
{
    return epa->p__epn;
}

inline bool
eds_epa__is_designated(const struct eds_object__epa *epa)
{
    return epa->p__epn != NULL;
}

inline const struct eds_object__smp*
eds_epa__smp(const struct eds_object__epa *epa)
{
    return &epa->p__smp;
}

eds__error
eds_epa__create(eds__sm_state *sm_initial_state,
    void *sm_workspace,
    const struct eds__agent_attr *attr,
    struct eds_object__epa **epa);

eds__error
eds_epa__send(struct eds_object__epa *epa, const struct eds_object__evt *evt);

eds__error
eds_epa__dispatch(struct eds_object__epa *agent, struct eds_port__critical *critical);

void
eds_epa__terminate(struct eds_object__epa *epa);

#endif /* NEON_KIT_GENERIC_SOURCE_EDS_EPA_H_ */
