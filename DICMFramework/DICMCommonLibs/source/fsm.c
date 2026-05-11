/*! \file fsm.c
	\brief Finite State Machine implementations

 */

#include "fsm.h"
#include <stddef.h>

static const fsm_event_t entry_event = {FSM_ENTRY_EVENT, NULL};
static const fsm_event_t exit_event = {FSM_EXIT_EVENT, NULL};

void fsm_initialize(fsm_t * const p_sm, fsm_state_fcn_t init_state)
{
	p_sm->m_state = init_state;
	fsm_state_dispatch(p_sm, &entry_event);
}

void fsm_state_dispatch(fsm_t * const p_sm, fsm_event_t const * const p_event)
{
	p_sm->m_state(p_sm, p_event);
}

void fsm_state_change(fsm_t * const p_sm, fsm_state_fcn_t new_state)
{
	fsm_state_dispatch(p_sm, &exit_event);
	p_sm->m_state = new_state;
	fsm_state_dispatch(p_sm, &entry_event);
}
