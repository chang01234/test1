/*
 * eds_etimer.c
 *
 *  Created on: Dec 9, 2021
 *      Author: nenad
 */

#include "sys/eds_etm.h"
#include "eds.h"
#include "sys/eds_epa.h"
#include "sys/eds_epn.h"
#include "sys/eds_mem.h"
#include "sys/eds_tmr.h"
#include "sys/eds_core.h"
#include "eds_port.h"
#include "eds_trace.h"

static void
etm__every_handler(struct eds_object__tmr_node *tmr)
{
    struct eds_object__etm_node *etm;

    etm = EDS_CORE__CONTAINER_OF(tmr, struct eds_object__etm_node, p__node);
    // TODO: Error handling when EPA queue is full
    if (eds_epa__is_designated(etm->p__epa) == true) {
        eds_epa__send(etm->p__epa, etm->p__evt);
        if (!eds_tmr__node_is_periodic(tmr))
        {
            eds_etm__designate(etm, NULL);
        }
    }
}

extern inline void
eds_etm__designate(struct eds_object__etm_node *etm, struct eds_object__epa *epa);

extern inline bool
eds_etm__is_designated(const struct eds_object__etm_node *etm);

extern inline void
eds_etm__set_event(struct eds_object__etm_node *etm, const struct eds_object__evt *evt);

eds__error
eds_etm__create(const struct eds__etimer_attr *attr, eds__etimer **etm)
{
    struct eds_object__mem *mem;
    struct eds_object__etm_node *l_etm;

    if (attr->static_instance == NULL) {
        struct eds_port__critical critical;

        mem = eds_mem__find(&mem__instances, sizeof(*l_etm));
        if (mem == NULL) {
            EDS_TRACE__EXIT(EDS_TRACE__SOURCE_ETIMER_CREATE, EDS__ERROR_NO_RESOURCE, "%u", sizeof(*l_etm));
            return EDS__ERROR_NO_RESOURCE;
        }
        eds_port__critical_lock(&critical);
        l_etm = eds_mem__allocate_from(mem, sizeof(*l_etm));
        eds_port__critical_unlock(&critical);
        if (l_etm == NULL) {
            EDS_TRACE__EXIT(EDS_TRACE__SOURCE_ETIMER_CREATE, EDS__ERROR_NO_MEMORY, "%u from %p", sizeof(*l_etm), mem);
            return EDS__ERROR_NO_MEMORY;
        }
    } else {
        mem = NULL;
        l_etm = attr->static_instance;
    }
    eds_tmr__node_init(&l_etm->p__node, &etm__every_handler);
    l_etm->p__evt = NULL; /* No associated event with this timer */
    l_etm->p__epa = NULL; /* No associated agent with this timer */
    l_etm->p__mem = mem;
    *etm = l_etm;
    EDS_TRACE__EXIT(EDS_TRACE__SOURCE_ETIMER_CREATE, EDS__ERROR_NONE, "id = (%p)", l_etm);
    return EDS__ERROR_NONE;
}

eds__error
eds_etm__delete(eds__etimer * etimer)
{
    struct eds_port__critical critical;

    eds_port__critical_lock(&critical);
    if (eds_etm__is_designated(etimer) == false) {
        eds_port__critical_unlock(&critical);
        return EDS__ERROR_NOT_EXISTS;
    }
    if (etimer->p__mem != NULL) {
        eds_mem__deallocate_to(etimer->p__mem, etimer);
    }
    eds_port__critical_unlock(&critical);
    return EDS__ERROR_NONE;
}

struct tuple_service_epa
{
    struct eds_object__epa * epa;
    struct eds_object__etm * etm_service;
};

static void match_and_delete(struct eds_object__tmr_node * tmr_node, void * arg)
{
    struct tuple_service_epa * tuple_service_epa = arg;
    struct eds_object__etm_node *etm;

    etm = EDS_CORE__CONTAINER_OF(tmr_node, struct eds_object__etm_node, p__node);
    if (tuple_service_epa->epa == etm->p__epa) {
        eds_tmr__cancel(&tuple_service_epa->etm_service->p__tmr, &etm->p__node);
        if (etm->p__mem != NULL) {
            eds_mem__deallocate_to(etm->p__mem, etm);
        }
    }
}

void
eds_etm_service__delete_all(struct eds_object__etm *etm_service, struct eds_object__epa *epa)
{
    struct tuple_service_epa tuple_service_epa = {
        .epa = epa,
        .etm_service = etm_service
    };
    eds_tmr__for_each_node(&etm_service->p__tmr, match_and_delete, &tuple_service_epa);
}

void
eds_etm_service__init(struct eds_object__etm *etm_service)
{
    eds_tmr__init(&etm_service->p__tmr);
}

void
eds_etm_service__start_once(struct eds_object__etm *etm_service,
    struct eds_object__etm_node *etm,
    uint32_t ms)
{
    eds_tmr__start_once(&etm_service->p__tmr, &etm->p__node, ms);
}

void
eds_etm_service__start_periodic(struct eds_object__etm *etm_service,
    struct eds_object__etm_node *etm,
    uint32_t ms)
{
    eds_tmr__start_periodic(&etm_service->p__tmr, &etm->p__node, ms);
}

void
eds_etm_service__cancel(struct eds_object__etm *etm_service, struct eds_object__etm_node *etm)
{
    eds_tmr__cancel(&etm_service->p__tmr, &etm->p__node);
}

void
eds_etm_service__tick(struct eds_object__etm *etm_service)
{
    eds_tmr__process_timers(&etm_service->p__tmr);
}
