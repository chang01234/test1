/*! \file fsm.h
	\brief Finite State Machine

	This module offers initialization and execution of FSM (Finite State Machines).
*/

#ifndef FSM_H_
#define FSM_H_

#include <stdint.h>

/**
 * @brief 	Entry event identifier constant
 *
 * When entering a state ENTRY event will be dispatched to the state.
 */
#define FSM_ENTRY_EVENT 					(0u)

/**
 * @brief 	Exit event identifier constant
 *
 * When exiting a state EXIT event will be dispatched to the exiting state.
 */
#define FSM_EXIT_EVENT 						(1u)

/**
 * @brief 	Undefined (NULL) event identifier constant
 *
 * This identifier indicates that the event is a NULL event, e.g. no operation is needed.
 *
 * This identifier is usually used to check in boiler plate code if there is an event pending for
 * processing or not.
 */
#define FSM_UNDEFINED_EVENT 				(255u)

/**
 * @brief 	User defined event identifier
 *
 * Use this identifier as a starting point for user event identifiers. By assigning the first user
 * event this identifier you are making sure that there will be no overlap between user events and
 * FSM events.
 *
 * @code
 * enum my_events_id
 * {
 * 		MY_EVENTS_ID_0 = FSM_USER_EVENT,
 * 	    MY_EVENTS_ID_1,
 * 		MY_EVENTS_ID_2
 * };
 * @endcode
 */
#define FSM_USER_EVENT						(2u)

/**
 * @brief 	FSM state handler cast
 *
 * Use this macro to cast FSM state function which do not conform to @ref fsm_state_fcn_t prototype.
 * This macro is usually used when state function is expecting some user defined structure in first
 * parameter instead of fsm structure. This is doable only when the first member of user defined
 * structure is the fsm structure.
 *
 * @note	Use this macro with caution since an improper function can be casted to state prototype.
 */
#define FSM_STATE_HANDLER(state)			((fsm_state_fcn_t)state)

/**
 * @brief 	Event structure
 *
 * Each event is composed of:
 * - Event identifier, which is an integer unique to each event. Do not use predefined
 * 	 state machine event identifiers, such as @ref FSM_ENTRY_EVENT and @ref FSM_EXIT_EVENT for
 *   custom user events. For details, refer to @ref FSM_USER_EVENT.
 * - Event data, which is a pointer to some data structure associated with the event. If event has
 *   no data, this pointer should be set to NULL.
 */
typedef struct fsm_event
{
	uint32_t id;							//!< Event identifier
	void *p_data;							//!< Data associated with the event
} fsm_event_t;

/**
 * @brief 	Forward declaration of FSM object.
 */
typedef struct fsm fsm_t;

/**
 * @brief 	FSM state function prototype
 *
 * Each FSM state function receives pointer to the FSM object and pointer to an event that needs to
 * be processed.
 */
typedef void (*fsm_state_fcn_t)(fsm_t * const fsm, fsm_event_t const * const event);

/**
 * @brief 	Structure of FSM object
 *
 * FSM object keeps track which is the current state of state machine.
 */
typedef struct fsm
{
	fsm_state_fcn_t m_state;
} fsm_t;

/**
 * @brief 		Initializes the FSM object
 *
 * @param 		p_sm Pointer to allocated FSM object which is to be initialized
 * @param 		initial_state Initial state of the state machine being initialized
 */
void fsm_initialize(fsm_t * const p_sm, fsm_state_fcn_t initial_state);

/**
 * @brief 		Dispatch en event to a FSM object
 *
 * @param 		p_sm Pointer to initialized FSM object
 * @param 		p_event Pointer to event
 */
void fsm_state_dispatch(fsm_t * const p_sm, fsm_event_t const * const p_event);

/**
 * @brief 		Execute a state transition
 *
 * @param 		p_sm Pointer to initialized FSM object
 * @param 		new_state New state function pointer
 */
void fsm_state_change(fsm_t * const p_sm, fsm_state_fcn_t new_state);

#endif /* FSM_H_ */
