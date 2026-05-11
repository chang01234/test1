#ifndef NEON_KIT_EDS_SOURCE_EDS_NETWORK_H_
#define NEON_KIT_EDS_SOURCE_EDS_NETWORK_H_

#include "eds_object.h"

inline struct eds_object__tasker*
eds_epn__tasker(struct eds_object__epn *epn)
{
    return &epn->p__tasker;
}

inline struct eds_object__etm*
eds_epn__etm_service(struct eds_object__epn *epn)
{
    return &epn->p__etm;
}

void
eds_epn__sleep_init(struct eds_object__epn *epn);

void
eds_epn__sleep_wake_up(struct eds_object__epn *epn);

void
eds_epn__sleep_wait(struct eds_object__epn *epn);

#endif /* NEON_KIT_EDS_SOURCE_EDS_NETWORK_H_ */
