#include "sys/eds_epn.h"
#include "eds_port.h"

extern inline struct eds_object__tasker*
eds_epn__tasker(struct eds_object__epn * epn);

extern inline struct eds_object__etm*
eds_epn__etm_service(struct eds_object__epn *epn);

void
eds_epn__sleep_init(struct eds_object__epn *epn)
{
    eds_port__sleep_init(&epn->p__sleep);
}

void
eds_epn__sleep_wake_up(struct eds_object__epn *epn)
{
    eds_port__sleep_signal(&epn->p__sleep);
}

void
eds_epn__sleep_wait(struct eds_object__epn *epn)
{
    eds_port__sleep_wait(&epn->p__sleep);
}
