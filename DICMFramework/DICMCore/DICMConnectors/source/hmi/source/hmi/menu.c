/*! \file menu.c
 *  \brief Module for converting events into actions based on current HMI state.
 *
 *  The menu module provides menu_update() which is used to convert events,
 *  usually user inputs or incoming data, into actions that send data or alter
 *  the state of the HMI. It also provides menu_init() which is used to set the
 *  initial menu state on boot.
 *
 *  An implementation must call menu_init() after booting before it may touch
 *  menu_update(). It then calls menu_update() every time it wishes to provide a
 *  new event which will trigger zero or more actions depending on the current
 *  menu state.
 *
 *  The HMI menu itself is organized into states and actions.
 *
 *  Menu states provide the layout of the menu. Each state contains a screen to
 *  be displayed and an array of pointers to possible actions indexed by the
 *  menu_event input parameter to menu_update(). These action pointers can also
 *  be NULL to specify that a certain event index should trigger no actions for
 *  this state.
 *
 *  Menu actions provide the instructions for how the HMI should react to
 *  events. Each action consists of a type and a set of data specific to that
 *  type. Actions are run by selecting an appropriate handler based on the type
 *  which then executes the action based on the provided data.
 *
 *  Each action also has a next_action pointer which can be used to run multiple
 *  actions for each event. Branch type actions further extend this by selecting
 *  one of two possible next_actions depending on current system state.
 *
 *  Finally menu events are simply numbers between 0 and MAX_MENU_EVENTS which
 *  are used to select menu actions. Each value except 0 should correspond to a
 *  specific user input (or other event) that needs to trigger one or more
 *  actions. Menu event index 0 is special and means no event. It will usually
 *  not map to any actions.
 */

/** Includes ******************************************************************/
#include "configuration.h"

#include "menu.h"

#include <stddef.h>
#include <stdint.h>

#include "menu_data.h"
#include "screen.h"
#include "varstate.h"
#include "event.h"
#include "data_interface.h"
#include "varstate_data.h"

/** Defines *******************************************************************/
#define	LEAP_YEAR_NO   0
#define	LEAP_YEAR_YES  1
#define	LEAP_YEAR_ERR  2

/** Variables *****************************************************************/
static const MENU_STATE *current_state = NULL;  //!< Current menu state.
static menu_ddmp_set_cb ddmp_set_callback = NULL;

/** Private prototypes ********************************************************/
static void state_change(const MENU_STATE *new_state);
static void var_change(const MENU_DATA_VAR_CHANGE *data);
static void leap_year(const MENU_DATA_LEAP_YEAR *data);
static bool menu_action_branch_compare(MENU_ACTION_TYPE type, int32_t left, int32_t right);

/*! \brief initialize menu and set menu state to initial state.
 *
 * 	\param boot_state   Pointer to initial menu state after boot.
 */
void menu_init(const MENU_STATE *boot_state, menu_ddmp_set_cb ddmp_set_cb)
{
    ddmp_set_callback = ddmp_set_cb;
    current_state = NULL;
    state_change(boot_state);
}

/*! \brief Execute menu actions specified by menu_event and current menu state.
 *
 *  Determines which actions to run by using menu_event to index into the array
 *  of menu action pointers attached to the currently active menu state. This
 *  can be NULL (no action) or a linked list of one or more actions.
 *
 *  For each action in the list selects and executes a handler based on the
 *  action type.
 *
 *  \param menu_event   Index of the menu event to run.
 */
void menu_update(uint8_t menu_event)
{
    const MENU_ACTION *action;

    assert(menu_event < MAX_MENU_EVENTS);   // Event should be in valid range.
    // Determine if there is actions in current state or something in global state.
    // Current state has priority and is used to override any possible actions defined
    // in global state.

    action = current_state->actions[menu_event];
    if (action == NULL)
    {
        if (def_data_header.menu_global_state != NULL)
        {
            action = ((const MENU_STATE *)HMI_DATA_ADDRESS(def_data_header.menu_global_state))->actions[menu_event];
            if (NULL != action)
            {
                action = (const MENU_ACTION *)HMI_DATA_ADDRESS(action);
#ifdef HMI_MENU_DEBUG_NAMES
                LOG(D, "Found global action (%s) to run (menu_event:%d)", (const char *)HMI_DATA_ADDRESS(action->name), menu_event);
#endif
            }
        }
    }
    else
    {
        action = (const MENU_ACTION *)HMI_DATA_ADDRESS(action);
#ifdef HMI_MENU_DEBUG_NAMES
        LOG(D, "Found state action (%s) to run (menu_event:%d)", (const char *)HMI_DATA_ADDRESS(action->name), menu_event);
#endif
    }
    while (action != NULL)
    {
        switch (action->type)
        {
        case MENU_ACTION_BRANCH_EQUAL:
            // fallthrough
        case MENU_ACTION_BRANCH_NOT_EQUAL:
            // fallthrough
        case MENU_ACTION_BRANCH_LESS_THAN:
            // fallthrough
        case MENU_ACTION_BRANCH_LESS_THAN_OR_EQUAL:
            // fallthrough
        case MENU_ACTION_BRANCH_GREATER_THAN:
            // fallthrough
        case MENU_ACTION_BRANCH_GREATER_THAN_OR_EQUAL:
        {
            bool is_struct = false;
            int32_t data;
            bool is_valid = varstate_get_validated_data(&is_struct, action->branch.var_index, &data, action->branch.offset, action->branch.len);
            if (is_valid && menu_action_branch_compare(action->type, data, action->branch.value))
            {
                action = (action->branch.target != NULL ? (const MENU_ACTION *)HMI_DATA_ADDRESS(action->branch.target) : NULL);
                continue;
            }
            break;
        }
        case MENU_ACTION_STATE_SET:
            state_change((const struct MENU_STATE *)HMI_DATA_ADDRESS(action->new_state));
            break;
        case MENU_ACTION_VAR_SET:
            varstate_set(action->var_set.var_index, action->var_set.value);
            break;
        case MENU_ACTION_VAR_CHANGE:
            var_change(&action->var_change);
            break;
        case MENU_ACTION_LEAP_YEAR:
            leap_year(&action->leap_year);
            break;
        case MENU_ACTION_VAR_COPY:
            {
                /* For big return values of varstate_get() the compiler implicitly casts the return value to float
                in order to multiply the scale factor with the return value. To omit this, the values are casted 
                expliclity to double type. */
                double scale_f = (double)action->var_copy.scale_factor;
                double var_get = (double)varstate_get(action->var_copy.source_index);
                varstate_set(action->var_copy.var_index, (int32_t)(scale_f * var_get));
            }
            break;
        case MENU_ACTION_VAR_COPY_OFFSET:
            {
                /* For big return values of varstate_get() the compiler implicitly casts the return value to float
                in order to multiply the scale factor with the return value. To omit this, the values are casted 
                expliclity to double type. */
                double scale_f = (double)action->var_copy_offset.scale_factor;
                double var_get;
                bool is_struct = false;
                int32_t data = 0;
                bool is_valid = varstate_get_validated_data(&is_struct, action->var_copy_offset.source_index, &data, action->var_copy_offset.offset, action->var_copy_offset.len);
                
                if (is_valid)
                {
                    var_get = (double)data;
                    varstate_set(action->var_copy_offset.var_index, (int32_t)(scale_f * var_get));
                }
                else
                {
                    // Data or action parameters not valid. Not possible to execute action
                   LOG(W, "MENU_ACTION_VAR_COPY_OFFSET not executed");
                }
            }
            break;
        case MENU_ACTION_DDMP_SET:
            if (ddmp_set_callback != NULL)
            {
                ddmp_set_callback(action->ddmp_set.parameter_id, action->ddmp_set.var_index);
            }
            break;
        case MENU_ACTION_IDLE_RESET:
            event_reset_idle_timers();
            break;
        case MENU_ACTION_DELAYED_START:
            LOG(D, "MENU_ACTION_DELAYED_START event_index=%u delay=%u", action->delayed_start.event_index, action->delayed_start.delay);
            event_delayed_start(action->delayed_start.event_index, action->delayed_start.delay);
            break;
        case MENU_ACTION_DELAYED_STOP:
            LOG(D, "MENU_ACTION_DELAYED_STOP event_index=%u", action->delayed_start.event_index);
            event_delayed_stop(action->delayed_stop.event_index);
            break;
        case MENU_ACTION_DISPLAY_ON:
            draw_set_suspend(HMI_WAKEUP);
            break;
        case MENU_ACTION_DISPLAY_OFF:
            draw_set_suspend(HMI_SLEEP);
            break;
        case MENU_ACTION_TIMER_START:
            event_restart_delay_timer(action->timer_start.event_index);
            break;
        case MENU_ACTION_TIMER_STOP:
            event_stop_delay_timer(action->timer_stop.event_index);
            break;
        case MENU_ACTION_DISPLAY_BRIGHTNESS_SET:
            screen_display_brightness_level_set(varstate_get(action->display_brightness_set.var_index));
            break;
        case MENU_ACTION_SAVE_HMI_SETTINGS:
            // Save internal variables like brightness to nvs
            screen_hmi_settings_save();
            break;
        case MENU_ACTION_NO_ACTION:
            // Do nothing
            break;
        default:
            assert(0);	// Unknown action type.
        }

        action = (action->next_action != NULL ? (const struct MENU_ACTION *)HMI_DATA_ADDRESS(action->next_action) : NULL);
    }
}

/*! \brief Change menu state to new_state and then redraw screen.
 *
 * 	Menu action handler for MENU_ACTION_STATE_CHANGE.
 *
 * 	\param new_state    Pointer to new menu state.
 */
static void state_change(const MENU_STATE *new_state)
{
    static bool isInRecursiveCall = false;
    if (NULL != current_state)
    {
        // Execute any state_exit actions here.
        // Get event from type
        uint8_t menu_index = event_get_menu_event_from_type_and_index(EVENT_TYPE_STATE_EXIT, 0u);
#ifdef HMI_MENU_DEBUG_NAMES
        LOG(D, "Executing EVENT_TYPE_STATE_EXIT (in %s): %d", (const char *)HMI_DATA_ADDRESS(current_state->name), menu_index);
#endif
        // Execute actions
        isInRecursiveCall = true;
        menu_update(menu_index);
        isInRecursiveCall = false;
    }
    // Not allowing recursive state changes for now
    if (!isInRecursiveCall)
    {
        // Change state
        current_state = new_state;
        // Execute any state_entry actions here.
        // Get event from type
        uint8_t menu_index = event_get_menu_event_from_type_and_index(EVENT_TYPE_STATE_ENTRY, 0u);
#ifdef HMI_MENU_DEBUG_NAMES
        LOG(D, "Executing EVENT_TYPE_STATE_ENTRY (in %s): %d", (const char *)HMI_DATA_ADDRESS(current_state->name), menu_index);
#endif
        // Execute actions
        isInRecursiveCall = true;
        menu_update(menu_index);
        isInRecursiveCall = false;
        // Redraw new screen
        screen_change((const SCREEN_DEF*)HMI_DATA_ADDRESS(current_state->screen));
    }
    else
    {
        LOG(E, "Recursive state_change called. Check your hmi data!");
    }
    isInRecursiveCall = false;	// Reset in case of recursive call
}

/*! \brief Increase a system state variable while respecting the specified
 * 	limit.
 *
 *  Menu action handler for MENU_ACTION_VAR_CHANGE.
 *
 * 	Increases the value of the specified system state variable by the provided
 * 	value. If value if negative system state will be decreased instead. If value
 * 	would increase system state above (or decrease below for negative values) the
 * 	supplied limit then new state will instead be the limit value.
 *
 * 	Will not allow the altered system state value to overflow.
 *
 * 	\param data Pointer to struct containing action parameters.
 */
static void var_change(const MENU_DATA_VAR_CHANGE *data)
{
    int32_t value = varstate_get(data->var_index);
    int32_t new_value = value + data->value;

    // Check for overflow and limit violations.
    if (data->value < 0)
    {
        if ((new_value > value) || (new_value < data->limit))
        {
            new_value = data->limit;
        }
    }
    else
    {
        if ((new_value < value) || (new_value > data->limit))
        {
            new_value = data->limit;
        }
    }

    varstate_set(data->var_index, new_value);
}

/*! \brief Handle year set for leap year check
 *
 *  Menu action handler for MENU_DATA_LEAP_YEAR.
 *
 *  \param data Pointer to struct containing action parameters.
 */
static void leap_year(const MENU_DATA_LEAP_YEAR *data)
{
    int32_t ret_value = LEAP_YEAR_ERR;
    /* Convert year to actual value, offset is 2000 */
    int32_t year = varstate_get(data->current_year_index) + 2000;

    /* Check if provided year is a leap year */
    if (!(year % 4))
    {
        if (!(year % 100))
        {
            ret_value = LEAP_YEAR_NO;
            if (!(year % 400))
            {
                ret_value = LEAP_YEAR_YES;
            }
        }
        else
        {
            ret_value = LEAP_YEAR_YES;
        }
    }
    else
    {
        ret_value = LEAP_YEAR_NO;
    }
    varstate_set(data->var_index, ret_value);
}

/*! \brief Evaluates branch compare conditions 
 *
 *  \param[in] type Type of comparison
 *  \param[in] left Left side data
 *  \param[in] right Right side data
 *
 * \return true if condition evaluates to true
 */
static bool menu_action_branch_compare(MENU_ACTION_TYPE type, int32_t left, int32_t right)
{
    bool compare_result = false;
    
    switch (type)
    {
        case MENU_ACTION_BRANCH_EQUAL:
            compare_result = (left == right);
            break;
        case MENU_ACTION_BRANCH_NOT_EQUAL:
            compare_result = (left != right);
            break;
        case MENU_ACTION_BRANCH_GREATER_THAN:
            compare_result = (left > right);
            break;
        case MENU_ACTION_BRANCH_LESS_THAN:
            compare_result = (left < right);
            break;
        case MENU_ACTION_BRANCH_GREATER_THAN_OR_EQUAL:
            compare_result = (left >= right);
            break;
        case MENU_ACTION_BRANCH_LESS_THAN_OR_EQUAL:
            compare_result = (left <= right);
            break;
        default:
            LOG(E, "Bad argument type: %d", type);
            break;
    }
    return compare_result;
}
