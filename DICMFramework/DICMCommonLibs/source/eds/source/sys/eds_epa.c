#include "sys/eds_epa.h"
#include "eds.h"
#include "eds_port.h"
#include "eds_trace.h"
#include "sys/eds_smp.h"
#include "sys/eds_mem.h"
#include "sys/eds_evt.h"
#include "sys/eds_epn.h"
#include "sys/eds_core.h"
#include "sys/eds_equeue.h"

extern inline void
eds_epa__designate(struct eds_object__epa *epa, struct eds_object__epn *epn);

extern inline struct eds_object__epn*
eds_epa__designation(const struct eds_object__epa *epa);

extern inline bool
eds_epa__is_designated(const struct eds_object__epa *epa);

extern inline const struct eds_object__smp*
eds_epa__smp(const struct eds_object__epa *epa);

eds__error
eds_epa__create(eds__sm_state *sm_initial_state,
    void *sm_workspace,
    const struct eds__agent_attr *attr,
    struct eds_object__epa **epa)
{
    struct eds_object__epa *l_epa;
    struct eds_object__evt **equeue_storage;
    struct eds_object__mem *mem;

    if (attr->static_instance == NULL) {
        struct eds_port__critical critical;
        size_t epa_size_bytes;

        epa_size_bytes = sizeof(struct eds_object__epa)
            + eds_equeue__calculate_storage_size(attr->equeue_entries);
        mem = eds_mem__find(&mem__instances, epa_size_bytes);
        if (mem == NULL) {
            EDS_TRACE__EXIT(EDS_TRACE__SOURCE_AGENT_CREATE, EDS__ERROR_NO_RESOURCE, "%u", epa_size_bytes);
            return EDS__ERROR_NO_RESOURCE;
        }
        eds_port__critical_lock(&critical);
        l_epa = eds_mem__allocate_from(mem, epa_size_bytes);
        eds_port__critical_unlock(&critical);
        if (l_epa == NULL) {
            EDS_TRACE__EXIT(EDS_TRACE__SOURCE_AGENT_CREATE, EDS__ERROR_NO_MEMORY, "%u from %p", epa_size_bytes, mem);
            return EDS__ERROR_NO_MEMORY;
        }
        /* Put the queue storage directly above the SM structure
         */
        equeue_storage = (struct eds_object__evt**) (l_epa + 1u);
        EDS_TRACE__INFO(EDS_TRACE__SOURCE_AGENT_CREATE, "allocated %u from %p", epa_size_bytes, mem);
    } else {
        mem = NULL;
        l_epa = attr->static_instance;
        equeue_storage = attr->static_equeue_storage;
    }
    eds_smp__init(&l_epa->p__smp, sm_initial_state, sm_workspace);
    eds_equeue__init(&l_epa->p__equeue, attr->equeue_entries, equeue_storage);
    eds_core__tasker_node_init(&l_epa->p__task, attr->prio);
    l_epa->p__mem = mem;
#if (EDS_CONFIG__AGENT__ENABLE_NAME != 0)
    l_epa->p__name = attr->name != NULL ? attr->name : EDS__DEFAULT_EPA_NAME;
#endif
    l_epa->p__epn = NULL;
    *epa = l_epa;
    EDS_TRACE__INFO(
            EDS_TRACE__SOURCE_AGENT_CREATE,
            "id, name, equeue_entries, prio = (%p, %s, %u, %u)",
            l_epa,
            attr->name,
            attr->equeue_entries,
            attr->prio);
    return EDS__ERROR_NONE;
}

eds__error
eds_epa__send(struct eds_object__epa *epa, const struct eds_object__evt *evt)
{
    eds__error error;

    if (eds_equeue__is_full(&epa->p__equeue) == false) {
        eds_evt__ref_up(evt);
        eds_equeue__push_back(&epa->p__equeue, evt);
        eds_core__tasker_run(eds_epn__tasker(eds_epa__designation(epa)), &epa->p__task);
        eds_epn__sleep_wake_up(eds_epa__designation(epa));
        error = EDS__ERROR_NONE;
    } else {
        error = EDS__ERROR_NO_SPACE;
    }
    return error;
}

eds__error
eds_epa__dispatch(struct eds_object__epa *epa, struct eds_port__critical *critical)
{
    const struct eds_object__evt *evt;
    eds__error error;

    evt = eds_equeue__pop(&epa->p__equeue);
    if (eds_equeue__is_empty(&epa->p__equeue)) {
        eds_core__tasker_pending_sleep(eds_epn__tasker(eds_epa__designation(epa)), &epa->p__task);
    }
    eds_port__critical_unlock(critical);
    error = eds_smp__dispatch(&epa->p__smp, evt);
    eds_port__critical_lock(critical);
    eds_evt__ref_down(evt);
    eds_evt__deallocate(evt);

    return error;
}

void
eds_epa__terminate(struct eds_object__epa *epa)
{
    /* Clear the queue */
    while (eds_equeue__is_empty(&epa->p__equeue) == false) {
        const struct eds_object__evt *event;

        event = eds_equeue__pop(&epa->p__equeue);
        eds_evt__ref_down(event);
        eds_evt__deallocate(event);
    }
    eds_core__tasker_pending_sleep(eds_epn__tasker(eds_epa__designation(epa)), &epa->p__task);
}