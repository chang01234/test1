/*! \file rule_engine.c
    \brief Rule engine implementation
    \author Borjan Bozhinovski
*/

#include "configuration.h"

#include <string.h>

#include "ddm2.h"
#include "rule_engine.h"
#include "rule_engine_def.h"
#include "rule_engine_exp_eval.h"
#include "rule_engine_memory_accessor.h"

#include "broker.h"
#include "connector_system.h"
#include "ddm_wrapper.h"
#include "freertos/FreeRTOS.h"
#include "sorted_container.h"
#include "utils.h"

/**********************************************************
 * Private types
 *********************************************************/
#ifndef RULE_ENGINE__TEST_VARIABLE_IDS
#define RULE_ENGINE__TEST_VARIABLE_IDS 0  // Set to 1 to check if RULE_ENGINE__VAR_TYPE_GLOBAL/LOCAL
                                          // has the same id's as any RULE_ENGINE__VAR_TYPE_DDM2
#endif

static const char RULE_ENGINE__MASK[] = "mask";
static const char RULE_ENGINE__DDM[] = "ddm";  // Create ddm parameter
static const char RULE_ENGINE__MASK_CONSTRAINT_OR_STR[] = "'or'";
static const char RULE_ENGINE__MASK_CONSTRAINT_AND_STR[] = "'and'";
static const char RULE_ENGINE__VAR_GLOBAL_PREFIX[] = "g_";
static const char RULE_ENGINE__INVENTORY[] = "GW0INV";
static const char *const RULE_ENGINE__TIMERS_TYPE[] = {"absolute", "relative"};

static int split_rule_and_sens_list(char *rule_str, char **rule, char **sens_list);
static int parse_sens_list(struct rule_engine__sensitivity_list_parser *const sens_list, char *sens_list_str);
static int bind_sens_list_vars_with_rule_list_vars(struct rule_engine__specification *const specification, const struct rule_engine__sensitivity_list_parser *const sens_list);
static void rule_engine_sens_list_variable_set_updated(struct rule_engine__sensitivity_list *variable, bool is_updated);
static bool rule_engine_sens_list_variable_is_updated(const struct rule_engine__sensitivity_list *const variable);
static void rule_engine_sens_list_variable_set_initialized(struct rule_engine__sensitivity_list *variable, bool is_initialized);
static bool rule_engine_sens_list_variable_is_initialized(const struct rule_engine__sensitivity_list *const variable);
static const struct rule_engine__sensitivity_list *rule_engine_sens_list_contains_expr_var(const struct rule_engine__sensitivity_list *const sens_list, size_t sens_list_elements, const struct expr_var *var);

static bool rule_engine_is_active(rule_engine_inst_t *const p_rule_engine_inst);
static ddmw_item_t *rule_engine_get_activation_parameter(rule_engine_inst_t *const p_rule_engine_inst);
static void rule_engine_resubscribe_rules_parameters(rule_engine_inst_t *const p_rule_engine_inst);
static bool rule_engine_should_evaluate_rules(rule_engine_inst_t *const p_rule_engine_inst);

static int create_rule(struct rule_engine__specification *specification, int rule_str_size);
static int create_rule_expression(struct rule_engine__specification *specification, const char *const rule, struct rule_engine__sensitivity_list_parser *const sens_list);
static int resolve_rule_expression_type(struct rule_engine__specification *const specification);
static int determine_rule_expression_type(const struct rule_engine__specification *specification, rule_engine__expression_type_t *expr_type);
static int set_user_functions_context(const struct rule_engine__specification *specification, struct expr *func_expr);
static int setup_user_functions(struct rule_engine__specification *specification);
static bool execute_rules(rule_engine_inst_t *const p_rule_engine_inst);
static void reset_rules_execution(rule_engine_inst_t *const p_rule_engine_inst);
static void resolve_rule_expression(struct rule_engine__specification *specification);
static void resolve_rule_expressions(struct rule_engine__specification *specification);

static int setup_inventory_callback(const struct rule_engine__specification *const specification);
static int setup_standard_rule_ddm2_parameters(const struct rule_engine__specification *const specification);
static int setup_ddm2_parameter(const struct rule_engine__specification *specification, const struct expr_var *var);
static int remove_ddm2_parameter(const struct rule_engine__specification *specification, const struct expr_var *var);
static void setup_ddm2_owned_parameter(const struct rule_engine__specification *specification, ddmw_item_t *item, const struct expr_var *var);
static void setup_ddm2_subscribed_parameter(const struct rule_engine__specification *specification, ddmw_item_t *item, const struct expr_var *var);
static ddmw_action_t determine_ddm2_variable_action_type(const struct expr_var *var);
static int detect_and_setup_ddm2_event_parameter(ddmw_item_t *item, const struct expr_var *var);
static int get_ddm2_type_for_write_action(const struct expr_var *var, DDM2_TYPE_ENUM *type);

static struct rule_engine__specification *create_new_rule_specification(rule_engine_inst_t *const p_rule_engine_inst, const struct rule_engine__rule *const rule, uint32_t *const rule_id);
static void delete_rule_specification(rule_engine_inst_t *const p_rule_engine_inst, uint32_t rule_id);

static int create_trigger_section(struct rule_engine__specification *specification, struct rule_engine__sensitivity_list_parser *sens_list);
static int update_trigger_section(struct rule_engine__specification *const specification, const struct expr_var *const variable, int32_t value, uint8_t type);
static void clear_trigger_section(struct rule_engine__specification *specification);
static int init_rule_variables(struct rule_engine__specification *const specification, const struct rule_engine__sensitivity_list_parser *const sens_list);
static int resolve_rule_variable_types(struct rule_engine__specification *const specification, struct rule_engine__sensitivity_list_parser *const sens_list);
static int create_rule_mask(const struct rule_engine__specification *const specification, struct rule_engine__sensitivity_list_parser *const sens_list);

static bool ddm2_parameter_updated(rule_engine_inst_t *const p_rule_engine_inst);
static void update_ddm2_variables(rule_engine_inst_t *const p_rule_engine_inst, uint32_t parameter, int32_t value);
static bool update_rules_variables_and_trigger_section(const struct rule_engine__specification *const specification);
static void rule_engine_update_variable_value(struct expr_var_list *updating_variables, struct expr_var *variable);
static struct expr_var *find_variable_in_other_specifications(const struct rule_engine__specification *specification, const char *variable_name);

static int verify_and_create_timer(struct rule_engine__specification *specification, struct expr *statement, uint8_t timer_type);
static int verify_pub_function(struct rule_engine__specification *specification, struct expr *statement);
static int verify_dyn_instance_function(struct rule_engine__specification *specification, struct expr *statement);
static int verify_inventory_function(struct rule_engine__specification *specification, struct expr *statement);
static int verify_ddm_get_data_function(struct rule_engine__specification *specification, struct expr *statement);
static int verify_ddm_set_data_function(struct rule_engine__specification *specification, struct expr *statement);
static int verify_ddm_get_array_length_function(struct rule_engine__specification *specification, struct expr *statement);
static int verify_ddm_find_array_value_function(struct rule_engine__specification *specification, struct expr *statement);

static void trigger_dynamic_resolution_rules(const struct rule_engine__specification *const specification, struct expr_var *instance_variable);
static void resolution_of_dynamic_instances_dyn_instance(rule_engine_inst_t *const p_rule_engine_inst);
static void resolution_of_dynamic_instances_inventory_cb(uint32_t parameter_class, void *context);
static void handle_dynamic_instance_resolution_state_change(struct rule_engine__specification *const specification, struct expr_var *instance_variable, const struct expr_var *instance_variable_old_state);
static void dynamic_variables_resolution(const struct rule_engine__specification *const specification, struct expr_var *instance_variable, const struct expr_var *instance_variable_old_state, uint8_t resolution_type);
static void resolve_rules_dynamic_variables(const struct rule_engine__specification *const specification, struct expr_var *instance_variable, const struct expr_var *instance_variable_old_state);
static void unresolve_rules_dynamic_variables(const struct rule_engine__specification *const specification, struct expr_var *instance_variable, const struct expr_var *instance_variable_old_state);
static void dynamic_variables_subscription(const struct rule_engine__specification *const specification, const struct expr_var *instance_variable, const struct expr_var *instance_variable_old_state, uint8_t subscription);
static void subscribe_rules_dynamic_variables(const struct rule_engine__specification *const specification, const struct expr_var *instance_variable, const struct expr_var *instance_variable_old_state);
static void unsubscribe_rules_dynamic_variables(const struct rule_engine__specification *const specification, const struct expr_var *instance_variable, const struct expr_var *instance_variable_old_state);
static void resolve_dynamic_parameter_variable(struct expr_var *ddm2_dynamic_variable, uint32_t instance_id);
static void unresolve_dynamic_parameter_variable(struct expr_var *ddm2_dynamic_variable);
static void resolve_dynamic_instance_variable(struct expr_var *instance_variable, uint32_t instance_id);
static void unresolve_dynamic_instance_variable(struct expr_var *instance_variable);
static bool is_dynamic_parameter_associated_with_instance_variable(const struct expr_var *dynamic_parameter, const struct expr_var *instance_variable);
static bool is_the_same_dynamic_variable(const struct expr_var *dynamic_variable1, const struct expr_var *dynamic_variable2);

static const char *decode_variable_type(const struct expr_var *variable);
static const char *decode_variable_type_is_resolved(const struct expr_var *variable);
static void log_dynamic_variables_relationships(const rule_engine_inst_t *p_rule_engine_inst, void *affected_instance);

static size_t copy_string_to_buffer(const char *const src, char *dest, size_t dest_size);
static bool strings_equal(const char *const s1, size_t s1_len, const char *const s2, size_t s2_len);

static void rule_engine_update_timers(rule_engine_inst_t *const p_rule_engine_inst);
static int rule_engine_timer_start(struct rule_engine__timer *timer);
static void rule_engine_timer_stop(struct rule_engine__timer *timer);
static uint32_t rule_engine_get_timer_period_in_ticks(uint8_t timer_type, uint32_t time_ms);
static uint32_t rule_engine_calculate_absolute_time(uint32_t *const time_s);
static void rule_engine_update_timer_instance_value(struct rule_engine__timer *timer);
static void rule_engine_update_timer_instance_state_and_value(struct rule_engine__timer *timer, uint8_t timer_state);
static struct rule_engine__timer *rule_engine_timer_create(const rule_engine_inst_t *p_inst, struct expr_var *const timer_instance, uint8_t timer_type, uint32_t time_in_ticks, uint32_t rule_id);
static void rule_engine_timer_callback(TimerHandle_t xTimer);

// User functions
static int32_t rule_func_ref_variable(const struct expr_func *const f, vec_expr_t *args, void *c, uint32_t expr_id);
static int32_t rule_func_set(const struct expr_func *const f, vec_expr_t *args, void *c, uint32_t expr_id);
static int32_t rule_func_print(const struct expr_func *const f, vec_expr_t *args, void *c, uint32_t expr_id);
static int32_t rule_func_if(const struct expr_func *const f, vec_expr_t *args, void *c, uint32_t caller_id);
// timer functions
static int32_t rule_func_timer_on(const struct expr_func *const f, vec_expr_t *args, void *c, uint32_t expr_id);
static int32_t rule_func_timer_off(const struct expr_func *const f, vec_expr_t *args, void *c, uint32_t expr_id);
// memory accessor functions (nested struct and array)
static int32_t rule_func_set_data(const struct expr_func *const f, vec_expr_t *args, void *c, uint32_t expr_id);
static int32_t rule_func_get_data(const struct expr_func *const f, vec_expr_t *args, void *c, uint32_t expr_id);
// find value in a array(array of structs)
static int32_t rule_func_find_array_value(const struct expr_func *const f, vec_expr_t *args, void *c, uint32_t expr_id);
// get array length
static int32_t rule_func_get_array_length(const struct expr_func *const f, vec_expr_t *args, void *c, uint32_t expr_id);
// dynamic instance resolution functions
static int32_t dyn_instance(const struct expr_func *const f, vec_expr_t *args, void *c, uint32_t expr_id);
static int32_t inventory(const struct expr_func *const f, vec_expr_t *args, void *c, uint32_t expr_id);

static const struct expr_func l_user_funcs[RULE_ENGINE__NUM_USER_FUNCS + 1] =
    {
        {"ref", rule_func_ref_variable, NULL, sizeof(rule_engine_func_context_t), 0},
        {"pub", rule_func_set, NULL, sizeof(rule_engine_func_context_t), 0},
        {"if", rule_func_if, NULL, sizeof(rule_engine_func_context_t), 0},
        {"print", rule_func_print, NULL, sizeof(rule_engine_func_context_t), 0},
        {"timer_on", rule_func_timer_on, NULL, sizeof(rule_engine_func_context_t), 0},
#if defined(CONNECTOR_SYSTEM)
        {"timer_abs_on", rule_func_timer_on, NULL, sizeof(rule_engine_func_context_t), 0},
#endif
        {"timer_off", rule_func_timer_off, NULL, sizeof(rule_engine_func_context_t), 0},
        {"dyn_instance", dyn_instance, NULL, sizeof(rule_engine_func_context_t), 0},
        {"inventory", inventory, NULL, sizeof(rule_engine_func_context_t), 0},
        {"ddm_set_data", rule_func_set_data, NULL, sizeof(rule_engine_func_context_t), 0},
        {"ddm_get_data", rule_func_get_data, NULL, sizeof(rule_engine_func_context_t), 0},
        {"ddm_find_array_value", rule_func_find_array_value, NULL, sizeof(rule_engine_func_context_t), 0},
        {"ddm_get_array_length", rule_func_get_array_length, NULL, sizeof(rule_engine_func_context_t), 0},
        // Sentinel to mark the end of the user functions array
        {NULL, NULL, NULL, 0, 0},
};

/*!< Structure that holds the data of the timer used for resolving the rule for
    which the timer kicked off. It will be used by rule_engine__evaluate_timers()
    once the connector_task is unblocked.
 */
struct rule_engine__timer_data
{
    struct expr_var *instance;  //!< \~ Timer instance bound to global variable
};

/**********************************************************
 * Implementation
 *********************************************************/
static bool strings_equal(const char *const s1, size_t s1_len, const char *const s2, size_t s2_len)
{
    // Add safety checks for NULL pointers
    if ((s1 == NULL) || (s2 == NULL))
    {
        return false;
    }

    return ((s1_len == s2_len) && (strncmp(s1, s2, s1_len) == 0));
}

static size_t copy_string_to_buffer(const char *const src, char *dest, size_t dest_size)
{
    size_t length = strnlen(src, dest_size - 1);
    strncpy(dest, src, length);
    dest[length] = 0;

    return length;
}

/**********************************************************
 * Timers implementation
 *********************************************************/
static void rule_engine_timer_callback(TimerHandle_t xTimer)
{
    struct rule_engine__timer *timer = (struct rule_engine__timer *)pvTimerGetTimerID(xTimer);
    struct rule_engine__timer_data timer_data =
        {
            .instance = timer->instance,
        };

    // Forward timer data back to the rule engine task(connector_task), so we can trigger time delayed rules dependent on this timer
    connector_send_frame_to_connector(DDMP2_CONTROL_GENERIC, RULE_ENGINE__TIMER_TRIGG_EVENT, &timer_data, sizeof(timer_data), timer->p_rule_engine_inst->wrapper->connector->connector_id, (TickType_t)portMAX_DELAY);
#if 0
    struct rule_engine__specification * rule = sorted_container__access(timer->p_rule_engine_inst->p_rule_engine_specifications, timer->rule_id);

    for (size_t s = 0; s < sorted_container__occupied(timer->p_rule_engine_inst->p_rule_engine_specifications); s++)
    {
        uint32_t spec_key;
        struct rule_engine__specification * specs;

        sorted_container__iterate(timer->p_rule_engine_inst->p_rule_engine_specifications, s, (void**)&specs, &spec_key);

        if (specs != rule)
        {
            update_trigger_section(specs, timer->instance, 1, RULE_ENGINE__VAR_TYPE_GLOBAL);
        }
    }
#endif
}

static void rule_engine_update_timer_instance_value(struct rule_engine__timer *timer)
{
    struct rule_engine__specification *rule = sorted_container__access(timer->p_rule_engine_inst->p_rule_engine_specifications, timer->rule_id);
    for (size_t s = 0; s < sorted_container__occupied(timer->p_rule_engine_inst->p_rule_engine_specifications); s++)
    {
        uint32_t sspec_key;
        struct rule_engine__specification *specs;

        sorted_container__iterate(timer->p_rule_engine_inst->p_rule_engine_specifications, s, (void **)&specs, &sspec_key);

        if (specs != rule)
        {
            rule_engine_update_variable_value(&specs->expr.vars, timer->instance);
        }
    }
}

static void rule_engine_update_timer_instance_state_and_value(struct rule_engine__timer *timer, uint8_t timer_state)
{
    // update timer instance value to correspond with the
    // timer's state(RULE_ENGINE__TIMER_ACTIVE, RULE_ENGINE__TIMER_OFF)
    timer->state = timer_state;
    rule_engine_update_timer_instance_value(timer);
}

static void rule_engine_timer_stop(struct rule_engine__timer *timer)
{
    TRUE_CHECK_RETURN(xTimerStop(timer->timer_handle, portMAX_DELAY));
    // set timer instance value to RULE_ENGINE__TIMER_OFF, so it's value
    // can be used to generate additional logic in the rules
    rule_engine_update_timer_instance_state_and_value(timer, RULE_ENGINE__TIMER_OFF);

    // LOG(I, "Timer[%ds] off", pdTICKS_TO_MS(timer->time_ticks) / 1000);
}
#if 0
static bool rule_engine_timer_off(rule_engine_inst_t *p_rule_engine_instance, struct expr_var * const timer_instance, uint32_t rule_id)
{
    bool timer_exists = false;
    struct rule_engine__timer * timer;
    struct rule_engine__specification * rule = sorted_container__access(p_rule_engine_instance->p_rule_engine_specifications, rule_id);

    for (size_t n_timers = 0; n_timers < sorted_container__occupied(p_rule_engine_instance->p_rule_engine_set_timers); ++n_timers)
    {
        uint32_t timer_key;

        sorted_container__iterate(p_rule_engine_instance->p_rule_engine_set_timers, n_timers, (void**)&timer, &timer_key);

        if (expr_var_id(timer->instance) == expr_var_id(timer_instance))
        {
            timer_exists = true;
            break;
        }
    }

    if (timer_exists == true)
    {
        if (timer->state != RULE_ENGINE__TIMER_OFF)
        {
            rule_engine_timer_stop(timer);
        }
        else
        {
            LOG(W, "Rule[%s]: Timer[%s] not started!", rule->name, timer->instance->name)
        }
    }
    else
    {
        LOG(W, "Rule[%s]: Timer[%s] not yet initialized!", rule->name, timer_instance->name)
    }

    return true;
}
#endif
static int rule_engine_timer_start(struct rule_engine__timer *timer)
{
    // xTimerChangePeriod() has equivalent functionality to the
    // xTimerStart() API function even if the timer was in the dormant state.
    // Also, update the rule_id with the rule's id that triggers the timer.
    TRUE_CHECK_RETURN0(xTimerChangePeriod(timer->timer_handle, timer->time_ticks, portMAX_DELAY));

    // set timer instance value to RULE_ENGINE__TIMER_ACTIVE, so it's value
    // can be used to generate additional logic in the rules
    rule_engine_update_timer_instance_state_and_value(timer, RULE_ENGINE__TIMER_ACTIVE);

    // LOG(I, "Timer[%ds] on", pdTICKS_TO_MS(timer->time_ticks) / 1000);
    return 1;
}

#if defined(CONNECTOR_SYSTEM)
static uint32_t rule_engine_calculate_absolute_time(uint32_t *const time_s)
{
    time_t time_now;
    struct tm date_time;
    time_t time_in_s = *time_s;

    if (connector_system_get_local_time(&date_time) == true)
    {
        time_now = mktime(&date_time);
        if (time_in_s > time_now)
        {
            *time_s -= time_now;
        }
        else
        {
            LOG(E, "Invalid absolute time, %us <= %lds", *time_s, time_now);
            return 0;
        }
    }
    else
    {
        LOG(E, "System time not set!");
        return 0;
    }

    return *time_s;
}
#else
static uint32_t rule_engine_calculate_absolute_time(uint32_t *const time_s)
{
    return 0;
}
#endif  // CONNECTOR_SYSTEM

static uint32_t rule_engine_get_timer_period_in_ticks(uint8_t timer_type, uint32_t time_ms)
{
    uint32_t timer_period_ticks = 0;
    uint32_t timer_period_ms = time_ms;

    if (timer_type == RULE_ENGINE__ABSOLUTE_TIMER)
    {
        // Absolute time in seconds
        uint32_t timer_period_s = timer_period_ms;
        if (rule_engine_calculate_absolute_time(&timer_period_s) == 0)
        {
            return 0;
        }
        timer_period_ms = timer_period_s * 1000;
    }

    timer_period_ticks = pdMS_TO_TICKS(timer_period_ms);
    if (timer_period_ticks == 0)
    {
        LOG(E, "Lowest timer value in milliseconds can be set to[%d].", pdTICKS_TO_MS(1));
        return 0;
    }

    return timer_period_ticks;
}

static void rule_engine_update_timers(rule_engine_inst_t *const p_rule_engine_inst)
{
    for (size_t n_timers = 0; n_timers < sorted_container__occupied(p_rule_engine_inst->p_rule_engine_set_timers); ++n_timers)
    {
        uint32_t timer_key;
        struct rule_engine__timer *timer_instance;

        sorted_container__iterate(p_rule_engine_inst->p_rule_engine_set_timers, n_timers, (void **)&timer_instance, &timer_key);

        if (p_rule_engine_inst->rules_valid)
        {
            if (timer_instance->state == RULE_ENGINE__TIMER_SET_ON)
            {
                if (rule_engine_timer_start(timer_instance) == 0)
                {
                    // Timer cannot be started. Turn off all the timers
                    // and do not allow SET of DDM2 parameters, if any is pending(set_parameters())
                    p_rule_engine_inst->rules_valid = false;
                    n_timers = 0;  // iterrate trough all the timers again, and turn them off.
                }
            }
            else if (timer_instance->state == RULE_ENGINE__TIMER_SET_OFF)
            {
                rule_engine_timer_stop(timer_instance);
            }
        }
        else
        {
            rule_engine_timer_stop(timer_instance);
        }
    }
}

static struct rule_engine__timer *rule_engine_timer_create(const rule_engine_inst_t *p_inst, struct expr_var *const timer_instance, uint8_t timer_type, uint32_t time_in_ticks, uint32_t rule_id)
{
    struct rule_engine__timer *timer = NULL;
    struct rule_engine__specification *rule = sorted_container__access(p_inst->p_rule_engine_specifications, rule_id);

    // Timer variable instance is mapped with rule_engine__timer struct using the variable id
    timer = sorted_container__access(p_inst->p_rule_engine_set_timers, timer_instance->id);
    if (timer != NULL)  // Timer exists, use the same instance of the timer
    {
        if (expr_var_id(timer->instance) == expr_var_id(timer_instance))
        {
            if (timer->type != timer_type)
            {
                LOG(E, "Rule[%s:%d]: Timer[%s] already created as %s type", rule->name, rule->id, timer_instance->name, RULE_ENGINE__TIMERS_TYPE[timer->type]);
                return NULL;
            }
        }
    }
    else  // Timer does not exist, create new instance for the timer
    {
        timer = sorted_container__new(p_inst->p_rule_engine_set_timers, timer_instance->id);  // map timer variable instance with rule_engine__timer struct
        if (timer == NULL)
        {
            LOG(E, "Rule[%s:%d]: No available slot for storing timer instance[%s]", rule->name, rule->id, timer_instance->name);
            return NULL;
        }

        memset(timer, 0, sizeof(struct rule_engine__timer));
    }

    /* Set timer's period to the init value of the rule. If more than one rule is using the same timer instance,
    the timer's period will be overriden before the timer is started */
    timer->instance = timer_instance;
    timer->time_ticks = time_in_ticks;
    timer->type = timer_type;
    timer->p_rule_engine_inst = p_inst;
    /* If timer is already in ACTIVE/SET states, do not override it.
    Case when new rule is added additionally, and the rule engine is already active. */
    timer->state = (timer->state != RULE_ENGINE__TIMER_OFF) ? timer->state : RULE_ENGINE__TIMER_OFF;

    timer->timer_handle = xTimerCreateStatic(NULL, timer->time_ticks, false, timer, rule_engine_timer_callback, &timer->timer_buffer);
    if (timer->timer_handle == NULL)
    {
        LOG(E, "Rule[%s:%d]: RTOS timer cannot be created!", rule->name, rule->id);
        sorted_container__delete(p_inst->p_rule_engine_set_timers, timer_instance->id);
        return NULL;
    }

    return timer;
}

/*******************************************************
 * Logging and debugging functions
 ********************************************************/
static const char *decode_variable_type(const struct expr_var *variable)
{
    switch (expr_var_type(variable))
    {
    case RULE_ENGINE__VAR_TYPE_DDM2:
        return "DDM2";
    case RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC:
        return "DDM2_DYNAMIC";
    case RULE_ENGINE__VAR_TYPE_GLOBAL:
        return "GLOBAL";
    case RULE_ENGINE__VAR_TYPE_LOCAL:
        return "LOCAL";
    case RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE:
        return "DDM2_INSTANCE";
    default:
        return "UNKNOWN";
    }
}

static const char *decode_variable_type_is_resolved(const struct expr_var *variable)
{
    switch (expr_var_type_is_resolved(variable))
    {
    case RULE_ENGINE__VAR_TYPE_UNRESOLVED:
        return "UNRESOLVED";
    case RULE_ENGINE__VAR_TYPE_RESOLVED:
        return "RESOLVED";
    case RULE_ENGINE__VAR_TYPE_UNAPLICABLE:
        return "UNAPLICABLE";
    default:
        return "UNKNOWN";
    }
}

/**
 * @brief Debugs DDM2 instance variables and their linked variables
 *
 * @param p_rule_engine_inst Rule engine instance
 * @param affected_instance_var If non-NULL, only show information about this specific instance variable
 */
static void log_dynamic_variables_relationships(const rule_engine_inst_t *p_rule_engine_inst, void *affected_instance)
{
    const struct expr_var *affected_instance_var = (const struct expr_var *)affected_instance;
    bool show_all_instances = (affected_instance_var == NULL);
    const char *title_suffix = show_all_instances ? "All Instances" : "Affected Instance";
    LOG(D, "=== DDM2 %s and Their Linked Variables ===", title_suffix);

    // First pass: Find all DDM2 instance variables or just the affected one
    for (size_t spec_idx = 0; spec_idx < sorted_container__occupied(p_rule_engine_inst->p_rule_engine_specifications); spec_idx++)
    {
        uint32_t spec_key;
        struct rule_engine__specification *spec;
        sorted_container__iterate(p_rule_engine_inst->p_rule_engine_specifications, spec_idx, (void **)&spec, &spec_key);

        // Look for DDM2 instance variables in this rule
        for (struct expr_var *var = spec->expr.vars.head; var; var = var->next)
        {
            // Only process DDM2 instance variables
            if (expr_var_type(var) == RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE)
            {
                // Skip if we're only showing affected instance and this isn't it
                if (!show_all_instances && !is_the_same_dynamic_variable(var, affected_instance_var))
                {
                    continue;
                }

                LOG(D, "--- INSTANCE: Rule[%s:%d] var='%s' ---",
                    spec->name, spec->id, var->name);
                LOG(D, "    Class ID: 0x%08X, Type ID: %d, Value: %d, Resolved: %s",
                    expr_var_id(var), expr_var_type_id(var), expr_var_value(var),
                    decode_variable_type_is_resolved(var));

                // Now find all variables that are linked to this instance
                int linked_dynamic_count = 0;
                int linked_same_instance_count = 0;

                // Search all rules for variables linked to this instance
                for (size_t search_spec_idx = 0; search_spec_idx < sorted_container__occupied(p_rule_engine_inst->p_rule_engine_specifications); search_spec_idx++)
                {
                    uint32_t search_spec_key;
                    struct rule_engine__specification *search_spec;
                    sorted_container__iterate(p_rule_engine_inst->p_rule_engine_specifications, search_spec_idx, (void **)&search_spec, &search_spec_key);

                    if (search_spec == NULL)
                    {
                        continue;
                    }

                    for (struct expr_var *search_var = search_spec->expr.vars.head; search_var; search_var = search_var->next)
                    {
                        bool is_linked = false;
                        const char *link_type = "";

                        // Check for DDM2 dynamic variables that reference this instance
                        if (expr_var_type(search_var) == RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC)
                        {
                            // Use the direct association check function instead of string transformation
                            if (is_dynamic_parameter_associated_with_instance_variable(search_var, var))
                            {
                                is_linked = true;
                                link_type = "DYNAMIC_REFERENCE";
                                linked_dynamic_count++;
                            }
                        }

                        if (is_linked)
                        {
                            LOG(D, "    -> %s: Rule[%s:%d] var='%s' (type=%s, type_id=%d, id=0x%08X, value=%d)",
                                link_type,
                                search_spec->name, search_spec->id,
                                search_var->name,
                                decode_variable_type(search_var),
                                expr_var_type_id(search_var),
                                expr_var_id(search_var),
                                expr_var_value(search_var));
                        }
                    }
                }

                LOG(D, "    Summary: %d dynamic references, %d same instances across rules",
                    linked_dynamic_count, linked_same_instance_count);
                LOG(D, "");
            }
        }
    }

    // Only show orphaned variables in full debug mode or if no specific instance was provided
    if (show_all_instances)
    {
        // Summary section: Show dynamic variables without valid instances
        LOG(D, "--- ORPHANED DYNAMIC VARIABLES (no matching instance) ---");
        int orphaned_count = 0;

        for (size_t spec_idx = 0; spec_idx < sorted_container__occupied(p_rule_engine_inst->p_rule_engine_specifications); spec_idx++)
        {
            uint32_t spec_key;
            struct rule_engine__specification *spec;
            sorted_container__iterate(p_rule_engine_inst->p_rule_engine_specifications, spec_idx, (void **)&spec, &spec_key);

            if (spec == NULL)
            {
                continue;
            }

            for (struct expr_var *var = spec->expr.vars.head; var; var = var->next)
            {
                if (expr_var_type(var) == RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC)
                {
                    // Look for matching instance variable using ID-based comparison
                    bool instance_found = false;
                    uint32_t dynamic_var_class_id = DDM2_PARAMETER_CLASS(var->id);

                    for (size_t search_spec_idx = 0; search_spec_idx < sorted_container__occupied(p_rule_engine_inst->p_rule_engine_specifications); search_spec_idx++)
                    {
                        uint32_t search_spec_key;
                        struct rule_engine__specification *search_spec;
                        sorted_container__iterate(p_rule_engine_inst->p_rule_engine_specifications, search_spec_idx, (void **)&search_spec, &search_spec_key);

                        if (search_spec == NULL)
                        {
                            continue;
                        }

                        for (struct expr_var *search_var = search_spec->expr.vars.head; search_var; search_var = search_var->next)
                        {
                            if (expr_var_type(search_var) == RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE)
                            {
                                // Check if this instance matches the dynamic variable's class
                                if (is_dynamic_parameter_associated_with_instance_variable(var, search_var) ||
                                    DDM2_PARAMETER_CLASS(search_var->id) == dynamic_var_class_id)
                                {
                                    instance_found = true;
                                    break;
                                }
                            }
                        }
                        if (instance_found)
                        {
                            break;
                        }
                    }

                    if (!instance_found)
                    {
                        LOG(D, "    ORPHANED: Rule[%s:%d] var='%s' (class=0x%08X, type_id=%d)",
                            spec->name, spec->id, var->name, dynamic_var_class_id, expr_var_type_id(var));
                        orphaned_count++;
                    }
                }
            }
        }

        if (orphaned_count == 0)
        {
            LOG(D, "    No orphaned dynamic variables found");
        }
    }

    LOG(D, "=== END DDM2 INSTANCE ANALYSIS ===");
}

/**********************************************************
 * Dynamic resolution helper functions
 *********************************************************/
static void subscribe_rules_dynamic_variables(const struct rule_engine__specification *const specification, const struct expr_var *instance_variable, const struct expr_var *instance_variable_old_state)
{
    dynamic_variables_subscription(specification, instance_variable, instance_variable_old_state, true);
}

static void unsubscribe_rules_dynamic_variables(const struct rule_engine__specification *const specification, const struct expr_var *instance_variable, const struct expr_var *instance_variable_old_state)
{
    dynamic_variables_subscription(specification, instance_variable, instance_variable_old_state, false);
}

static void resolve_rules_dynamic_variables(const struct rule_engine__specification *const specification, struct expr_var *instance_variable, const struct expr_var *instance_variable_old_state)
{
    dynamic_variables_resolution(specification, instance_variable, instance_variable_old_state, RULE_ENGINE__VAR_TYPE_RESOLVED);
}

static void unresolve_rules_dynamic_variables(const struct rule_engine__specification *const specification, struct expr_var *instance_variable, const struct expr_var *instance_variable_old_state)
{
    dynamic_variables_resolution(specification, instance_variable, instance_variable_old_state, RULE_ENGINE__VAR_TYPE_UNRESOLVED);
}

static void resolve_dynamic_parameter_variable(struct expr_var *ddm2_dynamic_variable, uint32_t instance_id)
{
    // Set the instance variable ID to the class|instance
    // Use DDM2_PARAMETER_CLASS to always reset the instance ID before setting the new one
    expr_set_var_id(ddm2_dynamic_variable, DDM2_PARAMETER_BASE_INSTANCE(expr_var_id(ddm2_dynamic_variable)) | DDM2_PARAMETER_INSTANCE(instance_id));
    expr_set_var_type_is_resolved(ddm2_dynamic_variable, RULE_ENGINE__VAR_TYPE_RESOLVED);
}

static void unresolve_dynamic_parameter_variable(struct expr_var *ddm2_dynamic_variable)
{
    // Set the instance variable ID to the class|instance
    // Use DDM2_PARAMETER_CLASS to always reset the instance ID before setting the new one
    expr_set_var_id(ddm2_dynamic_variable, DDM2_PARAMETER_BASE_INSTANCE(expr_var_id(ddm2_dynamic_variable)) | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    expr_set_var_type_is_resolved(ddm2_dynamic_variable, RULE_ENGINE__VAR_TYPE_UNRESOLVED);
}

static void resolve_dynamic_instance_variable(struct expr_var *instance_variable, uint32_t instance_id)
{
    // Set the instance variable ID to the class|instance
    // Use DDM2_PARAMETER_CLASS to always reset the instance ID before setting the new one
    expr_set_var_id(instance_variable, DDM2_PARAMETER_CLASS(expr_var_id(instance_variable)) | DDM2_PARAMETER_INSTANCE(instance_id));
    expr_set_var_type_is_resolved(instance_variable, RULE_ENGINE__VAR_TYPE_RESOLVED);
}

static void unresolve_dynamic_instance_variable(struct expr_var *instance_variable)
{
    // Set the instance variable ID to the class|instance
    // Use DDM2_PARAMETER_CLASS to always reset the instance ID before setting the new one
    expr_set_var_id(instance_variable, DDM2_PARAMETER_CLASS(expr_var_id(instance_variable)) | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
    expr_set_var_type_is_resolved(instance_variable, RULE_ENGINE__VAR_TYPE_UNRESOLVED);
}

static bool is_dynamic_parameter_associated_with_instance_variable(const struct expr_var *dynamic_parameter, const struct expr_var *instance_variable)
{
    return ((DDM2_PARAMETER_CLASS_INSTANCE(expr_var_id(dynamic_parameter)) == expr_var_id(instance_variable)) &&
            (expr_var_type_id(dynamic_parameter) == expr_var_type_id(instance_variable)));
}

static bool is_the_same_dynamic_variable(const struct expr_var *dynamic_variable1, const struct expr_var *dynamic_variable2)
{
    // Check if the dynamic variables are the same by comparing their IDs and types
    return ((expr_var_id(dynamic_variable1)) == expr_var_id(dynamic_variable2) &&
            (expr_var_type_id(dynamic_variable1) == expr_var_type_id(dynamic_variable2)));
}

static void handle_dynamic_instance_resolution_state_change(struct rule_engine__specification *const specification, struct expr_var *instance_variable, const struct expr_var *instance_variable_old_state)
{
    LOG(D, "Rule[%s:%d]: Instance[%s] state changed to %s (class=0x%08X, instance=%d)",
        specification->name,
        specification->id,
        instance_variable->name,
        decode_variable_type_is_resolved(instance_variable),
        DDM2_PARAMETER_CLASS(expr_var_id(instance_variable)),
        DDM2_PARAMETER_INSTANCE_FIELD(expr_var_id(instance_variable)));

    if (expr_var_type_is_resolved(instance_variable) == RULE_ENGINE__VAR_TYPE_RESOLVED)
    {
        resolve_rules_dynamic_variables(specification, instance_variable, instance_variable_old_state);
        subscribe_rules_dynamic_variables(specification, instance_variable, instance_variable_old_state);
    }
    else
    {
        unsubscribe_rules_dynamic_variables(specification, instance_variable, instance_variable_old_state);
        unresolve_rules_dynamic_variables(specification, instance_variable, instance_variable_old_state);

        // Trigger only on unresolution, since the resolution is done trough subscribe_rules_dynamic_variables()
        trigger_dynamic_resolution_rules(specification, instance_variable);
    }

    resolve_rule_expressions(specification);

    // Debug only the affected instance and its dynamic variables
    log_dynamic_variables_relationships(specification->p_rule_engine_inst, instance_variable);
}

/**********************************************************
 * User function implementation
 *********************************************************/
/**
 * @brief Handles inventory-based instance resolution for DDM2 parameters
 *
 * This function manages the dynamic resolution and unresolution of DDM2 instance variables
 * based on inventory availability and user-defined callback logic. It supports two modes:
 *
 * **Relevance Filtering:**
 * - Only processes inventory updates that are relevant to the specific rule
 * - Relevant updates are those for the same class the rule is interested in, OR
 * - Updates for the class the rule is currently resolved to (for potential unresolution)
 * - This prevents unnecessary callback invocations and improves performance
 *
 **Callback Return Value Semantics (regardless of AVL=0 or AVL=1):**
 ** Return DDMP2_INVALID_INSTANCE**: Always unresolves the instance variable
 * Works for both AVL=0 (inventory unavailable) and AVL=1 (inventory available)
 * Forces unresolution regardless of current state or AVL flag
 *
 ** Return specific instance number**: Always resolves/maintains resolution
 * Works for both AVL=0 and AVL=1 scenarios
 * If currently unresolved: resolves to the returned instance
 * If currently resolved to same instance: maintains current resolution
 * If currently resolved to different instance: switches to returned instance
 * Overrides AVL flag completely - callback has final authority
 *
 * **Default Mode (no callback):**
 * - Uses DDMP2_INVENTORY_AVL flag to determine availability
 * - Only processes inventory for matching parameter classes
 * - When AVL=1: Resolves if currently unresolved
 * - When AVL=0: Unresolves if currently resolved (should be resolved if class matches)
 *
 * **Resolution Process:**
 * - Sets instance variable ID to inventory class|instance
 * - Marks variable as RESOLVED
 * - Calls resolve_rules_dynamic_variables() to resolve dependent variables
 * - Calls subscribe_rules_dynamic_variables() to subscribe to new parameters
 * - Updates rule expressions via resolve_rule_expressions()
 *
 * **Unresolution Process:**
 * - Sets instance variable ID to class|INVALID_INSTANCE
 * - Marks variable as UNRESOLVED
 * - Calls unresolve_rules_dynamic_variables() to unresolve dependent variables
 * - Calls unsubscribe_rules_dynamic_variables() to unsubscribe from parameters
 * - Updates rule expressions via resolve_rule_expressions()
 *
 * **Multi-Rule Support:**
 * - Each rule's instance variable is handled independently
 * - Same inventory class can resolve different instances for different rules only in User Callback Mode
 * - Callback receives rule-specific context for custom logic
 * - Irrelevant inventory updates are ignored to prevent unnecessary processing
 *
 * @param f Function context (unused)
 * @param args Function arguments: [instance_var, class_var]
 * @param c Rule engine context containing instance and rule information
 * @param expr_id Rule ID for callback and logging
 * @return 1 on success, 0 on failure
 */
static int32_t inventory(const struct expr_func *f, vec_expr_t *args, void *c, uint32_t expr_id)
{
    bool should_resolve_ddm2_dynamic_parameters = false;
    rule_engine_inst_t *p_inst = ((rule_engine_func_context_t *)c)->p_rule_engine_inst;
    struct rule_engine__specification *rule = sorted_container__access(p_inst->p_rule_engine_specifications, expr_id);

    struct expr ddm2_parameter_instance_expr = vec_nth(args, 0);
    struct expr ddm2_parameter_class_expr = vec_nth(args, 1);
    struct expr_var *gw0inv_local_var = expr_get_var_by_id_and_type(&rule->expr.vars, GW0INV, RULE_ENGINE__VAR_TYPE_LOCAL);
    TRUE_CHECK_RETURN0(gw0inv_local_var != NULL);
    struct expr_var *ddm2_parameter_instance_var = expr_get_var_by_id_and_type(&rule->expr.vars, *ddm2_parameter_instance_expr.param.var.id, RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    TRUE_CHECK_RETURN0(ddm2_parameter_instance_var != NULL);
    struct expr_var *ddm2_parameter_class_var = expr_get_var_by_id_and_type(&rule->expr.vars, *ddm2_parameter_class_expr.param.var.id, RULE_ENGINE__VAR_TYPE_DDM2);
    TRUE_CHECK_RETURN0(ddm2_parameter_class_var != NULL);

    uint32_t gw0inv_ddm2_class = DDM2_PARAMETER_CLASS_INSTANCE(expr_var_value(gw0inv_local_var));
    uint32_t ddm2_parameter_class = expr_var_id(ddm2_parameter_class_var);
    uint32_t current_instance = DDM2_PARAMETER_INSTANCE_FIELD(expr_var_id(ddm2_parameter_instance_var));
    current_instance = (current_instance != 0xFF ? current_instance : DDMP2_INVALID_INSTANCE);  // so we can properly compare with DDMP2_INVALID_INSTANCE

    uint32_t inventory_class = DDMP2_INVENTORY_CLASS(gw0inv_ddm2_class);

    // Check if this inventory update is relevant to this rule
    bool is_relevant_inventory = (inventory_class == DDM2_PARAMETER_CLASS(ddm2_parameter_class)) ||
                                 ((current_instance != DDMP2_INVALID_INSTANCE) &&
                                  (inventory_class == DDM2_PARAMETER_BASE_INSTANCE(expr_var_id(ddm2_parameter_instance_var))));

    // Shallow copy of the current state of the DDM2 parameter instance variable
    struct expr_var ddm2_instance_current_state = *ddm2_parameter_instance_var;

    // Has the callback been defined? If so, let the user decide how the inventory should be handled
    if (p_inst->inventory_user_cb != NULL)
    {
        // Call the callback with current instance information
        uint32_t desired_instance = p_inst->inventory_user_cb(rule->id, expr_var_value(gw0inv_local_var), current_instance);
        bool callback_wants_resolved = (desired_instance != DDMP2_INVALID_INSTANCE);
        bool currently_resolved = (current_instance != DDMP2_INVALID_INSTANCE);

        // Consistent rule: Always follow callback decision regardless of AVL flag
        if (callback_wants_resolved && !currently_resolved)
        {
            // Callback wants it resolved and it's currently unresolved - resolve it
            should_resolve_ddm2_dynamic_parameters = true;
            resolve_dynamic_instance_variable(ddm2_parameter_instance_var, desired_instance);
            LOG(D, "Rule[%s:%d]: Resolving DDM2 parameter instance[%s] to instance[%d] based on callback decision!",
                rule->name, rule->id, ddm2_parameter_instance_var->name, desired_instance);
        }
        else if (callback_wants_resolved && currently_resolved && (desired_instance != current_instance))
        {
            // Callback wants a different instance - switch to it
            should_resolve_ddm2_dynamic_parameters = true;
            resolve_dynamic_instance_variable(ddm2_parameter_instance_var, desired_instance);
            LOG(D, "Rule[%s:%d]: Switching DDM2 parameter instance[%s] from instance[%d] to instance[%d] based on callback decision!",
                rule->name, rule->id, ddm2_parameter_instance_var->name, current_instance, desired_instance);
        }
        else if (!callback_wants_resolved && currently_resolved)
        {
            // Callback wants it unresolved and it's currently resolved - unresolve it
            should_resolve_ddm2_dynamic_parameters = true;
            unresolve_dynamic_instance_variable(ddm2_parameter_instance_var);
            LOG(D, "Rule[%s:%d]: Unresolving DDM2 parameter instance[%s] (was instance[%d]) based on callback decision!",
                rule->name, rule->id, ddm2_parameter_instance_var->name, current_instance);
        }
        else
        {
            // No state change needed
            should_resolve_ddm2_dynamic_parameters = false;
            LOG(D, "Rule[%s:%d]: No state change needed for DDM2 parameter instance[%s] (callback_wants_resolved=%d, currently_resolved=%d, current_instance=%d, desired_instance=%d)",
                rule->name, rule->id, ddm2_parameter_instance_var->name, callback_wants_resolved, currently_resolved, current_instance, desired_instance);
        }
    }
    else if ((p_inst->inventory_user_cb == NULL) && is_relevant_inventory)  // callback not defined, use the default inventory handling
    {
        // Is/was available in the inventory?
        if (gw0inv_ddm2_class == ddm2_parameter_class)
        {
            // Is now available in the inventory?
            if (DDMP2_INVENTORY_AVL(expr_var_value(gw0inv_local_var)))
            {
                // is not resolved yet? Then resolve it
                if (expr_var_type_is_resolved(ddm2_parameter_instance_var) == RULE_ENGINE__VAR_TYPE_UNRESOLVED)
                {
                    should_resolve_ddm2_dynamic_parameters = true;
                    resolve_dynamic_instance_variable(ddm2_parameter_instance_var, DDM2_PARAMETER_INSTANCE_FIELD(ddm2_parameter_class));
                }
                else
                {
                    // if the type is already resolved, no action is needed
                    should_resolve_ddm2_dynamic_parameters = false;
                }
            }
            else  // No, it is not avaiable in the inventory
            {
                // is not resolved yet? Then you should not received AVL parameter with value 0
                if (expr_var_type_is_resolved(ddm2_parameter_instance_var) == RULE_ENGINE__VAR_TYPE_UNRESOLVED)
                {
                    should_resolve_ddm2_dynamic_parameters = false;

                    // should not end-up here, since gw0inv_ddm2_class == ddm2_parameter_class which means
                    // at some point it existed in the inventory. Didn't we resolve it earlier?
                    LOG(E, "Rule[%s:%d]: gw0inv_ddm2_class[%X] should have been resolved eariler!",
                        rule->name, rule->id, ddm2_parameter_class);
                }
                else
                {
                    should_resolve_ddm2_dynamic_parameters = true;
                    unresolve_dynamic_instance_variable(ddm2_parameter_instance_var);
                }
            }
        }
    }
    else
    {
        // Inventory update is not relevant to this rule - no action needed
        // This means that the rule is not interested in this inventory class
        should_resolve_ddm2_dynamic_parameters = false;
    }

    if (should_resolve_ddm2_dynamic_parameters == true)
    {
        handle_dynamic_instance_resolution_state_change(rule, ddm2_parameter_instance_var, &ddm2_instance_current_state);
    }
    else
    {
        LOG(D, "Rule[%s:%d]: DDM2 parameter instance[%s] should not (un)resolved!", rule->name, rule->id, ddm2_parameter_instance_var->name);
    }

    return 1;
}

/**
 * @brief Handles dynamic instance resolution based on linked classes data
 *
 * This function manages DDM2 instance variable resolution by analyzing linked classes
 * data from dynamic parameters. It provides a mechanism for resolving instances based
 * on relationships between different parameter classes rather than inventory availability.
 *
 * **Parameters:**
 * - instance_var: DDM2 instance variable to resolve/unresolve (RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE)
 * - ddm2_dynamic_parameter: Dynamic parameter containing linked classes data (RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC)
 * - ddm2_class: Target class to search for in linked classes in ddm2_dynamic_parameter (RULE_ENGINE__VAR_TYPE_DDM2)
 * - ddm2_class_position: Position index of the target class in linked classes array
 *
 * **Resolution Logic:**
 * - Retrieves linked classes data from the dynamic parameter subscription
 * - Searches for the target class at the specified position in the linked classes array
 * - If target class found at position: resolves instance variable to the found instance
 * - If target class not found or no linked data: unresolves instance variable
 * - Only changes state when necessary (unresolved→resolved or resolved→unresolved)
 *
 * **Use Cases:**
 * - Resolving instances based on parameter relationships(subscriptions) rather than inventory
 * - Supporting complex multi-parameter dependencies in rule logic
 * - Enabling position-based instance selection from linked parameter arrays
 *
 * **Difference from inventory():**
 * - Works with linked classes data instead of inventory availability
 * - Supports position-based selection from multiple linked instances
 *
 * @param f Function context (unused)
 * @param args Function arguments: [instance_var, ddm2_dynamic_parameter, ddm2_class, ddm2_class_position]
 * @param c Rule engine context containing instance and rule information
 * @param expr_id Rule ID for logging and context
 * @return 1 on success, 0 on failure
 */
static int32_t dyn_instance(const struct expr_func *f, vec_expr_t *args, void *c, uint32_t expr_id)
{
    bool should_resolve_ddm2_instance = false;
    uint32_t instance_resolved = DDMP2_INVALID_INSTANCE;
    rule_engine_inst_t *p_inst = ((rule_engine_func_context_t *)c)->p_rule_engine_inst;
    struct rule_engine__specification *rule = sorted_container__access(p_inst->p_rule_engine_specifications, expr_id);

    struct expr ddm2_parameter_instance = vec_nth(args, 0);
    struct expr ddm2_dynamic_parameter = vec_nth(args, 1);
    struct expr ddm2_parameter_class = vec_nth(args, 2);
    struct expr ddm2_parameter_class_position = vec_nth(args, 3);
    struct expr_var *ddm2_parameter_instance_var = expr_get_var_by_id_and_type(&rule->expr.vars, *ddm2_parameter_instance.param.var.id, RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
    struct expr_var *ddm2_dynamic_parameter_var = expr_get_var_by_id_and_type(&rule->expr.vars, *ddm2_dynamic_parameter.param.var.id, RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
    struct expr_var *ddm2_parameter_class_var = expr_get_var_by_id_and_type(&rule->expr.vars, *ddm2_parameter_class.param.var.id, RULE_ENGINE__VAR_TYPE_DDM2);

    // Shallow copy of the current state of the DDM2 parameter instance variable
    struct expr_var ddm2_instance_current_state = *ddm2_parameter_instance_var;

    ddmw_item_t *ddm2_dynamic_parameter_item = sorted_container__access(p_inst->p_rule_engine_ddm2_subscription_table, expr_var_id(ddm2_dynamic_parameter_var));
    if (ddm2_dynamic_parameter_item == NULL)
    {
        // Should trigger unresolution of the DDM2 parameter instance variable
        // since it was lost or it was not available in the first place
        should_resolve_ddm2_instance = true;
        unresolve_dynamic_instance_variable(ddm2_parameter_instance_var);
    }
    else
    {
        uint32_t ddm2_linked_classes[DDMW_ITEM_DATA_CAPACITY / sizeof((LINKEDCLASS_T *)0)->classes[0]];
        int ddm2_linked_classes_elements = ddm2_dynamic_parameter_item->size / sizeof((LINKEDCLASS_T *)0)->classes[0];

        TRUE_CHECK_RETURN0(ddm2_dynamic_parameter_item->size <= DDMW_ITEM_DATA_CAPACITY);
        ddmw_get_data(ddm2_dynamic_parameter_item, ddm2_linked_classes, ddm2_dynamic_parameter_item->size);

        int32_t ddm2_class_possition = ddm2_parameter_class_position.param.num.value;
        for (int i = 0; i < ddm2_linked_classes_elements; ++i)
        {
            if (expr_var_id(ddm2_parameter_class_var) == DDM2_PARAMETER_BASE_INSTANCE(ddm2_linked_classes[i]))
            {
                if (ddm2_class_possition == 0)
                {
                    LOG(D, "Rule[%s:%d]: DDM2 parameter class[%s] found at position[%d]!",
                        rule->name,
                        rule->id,
                        ddm2_parameter_class_var->name,
                        ddm2_parameter_class_position.param.var.value);

                    instance_resolved = DDM2_PARAMETER_INSTANCE_FIELD(ddm2_linked_classes[i]);
                    break;
                }
                else
                {
                    ddm2_class_possition--;  // Decrement the position, since we are looking for the class on 'ddm2_parameter_class_position' position
                }
            }
        }

        if (instance_resolved != DDMP2_INVALID_INSTANCE)
        {
            if (expr_var_type_is_resolved(ddm2_parameter_instance_var) == RULE_ENGINE__VAR_TYPE_UNRESOLVED)
            {
                should_resolve_ddm2_instance = true;
                resolve_dynamic_instance_variable(ddm2_parameter_instance_var, instance_resolved);
            }
            else
            {
                // if the type is already resolved, no action is needed
                should_resolve_ddm2_instance = false;
            }
        }
        else
        {
            if (expr_var_type_is_resolved(ddm2_parameter_instance_var) == RULE_ENGINE__VAR_TYPE_UNRESOLVED)
            {
                // Still does not exist in the linked classes list
                should_resolve_ddm2_instance = false;
            }
            else
            {
                should_resolve_ddm2_instance = true;
                unresolve_dynamic_instance_variable(ddm2_parameter_instance_var);
            }
        }
    }

    if (should_resolve_ddm2_instance == true)
    {
        handle_dynamic_instance_resolution_state_change(rule, ddm2_parameter_instance_var, &ddm2_instance_current_state);
    }
    else
    {
        LOG(D, "Rule[%s:%d]: DDM2 parameter instance[%s] should not (un)resolve!", rule->name, rule->id, ddm2_parameter_instance.param.var.name);
    }

    return 1;
}

static int32_t rule_func_timer_off(const struct expr_func *f, vec_expr_t *args, void *c, uint32_t expr_id)
{
    rule_engine_inst_t *p_inst = ((rule_engine_func_context_t *)c)->p_rule_engine_inst;
    struct rule_engine__timer *timer_instance = NULL;
    struct rule_engine__specification *rule = sorted_container__access(p_inst->p_rule_engine_specifications, expr_id);

    if (p_inst->rules_valid == false)
    {
        return 0;
    }

    if (args->len != 1)
    {
        LOG(E, "Rule[%s:%d]: Invalid number[%d!=1] of arguments!", rule->name, rule->id, args->len);
        p_inst->rules_valid = false;
        return 0;
    }

    struct expr timer_name = vec_nth(args, 0);
    if (timer_name.type != OP_VAR)
    {
        LOG(E, "Rule[%s:%d]: Invalid 'timer instance' type[%d]! Type supported: %d", rule->name, rule->id, timer_name.type, OP_VAR);
        p_inst->rules_valid = false;
        return 0;
    }

    // Get timer variable from rule's variable list
    struct expr_var *timer_variable = expr_get_var_by_id_and_type(&rule->expr.vars, *timer_name.param.var.id, RULE_ENGINE__VAR_TYPE_GLOBAL);
    if (timer_variable == NULL)
    {
        LOG(E, "Rule[%s:%d]: Timer instance variable[%s] does not exist in variables list.", rule->name, rule->id, timer_name.param.var.name);
        p_inst->rules_valid = false;
        return 0;
    }

    // Get timer instance from timers container, using the timer_variable id as key for the timer instance
    timer_instance = sorted_container__access(p_inst->p_rule_engine_set_timers, timer_variable->id);
    if (timer_instance == NULL)
    {
        LOG(E, "Rule[%s:%d]: Timer instance[%s] has not been created!", rule->name, rule->id, timer_name.param.var.name);
        p_inst->rules_valid = false;
        return 0;
    }

    // Mark timer_instance to SET_OFF, so we can stop the timer in rule_engine_update_timers() only
    // if all the triggered and executed rules are valid(rules_valid)
    timer_instance->state = RULE_ENGINE__TIMER_SET_OFF;

#if 0
    bool timer_exist = rule_engine_timer_off(p_inst, timer_instance, expr_id);
    if (timer_exist == false)
    {
        p_inst->rules_valid = false;
        return 0;
    }
#endif
    return 1;
}

static int32_t rule_func_timer_on(const struct expr_func *f, vec_expr_t *args, void *c, uint32_t expr_id)
{
    uint32_t time_ticks = 0;
    struct expr time_s;
    struct expr timer;
    struct expr_var *timer_variable = NULL;
    struct rule_engine__timer *timer_instance = NULL;
    rule_engine_inst_t *p_inst = ((rule_engine_func_context_t *)c)->p_rule_engine_inst;
    struct rule_engine__specification *rule = sorted_container__access(p_inst->p_rule_engine_specifications, expr_id);

    // Break execution of rules, if any of the previosly executed rules falied for some reason
    if (p_inst->rules_valid == false)
    {
        return 0;
    }

    // Timer function can have only 2 arguments: timer name(instance) and timer period
    if (args->len != 2)
    {
        LOG(E, "Rule[%s:%d]: Invalid number[%d!=2] of arguments!", rule->name, rule->id, args->len);
        p_inst->rules_valid = false;
        return 0;
    }

    timer = vec_nth(args, 0);   // get first argument of the function(timer instance)
    time_s = vec_nth(args, 1);  // get second argument of the function(timer period). time_s can only be OP_CONST or OP_VAR.

    // Get timer variable from rule's variable list
    timer_variable = expr_get_var_by_id_and_type(&rule->expr.vars, *timer.param.var.id, RULE_ENGINE__VAR_TYPE_GLOBAL);
    // timer_variable = expr_get_var_by_name(&rule->expr.vars, timer.param.var.name, strlen(timer.param.var.name));
    if (timer_variable == NULL)
    {
        LOG(E, "Rule[%s:%d]: Timer variable[%s] does not exist in rule's variables list!", rule->name, rule->id, timer.param.var.name);
        p_inst->rules_valid = false;
        return 0;
    }

    // Get timer instance from timers container, using the timer_variable id as key for the timer instance
    timer_instance = sorted_container__access(p_inst->p_rule_engine_set_timers, timer_variable->id);
    if (timer_instance == NULL)
    {
        LOG(E, "Rule[%s:%d]: Timer instance[%s] has not been created!", rule->name, rule->id, timer.param.var.name);
        p_inst->rules_valid = false;
        return 0;
    }

    time_ticks = rule_engine_get_timer_period_in_ticks(timer_instance->type, (uint32_t)expr_eval(&time_s));
    if (time_ticks == 0)
    {
        LOG(E, "Rule[%s:%d]: Invalid timer value[%u]", rule->name, rule->id, time_ticks);
        p_inst->rules_valid = false;
        return 0;
    }

    // Always update the period, since the same timer_instance can
    // be used from different rules, that sets to different time periods
    timer_instance->time_ticks = time_ticks;

    // Mark timer_instance to SET_ON, so we can start the timer in rule_engine_update_timers() only
    // if all the triggered and executed rules are valid(rules_valid)
    timer_instance->state = RULE_ENGINE__TIMER_SET_ON;

    return 1;
}

static int32_t rule_func_print(const struct expr_func *const f, vec_expr_t *args, void *c, uint32_t expr_id)
{
    struct rule_engine__specification *rule = sorted_container__access((((rule_engine_func_context_t *)c)->p_rule_engine_inst)->p_rule_engine_specifications, expr_id);

    LOG(I, "Rule[%s:%d]", rule->name, rule->id);
    RULE_ENGINE__PRINT_VECTOR_VARIABLES(args);

    return 1;
}

static int32_t rule_func_ref_variable(const struct expr_func *const f, vec_expr_t *args, void *c, uint32_t expr_id)
{
    rule_engine_inst_t *p_inst = (((rule_engine_func_context_t *)c)->p_rule_engine_inst);

    struct rule_engine__specification *rule = sorted_container__access(p_inst->p_rule_engine_specifications, expr_id);

    if (args->len == 0)
    {
        LOG(E, "Rule[%s:%d]: No arguments inserted!", rule->name, rule->id);
        p_inst->rules_valid = false;
        return p_inst->rules_valid;
    }
    struct expr var_name = vec_nth(args, 0);
    struct expr var_ref = vec_nth(args, 1);

    // Is the var_ref already a ddm item?
    // LOG(D, "Check for new ddmw (%s): 0x%x", var_name.param.var.name, *var_name.param.var.value);
    // LOG(D, "Check for new ddmw (%s): 0x%x", var_ref.param.var.name, *var_ref.param.var.value);

    ddmw_item_t *ddm2_var = sorted_container__access(p_inst->p_rule_engine_ddm2_subscription_table, *var_ref.param.var.value);

    if (ddm2_var)
    {
        // LOG(D, "Found ddm variable, 0x%x", ddm2_var->parameter);
    }
    else
    {
        struct expr_var *new_var = expr_get_var_by_name(&rule->expr.vars, var_name.param.var.name, strlen(var_name.param.var.name));
        if (new_var)
        {
            // LOG(D, "Found expr_var: %s: %d: %d, %d", new_var->name, new_var->id, new_var->type, new_var->value);
            //  Set to RULE_ENGINE__VAR_TYPE_DDM2 as we will now subscribe to it
            expr_set_var_type(new_var, RULE_ENGINE__VAR_TYPE_DDM2);
            expr_set_var_id(new_var, (uint32_t)(*var_ref.param.var.value));
            expr_set_var(new_var, 0);

            // Find trigger reference
            for (uint32_t var = 0; var < rule->trigger.n_params; var++)
            {
                if ((expr_var_type(rule->trigger.variables[var].variable) == RULE_ENGINE__VAR_TYPE_DDM2) &&
                    (expr_var_id(rule->trigger.variables[var].variable) == (uint32_t)(*var_ref.param.var.value)))
                {
                    // LOG(D, "Found trigger var: (%s) 0x%x", rule->trigger.variables[var].variable->name, rule->trigger.variables[var].parameter);
                    rule->trigger.variables[var].parameter = (uint32_t)(*var_ref.param.var.value);
                }
            }
        }
        ddm2_var = sorted_container__new(p_inst->p_rule_engine_ddm2_subscription_table, *var_ref.param.var.value);
        if (ddm2_var == NULL)
        {
            LOG(E, "Subscription container depth overflow!");
            return RULE_ENGINE__SUBS_DEPTH;
        }
        ddmw_add(p_inst->wrapper, ddm2_var, (uint32_t)(*var_ref.param.var.value), DDM2_PARAMETER_INSTANCE_FIELD(*var_ref.param.var.value));
        ddmw_subscribe(ddm2_var);
    }
    return 0;
}

/**
 * @brief Handles the 'set' rule function for assigning values to DDM2 parameters.
 *
 * This function processes each argument (expression) provided to the 'set' rule function.
 * For each argument, it determines the target DDM2 parameter (either from an assignment or direct variable),
 * evaluates the value to assign, and stores both the parameter ID and value in the rule engine's set parameter table.
 *
 * Assignment is only allowed if the parameter type is INT32_T (enforced during parsing).
 * If no available slot exists for a parameter, the rule is marked invalid.
 *
 * @param f        Pointer to the rule function descriptor.
 * @param args     Vector of expressions representing parameters to set.
 * @param c        Rule engine context.
 * @param expr_id  Expression/rule identifier.
 * @return         true if successful, false if rule is invalid or no slot is available.
 */
static int32_t rule_func_set(const struct expr_func *const f, vec_expr_t *args, void *c, uint32_t expr_id)
{
    rule_engine_inst_t *p_inst = (((rule_engine_func_context_t *)c)->p_rule_engine_inst);

    struct rule_engine__specification *rule = sorted_container__access(p_inst->p_rule_engine_specifications, expr_id);

    if (p_inst->rules_valid == false)
    {
        return 0;
    }

    int i;
    struct expr expression;
    vec_foreach(args, expression, i)
    {
        void *ddm2_data;
        size_t ddm2_data_size;
        int32_t ddm2_i32_data;
        // The expression can be either an assignment (OP_ASSIGN) or a direct variable (OP_VAR).
        // In both cases, we want to extract the variable part (the parameter to set).
        // - For OP_ASSIGN, the left-hand side (buf[0]) is always the variable to set.
        // - For OP_VAR, the expression itself is the variable.
        // Assignment is only allowed if the parameter type is INT32_T. This is checked during parsing in verify_pub_function().
        struct expr *ddm2_parameter_expr = expression.type == OP_ASSIGN ? &expression.param.op.args.buf[0] : &expression;
        struct expr_var *ddm2_parameter_var = expr_get_var_by_name(&rule->expr.vars, ddm2_parameter_expr->param.var.name, strlen(ddm2_parameter_expr->param.var.name));
        TRUE_CHECK_RETURN0(ddm2_parameter_var != NULL);
        ddmw_item_t *ddm2_item = sorted_container__access(p_inst->p_rule_engine_ddm2_subscription_table, expr_var_id(ddm2_parameter_var));
        TRUE_CHECK_RETURN0(ddm2_item != NULL);

        DDM2_TYPE_ENUM type;
        TRUE_CHECK_RETURN0(get_ddm2_type_for_write_action(ddm2_parameter_var, &type) != 0);
        if (type == DDM2_TYPE_INT32_T)
        {
            ddm2_i32_data = expr_eval(&expression);  // Evaluate the expression to get the value from expr_var
            ddm2_data = &ddm2_i32_data;
            ddm2_data_size = sizeof(ddm2_i32_data);
        }
        else
        {
            // For STRUCT/OTHER types, the data is already written in the
            // ddm2_item, so we just need to force update the 'modified' flag.
            ddm2_data = ddm2_item->data;
            ddm2_data_size = ddm2_item->size;
            // Mark the item as modified
            ddm2_item->modified = true;
        }
        ddmw_set_data(ddm2_item, ddm2_data, ddm2_data_size);
    }

    return 1;
}

static int32_t rule_func_if(const struct expr_func *const f, vec_expr_t *args, void *c, uint32_t expr_id)
{
    int arg = 0;
    if (vec_len(args) < 1)
    {
        return 0;
    }

    struct expr statement = vec_nth(args, arg++);
    if (expr_eval(&statement))
    {
        struct expr compound_statement;
        vec_foreach(args, compound_statement, arg)
        {
            expr_eval(&compound_statement);
        }
    }

    return 1;
}

/**
 * @brief Sets the value of a field in a DDM2 parameter's memory region.
 *
 * This function modifies the value of a field within a DDM2 parameter whose memory is allocated and managed
 * by the DDMW library. It only operates on parameters for which DDMW has provided memory, and does not
 * perform any allocation itself.
 *
 * NOTE: It is designed as the parameter will be published/set under the same rule engine
 * iteration using rule_func_set(pub) user function. Otherwise inbetween rule engine iterations, the parameter's
 * value will be override if new publish/set is recevied or it will produce partial updates of the ddm2 parameter
 * if subscription is received when the ddm2 is owned by the rule engine instance.
 *
 * If it is neccessary to update the DDM2 parameters of type struct/other trough more than one rule engine
 * iteration, it is recommended to use local/global variables as intermediate storage and then use rule_func_set_data()
 * when all data is ready to be set/published.
 *
 * NOTE: Offset of 0 indicates overwrite mode, while non-zero offsets allow appending data at specified positions.
 *
 * @param f Function context (unused)
 * @param args Function arguments:
 *                  - args[0]: DDM2 parameter variable (target parameter whose memory will be modified)
 *                  - args[1]: Offset (byte offset within the parameter's memory region)
 *                  - args[2]: Size (number of bytes to write)
 *                  - args[3]: Value (the value to set at the specified offset and size)
 * @param c Rule engine context
 * @param expr_id Rule ID for context
 * @return int32_t 1 on success, -1 on error
 *
 * @note All @p args (except the DDM2 parameter) are evaluated to their numeric values once the function is invoked.
 * This enables the use of constants, variables, computed expressions, or function results as arguments.
 *
 * @code
 * // Example: Write a 2-byte integer value from offset 0 in a DDM2 parameter
 * typedef struct ERROR_T
 *  {
 *      uint16_t error[0];    //error
 *  } PACKED ERROR_T;
 *
 * // Rules
 * "g_trigger: ddm_set_data(mccc0errst, 0, 2, 120)"
 * // args[0]: mccc0errst  // DDM2 parameter
 * // args[1]: 0           // offset in bytes
 * // args[2]: 2           // size in bytes
 * // args[3]: 120         // value to set
 *
 * // data in mccc0errst after rule executed:
 * // offset 0-1: 120
 * @endcode
 *
 * @code
 * // Example: Append next 2-byte integer value at offset 2 in a DDM2 parameter
 *
 * // Rules
 * "g_trigger: ddm_set_data(mccc0errst, 2, 2, 150)"
 * // args[0]: mccc0errst  // DDM2 parameter
 * // args[1]: 2           // offset in bytes
 * // args[2]: 2           // size in bytes
 * // args[3]: 150         // value to set
 *
 * // data in mccc0errst after both rules executed:
 * // offset 0-1: 120
 * // offset 2-3: 150
 * @endcode
 */
static int32_t rule_func_set_data(const struct expr_func *const f, vec_expr_t *args, void *c, uint32_t expr_id)
{
    int32_t status;
    rule_engine_inst_t *p_inst = ((rule_engine_func_context_t *)c)->p_rule_engine_inst;

    struct expr ddm2_parameter_expr = vec_nth(args, 0);
    struct expr offset_expr = vec_nth(args, 1);
    struct expr size_expr = vec_nth(args, 2);
    struct expr value_expr = vec_nth(args, 3);

    /* expr_eval as it can be OP_VAR, OP_CONST or any node type e.g.
     * logical/aritmethic operation, function etc... Let it evaluate
     * to a number */
    size_t offset = expr_eval(&offset_expr);
    size_t size = expr_eval(&size_expr);
    int32_t value = expr_eval(&value_expr);

    ddmw_item_t *ddm2_parameter_item = sorted_container__access(p_inst->p_rule_engine_ddm2_subscription_table, *ddm2_parameter_expr.param.var.id);
    TRUE_CHECK_RETURNX(-1, ddm2_parameter_item != NULL);

    // use ddmw storage as temporary buffer to set data, which will later be sent to DDM2 parameter trough rule_func_set
    if ((status = rule_engine_memory_accessor_set_data(ddm2_parameter_item->data, ddm2_parameter_item->maxsize, offset, size, value)))
    {
        if (offset == 0)
        {
            // Overwrite mode: reset size
            ddm2_parameter_item->size = size;
        }
        else
        {
            // Append mode: extend if needed
            if ((offset + size) > (size_t)ddm2_parameter_item->size)
            {
                ddm2_parameter_item->size = offset + size;
            }
            else
            {
                // No size change needed, overwrite within existing size
                // as we are writing in the middle of existing data
            }
        }
    }

    return status;
}

/**
 * @brief Gets the value of a field in a DDM2 parameter's memory region.
 *
 * This function retrieves the value from a DDM2 parameter whose memory is allocated and managed
 * by the DDMW library.
 *
 * @param f         Function context (unused)
 * @param args      Function arguments:
 *                  - args[0]: DDM2 parameter variable (target parameter whose memory will be read)
 *                  - args[1]: Offset (byte offset within the parameter's memory region)
 *                  - args[2]: Size (number of bytes to read)
 * @param c         Rule engine context
 * @param expr_id   Rule ID for context
 * @return int32_t  The value read, or -1 on error
 *
 * @note All @p args (except the DDM2 parameter) are evaluated to their numeric values once the function is invoked.
 * This enables the use of constants, variables, computed expressions, or function results as arguments.
 *
 * @code
 * // Example: Read a 2-byte integer value from offset 0 in a DDM2 parameter
 * typedef struct ERROR_T
 *  {
 *      uint16_t error[0];    //error
 *  } PACKED ERROR_T;
 *
 * // Rules
 * "mccc0errst: data = ddm_get_data(mccc0errst, 0, 2)"
 * // args[0]: mccc0errst  // DDM2 parameter
 * // args[1]: 0           // offset in bytes
 * // args[2]: 2           // size in bytes
 * @endcode
 */
static int32_t rule_func_get_data(const struct expr_func *const f, vec_expr_t *args, void *c, uint32_t expr_id)
{
    int32_t value;
    rule_engine_inst_t *p_inst = ((rule_engine_func_context_t *)c)->p_rule_engine_inst;

    struct expr ddm2_parameter_expr = vec_nth(args, 0);
    struct expr offset_expr = vec_nth(args, 1);
    struct expr size_expr = vec_nth(args, 2);

    /* expr_eval as it can be OP_VAR, OP_CONST or any node type e.g.
     * logical/aritmethic operation, function etc... Let it evaluate
     * to a number */
    size_t offset = expr_eval(&offset_expr);
    size_t size = expr_eval(&size_expr);

    ddmw_item_t *ddm2_parameter_item = sorted_container__access(p_inst->p_rule_engine_ddm2_subscription_table, *ddm2_parameter_expr.param.var.id);
    TRUE_CHECK_RETURNX(-1, ddm2_parameter_item != NULL);

    return rule_engine_memory_accessor_get_data(ddm2_parameter_item->data, ddm2_parameter_item->size, offset, size, &value) == -1 ? -1 : value;
}

/**
 * @brief Searches for a specific value within an array in a DDM2 parameter's memory region.
 *
 * This function searches for a specific value in an array managed by the DDMW library.
 * It automatically calculates the maximum number of elements that can be safely accessed
 * and iterates through them sequentially until the target value is found or all elements
 * are checked.
 *
 * @param f         Function context (unused)
 * @param args      Function arguments:
 *                  - args[0]: DDM2 parameter variable (target array)
 *                  - args[1]: Offset (byte offset to start search)
 *                  - args[2]: Stride (byte distance between consecutive array elements)
 *                  - args[3]: Element size (size in bytes of the field being searched)
 *                  - args[4]: Value (value to search for)
 *                  - args[5]: Trailing size (number of bytes at the end of array that should be
 *                             excluded from search e.g. if array is not last member in a struct)
 * @param c         Rule engine context
 * @param expr_id   Rule ID for context
 * @return int32_t  Index of found value, or -1 if not found
 *
 * @note All @p args (except the DDM2 parameter) are evaluated to their numeric values once the function is invoked.
 * This enables the use of constants, variables, computed expressions, or function results as arguments.
 *
 * @code
 * // Example: Find a 4-byte integer from offset 0(mintemp) and 4(maxtemp) in mccc0ctemprng
 * typedef struct TEMPRANGEARR_T
 *    {
 *        struct TEMP_RANGE
 *        {
 *            int32_t mintemp;    //^C
 *            int32_t maxtemp;    //^C
 *        } temp_range[0];
 *    } PACKED TEMPRANGEARR_T;
 *
 * // Rules
 * "mccc0ctemprng: min = ddm_find_array_value(mccc0ctemprng, 0, 8, 4, 20, 0)"
 * "mccc0ctemprng: max = ddm_find_array_value(mccc0ctemprng, 4, 8, 4, 30, 0)"
 * // args[0]: mccc0ctemprng   // DDM2 parameter (target array)
 * // args[1]: 0 or 4          // offset to start (0 for mintemp, 4 for maxtemp)
 * // args[2]: 8               // stride (struct size)
 * // args[3]: 4               // element size
 * // args[4]: 20 or 30        // value to search for (20 for mintemp, 30 for maxtemp)
 * // args[5]: 0               // trailing size (0 as there are no additional structure members after 'temp_range')
 * @endcode
 */
static int32_t rule_func_find_array_value(const struct expr_func *const f, vec_expr_t *args, void *c, uint32_t expr_id)
{
    rule_engine_inst_t *p_inst = ((rule_engine_func_context_t *)c)->p_rule_engine_inst;

    struct expr ddm2_parameter_expr = vec_nth(args, 0);
    struct expr offset_expr = vec_nth(args, 1);
    struct expr stride_expr = vec_nth(args, 2);
    struct expr element_size_expr = vec_nth(args, 3);
    struct expr value_expr = vec_nth(args, 4);
    struct expr trailing_size_expr = vec_nth(args, 5);

    /* expr_eval as it can be OP_VAR, OP_CONST or any node type e.g.
     * logical/aritmethic operation, function etc... Let it evaluate
     * to a number */
    int32_t value = expr_eval(&value_expr);
    size_t offset = expr_eval(&offset_expr);
    size_t stride = expr_eval(&stride_expr);
    size_t element_size = expr_eval(&element_size_expr);
    size_t trailing_size = expr_eval(&trailing_size_expr);

    ddmw_item_t *ddm2_parameter_item = sorted_container__access(p_inst->p_rule_engine_ddm2_subscription_table, *ddm2_parameter_expr.param.var.id);
    TRUE_CHECK_RETURNX(-1, ddm2_parameter_item != NULL);

    return rule_engine_memory_accessor_find_value(ddm2_parameter_item->data, ddm2_parameter_item->size, offset, stride, element_size, value, trailing_size);
}

/**
 * @brief Gets the number of elements in an array within a DDM2 parameter's memory region.
 *
 * This function calculates the length of an array managed by the DDMW library, based on the total
 * memory size, a starting offset, and the stride (size of each array element). It is designed for
 * C99-compliant usage with flexible arrays positioned at the end of a struct. It only operates on
 * parameters for which DDMW has provided memory.
 *
 * @param f         Function context (unused)
 * @param args      Function arguments:
 *                  - args[0]: DDM2 parameter variable (target array)
 *                  - args[1]: Offset (byte offset to start of array)
 *                  - args[2]: Stride (size in bytes of each array element)
 * @param c         Rule engine context
 * @param expr_id   Rule ID for context
 * @return int32_t  Number of elements in the array, or -1 on error
 *
 * @note All @p args (except the DDM2 parameter) are evaluated to their numeric values once the function is invoked.
 * This enables the use of constants, variables, computed expressions, or function results as arguments.
 *
 * @code
 * // Example: Get the number of elements in a temperature range array
 * typedef struct TEMPRANGEARR_T
 * {
 *     struct TEMP_RANGE
 *     {
 *         int32_t mintemp;
 *         int32_t maxtemp;
 *     } temp_range[0];
 * } PACKED TEMPRANGEARR_T;
 *
 * // Rules
 * "mccc0ctemprng: len = ddm_get_array_length(mccc0ctemprng, 0, 8)"
 * // args[0]: mccc0ctemprng   // DDM2 parameter (target array)
 * // args[1]: 0               // offset to start of array
 * // args[2]: 8               // stride (struct size)
 * @endcode
 */
static int32_t rule_func_get_array_length(const struct expr_func *const f, vec_expr_t *args, void *c, uint32_t expr_id)
{
    rule_engine_inst_t *p_inst = ((rule_engine_func_context_t *)c)->p_rule_engine_inst;

    struct expr ddm2_parameter_expr = vec_nth(args, 0);
    struct expr offset_expr = vec_nth(args, 1);
    struct expr stride_expr = vec_nth(args, 2);

    /* expr_eval as it can be OP_VAR, OP_CONST or any node type e.g.
     * logical/aritmethic operation, function etc... Let it evaluate
     * to a number */
    size_t offset = expr_eval(&offset_expr);
    size_t stride = expr_eval(&stride_expr);

    ddmw_item_t *ddm2_parameter_item = sorted_container__access(p_inst->p_rule_engine_ddm2_subscription_table, *ddm2_parameter_expr.param.var.id);
    TRUE_CHECK_RETURNX(-1, ddm2_parameter_item != NULL);

    return rule_engine_memory_accessor_get_array_length(ddm2_parameter_item->size, offset, stride);
}

/**********************************************************
 * Rule engine core
 *********************************************************/
static void rule_engine_sens_list_variable_set_updated(struct rule_engine__sensitivity_list *variable, bool is_updated)
{
    variable->updated = (is_updated == true) ? 1 : 0;
}

static bool rule_engine_sens_list_variable_is_updated(const struct rule_engine__sensitivity_list *const variable)
{
    return (variable->updated == 1) ? true : false;
}

static void rule_engine_sens_list_variable_set_initialized(struct rule_engine__sensitivity_list *variable, bool is_initialized)
{
    variable->initialized = (is_initialized == true) ? 1 : 0;
}

static bool rule_engine_sens_list_variable_is_initialized(const struct rule_engine__sensitivity_list *const variable)
{
    return (variable->initialized == 1) ? true : false;
}

// Checks if a variable is in the sensitivity list
static const struct rule_engine__sensitivity_list *rule_engine_sens_list_contains_expr_var(const struct rule_engine__sensitivity_list *const sens_list, size_t sens_list_elements, const struct expr_var *var)
{
    for (size_t i = 0; i < sens_list_elements; i++)
    {
        if (sens_list[i].variable == var)
        {
            return &sens_list[i];
        }
    }
    return NULL;
}

static void rule_engine_update_variable_value(struct expr_var_list *updating_variables, struct expr_var *variable)
{
    struct expr_var *update_var = expr_get_var_by_id_and_type(updating_variables, expr_var_id(variable), expr_var_type(variable));

    if (update_var)
    {
        expr_set_var(update_var, variable->value);
    }
}

static void update_ddm2_variables(rule_engine_inst_t *const p_rule_engine_inst, uint32_t parameter, int32_t value)
{
    for (size_t spec = 0; spec < sorted_container__occupied(p_rule_engine_inst->p_rule_engine_specifications); spec++)
    {
        uint32_t spec_key;
        struct rule_engine__specification *specification;

        sorted_container__iterate(p_rule_engine_inst->p_rule_engine_specifications, spec, (void **)&specification, &spec_key);

        // Update variable value
        struct expr_var *variable = NULL;
        if ((variable = expr_get_var_by_id_and_type(&specification->expr.vars, parameter, RULE_ENGINE__VAR_TYPE_DDM2)))
        {
            LOG(D, "DDM2 variable found (%s) 0x%x", variable->name, parameter);
            expr_set_var(variable, value);

            update_trigger_section(specification, variable, value, RULE_ENGINE__VAR_TYPE_DDM2);
        }
        else if ((variable = expr_get_var_by_id_and_type(&specification->expr.vars, parameter, RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC)))
        {
            // If variable is not found, try to find it as a dynamic variable
            LOG(D, "DDM2 dynamic variable found (%s) 0x%x", variable->name, parameter);
            if (expr_var_type_is_resolved(variable) == RULE_ENGINE__VAR_TYPE_RESOLVED)
            {
                expr_set_var(variable, value);

                update_trigger_section(specification, variable, value, RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
            }
            else
            {
                // We should never end up here, since the dynamic variable should be resolved
                LOG(E, "DDM2 dynamic variable[%s] is not resolved, cannot update value to %d!", variable->name, value);
            }
        }
        else
        {
            // continue
        }
    }
}

static void dynamic_variables_subscription(const struct rule_engine__specification *const specification, const struct expr_var *instance_variable, const struct expr_var *instance_variable_old_state, uint8_t subscription)
{
    TRUE_CHECK_RETURN((expr_var_type(instance_variable) == RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE));

    for (size_t index = 0; index < sorted_container__occupied(specification->p_rule_engine_inst->p_rule_engine_specifications); index++)
    {
        uint32_t spec_key;
        struct rule_engine__specification *spec;

        sorted_container__iterate(specification->p_rule_engine_inst->p_rule_engine_specifications, index, (void **)&spec, &spec_key);
        if (specification != spec)
        {
            for (struct expr_var *v = spec->expr.vars.head; v; v = v->next)
            {
                // Check if the variable is a dynamic variable and if it mathes the class and instance id
                if ((expr_var_type(v) == RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC))
                {
                    const struct expr_var *instance = subscription == true ? instance_variable : instance_variable_old_state;
                    if (is_dynamic_parameter_associated_with_instance_variable(v, instance))
                    {
                        if (subscription == true)  // subscribe
                        {
                            setup_ddm2_parameter(spec, v);
                            LOG(D, "Rule[%s:%d]: Dynamic variable[%s] registered to DDM2 parameter[%d] with instance id[%d].",
                                spec->name, spec->id, v->name, DDM2_PARAMETER_BASE_INSTANCE(expr_var_id(v)), DDM2_PARAMETER_INSTANCE_FIELD(expr_var_id(v)));
                        }
                        else
                        {
                            remove_ddm2_parameter(spec, v);
                            LOG(D, "Rule[%s:%d]: Dynamic variable[%s] unregistered from DDM2 parameter[%d] with instance id[%d].",
                                spec->name, spec->id, v->name, DDM2_PARAMETER_BASE_INSTANCE(expr_var_id(v)), DDM2_PARAMETER_INSTANCE_FIELD(expr_var_id(v)));
                        }
                    }
                    else
                    {
                        // This dynamic variable relates to another instance variable, so do not subscribe/unsubscribe it
                        // Dynamic variable type does not match instance variable type or class instance does not match instance variable class
                    }
                }
            }
        }
    }
}

static void dynamic_variables_resolution(const struct rule_engine__specification *const specification, struct expr_var *instance_variable, const struct expr_var *instance_variable_old_state, uint8_t resolution_type)
{
    TRUE_CHECK_RETURN((expr_var_type(instance_variable) == RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE));

    for (size_t index = 0; index < sorted_container__occupied(specification->p_rule_engine_inst->p_rule_engine_specifications); index++)
    {
        uint32_t spec_key;
        struct rule_engine__specification *spec;

        sorted_container__iterate(specification->p_rule_engine_inst->p_rule_engine_specifications, index, (void **)&spec, &spec_key);
        if (specification != spec)
        {
            struct expr_var *v = NULL;
            for (v = spec->expr.vars.head; v; v = v->next)
            {
                // Check if the variable is a dynamic variable and if it matches the parameter class id
                if (expr_var_type(v) == RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC)
                {
                    // Is the type of the variable matching the instance variable?
                    if (is_dynamic_parameter_associated_with_instance_variable(v, instance_variable_old_state))
                    {
                        if (resolution_type == RULE_ENGINE__VAR_TYPE_UNRESOLVED)
                        {
                            // Unresolve the dynamic variable
                            unresolve_dynamic_parameter_variable(v);
                        }
                        else
                        {
                            // Resolve the dynamic variable
                            resolve_dynamic_parameter_variable(v, DDM2_PARAMETER_INSTANCE_FIELD(expr_var_id(instance_variable)));
                        }
                    }
                    else
                    {
                        // This dynamic variable relates to another instance variable, so do not resolve it.
                        // Dynamic variable type does not match instance variable type or class instance does not match instance variable class,
                    }
                }
            }
        }
    }
}

static void trigger_dynamic_resolution_rules(const struct rule_engine__specification *const specification, struct expr_var *instance_variable)
{
    TRUE_CHECK_RETURN((expr_var_type(instance_variable) == RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE));

    for (size_t index = 0; index < sorted_container__occupied(specification->p_rule_engine_inst->p_rule_engine_specifications); index++)
    {
        uint32_t spec_key;
        struct rule_engine__specification *spec;

        sorted_container__iterate(specification->p_rule_engine_inst->p_rule_engine_specifications, index, (void **)&spec, &spec_key);
        if (specification != spec)
        {
            switch (spec->expr.type.type)
            {
            case RULE_ENGINE__EXPRESSION_TYPE_INSTANCE_RESOLUTION_DYN_INST:
            {
                struct expr_var *v = NULL;
                for (v = spec->expr.vars.head; v; v = v->next)
                {
                    // Check if the variable is a dynamic variable and if it matches the parameter class id
                    if ((expr_var_type(v) == RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC))
                    {
                        // Is the type of the variable associated with the instance variable?
                        if (is_dynamic_parameter_associated_with_instance_variable(v, instance_variable))
                        {
                            update_trigger_section(spec, v, expr_var_value(v), RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
                            LOG(D, "Rule[%s:%d]: Dynamic variable[%s] triggered for resolution.",
                                spec->name, spec->id, v->name);
                        }
                    }
                }
                break;
            }
            case RULE_ENGINE__EXPRESSION_TYPE_INSTANCE_RESOLUTION_INV:
            case RULE_ENGINE__EXPRESSION_TYPE_STANDARD:
                break;
            default:
                break;
            }
        }
    }
}

static void resolve_rule_expression(struct rule_engine__specification *specification)
{
    bool is_resolved = true;
    switch (specification->expr.type.type)
    {
    case RULE_ENGINE__EXPRESSION_TYPE_INSTANCE_RESOLUTION_INV:
        is_resolved = true;  // Inventory resolution is always resolved as GW0 inventory is always available
        break;
    case RULE_ENGINE__EXPRESSION_TYPE_INSTANCE_RESOLUTION_DYN_INST:
        is_resolved = true;  // Always resolved as dyn_inst has to be triggered to reoslve/unresolve DDM2 parameters
        break;
    case RULE_ENGINE__EXPRESSION_TYPE_STANDARD:
        for (struct expr_var *v = specification->expr.vars.head; v; v = v->next)
        {
            if ((expr_var_type_is_resolved(v) == RULE_ENGINE__VAR_TYPE_UNRESOLVED))
            {
                is_resolved = false;
                break;
            }
        }
    default:
        break;
    }

    specification->resolved = is_resolved;
}

static void resolve_rule_expressions(struct rule_engine__specification *specification)
{
    for (size_t index = 0; index < sorted_container__occupied(specification->p_rule_engine_inst->p_rule_engine_specifications); index++)
    {
        uint32_t spec_key;
        struct rule_engine__specification *spec;

        sorted_container__iterate(specification->p_rule_engine_inst->p_rule_engine_specifications, index, (void **)&spec, &spec_key);
        if (specification != spec)
        {
            resolve_rule_expression(spec);
        }
    }
}

static bool update_rules_variables_and_trigger_section(const struct rule_engine__specification *const specification)
{
    bool is_specification_updated = false;

    for (struct expr_var *v = specification->expr.vars.head; v; v = v->next)
    {
        if ((expr_var_type(v) != RULE_ENGINE__VAR_TYPE_LOCAL) &&
            (expr_var_type(v) != RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE))
        {
            if (expr_var_updated(v))
            {
                for (size_t s = 0; s < sorted_container__occupied(specification->p_rule_engine_inst->p_rule_engine_specifications); s++)
                {
                    uint32_t sspec_key;
                    struct rule_engine__specification *specs;

                    sorted_container__iterate(specification->p_rule_engine_inst->p_rule_engine_specifications, s, (void **)&specs, &sspec_key);

                    if (specs != specification)
                    {
                        rule_engine_update_variable_value(&specs->expr.vars, v);

                        bool is_updated = update_trigger_section(specs, v, v->value, RULE_ENGINE__VAR_TYPE_GLOBAL);
                        if (is_specification_updated == false)
                        {
                            is_specification_updated = is_updated;
                        }
                    }
                }
            }
        }
    }

    return is_specification_updated;
}

static void reset_rules_execution(rule_engine_inst_t *const p_rule_engine_inst)
{
    for (size_t spec = 0; spec < sorted_container__occupied(p_rule_engine_inst->p_rule_engine_specifications); spec++)
    {
        uint32_t spec_key;
        struct rule_engine__specification *specification;

        sorted_container__iterate(p_rule_engine_inst->p_rule_engine_specifications, spec, (void **)&specification, &spec_key);
        specification->execute = false;
    }
}

static bool ddm2_parameter_updated(rule_engine_inst_t *const p_rule_engine_inst)
{
    bool ddm2_param_updated = false;

    for (size_t st = 0; st < sorted_container__occupied(p_rule_engine_inst->p_rule_engine_ddm2_subscription_table); st++)
    {
        uint32_t st_key;
        ddmw_item_t *item;

        sorted_container__iterate(p_rule_engine_inst->p_rule_engine_ddm2_subscription_table, st, (void **)&item, &st_key);

        if (ddmw_is_updated(item))
        {
            // LOG(D, "Updated: 0x%x : %d", item->parameter, ddmw_get_i32(item));
            ddm2_param_updated = true;
            update_ddm2_variables(p_rule_engine_inst, item->parameter, ddmw_get_i32(item));
        }
    }

    return ddm2_param_updated;
}

void rule_engine__evaluate_rules(rule_engine_inst_t *const p_rule_engine_inst)
{
    // Execute all the rules whose trigger section is updated.
    // The order of how the rules are updated
    while (execute_rules(p_rule_engine_inst))
        ;

    // Start timers that are in RULE_ENGINE__TIMER_SET_ON state
    // and turn off timers that in RULE_ENGINE__TIMER_SET_OFF state.
    rule_engine_update_timers(p_rule_engine_inst);

    // Reset rules execute flag, so they can be triggered again
    // when new ddm2 parameter or a timer event is provided
    reset_rules_execution(p_rule_engine_inst);

    // reset validation flag to prepare it for next iteration
    p_rule_engine_inst->rules_valid = true;
}

bool rule_engine__evaluate_timers(rule_engine_inst_t *const p_rule_engine_inst, const void *const data, uint32_t data_size)
{
    bool is_trigger_section_updated = false;
    struct rule_engine__timer_data *timer_data;
    struct rule_engine__timer *timer_instance;

    TRUE_CHECK_RETURN0((data != NULL));
    TRUE_CHECK_RETURNX(is_trigger_section_updated, data_size == sizeof(struct rule_engine__timer_data));

    // generic data frame forwarded from the timer callback function
    // provides the RTOS Timer handle that can be used to get rule engine's timer instance
    timer_data = (struct rule_engine__timer_data *)data;

    // Get timer instace
    timer_instance = sorted_container__access(p_rule_engine_inst->p_rule_engine_set_timers, timer_data->instance->id);
    if (timer_instance == NULL)
    {
        LOG(E, "Timer instance does not exist!");
        return is_trigger_section_updated;
    }

    // Same timer instance can be used by one or more rules. The timer can be started/restarted/reset no matter
    // of the state of timer(active or off) by calling the timer_on() function.
    // Check the current status of the RTOS timer. If the timer has not been started in the
    // time between the callback function has been triggered(which set the timer to !tmrSTATUS_IS_ACTIVE state)
    // that provides the event to trigger this function and the execution of this function, then the timer is off.
    // Set timer instance value to RULE_ENGINE__TIMER_OFF, so it's value can be used to generate additional logic in the rules.
    // e.g.
    //  "<trig_variable> : timer_on(g_timer_on, 5)"
    //   ....
    //  "<trig_variable1> : if(g_timer_on == 0, <do_something>)"
    if (xTimerIsTimerActive(timer_instance->timer_handle) == RULE_ENGINE__TIMER_OFF)
    {
        rule_engine_update_timer_instance_state_and_value(timer_instance, RULE_ENGINE__TIMER_OFF);
    }

    // Update trigger section of the rules that should be trriggered when timer kiks off
    for (size_t s = 0; s < sorted_container__occupied(p_rule_engine_inst->p_rule_engine_specifications); s++)
    {
        uint32_t spec_key;
        struct rule_engine__specification *specs;

        sorted_container__iterate(p_rule_engine_inst->p_rule_engine_specifications, s, (void **)&specs, &spec_key);

        bool is_updated = update_trigger_section(specs, timer_data->instance, 1, RULE_ENGINE__VAR_TYPE_GLOBAL);
        if (is_trigger_section_updated == false)
        {
            is_trigger_section_updated = is_updated;
        }
    }

    LOG(I, "Timer[%s] fired!", timer_instance->instance->name);
    return is_trigger_section_updated;
}

bool rule_engine__evaluate_ddm2_parameters(rule_engine_inst_t *const p_rule_engine_inst)
{
    return ddm2_parameter_updated(p_rule_engine_inst);
}

/*! \brief Determines whether the rule should be triggered

    Determines whether the rules should be triggered, depending on the mask type
    If the mask type is RULE_ENGINE__MASK_CONSTRAINT_OR

    \param specification    rule specification, which parameters should be verified if
                            they have been updated while evaluating the rules

    \retval true    RULE_ENGINE__MASK_CONSTRAINT_OR, if any of the variables is updated
    \retval true    RULE_ENGINE__MASK_CONSTRAINT_AND, if all of the varaibles are updated
    \retval false   if any of the requried variables in the variables array is not updated.
*/
static bool trigger_rule(const struct rule_engine__specification *const specification)
{
    uint32_t mask = 0;

    // set mask bit for each variable that has been updated. Bit location
    // corresponds with the index number of the parameter in the trigger's
    // variable array. trigger.variables[0] is the LSB.
    for (uint32_t var = 0; var < specification->trigger.n_params; var++)
    {
        if (rule_engine_sens_list_variable_is_updated(&specification->trigger.variables[var]) == true)
        {
            RULE_ENGINE__SET_MASK(mask, var);
        }
    }

    if (specification->trigger.mask_type == RULE_ENGINE__MASK_CONSTRAINT_OR)
    {
        // if any of the variables has been updated
        if (mask > 0)
        {
            return true;
        }
    }
    else
    {
        // if all of the variables has been updated
        if (mask == specification->trigger.mask)
        {
            return true;
        }
    }

    return false;
}

static bool execute_rules(rule_engine_inst_t *const p_rule_engine_inst)
{
    bool is_any_specifications_updated = false;

    for (size_t spec = 0; spec < sorted_container__occupied(p_rule_engine_inst->p_rule_engine_specifications); spec++)
    {
        uint32_t spec_key;
        struct rule_engine__specification *specification;

        sorted_container__iterate(p_rule_engine_inst->p_rule_engine_specifications, spec, (void **)&specification, &spec_key);

        switch (specification->expr.type.type)
        {
        case RULE_ENGINE__EXPRESSION_TYPE_INSTANCE_RESOLUTION_INV:
        case RULE_ENGINE__EXPRESSION_TYPE_INSTANCE_RESOLUTION_DYN_INST:
            // Not executed as part of the standard rule flow.
            break;
        case RULE_ENGINE__EXPRESSION_TYPE_STANDARD:
            if ((specification->resolved == true) && trigger_rule(specification))
            {
                // execute_rules() function will be called as long as there is at least one specification(rule)
                // which trigger section has been updated. 'execute' flag helps to detect that the specification has
                // been already executed, so it will not allow an execution of the same rule more than once in
                // the same iterration of execute_rules() function. Executing the same rule in the same iterration
                // might lead to recursive calls between two rules, which will lead to infinite calls between those
                // two rules.
                if (specification->execute == false)
                {
                    // evaluate the rule expression
                    expr_eval(specification->expr.rule);
                    specification->execute = true;

                    // update trigger sections and variables value of all the rules that
                    // has the same GLOBAL/DDM2 variables in their definition, as the ones
                    // updated during the expr_eval() of the current 'specification'.
                    bool is_updated = update_rules_variables_and_trigger_section(specification);
                    if (is_any_specifications_updated == false)
                    {
                        is_any_specifications_updated = is_updated;
                    }

                    // clear trigger section for current rule
                    clear_trigger_section(specification);
                }
            }
            break;
        default:
            break;
        }
    }

    return is_any_specifications_updated;
}

static int set_user_functions_context(const struct rule_engine__specification *specification, struct expr *func_expr)
{
    int status = RULE_ENGINE__OK;
    TRUE_CHECK_RETURNX((func_expr->param.func.context != NULL), RULE_ENGINE__INV_FUNC_CONFIG);

    // Set the context for each function node, assign the rule engine instance
    rule_engine_func_context_t *func_context = (rule_engine_func_context_t *)func_expr->param.func.context;
    func_context->p_rule_engine_inst = (rule_engine_inst_t *)specification->p_rule_engine_inst;

    return status;
}

/**
 * @brief Sets up all user function nodes in a rule's AST
 *
 * Identifies all function nodes in the rule's expression tree using AST traversal,
 * then configures each function with proper context and performs validation of
 * specific function types (timer_on, inventory, dyn_instance, etc.).
 *
 * @param specification Pointer to the rule specification containing the AST to process
 * @return RULE_ENGINE__OK if successful, otherwise error code
 */
static int setup_user_functions(struct rule_engine__specification *specification)
{
    int status = RULE_ENGINE__OK;
    vec_expr_ptr_t func_vector = vec_init();

    // Get all user function pointers (original nodes) used in a rule
    expr_get_node_for_type_from_ast(specification->expr.rule, OP_FUNC, &func_vector);

    for (int i = 0; i < func_vector.len; i++)
    {
        struct expr *expression = func_vector.buf[i];

        // double check if all expressions in func_vector are actually function expressions
        if (expression->type == OP_FUNC)
        {
            status = set_user_functions_context(specification, expression);
            if (status != RULE_ENGINE__OK)
            {
                LOG(E, "Rule[%s:%d]: Invalid function[%s] context!", specification->name, specification->id, expression->param.func.f->name);
                break;
            }

            if (strings_equal("timer_on", strlen("timer_on"), expression->param.func.f->name, strlen(expression->param.func.f->name)) == true)
            {
                status = verify_and_create_timer(specification, expression, RULE_ENGINE__RELATIVE_TIMER);
            }
            else if (strings_equal("timer_abs_on", strlen("timer_abs_on"), expression->param.func.f->name, strlen(expression->param.func.f->name)) == true)
            {
                status = verify_and_create_timer(specification, expression, RULE_ENGINE__ABSOLUTE_TIMER);
            }
            else if (strings_equal("inventory", strlen("inventory"), expression->param.func.f->name, strlen(expression->param.func.f->name)) == true)
            {
                status = verify_inventory_function(specification, expression);
            }
            else if (strings_equal("dyn_instance", strlen("dyn_instance"), expression->param.func.f->name, strlen(expression->param.func.f->name)) == true)
            {
                status = verify_dyn_instance_function(specification, expression);
            }
            else if (strings_equal("pub", strlen("pub"), expression->param.func.f->name, strlen(expression->param.func.f->name)) == true)
            {
                status = verify_pub_function(specification, expression);
            }
            else if (strings_equal("ddm_get_data", strlen("ddm_get_data"), expression->param.func.f->name, strlen(expression->param.func.f->name)) == true)
            {
                status = verify_ddm_get_data_function(specification, expression);
            }
            else if (strings_equal("ddm_set_data", strlen("ddm_set_data"), expression->param.func.f->name, strlen(expression->param.func.f->name)) == true)
            {
                status = verify_ddm_set_data_function(specification, expression);
            }
            else if (strings_equal("ddm_get_array_length", strlen("ddm_get_array_length"), expression->param.func.f->name, strlen(expression->param.func.f->name)) == true)
            {
                status = verify_ddm_get_array_length_function(specification, expression);
            }
            else if (strings_equal("ddm_find_array_value", strlen("ddm_find_array_value"), expression->param.func.f->name, strlen(expression->param.func.f->name)) == true)
            {
                status = verify_ddm_find_array_value_function(specification, expression);
            }

            if (status != RULE_ENGINE__OK)
            {
                LOG(E, "Rule[%s:%d]: Invalid function[%s] parameters!", specification->name, specification->id, expression->param.func.f->name);
                break;
            }
        }
    }

    vec_free(&func_vector);

    return status;
}

static int verify_ddm_get_data_function(struct rule_engine__specification *specification, struct expr *statement)
{
    if (vec_len(&statement->param.func.args) != 3)
    {
        LOG(E, "Rule[%s:%d]: Invalid number[3] of arguments!", specification->name, specification->id, statement->param.func.args.len);
        return RULE_ENGINE__INV_RULE_FORMAT;
    }

    struct expr expression = vec_nth(&statement->param.func.args, 0);
    if (expression.type != OP_VAR)
    {
        LOG(E, "Rule[%s:%d]: Invalid expression type[%d] as input argument!", specification->name, specification->id, expression.type);
        return RULE_ENGINE__INV_RULE_FORMAT;
    }

    struct expr *ddm2_parameter = &expression;
    struct expr_var *ddm2_parameter_var = expr_get_var_by_name(&specification->expr.vars, ddm2_parameter->param.var.name, strlen(ddm2_parameter->param.var.name));
    if (ddm2_parameter_var == NULL)
    {
        LOG(E, "Rule[%s:%d]: DDM2 parameter[%s] does not exists in variable list!", specification->name, specification->id, ddm2_parameter->param.var.name);
        return RULE_ENGINE__INV_RULE_FORMAT;
    }

    uint8_t ddm2_parameter_type = expr_var_type(ddm2_parameter_var);
    if ((ddm2_parameter_type != RULE_ENGINE__VAR_TYPE_DDM2) && (ddm2_parameter_type != RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC))
    {
        LOG(E, "Invalid 'ddm2 parameter' type[%d]! Type supported: %d:[%d:%d]", ddm2_parameter_type, OP_VAR, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
        return RULE_ENGINE__INV_RULE_FORMAT;
    }

    int index = ddm2_parameter_list_lookup(DDM2_PARAMETER_BASE_INSTANCE(expr_var_id(ddm2_parameter_var)));
    if (index == -1)
    {
        LOG(E, "Rule[%s:%d]: Invalid DDM2 parameter[%s] inserted!", specification->name, specification->id, ddm2_parameter->param.var.name);
        return RULE_ENGINE__INV_RULE_FORMAT;
    }

    return RULE_ENGINE__OK;
}

static int verify_ddm_set_data_function(struct rule_engine__specification *specification, struct expr *statement)
{
    if (vec_len(&statement->param.func.args) != 4)
    {
        LOG(E, "Rule[%s:%d]: Invalid number[4] of arguments!", specification->name, specification->id, statement->param.func.args.len);
        return RULE_ENGINE__INV_RULE_FORMAT;
    }

    struct expr expression = vec_nth(&statement->param.func.args, 0);
    if (expression.type != OP_VAR)
    {
        LOG(E, "Rule[%s:%d]: Invalid expression type[%d] as input argument!", specification->name, specification->id, expression.type);
        return RULE_ENGINE__INV_RULE_FORMAT;
    }

    struct expr *ddm2_parameter = &expression;
    struct expr_var *ddm2_parameter_var = expr_get_var_by_name(&specification->expr.vars, ddm2_parameter->param.var.name, strlen(ddm2_parameter->param.var.name));
    if (ddm2_parameter_var == NULL)
    {
        LOG(E, "Rule[%s:%d]: DDM2 parameter[%s] does not exists in variable list!", specification->name, specification->id, ddm2_parameter->param.var.name);
        return RULE_ENGINE__INV_RULE_FORMAT;
    }

    uint8_t ddm2_parameter_type = expr_var_type(ddm2_parameter_var);
    if ((ddm2_parameter_type != RULE_ENGINE__VAR_TYPE_DDM2) && (ddm2_parameter_type != RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC))
    {
        LOG(E, "Invalid 'ddm2 parameter' type[%d]! Type supported: %d:[%d:%d]", ddm2_parameter_type, OP_VAR, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
        return RULE_ENGINE__INV_RULE_FORMAT;
    }

    int index = ddm2_parameter_list_lookup(DDM2_PARAMETER_BASE_INSTANCE(expr_var_id(ddm2_parameter_var)));
    if (index == -1)
    {
        LOG(E, "Rule[%s:%d]: Invalid DDM2 parameter[%s] inserted!", specification->name, specification->id, ddm2_parameter->param.var.name);
        return RULE_ENGINE__INV_RULE_FORMAT;
    }

    return RULE_ENGINE__OK;
}

static int verify_ddm_get_array_length_function(struct rule_engine__specification *specification, struct expr *statement)
{
    if (vec_len(&statement->param.func.args) != 3)
    {
        LOG(E, "Rule[%s:%d]: Invalid number[3] of arguments!", specification->name, specification->id, statement->param.func.args.len);
        return RULE_ENGINE__INV_RULE_FORMAT;
    }

    struct expr expression = vec_nth(&statement->param.func.args, 0);
    if (expression.type != OP_VAR)
    {
        LOG(E, "Rule[%s:%d]: Invalid expression type[%d] as input argument!", specification->name, specification->id, expression.type);
        return RULE_ENGINE__INV_RULE_FORMAT;
    }

    struct expr *ddm2_parameter = &expression;
    struct expr_var *ddm2_parameter_var = expr_get_var_by_name(&specification->expr.vars, ddm2_parameter->param.var.name, strlen(ddm2_parameter->param.var.name));
    if (ddm2_parameter_var == NULL)
    {
        LOG(E, "Rule[%s:%d]: DDM2 parameter[%s] does not exists in variable list!", specification->name, specification->id, ddm2_parameter->param.var.name);
        return RULE_ENGINE__INV_RULE_FORMAT;
    }

    uint8_t ddm2_parameter_type = expr_var_type(ddm2_parameter_var);
    if ((ddm2_parameter_type != RULE_ENGINE__VAR_TYPE_DDM2) && (ddm2_parameter_type != RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC))
    {
        LOG(E, "Invalid 'ddm2 parameter' type[%d]! Type supported: %d:[%d:%d]", ddm2_parameter_type, OP_VAR, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
        return RULE_ENGINE__INV_RULE_FORMAT;
    }

    int index = ddm2_parameter_list_lookup(DDM2_PARAMETER_BASE_INSTANCE(expr_var_id(ddm2_parameter_var)));
    if (index == -1)
    {
        LOG(E, "Rule[%s:%d]: Invalid DDM2 parameter[%s] inserted!", specification->name, specification->id, ddm2_parameter->param.var.name);
        return RULE_ENGINE__INV_RULE_FORMAT;
    }

    return RULE_ENGINE__OK;
}

static int verify_ddm_find_array_value_function(struct rule_engine__specification *specification, struct expr *statement)
{
    if (vec_len(&statement->param.func.args) != 6)
    {
        LOG(E, "Rule[%s:%d]: Invalid number[6] of arguments!", specification->name, specification->id, statement->param.func.args.len);
        return RULE_ENGINE__INV_RULE_FORMAT;
    }

    struct expr expression = vec_nth(&statement->param.func.args, 0);
    if (expression.type != OP_VAR)
    {
        LOG(E, "Rule[%s:%d]: Invalid expression type[%d] as input argument!", specification->name, specification->id, expression.type);
        return RULE_ENGINE__INV_RULE_FORMAT;
    }

    struct expr *ddm2_parameter = &expression;
    struct expr_var *ddm2_parameter_var = expr_get_var_by_name(&specification->expr.vars, ddm2_parameter->param.var.name, strlen(ddm2_parameter->param.var.name));
    if (ddm2_parameter_var == NULL)
    {
        LOG(E, "Rule[%s:%d]: DDM2 parameter[%s] does not exists in variable list!", specification->name, specification->id, ddm2_parameter->param.var.name);
        return RULE_ENGINE__INV_RULE_FORMAT;
    }

    uint8_t ddm2_parameter_type = expr_var_type(ddm2_parameter_var);
    if ((ddm2_parameter_type != RULE_ENGINE__VAR_TYPE_DDM2) && (ddm2_parameter_type != RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC))
    {
        LOG(E, "Invalid 'ddm2 parameter' type[%d]! Type supported: %d:[%d:%d]", ddm2_parameter_type, OP_VAR, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
        return RULE_ENGINE__INV_RULE_FORMAT;
    }

    int index = ddm2_parameter_list_lookup(DDM2_PARAMETER_BASE_INSTANCE(expr_var_id(ddm2_parameter_var)));
    if (index == -1)
    {
        LOG(E, "Rule[%s:%d]: Invalid DDM2 parameter[%s] inserted!", specification->name, specification->id, ddm2_parameter->param.var.name);
        return RULE_ENGINE__INV_RULE_FORMAT;
    }

    return RULE_ENGINE__OK;
}

static int verify_pub_function(struct rule_engine__specification *specification, struct expr *statement)
{
    if (vec_len(&statement->param.func.args) < 1)
    {
        LOG(E, "Rule[%s:%d]: Invalid number[<1] of arguments!", specification->name, specification->id, statement->param.func.args.len);
        return RULE_ENGINE__INV_RULE_FORMAT;
    }

    int arg_index = 0;
    struct expr expression;
    vec_foreach(&statement->param.func.args, expression, arg_index)
    {
        struct expr *ddm2_parameter = NULL;

        if (expression.type == OP_ASSIGN)
        {
            if ((expression.param.op.args.buf[0].type != OP_VAR) ||
                ((expression.param.op.args.buf[1].type != OP_CONST) && (expression.param.op.args.buf[1].type != OP_VAR)))
            {
                LOG(E, "Rule[%s:%d]: Argument should be an ddm2 parameter variable!", specification->name, specification->id);
                return RULE_ENGINE__INV_RULE_FORMAT;
            }

            ddm2_parameter = &expression.param.op.args.buf[0];
        }
        else if (expression.type == OP_VAR)
        {
            ddm2_parameter = &expression;
        }
        else
        {
            LOG(E, "Rule[%s:%d]: Invalid expression type[%d] as input argument!", specification->name, specification->id, expression.type);
            return RULE_ENGINE__INV_RULE_FORMAT;
        }

        struct expr_var *ddm2_parameter_var = expr_get_var_by_name(&specification->expr.vars, ddm2_parameter->param.var.name, strlen(ddm2_parameter->param.var.name));
        if (ddm2_parameter_var == NULL)
        {
            LOG(E, "Rule[%s:%d]: DDM2 parameter[%s] does not exists in variable list!", specification->name, specification->id, ddm2_parameter->param.var.name);
            return RULE_ENGINE__INV_RULE_FORMAT;
        }

        uint8_t ddm2_parameter_type = expr_var_type(ddm2_parameter_var);
        if ((ddm2_parameter_type != RULE_ENGINE__VAR_TYPE_DDM2) && (ddm2_parameter_type != RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC))
        {
            LOG(E, "Invalid 'ddm2 parameter' type[%d]! Type supported: %d:[%d:%d]", ddm2_parameter_type, OP_VAR, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
            return RULE_ENGINE__INV_RULE_FORMAT;
        }

        int index = ddm2_parameter_list_lookup(DDM2_PARAMETER_BASE_INSTANCE(expr_var_id(ddm2_parameter_var)));
        if (index == -1)
        {
            LOG(E, "Rule[%s:%d]: Invalid DDM2 parameter[%s] inserted!", specification->name, specification->id, ddm2_parameter->param.var.name);
            return RULE_ENGINE__INV_RULE_FORMAT;
        }

        DDM2_TYPE_ENUM type;
        TRUE_CHECK_RETURNX(RULE_ENGINE__INV_RULE_FORMAT, get_ddm2_type_for_write_action(ddm2_parameter_var, &type) != 0);
        if ((expression.type == OP_ASSIGN) && (type != DDM2_TYPE_INT32_T))
        {
            // Only DDM2 parameters of type int32_t support direct assignment in pub() function.
            // Other types (structs, arrays, etc.) require explicit memory operations via ddm_set_data().
            // This prevents type mismatches and ensures proper data handling for complex DDM2 types.
            LOG(E, "Rule[%s:%d]: Only DDM2 parameters of type 'int32_t' can be directly assigned!", specification->name, specification->id, ddm2_parameter->param.var.name);
            return RULE_ENGINE__INV_RULE_FORMAT;
        }
    }

    return RULE_ENGINE__OK;
}

static int verify_inventory_function(struct rule_engine__specification *specification, struct expr *statement)
{
    struct expr ddm2_parameter_instance;
    struct expr ddm2_parameter_class;
    struct expr_var *ddm2_parameter_instance_var;
    struct expr_var *ddm2_parameter_class_var;

    if (statement->param.func.args.len != 2)
    {
        LOG(E, "Rule[%s:%d]: Invalid number[%d!=2] of arguments!", specification->name, specification->id, statement->param.func.args.len);
        return RULE_ENGINE__INV_RULE_FORMAT;
    }

    ddm2_parameter_instance = vec_nth(&statement->param.func.args, 0);
    ddm2_parameter_class = vec_nth(&statement->param.func.args, 1);

    ddm2_parameter_instance_var = expr_get_var_by_name(&specification->expr.vars, ddm2_parameter_instance.param.var.name, strlen(ddm2_parameter_instance.param.var.name));
    if (ddm2_parameter_instance_var == NULL)
    {
        LOG(E, "'DDM2 parameter instance[%s]' does not exists in variable list!", ddm2_parameter_instance.param.var.name);
        return RULE_ENGINE__PARAM_NOT_EXIST;
    }
    if ((ddm2_parameter_instance.type != OP_VAR) || (expr_var_type(ddm2_parameter_instance_var) != RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE))
    {
        LOG(E, "Invalid 'ddm2 parameter instance' type[%d]! Type supported: %d:[%d]", ddm2_parameter_instance.type, OP_VAR, RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
        return RULE_ENGINE__INV_RULE_FORMAT;
    }

    ddm2_parameter_class_var = expr_get_var_by_name(&specification->expr.vars, ddm2_parameter_class.param.var.name, strlen(ddm2_parameter_class.param.var.name));
    if (ddm2_parameter_class_var == NULL)
    {
        LOG(E, "'DDM2 parameter instance[%s]' does not exists in variable list!", ddm2_parameter_class.param.var.name);
        return RULE_ENGINE__PARAM_NOT_EXIST;
    }
    if ((ddm2_parameter_class.type != OP_VAR) || (expr_var_type(ddm2_parameter_class_var) != RULE_ENGINE__VAR_TYPE_DDM2))
    {
        LOG(E, "Invalid 'ddm2 parameter class' type[%d]! Type supported: %d:[%d]", ddm2_parameter_instance.type, OP_VAR, RULE_ENGINE__VAR_TYPE_DDM2);
        return RULE_ENGINE__INV_RULE_FORMAT;
    }

    // Verify that the DDM2 parameter class with same instance is not already used in other inventory resolution rules.
    for (size_t s = 0; s < sorted_container__occupied(specification->p_rule_engine_inst->p_rule_engine_specifications); s++)
    {
        uint32_t sspec_key;
        struct rule_engine__specification *existing_spec;

        sorted_container__iterate(specification->p_rule_engine_inst->p_rule_engine_specifications, s, (void **)&existing_spec, &sspec_key);

        if (specification != existing_spec)
        {
            if (existing_spec->expr.type.type == RULE_ENGINE__EXPRESSION_TYPE_INSTANCE_RESOLUTION_INV)
            {
                // Only enforce the "same DDM2 class" constraint when no user callback is set
                // If user callback is set, allow multiple rules for the same DDM2 class
                if (specification->p_rule_engine_inst->inventory_user_cb == NULL)
                {
                    struct expr_var *existing_spec_ddm2_paramter_class = expr_get_var_by_id_and_type(
                        &existing_spec->expr.vars, expr_var_id(ddm2_parameter_class_var), expr_var_type(ddm2_parameter_class_var));
                    if (existing_spec_ddm2_paramter_class != NULL)
                    {
                        LOG(E, "Rule[%s:%d]: DDM2 parameter class[%s] already exists in rule[%s:%d]!", specification->name, specification->id, ddm2_parameter_class_var->name, existing_spec->name, existing_spec->id);
                        return RULE_ENGINE__INV_RULE_FORMAT;
                    }
                }
            }
        }
    }

    return RULE_ENGINE__OK;
}

static int verify_dyn_instance_function(struct rule_engine__specification *specification, struct expr *statement)
{
    struct expr ddm2_parameter_instance;
    struct expr ddm2_parameter;
    struct expr ddm2_parameter_class;
    struct expr ddm2_parameter_class_position;
    struct expr_var *ddm2_parameter_instance_var;
    struct expr_var *ddm2_parameter_var;
    struct expr_var *ddm2_parameter_class_var;

    if (statement->param.func.args.len != 4)
    {
        LOG(E, "Rule[%s:%d]: Invalid number[%d!=4 of arguments!", specification->name, specification->id, statement->param.func.args.len);
        return RULE_ENGINE__INV_RULE_FORMAT;
    }

    ddm2_parameter_instance = vec_nth(&statement->param.func.args, 0);
    ddm2_parameter = vec_nth(&statement->param.func.args, 1);
    ddm2_parameter_class = vec_nth(&statement->param.func.args, 2);
    ddm2_parameter_class_position = vec_nth(&statement->param.func.args, 3);

    // First argument(ddm2 parameter instnace) has to be variable of type RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE
    ddm2_parameter_instance_var = expr_get_var_by_name(&specification->expr.vars, ddm2_parameter_instance.param.var.name, strlen(ddm2_parameter_instance.param.var.name));
    if (ddm2_parameter_instance_var == NULL)
    {
        LOG(E, "'DDM2 parameter instance[%s]' does not exists in variable list!", ddm2_parameter_instance.param.var.name);
        return RULE_ENGINE__PARAM_NOT_EXIST;
    }
    if ((ddm2_parameter_instance.type != OP_VAR) || (expr_var_type(ddm2_parameter_instance_var) != RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE))
    {
        LOG(E, "Invalid 'ddm2 parameter instance' type[%d]! Type supported: %d:[%d]", ddm2_parameter_instance.type, OP_VAR, RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
        return RULE_ENGINE__INV_RULE_FORMAT;
    }

    // Second argument(ddm2 parameter) has to be variable of type RULE_ENGINE__VAR_TYPE_DDM2 or RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC
    ddm2_parameter_var = expr_get_var_by_name(&specification->expr.vars, ddm2_parameter.param.var.name, strlen(ddm2_parameter.param.var.name));
    if (ddm2_parameter_var == NULL)
    {
        LOG(E, "'DDM2 parameter[%s]' does not exists in variable list!", ddm2_parameter.param.var.name);
        return RULE_ENGINE__PARAM_NOT_EXIST;
    }
    if ((ddm2_parameter.type != OP_VAR) ||
        ((expr_var_type(ddm2_parameter_var) != RULE_ENGINE__VAR_TYPE_DDM2) && (expr_var_type(ddm2_parameter_var) != RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC)))
    {
        LOG(E, "Invalid 'ddm2 parameter' type[%d]! Type supported: %d:[%d:%d]", ddm2_parameter.type, OP_VAR, RULE_ENGINE__VAR_TYPE_DDM2, RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
        return RULE_ENGINE__INV_RULE_FORMAT;
    }

    // Third argument(ddm2 class) has to be variable of type RULE_ENGINE__VAR_TYPE_DDM2
    ddm2_parameter_class_var = expr_get_var_by_name(&specification->expr.vars, ddm2_parameter_class.param.var.name, strlen(ddm2_parameter_class.param.var.name));
    if (ddm2_parameter_class_var == NULL)
    {
        LOG(E, "'DDM2 parameter class[%s]' does not exists in variable list!", ddm2_parameter_class.param.var.name);
        return RULE_ENGINE__PARAM_NOT_EXIST;
    }
    if ((ddm2_parameter_class.type != OP_VAR) || (expr_var_type(ddm2_parameter_class_var) != RULE_ENGINE__VAR_TYPE_DDM2))
    {
        LOG(E, "Invalid 'ddm2 parameter class' type[%d]! Type supported: %d:[%d]", ddm2_parameter_instance.type, OP_VAR, RULE_ENGINE__VAR_TYPE_DDM2);
        return RULE_ENGINE__INV_RULE_FORMAT;
    }

    // Forth argument(ddm2 class position) has to be constant
    if ((ddm2_parameter_class_position.type != OP_CONST))
    {
        LOG(E, "Invalid 'ddm2 parameter class position' type[%d]! Type supported: %d", ddm2_parameter_class_position.type, OP_CONST);
        return RULE_ENGINE__INV_RULE_FORMAT;
    }

    int index = ddm2_parameter_list_lookup(DDM2_PARAMETER_BASE_INSTANCE(expr_var_id(ddm2_parameter_var)));
    if (index == -1)
    {
        LOG(E, "Rule[%s:%d]: Invalid DDM2 parameter[%s] inserted!", specification->name, specification->id, ddm2_parameter_var->name);
        return RULE_ENGINE__INV_DYN_VAR_NAME;
    }

    if ((Ddm2_parameter_list_data[index].out_type != DDM2_TYPE_STRUCT) || (Ddm2_parameter_list_data[index].out_unit != DDM2_UNIT_INSTANCE))
    {
        LOG(E, "Rule[%s:%d]: Invalid dynamic DDM2 parameter[%s] type[%d] or unit[%d!",
            specification->name, specification->id,
            ddm2_parameter_var->name,
            Ddm2_parameter_list_data[index].out_type, Ddm2_parameter_list_data[index].out_unit);
        return RULE_ENGINE__INV_DYN_VAR_NAME;
    }

    return RULE_ENGINE__OK;
}

static int verify_and_create_timer(struct rule_engine__specification *specification, struct expr *statement, uint8_t timer_type)
{
    struct expr timer;
    struct expr timer_name;
    struct expr_var *timer_instance;

    if (statement->param.func.args.len != 2)
    {
        LOG(E, "Rule[%s:%d]: Invalid number[%d!=2] of arguments!", specification->name, specification->id, statement->param.func.args.len);
        return RULE_ENGINE__INV_RULE_FORMAT;
    }

    timer_name = vec_nth(&statement->param.func.args, 0);
    timer = vec_nth(&statement->param.func.args, 1);

    // Get timer variable from rule's variable list
    timer_instance = expr_get_var_by_name(&specification->expr.vars, timer_name.param.var.name, strlen(timer_name.param.var.name));
    if (timer_instance == NULL)
    {
        LOG(E, "'Timer instance[%s]' does not exists in variable list!", timer_name.param.var.name);
        return RULE_ENGINE__PARAM_NOT_EXIST;
    }

    // First argument(timer instance) has to be global variable
    if ((timer_name.type != OP_VAR) || (expr_var_type(timer_instance) != RULE_ENGINE__VAR_TYPE_GLOBAL))
    {
        LOG(E, "Invalid 'timer instance' type[%d]! Type supported: %d:%d", timer_name.type, OP_VAR, RULE_ENGINE__VAR_TYPE_GLOBAL);
        return RULE_ENGINE__INV_RULE_FORMAT;
    }

    // Second argument(timer value) has to be variable or constant
    if ((timer.type != OP_CONST) && (timer.type != OP_VAR))
    {
        LOG(E, "Invalid 'time' type[%d]! Types supported: %d, %d", timer.type, OP_CONST, OP_VAR);
        return RULE_ENGINE__INV_RULE_FORMAT;
    }

    size_t time_ms = timer.type == OP_CONST ? timer.param.num.value : *timer.param.var.value;
    size_t time_in_ticks = rule_engine_get_timer_period_in_ticks(timer_type, time_ms);

    if ((timer.type == OP_CONST) && (time_in_ticks == 0))
    {
        LOG(E, "Rule[%s:%d]: Invalid timer value[%d]", specification->name, specification->id, time_in_ticks);
        return RULE_ENGINE__TIMER_NOT_CREATED;
    }
    else if ((timer.type == OP_VAR) && (time_in_ticks == 0))
    {
        // set tick time to a default value, since RTOS timer cannot be
        // created with timer period set to 0. This value will be preset
        // in runtime once the rule is triggered and evaluated
        time_in_ticks = UINT32_MAX;
    }

    struct rule_engine__timer *re_timer = rule_engine_timer_create(specification->p_rule_engine_inst, timer_instance, timer_type, time_in_ticks, specification->id);
    if (re_timer == NULL)
    {
        LOG(E, "Rule[%s:%d]: Timer creation failed!", specification->name, specification->id);
        return RULE_ENGINE__TIMER_NOT_CREATED;
    }

    return RULE_ENGINE__OK;
}

/**
 * @brief Creates the trigger section for a rule specification based on the sensitivity list and mask.
 *
 * This function populates the trigger section with variables from the sensitivity list
 * according to the mask created by create_rule_mask(). The trigger section determines
 * which variables can cause the rule to be executed when updated.
 *
 * Variable behavior by type:
 *
 * 1. DDM2 variables:
 *    - With auto-generated mask: ALWAYS included in the trigger section regardless of initialization
 *    - With user-defined mask: Only included if specified in the mask
 *    - The initialized flag is preserved from the sensitivity list parser
 *    - When initialized, initial values are set during DDM2 subscription i.e. subscribe_rule_specification()
 *
 * 2. GLOBAL variables:
 *    - With auto-generated mask: Only included if NOT initialized (e.g., "g_temp" but not "g_temp=15")
 *    - With user-defined mask: Included if specified in the mask regardless of initialization
 *    - The initialized flag is preserved from the sensitivity list parser
 *    - Initialized GLOBAL variables with auto-mask are purposely excluded from triggering rules
 *
 * 3. LOCAL variables:
 *    - NEVER included in the trigger section
 *    - Error is generated if a user-defined mask tries to include a local variable
 *
 * The function sets the `initialized` flag in rule_engine__sensitivity_list based on the
 * corresponding flag from the parser. This flag is important for:
 * - DDM2 params: Determines if an initial value should be set during subscription i.e. subscribe_rule_specification())
 * - GLOBAL vars: Indicates the variable was initialized in the rule sensitivity list
 *
 * @param specification Pointer to the rule specification to create trigger section for
 * @param sens_list Pointer to the sensitivity list parser with parsed variables
 * @return RULE_ENGINE__OK if successful, otherwise error code
 */
static int create_trigger_section(struct rule_engine__specification *specification, struct rule_engine__sensitivity_list_parser *sens_list)
{
    uint32_t parameter_count = 0;
    struct rule_engine__trigger_section *trigger = &specification->trigger;

    for (uint32_t i = 0; i < sens_list->n_variables; ++i)
    {
        if (parameter_count >= ELEMENTS(trigger->variables))
        {
            LOG(E, "Rule's[%s:%d] sensitivity list depth exceeded!", specification->name, specification->id);
            return RULE_ENGINE__RULES_DEPTH;
        }

        /* Check if this variable should be in the trigger section according to the mask.
         * The mask was either:
         * - Set by user: Explicitly defining which variables should be in trigger section
         * - Auto-generated by create_rule_mask: Including all DDM2 variables and only
         *   uninitialized GLOBAL variables (initialized globals are excluded) */
        if (RULE_ENGINE__IS_TRIGGER_PARAM(sens_list->mask, i))
        {
            uint32_t id = sens_list->variables[i].id;
            uint32_t type = sens_list->variables[i].type;
            uint32_t type_id = sens_list->variables[i].type_id;

            /* Add this variable to the trigger section mask */
            RULE_ENGINE__SET_MASK(trigger->mask, parameter_count);

            switch (type)
            {
            case RULE_ENGINE__VAR_TYPE_DDM2:
            {
                /* Create a new subscription entry for this DDM2 parameter in the rule engine's subscription table
                 * If it already exist, it will return the already allocated memory speace instead */
                ddmw_item_t *ddm2_var = sorted_container__new(specification->p_rule_engine_inst->p_rule_engine_ddm2_subscription_table, id);
                if (ddm2_var == NULL)
                {
                    LOG(E, "Subscription container depth overflow!");
                    return RULE_ENGINE__SUBS_DEPTH;
                }
                trigger->variables[parameter_count].variable = expr_get_var_by_id_and_type(&specification->expr.vars, id, type);

                if (trigger->variables[parameter_count].variable == NULL)
                {
                    LOG(E, "DDM2 variable[%s] does not exists", sens_list->variables[i].name);
                    return RULE_ENGINE__PARAM_NOT_EXIST;
                }

                /* Set up the DDM2 parameter in the trigger list:
                 * - Store the parameter ID for later matching with subscription updates
                 * - Initialize updated flag to false (not updated yet)
                 * - Copy the initialized flag from the sensitivity list parser
                 *   (this determines whether an initial value will be set during subscription) */
                trigger->variables[parameter_count].parameter = id;
                rule_engine_sens_list_variable_set_updated(&trigger->variables[parameter_count], false);
                rule_engine_sens_list_variable_set_initialized(&trigger->variables[parameter_count], sens_list->variables[i].initialized);
                parameter_count++;
                break;
            }
            case RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC:
            {
                /* Do not create a new subscription entry for this DDM2 parameter in the rule engine's subscription table
                 * It will be handled by the dynamic instance resolution flow once the parameter is resolved. */

                trigger->variables[parameter_count].variable = expr_get_var_by_id_and_type_id(&specification->expr.vars, id, type, type_id);
                if (trigger->variables[parameter_count].variable == NULL)
                {
                    LOG(E, "DDM2 variable[%s] does not exists", sens_list->variables[i].name);
                    return RULE_ENGINE__PARAM_NOT_EXIST;
                }

                /* Set up the DDM2 parameter in the trigger list:
                 * - Store the parameter ID for later matching with subscription updates
                 * - Initialize updated flag to false (not updated yet)
                 * - Copy the initialized flag from the sensitivity list parser
                 *   (this determines whether an initial value will be set during subscription) */
                trigger->variables[parameter_count].parameter = id;
                rule_engine_sens_list_variable_set_updated(&trigger->variables[parameter_count], false);
                rule_engine_sens_list_variable_set_initialized(&trigger->variables[parameter_count], sens_list->variables[i].initialized);
                parameter_count++;
                break;
            }
            case RULE_ENGINE__VAR_TYPE_GLOBAL:
            {
                /* Find the corresponding global variable in the rule's expression variable list */
                trigger->variables[parameter_count].variable = expr_get_var_by_id_and_type(&specification->expr.vars, id, RULE_ENGINE__VAR_TYPE_GLOBAL);
                if (trigger->variables[parameter_count].variable == NULL)
                {
                    LOG(E, "Global variable[%s] does not exists", sens_list->variables[i].name);
                    return RULE_ENGINE__PARAM_NOT_EXIST;
                }

                /* Set up the global variable in the trigger list:
                 * - Store the ID (hash-based for global variables)
                 * - Initialize updated flag to false (not updated yet)
                 * - Copy the initialized flag from sensitivity list parser
                 *   (indicates whether the variable was initialized in the rule definition)
                 *
                 * Note: For auto-generated masks, initialized globals are excluded from the
                 * trigger list by create_rule_mask(). If we reach here with an initialized global,
                 * it's because a user-defined mask explicitly included it. */
                trigger->variables[parameter_count].parameter = id;
                rule_engine_sens_list_variable_set_updated(&trigger->variables[parameter_count], false);
                rule_engine_sens_list_variable_set_initialized(&trigger->variables[parameter_count], sens_list->variables[i].initialized);
                parameter_count++;

                break;
            }
            case RULE_ENGINE__VAR_TYPE_LOCAL:  // fall-through
            case RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE:
            default:
                /* Local variables are never included in the trigger section
                 * If the mask includes a local variable (user-defined mask only),
                 * we simply ignore it here. create_rule_mask() will have generated
                 * an error for user-defined masks trying to include locals. */
                break;
            }
        }
        else
        {
            LOG(D, "Variable[%s] at index[%d] is not included in the trigger section mask.",
                sens_list->variables[i].name, i);
        }
    }

    /* Finalize the trigger section configuration */
    trigger->mask_type = sens_list->mask_type; /* AND/OR logic for triggers */
    trigger->n_params = parameter_count;       /* Number of variables that can trigger the rule */

    return RULE_ENGINE__OK;
}

static int update_trigger_section(struct rule_engine__specification *const specification, const struct expr_var *const variable, int32_t value, uint8_t type)
{
    bool is_updated = false;

    if (expr_var_type(variable) == type)
    {
        for (uint32_t vars = 0; vars < specification->trigger.n_params; vars++)
        {
            struct rule_engine__sensitivity_list *item = &specification->trigger.variables[vars];
            if (expr_var_type(variable) == expr_var_type(item->variable))
            {
                if (expr_var_id(item->variable) == expr_var_id(variable))
                {
                    rule_engine_sens_list_variable_set_updated(item, true);
                    is_updated = true;
                    break;
                }
            }
        }
    }

    return is_updated;
}

static void clear_trigger_section(struct rule_engine__specification *specification)
{
    for (uint32_t var = 0; var < specification->trigger.n_params; var++)
    {
        rule_engine_sens_list_variable_set_updated(&specification->trigger.variables[var], false);
    }
}

static int create_rule_mask(const struct rule_engine__specification *const specification, struct rule_engine__sensitivity_list_parser *const sens_list)
{
    // If mask is set by user
    if (sens_list->mask != 0)
    {
        uint32_t highest_var_indicated = RULE_ENGINE__HIGHEST_VAR_POSITION_IN_MASK(sens_list->mask);
        if (highest_var_indicated > sens_list->n_variables)
        {
            LOG(E, "Invalid mask value[%d]. Variable at position[%d] does not exist",
                sens_list->mask,
                highest_var_indicated);
            return RULE_ENGINE__INV_MASK;
        }

        for (uint32_t i = 0; i < sens_list->n_variables; ++i)
        {
            if (sens_list->variables[i].type == RULE_ENGINE__VAR_TYPE_LOCAL ||
                sens_list->variables[i].type == RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE)
            {
                // Local variables or instances are never included in the trigger section
                if (RULE_ENGINE__IS_TRIGGER_PARAM(sens_list->mask, i))
                {
                    LOG(E, "Local variable/instances[%s] set as trigger!"
                           "Rule will never be executed!",
                        sens_list->variables[i].name);
                    return RULE_ENGINE__INV_MASK;
                }
            }
        }
    }
    else
    {
        for (uint32_t i = 0; i < sens_list->n_variables; ++i)
        {
            if (sens_list->variables[i].type == RULE_ENGINE__VAR_TYPE_GLOBAL)
            {
                if (sens_list->variables[i].initialized == false)
                {
                    RULE_ENGINE__SET_MASK(sens_list->mask, i);
                }
            }
            else if (sens_list->variables[i].type == RULE_ENGINE__VAR_TYPE_DDM2 ||
                     sens_list->variables[i].type == RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC)
            {
                RULE_ENGINE__SET_MASK(sens_list->mask, i);
            }
            else
            {
                // skip RULE_ENGINE__VAR_TYPE_LOCAL and RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE
            }
        }

        if (sens_list->mask == 0)
        {
            if (specification->expr.type.type != RULE_ENGINE__EXPRESSION_TYPE_INSTANCE_RESOLUTION_INV)
            {
                LOG(E, "No valid trigger variable set!"
                       "Initilization of global variables is not considered as trigger.");
                return RULE_ENGINE__INV_MASK;
            }
        }
    }

    return RULE_ENGINE__OK;
}

/**
 * @brief Synchronizes variable values between rule specifications
 *
 * This function ensures consistent variable states across the rule engine by:
 * 1. Initializing the current rule's variables with values from existing rules
 * 2. Propagating new variable values from the current rule to other rules
 *
 * It maintains a shared variable state across all rule specifications while
 * respecting explicitly initialized values in the sensitivity list.
 *
 * The function handles variables according to the following rules:
 * - Variables from other rules are copied into the current rule (except local variables)
 * - Variables explicitly initialized in the sensitivity list get their specified value
 * - These initialized values are then propagated to all other rules
 * - Variables not explicitly initialized in the sensitivity list only get a default value
 *   if they currently have a value of 0
 *
 * This mechanism ensures that:
 * - Explicitly initialized variables always get their specified value
 * - Variable values are shared across all rules (for non-local variables)
 * - Rules added later inherit the state of already loaded rules
 * - Default values (0) are only applied to uninitialized variables with no existing value
 *
 * @param specification The rule specification being initialized
 * @param sens_list The sensitivity list parser containing variable initializations
 * @return RULE_ENGINE__OK on success, error code otherwise
 */
static int init_rule_variables(struct rule_engine__specification *const specification, const struct rule_engine__sensitivity_list_parser *const sens_list)
{
    // Init current rule variables with values from existing rules
    for (size_t s = 0; s < sorted_container__occupied(specification->p_rule_engine_inst->p_rule_engine_specifications); s++)
    {
        uint32_t sspec_key;
        struct rule_engine__specification *existing_spec;

        sorted_container__iterate(specification->p_rule_engine_inst->p_rule_engine_specifications, s, (void **)&existing_spec, &sspec_key);

        if (specification != existing_spec)
        {
            for (struct expr_var *existing_spec_var = existing_spec->expr.vars.head; existing_spec_var; existing_spec_var = existing_spec_var->next)
            {
                if ((expr_var_type(existing_spec_var) != RULE_ENGINE__VAR_TYPE_LOCAL) &&
                    (expr_var_type(existing_spec_var) != RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE))
                {
                    rule_engine_update_variable_value(&specification->expr.vars, existing_spec_var);
                }
            }
        }
    }

    // Init existing rules with variables from current rule
    for (uint32_t n_vars = 0; n_vars < sens_list->n_variables; ++n_vars)
    {
        struct expr_var *variable = expr_get_var_by_id_and_type(
            &specification->expr.vars,
            sens_list->variables[n_vars].id,
            sens_list->variables[n_vars].type);

        if (variable == NULL)
        {
            LOG(E, "Rule[%s:%d]:Variable[%s] does not exist.",
                specification->name, specification->id,
                sens_list->variables[n_vars].name);
            return RULE_ENGINE__INV_RULE_FORMAT;
        }

        // Initialize global and ddm2 parameters
        if (sens_list->variables[n_vars].initialized)
        {
            expr_set_var(variable, sens_list->variables[n_vars].value);

            if ((sens_list->variables[n_vars].type != RULE_ENGINE__VAR_TYPE_LOCAL &&
                 sens_list->variables[n_vars].type != RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE))
            {
                for (size_t s = 0; s < sorted_container__occupied(specification->p_rule_engine_inst->p_rule_engine_specifications); s++)
                {
                    uint32_t sspec_key;
                    struct rule_engine__specification *specs;

                    sorted_container__iterate(specification->p_rule_engine_inst->p_rule_engine_specifications, s, (void **)&specs, &sspec_key);

                    if (specs != specification)
                    {
                        rule_engine_update_variable_value(&specs->expr.vars, variable);
                    }
                }
            }
        }
        else
        {
            // do not override already set variables by existing rules
            if (variable->value == RULE_ENGINE__VAR_VALUE_INIT)
            {
                expr_set_var(variable, sens_list->variables[n_vars].value);
            }
        }
    }

    return RULE_ENGINE__OK;
}

static int is_expr_ddm2_variable(const struct expr_var *var)
{
    uint32_t parameter_id = 0;
    char parameter_name[RULE_ENGINE__VAR_NAME_LEN];
    size_t parameter_name_len = copy_string_to_buffer(var->name, parameter_name, sizeof(parameter_name));

    return ddm2_parse_parameter_string(&parameter_id, parameter_name, parameter_name_len);
}

static int is_expr_global_variable(const struct expr_var *var)
{
    return (strncmp(var->name, RULE_ENGINE__VAR_GLOBAL_PREFIX, strnlen(var->name, sizeof(RULE_ENGINE__VAR_GLOBAL_PREFIX) - 1)) == 0 ? 1 : -1);
}

/*! \brief Check if a variable is of type dynamic DDM2 (with {})
 *
 *  \param str      variable name which type should be determined
 *
 *  \retval  0      variable is not of dynamic type
 *  \retval  1      variable is of dynamic type
 *  \retval -1      varialbe is of dynamic type, but name format is invalid
 */
static int is_expr_ddm2_dynamic_variable(const struct expr_var *var)
{
    const char *str = var->name;

    // Find the first occurrence of {
    const char *open_brace = strstr(str, "{");
    if (!open_brace)
    {
        return 0;  // No opening brace found
    }

    // Find the first occurrence of } after {
    const char *close_brace = strstr(open_brace, "}");
    if (!close_brace)
    {
        return -1;  // Opening brace without closing brace
    }

    // Check for nested braces between the first { and }
    const char *nested_open = strstr(open_brace + 1, "{");
    if (nested_open && nested_open < close_brace)
    {
        return -1;  // Nested braces not allowed
    }

    // Check for any additional braces after the first pair
    const char *next_brace = strstr(close_brace + 1, "{");
    if (next_brace)
    {
        return -1;  // Multiple brace pairs found
    }

    // Check for any unmatched closing braces
    const char *extra_close = strstr(close_brace + 1, "}");
    if (extra_close)
    {
        return -1;  // Unmatched closing brace
    }

    return 1;  // Valid type 1 variable
}

/**
 * @brief Transforms a DDM2 dynamic variable into its component parts
 *
 * This function parses a DDM2 dynamic variable name (format: "class_name{instance_name}")
 * and extracts the parameter name and instance name components. It handles the specific
 * syntax used for dynamic variable references in DDM2 expressions, where the instance
 * name is enclosed in braces within the variable name.
 *
 * @param var The expression variable containing the dynamic variable name to parse
 * @param parameter_name Output buffer for the extracted parameter name
 * @param parameter_name_size Input/output: size of the parameter_name buffer
 * @param parameter_instance_name Output buffer for the extracted instance name
 * @param parameter_instance_name_size Input/output: size of the parameter_instance_name buffer
 * @return 1 if successful, 0 if parsing failed or buffers too small
 */
static int transform_ddm2_dynamic_variable(const struct expr_var *var, char *parameter_name, size_t *parameter_name_size,
                                           char *parameter_instance_name, size_t *parameter_instance_name_size)
{
    const char *var_name = var->name;

    // Find the braces
    const char *open_brace = strstr(var_name, "{");
    const char *close_brace = strstr(open_brace, "}");

    // Extract the parameter instance (content between braces)
    size_t parameter_instance_name_len = close_brace - (open_brace + 1);
    if (parameter_instance_name_len >= *parameter_instance_name_size)
    {
        return 0;  // Parameter instance buffer too small
    }
    strncpy(parameter_instance_name, open_brace + 1, parameter_instance_name_len);
    parameter_instance_name[parameter_instance_name_len] = '\0';

    // Calculate the length of the transformed name
    size_t parameter_name_prefix_len = open_brace - var_name;
    size_t parameter_name_suffix_len = strlen(close_brace + 1);
    size_t parameter_name_total_len = parameter_name_prefix_len + 1 + parameter_name_suffix_len;  // +1 for '0'

    if (parameter_name_total_len >= *parameter_name_size)
    {
        return 0;  // Output buffer too small
    }

    // Copy the prefix (before {)
    strncpy(parameter_name, var_name, parameter_name_prefix_len);
    parameter_name[parameter_name_prefix_len] = '\0';

    // Add '0' to replace the type
    strcat(parameter_name, "0");

    // Copy the suffix (after })
    strcat(parameter_name, close_brace + 1);

    // Update the size parameters with the actual lengths used
    *parameter_instance_name_size = parameter_instance_name_len + 1;  // +1 for null terminator
    *parameter_name_size = parameter_name_total_len + 1;              // +1 for null terminator

    return 1;  // Successfully transformed
}

/**
 * @brief Locates a specific function node in the rule specification's AST
 *
 * This function searches through the Abstract Syntax Tree of a rule specification
 * to find a function node with the given name. It uses pointer-based AST traversal
 * to ensure memory safety and returns a pointer to the original AST node if found.
 *
 * @param specification The rule specification containing the AST to search
 * @param func_name The name of the function to locate (e.g., "inventory", "dyn_instance")
 * @return Pointer to the function expression node if found, NULL otherwise
 */
static struct expr *get_function_from_specification(const struct rule_engine__specification *specification, const char *func_name)
{
    struct expr *func = NULL;

    vec_expr_ptr_t func_vector = vec_init();
    expr_get_node_for_type_from_ast(specification->expr.rule, OP_FUNC, &func_vector);

    for (int i = 0; i < func_vector.len; i++)
    {
        struct expr *func_expr = func_vector.buf[i];  // This is a pointer to the actual AST node
        if (func_expr->type == OP_FUNC)
        {
            if (strings_equal(func_name, strlen(func_name), func_expr->param.func.f->name, strlen(func_expr->param.func.f->name)))
            {
                func = func_expr;
                break;
            }
        }
    }

    vec_free(&func_vector);

    return func;
}

/**
 * @brief Determines the type of rule expression based on function presence
 *
 * This function analyzes a rule specification's AST to determine what type of
 * rule expression it represents. It searches for specific function names like
 * "inventory" or "dyn_instance" to classify the rule as either instance
 * resolution or dynamic instance type. The detected function node pointer
 * is stored in the expression type structure for later use.
 *
 * @param specification The rule specification to analyze
 * @param expr_type Output parameter that receives the detected expression type and function pointer
 * @return 0 on success, negative value on error
 */
static int determine_rule_expression_type(const struct rule_engine__specification *specification, rule_engine__expression_type_t *expr_type)
{
    struct expr *func = NULL;

    if ((func = get_function_from_specification(specification, "inventory")))
    {
        expr_type->func = func;
        expr_type->type = RULE_ENGINE__EXPRESSION_TYPE_INSTANCE_RESOLUTION_INV;
    }
    else if ((func = get_function_from_specification(specification, "dyn_instance")))
    {
        expr_type->func = func;
        expr_type->type = RULE_ENGINE__EXPRESSION_TYPE_INSTANCE_RESOLUTION_DYN_INST;
    }
    else
    {
        expr_type->func = NULL;
        expr_type->type = RULE_ENGINE__EXPRESSION_TYPE_STANDARD;
    }

    return expr_type->type;
}

/**
 * @brief Determines the type classification of an expression variable
 *
 * This function analyzes an expression variable to determine its type classification
 * such as DDM2 dynamic, DDM2 static, global, or local variable. The classification
 * is used throughout the rule engine to handle different variable types appropriately
 * during expression evaluation and variable resolution.
 *
 * @param variable Pointer to the expression variable to classify
 * @return Variable type constant (RULE_ENGINE__VAR_TYPE_*)
 */
static int determine_variable_expression_type(const struct expr_var *variable)
{
    uint8_t variable_type = 0;

    if (is_expr_ddm2_dynamic_variable(variable))
    {
        variable_type = RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC;
    }
    else if (is_expr_ddm2_variable(variable) != -1)
    {
        variable_type = RULE_ENGINE__VAR_TYPE_DDM2;
    }
    else if (is_expr_global_variable(variable) != -1)
    {
        variable_type = RULE_ENGINE__VAR_TYPE_GLOBAL;
    }
    else
    {
        variable_type = RULE_ENGINE__VAR_TYPE_LOCAL;
    }

    return variable_type;
}

/**
 * @brief Searches for a variable by name across all specifications except the calling one
 *
 * This function looks for a variable with the given name in all rule engine specifications
 * except for the one provided as a parameter. It's primarily used to find instance variables
 * that might be defined in other rule specifications.
 *
 * @param specification The specification to exclude from the search
 * @param variable_name The name of the variable to search for
 * @return The found variable if it exists, NULL otherwise
 */
static struct expr_var *find_variable_in_other_specifications(const struct rule_engine__specification *specification, const char *variable_name)
{
    struct expr_var *var = NULL;
    for (size_t index = 0; index < sorted_container__occupied(specification->p_rule_engine_inst->p_rule_engine_specifications); index++)
    {
        uint32_t spec_key;
        struct rule_engine__specification *spec;

        sorted_container__iterate(specification->p_rule_engine_inst->p_rule_engine_specifications, index, (void **)&spec, &spec_key);
        if (specification != spec)
        {
            if ((var = expr_get_var_by_name(&spec->expr.vars, variable_name, strlen(variable_name))))
            {
                break;
            }
        }
    }
    return var;
}

/**
 * @brief Extracts instance and parameter information from a dynamic DDM2 variable.
 *
 * This function processes a dynamic DDM2 variable to extract the associated instance
 * variable and parameter name information. It looks up the instance variable in other
 * specifications and performs validation checks.
 *
 * @param specification The rule engine specification context
 * @param ddm2_dynamic_variable The dynamic DDM2 variable to process
 * @param[out] parameter_name Optional buffer to receive the extracted parameter name
 * @param[in,out] parameter_name_len On input, size of parameter_name buffer; on output, actual length
 * @param[out] parameter_instance_name Optional buffer to receive the instance name
 * @param[in,out] parameter_instance_name_len On input, size of parameter_instance_name buffer; on output, actual length
 *
 * @return The instance variable if found and valid, NULL otherwise
 */
static struct expr_var *parse_dynamic_ddm2_parameter(
    struct rule_engine__specification *specification,
    struct expr_var *ddm2_dynamic_variable,
    char *parameter_name,
    size_t *parameter_name_len,
    char *parameter_instance_name,
    size_t *parameter_instance_name_len)
{
    TRUE_CHECK_RETURNX(NULL, (parameter_name != NULL));
    TRUE_CHECK_RETURNX(NULL, (parameter_name_len != NULL));
    TRUE_CHECK_RETURNX(NULL, (*parameter_name_len != 0));
    TRUE_CHECK_RETURNX(NULL, (parameter_instance_name != NULL));
    TRUE_CHECK_RETURNX(NULL, (parameter_instance_name_len != NULL));
    TRUE_CHECK_RETURNX(NULL, (*parameter_instance_name_len != 0));

    struct expr_var *instance_variable = NULL;
    if (transform_ddm2_dynamic_variable(ddm2_dynamic_variable, parameter_name, parameter_name_len,
                                        parameter_instance_name, parameter_instance_name_len))
    {
        // Find the specification in the rule engine instance that should resolve the parameter instance.
        // If it does not exist, error. Some other rule has to be responsible for creating and resolving the instance.
        struct expr_var *variable = find_variable_in_other_specifications(specification, parameter_instance_name);
        if (variable != NULL)
        {
            if (expr_var_type(variable) == RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE)
            {
                instance_variable = variable;
            }
            else
            {
                LOG(E, "Rule[%s:%d]: Variable[%s] found in other specifications, but not of type DDM2 instance!",
                    specification->name, specification->id,
                    parameter_instance_name);
            }
        }
        else
        {
            LOG(E, "Rule[%s:%d]: Instance variable[%s] not found in other specifications!",
                specification->name, specification->id,
                parameter_instance_name);
        }
    }
    else
    {
        LOG(E, "Failed to transform dynamic DDM2 variable [%s]: invalid format", ddm2_dynamic_variable->name);
    }

    return instance_variable;
}

/**
 * @brief Resolves the dynamic instance type ID for a variable
 *
 * This function resolves the type ID for a dynamic instance variable by:
 * 1. Checking if the exact same variable already exists in any specification
 * 2. If it doesn't, finding the maximum type ID used by any variable with the same DDM2 class
 * 3. Assigning a new ID that is one more than the max found (or 0 if no variables found)
 *
 * This ensures that type IDs are assigned incrementally without gaps and prevents ID conflicts
 * between variables with the same DDM2 class across different specifications.
 *
 * @param specification The rule engine specification containing the variable
 * @param dynamic_variable The variable to resolve the type ID for
 * @return int8_t The resolved type ID, or RULE_ENGINE__VAR_TYPE_ID_UNRESOLVED on error
 */
static int8_t resolve_dynamic_instance_type_id(struct rule_engine__specification *const specification, struct expr_var *dynamic_variable)
{
    int max_type_id = -1;  // Initialize to -1 to indicate no IDs found yet
    bool existing_variable_found = false;
    int next_free_type_id = RULE_ENGINE__VAR_TYPE_ID_UNRESOLVED;

    if (expr_var_type(dynamic_variable) != RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE)
    {
        LOG(E, "Variable[%s] is not of type DDM2 instance!", dynamic_variable->name);
        return RULE_ENGINE__VAR_TYPE_ID_UNRESOLVED;
    }

    for (size_t iterr = 0; iterr < sorted_container__occupied(specification->p_rule_engine_inst->p_rule_engine_specifications); ++iterr)
    {
        uint32_t sspec_key;
        struct rule_engine__specification *specs;

        sorted_container__iterate(specification->p_rule_engine_inst->p_rule_engine_specifications, iterr, (void **)&specs, &sspec_key);

        // Do not check against yourself
        if (specification != specs)
        {
            for (struct expr_var *v = specs->expr.vars.head; v; v = v->next)
            {
                if (expr_var_type(v) == RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE)
                {
                    // Variable instance exists, take it the already assigned type ID
                    // Use hash to compare the variable names faster, instead of using strcmp()
                    if (expr_generate_hash_key(v->name) == expr_generate_hash_key(dynamic_variable->name))
                    {
                        // Found the exact same variable in another specification
                        next_free_type_id = expr_var_type_id(v);
                        existing_variable_found = true;
                        LOG(D, "Instance variable[%s] type ID already exists in another specification. Using: %d",
                            dynamic_variable->name, next_free_type_id);
                        break;
                    }
                    // Not the same variable instance, so check if it refers to the same DDM2 class
                    else if (DDMP2_INVENTORY_CLASS(expr_var_id(v)) == DDMP2_INVENTORY_CLASS(expr_var_id(dynamic_variable)))
                    {
                        // Track maximum type ID for this class
                        int current_type_id = expr_var_type_id(v);
                        if (current_type_id > max_type_id)
                        {
                            max_type_id = current_type_id;
                            LOG(D, "Found variable of same class[%s] with type ID: %d, max is now: %d",
                                v->name, current_type_id, max_type_id);
                        }
                    }
                    else
                    {
                        // continue checking other variables in this specification
                    }
                }
            }
        }

        if (existing_variable_found)
        {
            break;  // Found the exact same variable, no need to check other specifications
        }
    }

    // After checking the specifications, determine the next type ID
    if (!existing_variable_found)
    {
        if (max_type_id >= 0)
        {
            // We found variables with the same class, use max + 1 as next ID
            next_free_type_id = max_type_id + 1;
            LOG(D, "Instance variable[%s] assigned new type ID: %d (based on max ID: %d)",
                dynamic_variable->name, next_free_type_id, max_type_id);
        }
        else
        {
            // No variables with the same class found, start with ID 0
            next_free_type_id = 0;
            LOG(D, "Instance variable[%s] assigned initial type ID: %d",
                dynamic_variable->name, next_free_type_id);
        }
    }

    // Check for overflow
    if (next_free_type_id >= RULE_ENGINE__VAR_TYPE_ID_UNRESOLVED)
    {
        next_free_type_id = RULE_ENGINE__VAR_TYPE_ID_UNRESOLVED;
        LOG(E, "Instance variable[%s] type ID overflowed! Maximum instances per DDM2 class is: %d",
            dynamic_variable->name,
            RULE_ENGINE__VAR_TYPE_ID_UNRESOLVED - 1);
    }

    return next_free_type_id;
}

/** @brief Converts a class name to a DDM2 class ID
 *
 * Converts a class name to a DDM2 class ID by appending "AVL" to the class name
 * and then parsing the resulting string.
 *
 * @param ddm2_class Pointer to store the resulting DDM2 class ID
 * @param class_name The class name to convert
 * @param class_name_len The length of the class name
 * @return int The DDM2 class index, or -1 if conversion failed
 */
static int ddm2_class_id(uint32_t *ddm2_class, const char *class_name, size_t class_name_len)
{
    int ddm2_class_index = -1;

    // Input validation
    if (ddm2_class == NULL || class_name == NULL || class_name_len == 0)
    {
        LOG(E, "Invalid input parameters");
        return -1;
    }

    char class_name_avl[32];
    size_t class_name_size = sizeof(class_name_avl);
    const char *suffix = "AVL";
    size_t suffix_len = strlen(suffix);

    /* Copy the class name to our buffer - ensure room for suffix and null termination */
    if (class_name_len < (class_name_size - suffix_len - 1))
    {
        strncpy(class_name_avl, class_name, class_name_len);
        /* Ensure null termination after the class name */
        class_name_avl[class_name_len] = '\0';
        /* Append "AVL" to the class name */
        strcat(class_name_avl, suffix);

        /* Now, provide CLASS0AVL string as input argument to the conversion function */
        LOG(D, "Converting class name '%s' to DDM2 class ID", class_name_avl);
        ddm2_class_index = ddm2_parse_parameter_string(ddm2_class, class_name_avl, strlen(class_name_avl));

        if (ddm2_class_index == -1)
        {
            LOG(E, "Failed to parse DDM2 parameter string: %s", class_name_avl);
        }
    }
    else
    {
        LOG(E, "Class name too long: %s (length: %zu), max allowed: %zu",
            class_name, class_name_len, class_name_size - suffix_len - 1);
    }

    return ddm2_class_index; /* Return the class ID */
}

/**
 * @brief Analyzes variables in a rule engine specification, determines their types, and sets up appropriate identifiers.
 *
 * This function processes all variables in a rule engine specification based on the expression type:
 * - For inventory resolution expressions, it sets up instance and class variables
 * - For dynamic instance resolution expressions, it configures instance, parameter, and class variables
 * - For standard expressions, it processes all variables and determines their proper types (DDM2, dynamic DDM2, global, local)
 *
 * For each variable, the function:
 * 1. Determines the correct type (DDM2, dynamic DDM2, global, local)
 * 2. Assigns appropriate parameter IDs
 * 3. Configures resolution status and creation flags
 * 4. Updates the sensitivity list with variable information
 *
 * @param specification Pointer to the rule engine specification containing expression variables
 * @param sens_list Pointer to the sensitivity list to be populated with variable information
 *
 * @return RULE_ENGINE__OK on success, or appropriate error code on failure
 */
static int resolve_rule_variable_types(struct rule_engine__specification *const specification, struct rule_engine__sensitivity_list_parser *const sens_list)
{
    // Use the expression type that was determined and stored by resolve_rule_expression_type
    const rule_engine__expression_type_t *expr_type = &specification->expr.type;

    switch (expr_type->type)
    {
    case RULE_ENGINE__EXPRESSION_TYPE_INSTANCE_RESOLUTION_INV:
    {
        // extract user function arguments
        struct expr *ddm2_instance_expr = &expr_type->func->param.func.args.buf[0];
        TRUE_CHECK_RETURNX(RULE_ENGINE__INV_PTR, (ddm2_instance_expr != NULL));
        struct expr_var *ddm2_instance_var = expr_var(&specification->expr.vars, ddm2_instance_expr->param.var.name, strlen(ddm2_instance_expr->param.var.name));
        TRUE_CHECK_RETURNX(RULE_ENGINE__INV_PTR, (ddm2_instance_var != NULL));
        struct expr *ddm2_class_expr = &expr_type->func->param.func.args.buf[1];
        TRUE_CHECK_RETURNX(RULE_ENGINE__INV_PTR, (ddm2_class_expr != NULL));
        struct expr_var *ddm2_class_var = expr_var(&specification->expr.vars, ddm2_class_expr->param.var.name, strlen(ddm2_class_expr->param.var.name));
        TRUE_CHECK_RETURNX(RULE_ENGINE__INV_PTR, (ddm2_class_var != NULL));

        if ((determine_variable_expression_type(ddm2_instance_var) != RULE_ENGINE__VAR_TYPE_GLOBAL) &&
            (determine_variable_expression_type(ddm2_class_var) != RULE_ENGINE__VAR_TYPE_LOCAL))
        {
            // Should not be the case!
            return RULE_ENGINE__INV_VAR_TYPE;
        }

        // create GW0INV variable, so we can implicitly subscribe to the inventory
        struct expr_var *gw0inv_local_var = expr_var(&specification->expr.vars, RULE_ENGINE__INVENTORY, sizeof(RULE_ENGINE__INVENTORY) - 1);
        if (gw0inv_local_var == NULL)
        {
            LOG(E, "Rule[%s:%d]:Variable[%s] not created. Insufficient heap memory",
                specification->name, specification->id, RULE_ENGINE__INVENTORY);
            return RULE_ENGINE__INV_PTR;
        }
        // Use it as local variable, as a way to transfer ddm2 class parameters updates from inventory
        expr_set_var(gw0inv_local_var, 0);
        expr_set_var_id(gw0inv_local_var, GW0INV);
        expr_set_var_type(gw0inv_local_var, RULE_ENGINE__VAR_TYPE_LOCAL);
        expr_set_var_type_id(gw0inv_local_var, RULE_ENGINE__VAR_TYTE_ID_UNAPLICABLE);  // unapplicable
        expr_set_var_type_is_resolved(gw0inv_local_var, RULE_ENGINE__VAR_TYPE_RESOLVED);
        expr_set_var_type_ddm_create(gw0inv_local_var, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);

        // create it as instance instead of GLOBAL
        uint32_t ddm2_class = DDMP2_INVALID_CLASS;
        if (ddm2_class_id(&ddm2_class, ddm2_class_var->name, strlen(ddm2_class_var->name)) == -1)
        {
            LOG(E, "Invalid DDM2 class name[%s]:%d", ddm2_class_var->name, ddm2_class);
            return RULE_ENGINE__INV_VAR_TYPE;
        }
        expr_set_var(ddm2_instance_var, RULE_ENGINE__VAR_VALUE_INIT);
        expr_set_var_id(ddm2_instance_var, ddm2_class | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
        expr_set_var_type(ddm2_instance_var, RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
        expr_set_var_type_id(ddm2_instance_var, resolve_dynamic_instance_type_id(specification, ddm2_instance_var));
        expr_set_var_type_is_resolved(ddm2_instance_var, RULE_ENGINE__VAR_TYPE_UNRESOLVED);
        expr_set_var_type_ddm_create(ddm2_instance_var, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);

        expr_set_var(ddm2_class_var, RULE_ENGINE__VAR_VALUE_INIT);
        expr_set_var_id(ddm2_class_var, ddm2_class);
        expr_set_var_type(ddm2_class_var, RULE_ENGINE__VAR_TYPE_DDM2);
        expr_set_var_type_id(ddm2_class_var, RULE_ENGINE__VAR_TYTE_ID_UNAPLICABLE);
        expr_set_var_type_is_resolved(ddm2_class_var, RULE_ENGINE__VAR_TYPE_RESOLVED);
        expr_set_var_type_ddm_create(ddm2_class_var, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED);
        break;
    }
    case RULE_ENGINE__EXPRESSION_TYPE_INSTANCE_RESOLUTION_DYN_INST:
    {
        int variable_exp_type;

        TRUE_CHECK_RETURNX(RULE_ENGINE__INV_PTR, (expr_type->func != NULL));
        TRUE_CHECK_RETURNX(RULE_ENGINE__INV_PTR, (expr_type->func->param.func.args.buf != NULL));
        TRUE_CHECK_RETURNX(RULE_ENGINE__INV_PTR, (expr_type->func->param.func.args.len == 4));

        // extract user function arguments - use pointer access to avoid struct copying
        struct expr *ddm2_instance_expr = &expr_type->func->param.func.args.buf[0];
        TRUE_CHECK_RETURNX(RULE_ENGINE__INV_PTR, (ddm2_instance_expr != NULL));
        struct expr_var *ddm2_instance_var = expr_var(&specification->expr.vars, ddm2_instance_expr->param.var.name, strlen(ddm2_instance_expr->param.var.name));
        TRUE_CHECK_RETURNX(RULE_ENGINE__INV_PTR, (ddm2_instance_var != NULL));
        struct expr *ddm2_parameter_expr = &expr_type->func->param.func.args.buf[1];
        TRUE_CHECK_RETURNX(RULE_ENGINE__INV_PTR, (ddm2_parameter_expr != NULL));
        struct expr_var *ddm2_parameter_var = expr_var(&specification->expr.vars, ddm2_parameter_expr->param.var.name, strlen(ddm2_parameter_expr->param.var.name));
        TRUE_CHECK_RETURNX(RULE_ENGINE__INV_PTR, (ddm2_parameter_var != NULL));
        struct expr *ddm2_class_expr = &expr_type->func->param.func.args.buf[2];
        TRUE_CHECK_RETURNX(RULE_ENGINE__INV_PTR, (ddm2_class_expr != NULL));
        struct expr_var *ddm2_class_var = expr_var(&specification->expr.vars, ddm2_class_expr->param.var.name, strlen(ddm2_class_expr->param.var.name));
        TRUE_CHECK_RETURNX(RULE_ENGINE__INV_PTR, (ddm2_class_var != NULL));

        if ((determine_variable_expression_type(ddm2_instance_var) != RULE_ENGINE__VAR_TYPE_GLOBAL))
        {
            // Should not be the case!
            LOG(E, "ddm2_instance_var: %s:%d", ddm2_instance_var->name, determine_variable_expression_type(ddm2_instance_var));
            return RULE_ENGINE__INV_VAR_TYPE;
        }

        // create it as instance instead of GLOBAL
        uint32_t ddm2_class = DDMP2_INVALID_CLASS;
        if (ddm2_class_id(&ddm2_class, ddm2_class_var->name, strlen(ddm2_class_var->name)) == -1)
        {
            LOG(E, "Invalid DDM2 class name[%s]:%d", ddm2_class_var->name, ddm2_class);
            return RULE_ENGINE__INV_VAR_TYPE;
        }
        expr_set_var(ddm2_instance_var, RULE_ENGINE__VAR_VALUE_INIT);
        expr_set_var_id(ddm2_instance_var, ddm2_class | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
        expr_set_var_type(ddm2_instance_var, RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE);
        expr_set_var_type_id(ddm2_instance_var, resolve_dynamic_instance_type_id(specification, ddm2_instance_var));
        expr_set_var_type_is_resolved(ddm2_instance_var, RULE_ENGINE__VAR_TYPE_UNRESOLVED);
        expr_set_var_type_ddm_create(ddm2_instance_var, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);

        variable_exp_type = determine_variable_expression_type(ddm2_parameter_var);
        if ((variable_exp_type != RULE_ENGINE__VAR_TYPE_DDM2) &&
            (variable_exp_type != RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC))
        {
            // Should not be the case!
            LOG(E, "ddm2_parameter_var: %s:%d", ddm2_parameter_var->name, variable_exp_type);
            return RULE_ENGINE__INV_VAR_TYPE;
        }

        uint32_t ddm2_parameter_id;
        char ddm2_parameter_name[RULE_ENGINE__VAR_NAME_LEN];
        size_t ddm2_parameter_name_len = sizeof(ddm2_parameter_name);
        char ddm2_parameter_instance_name[RULE_ENGINE__VAR_NAME_LEN];
        size_t ddm2_parameter_instance_name_len = sizeof(ddm2_parameter_instance_name);
        if (variable_exp_type == RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC)
        {
            struct expr_var *parameter_instance_var = parse_dynamic_ddm2_parameter(
                specification,
                ddm2_parameter_var,
                ddm2_parameter_name,
                &ddm2_parameter_name_len,
                ddm2_parameter_instance_name,
                &ddm2_parameter_instance_name_len);

            if (parameter_instance_var != NULL)
            {
                // Now, the dynamic ddm2_parameter
                int index = ddm2_parse_parameter_string(&ddm2_parameter_id, ddm2_parameter_name, ddm2_parameter_name_len);
                if (index != -1)
                {
                    // Create dynamic parameter
                    expr_set_var(ddm2_parameter_var, RULE_ENGINE__VAR_VALUE_INIT);
                    expr_set_var_id(ddm2_parameter_var, ddm2_parameter_id | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
                    expr_set_var_type(ddm2_parameter_var, RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
                    expr_set_var_type_id(ddm2_parameter_var, resolve_dynamic_instance_type_id(specification, parameter_instance_var));
                    expr_set_var_type_is_resolved(ddm2_parameter_var, RULE_ENGINE__VAR_TYPE_UNRESOLVED);
                    expr_set_var_type_ddm_create(ddm2_parameter_var, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED);
                }
                else
                {
                    // Error, has to be ddm2_parameter
                    LOG(E, "Invalid DDM2 parameter name: %s", ddm2_parameter_name);
                    return RULE_ENGINE__INV_VAR_TYPE;
                }
            }
            else
            {
                LOG(E, "Invalid DDM2 dynamic parameter name: %s", ddm2_parameter_name);
                return RULE_ENGINE__INV_DYN_VAR_NAME;
            }
        }
        else if (variable_exp_type == RULE_ENGINE__VAR_TYPE_DDM2)
        {
            ddm2_parameter_name_len = copy_string_to_buffer(ddm2_parameter_var->name, ddm2_parameter_name, ddm2_parameter_name_len);
            int index = ddm2_parse_parameter_string(&ddm2_parameter_id, ddm2_parameter_name, ddm2_parameter_name_len);
            if (index != -1)
            {
                // Create parameter_instance_var as instance instead of GLOBAL
                expr_set_var(ddm2_parameter_var, RULE_ENGINE__VAR_VALUE_INIT);
                expr_set_var_id(ddm2_parameter_var, ddm2_parameter_id);
                expr_set_var_type(ddm2_parameter_var, RULE_ENGINE__VAR_TYPE_DDM2);
                expr_set_var_type_id(ddm2_parameter_var, RULE_ENGINE__VAR_TYTE_ID_UNAPLICABLE);
                expr_set_var_type_is_resolved(ddm2_parameter_var, RULE_ENGINE__VAR_TYPE_RESOLVED);  // resolved, you know your instance
                expr_set_var_type_ddm_create(ddm2_parameter_var, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED);
            }
            else
            {
                // Error, has to be ddm2_parameter
                return RULE_ENGINE__INV_VAR_TYPE;
            }
        }

        expr_set_var(ddm2_class_var, RULE_ENGINE__VAR_VALUE_INIT);
        expr_set_var_id(ddm2_class_var, ddm2_class);
        expr_set_var_type(ddm2_class_var, RULE_ENGINE__VAR_TYPE_DDM2);
        expr_set_var_type_id(ddm2_class_var, RULE_ENGINE__VAR_TYPE_ID_UNRESOLVED);
        expr_set_var_type_is_resolved(ddm2_class_var, RULE_ENGINE__VAR_TYPE_RESOLVED);
        expr_set_var_type_ddm_create(ddm2_class_var, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED);

        // Add ddm2 as trigger of the rule
        sens_list->variables[sens_list->n_variables].name = ddm2_parameter_var->name;
        sens_list->variables[sens_list->n_variables].value = expr_var_value(ddm2_parameter_var);
        sens_list->variables[sens_list->n_variables].id = expr_var_id(ddm2_parameter_var);
        sens_list->variables[sens_list->n_variables].type = expr_var_type(ddm2_parameter_var);
        sens_list->variables[sens_list->n_variables].type_id = expr_var_type_id(ddm2_parameter_var);
        sens_list->variables[sens_list->n_variables].type_is_resolved = expr_var_type_is_resolved(ddm2_parameter_var);
        sens_list->n_variables++;

        break;
    }
    case RULE_ENGINE__EXPRESSION_TYPE_STANDARD:
    {
        // detect correct variable type and assign correct parameter id depending
        // on variable type: RULE_ENGINE__VAR_TYPE_DDM2 gets the id from ddm2 table,
        // RULE_ENGINE__VAR_TYPE_GLOBAL/LOCAL gets hash genereated id from expr_generate_hash_key()
        // RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC gets the id from ddm2 table, but also
        // resolves the dynamic instance type ID and assigns it to the instance variable

        for (struct expr_var *var = specification->expr.vars.head; var; var = var->next)
        {
            switch (determine_variable_expression_type(var))
            {
            case RULE_ENGINE__VAR_TYPE_DDM2:
            {
                uint32_t ddm2_parameter_id;
                char ddm2_parameter_name[RULE_ENGINE__VAR_NAME_LEN];
                size_t ddm2_parameter_name_len = sizeof(ddm2_parameter_name);

                ddm2_parameter_name_len = copy_string_to_buffer(var->name, ddm2_parameter_name, ddm2_parameter_name_len);
                int index = ddm2_parse_parameter_string(&ddm2_parameter_id, ddm2_parameter_name, ddm2_parameter_name_len);
                if (index == -1)
                {
                    LOG(E, "Invalid DDM2 parameter name: %s", ddm2_parameter_name);
                    return RULE_ENGINE__INV_VAR_TYPE;
                }

                char class_name[RULE_ENGINE__VAR_NAME_LEN];
                size_t class_name_size = sizeof(class_name);
                ddm2_instance_name(ddm2_parameter_id, class_name, &class_name_size);
                if (class_name_size == 0)
                {
                    LOG(E, "Failed ddm2_instance_name: %s ", ddm2_parameter_name);
                }
                // Do we need to create the instance?
                if (expr_var_type_ddm_create(var) && (RULE0ACT != DDM2_PARAMETER_BASE_INSTANCE(ddm2_parameter_id)))
                {
                    uint32_t *p_class_instance = NULL;
                    if ((p_class_instance = sorted_container__access(specification->p_rule_engine_inst->p_rule_engine_ddm_map, expr_generate_hash_key(class_name))) == NULL)
                    {
                        // Not found.
                        // Register class, request broker instance
                        int inst = broker_register_instance(&ddm2_parameter_id, specification->p_rule_engine_inst->wrapper->connector->connector_id);

                        p_class_instance = sorted_container__new(specification->p_rule_engine_inst->p_rule_engine_ddm_map, expr_generate_hash_key(class_name));
                        if (p_class_instance == NULL)
                        {
                            LOG(E, "Could not store ddm class instance map for %s", ddm2_parameter_name);
                        }
                        else
                        {
                            LOG(D, "Create ddm class (%s) instance: %d", ddm2_parameter_name, inst);
                            // Save instance
                            *p_class_instance = inst;
                            ddm2_parameter_id = (DDM2_PARAMETER_BASE_INSTANCE(ddm2_parameter_id) | DDM2_PARAMETER_INSTANCE(*p_class_instance));
                        }
                    }
                    else
                    {
                        LOG(D, "Found existing ddm class (%s) instance: %d", ddm2_parameter_name, *p_class_instance);
                        ddm2_parameter_id = (DDM2_PARAMETER_BASE_INSTANCE(ddm2_parameter_id) | DDM2_PARAMETER_INSTANCE(*p_class_instance));
                    }
                }
                else
                {
                    // Lookup in storage
                    uint32_t *p_class_instance = NULL;
                    if ((p_class_instance = sorted_container__access(specification->p_rule_engine_inst->p_rule_engine_ddm_map, expr_generate_hash_key(class_name) /*DDM2_PARAMETER_CLASS_INSTANCE(param_id)*/)) != NULL)
                    {
                        // Use stored instance
                        // LOG(D, "Found existing ddm class (%s) instance: %d", parameter_name, *p_class_instance);
                        ddm2_parameter_id = (DDM2_PARAMETER_BASE_INSTANCE(ddm2_parameter_id) | DDM2_PARAMETER_INSTANCE(*p_class_instance));
                    }
                    else
                    {
                        // LOG(E, "Could not find ddm class instance map for %s", parameter_name);
                    }
                }

                // Now we have correct parameter value.
                expr_set_var_id(var, ddm2_parameter_id);
                expr_set_var_type(var, RULE_ENGINE__VAR_TYPE_DDM2);
                expr_set_var_type_id(var, RULE_ENGINE__VAR_TYTE_ID_UNAPLICABLE);
                expr_set_var_type_is_resolved(var, RULE_ENGINE__VAR_TYPE_RESOLVED);

                break;
            }
            case RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC:
            {
                uint32_t ddm2_parameter_id;
                char ddm2_parameter_name[RULE_ENGINE__VAR_NAME_LEN];
                size_t ddm2_parameter_name_len = sizeof(ddm2_parameter_name);
                char ddm2_parameter_instance_name[RULE_ENGINE__VAR_NAME_LEN];
                size_t ddm2_parameter_instance_name_len = sizeof(ddm2_parameter_instance_name);

                struct expr_var *ddm2_parameter_var = var;
                struct expr_var *parameter_instance_var = parse_dynamic_ddm2_parameter(
                    specification,
                    ddm2_parameter_var,
                    ddm2_parameter_name,
                    &ddm2_parameter_name_len,
                    ddm2_parameter_instance_name,
                    &ddm2_parameter_instance_name_len);

                if (parameter_instance_var != NULL)
                {
                    // Now, the dynamic ddm2_parameter
                    int index = ddm2_parse_parameter_string(&ddm2_parameter_id, ddm2_parameter_name, ddm2_parameter_name_len);
                    if (index != -1)
                    {
                        expr_set_var(ddm2_parameter_var, RULE_ENGINE__VAR_VALUE_INIT);
                        expr_set_var_id(ddm2_parameter_var, ddm2_parameter_id | DDM2_PARAMETER_INSTANCE(DDMP2_INVALID_INSTANCE));
                        expr_set_var_type(ddm2_parameter_var, RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC);
                        expr_set_var_type_id(ddm2_parameter_var, resolve_dynamic_instance_type_id(specification, parameter_instance_var));
                        expr_set_var_type_is_resolved(ddm2_parameter_var, RULE_ENGINE__VAR_TYPE_UNRESOLVED);
                        expr_set_var_type_ddm_create(ddm2_parameter_var, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED);
                    }
                    else
                    {
                        // Error, has to be ddm2_parameter
                        LOG(E, "Invalid DDM2 parameter name: %s", ddm2_parameter_name);
                        return RULE_ENGINE__INV_VAR_TYPE;
                    }
                }
                else
                {
                    LOG(E, "Invalid DDM2 dynamic parameter name: %s", ddm2_parameter_name);
                    return RULE_ENGINE__INV_DYN_VAR_NAME;
                }

                break;
            }
            case RULE_ENGINE__VAR_TYPE_GLOBAL:
            {
                // Global variable
                expr_set_var_id(var, expr_generate_hash_key(var->name));
                expr_set_var_type(var, RULE_ENGINE__VAR_TYPE_GLOBAL);
                expr_set_var_type_id(var, RULE_ENGINE__VAR_TYTE_ID_UNAPLICABLE);
                expr_set_var_type_is_resolved(var, RULE_ENGINE__VAR_TYPE_RESOLVED);
                expr_set_var_type_ddm_create(var, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);
                break;
            }
            case RULE_ENGINE__VAR_TYPE_LOCAL:
            {
                // Local variable
                expr_set_var_id(var, expr_generate_hash_key(var->name));
                expr_set_var_type(var, RULE_ENGINE__VAR_TYPE_LOCAL);
                expr_set_var_type_id(var, RULE_ENGINE__VAR_TYTE_ID_UNAPLICABLE);
                expr_set_var_type_is_resolved(var, RULE_ENGINE__VAR_TYPE_RESOLVED);
                expr_set_var_type_ddm_create(var, RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE);
                break;
            }
            case RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE:
            {
                LOG(E, "Variable[%s] is of type DDM2 instance, but it should not be used in this context!", var->name);
                return RULE_ENGINE__INV_VAR_TYPE;
            }
            default:
            {
                LOG(E, "Unknown variable type for variable: %s", var->name);
                return RULE_ENGINE__INV_VAR_TYPE;
            }
            }

            // Add all variables to the sensitivity list
            for (uint32_t i = 0; (i < sens_list->n_variables) && (sens_list->variables[i].name != NULL); ++i)
            {
                if (strings_equal(sens_list->variables[i].name, strlen(sens_list->variables[i].name), var->name, strlen(var->name)) == true)
                {
                    sens_list->variables[i].id = expr_var_id(var);
                    sens_list->variables[i].type = expr_var_type(var);
                    sens_list->variables[i].type_id = expr_var_type_id(var);
                    sens_list->variables[i].type_is_resolved = expr_var_type_is_resolved(var);
                }
            }
        }
        break;
    }
    default:
        LOG(E, "Unknown rule expression type: %d", expr_type->type);
        return RULE_ENGINE__INV_RULE_TYPE;
    }

    return RULE_ENGINE__OK;
}

/**
 * @brief Links variables from sensitivity list with rule expression variables
 *
 * This function ensures that all variables defined in a rule's sensitivity list
 * are also available in the rule's expression variable list. It serves two main purposes:
 *
 * 1. For variables that exist only in the sensitivity list but not in the rule expression
 *    (i.e., variables used only as triggers), it creates the corresponding variable
 *    in the expression variable list.
 *
 * 2. For variables that exist in both lists, it ensures that important properties like
 *    ddm_create flag are preserved from the sensitivity list definition.
 *
 * This synchronization is essential as variables can appear in either:
 * - The sensitivity list only (as trigger variables)
 * - The rule expression only (as condition variables)
 * - Both places (as both trigger and condition variables)
 *
 * @param specification The rule engine specification containing the expression variable list
 * @param sens_list The parsed sensitivity list containing variables that trigger the rule
 * @return RULE_ENGINE__OK on success, or appropriate error code on failure
 */
static int bind_sens_list_vars_with_rule_list_vars(struct rule_engine__specification *const specification, const struct rule_engine__sensitivity_list_parser *const sens_list)
{
    for (uint32_t n_vars = 0; n_vars < sens_list->n_variables; ++n_vars)
    {
        struct expr_var *exp_var_exist = NULL;
        if ((exp_var_exist = expr_get_var_by_name(&specification->expr.vars, sens_list->variables[n_vars].name, strlen(sens_list->variables[n_vars].name))) == NULL)
        {
            // Variable does not exists in rule's definition, create it manually.
            // This is a case when a variable is used only as a trigger of the rule.
            struct expr_var *exp_var;
            if ((exp_var = expr_var(&specification->expr.vars, sens_list->variables[n_vars].name, strlen(sens_list->variables[n_vars].name))) == NULL)
            {
                LOG(E, "Rule[%s:%d]:Variable[%s] not created. Insufficient heap memory",
                    specification->name, specification->id,
                    sens_list->variables[n_vars].name);
                return RULE_ENGINE__INV_PTR;
            }
            // Make sure the parsed ddm create is kept
            expr_set_var_type_ddm_create(exp_var, sens_list->variables[n_vars].ddm_create);
        }
        else
        {
            // Make sure the parsed ddm create is kept
            expr_set_var_type_ddm_create(exp_var_exist, sens_list->variables[n_vars].ddm_create);
        }
    }

    return RULE_ENGINE__OK;
}

static int resolve_rule_expression_type(struct rule_engine__specification *const specification)
{
    // Store the expression type in the specification for later use
    rule_engine__expression_type_t *expr_type = &specification->expr.type;

    int type = determine_rule_expression_type(specification, expr_type);
    switch (type)
    {
    case RULE_ENGINE__EXPRESSION_TYPE_INSTANCE_RESOLUTION_INV:
    {
        if (specification->p_rule_engine_inst->wrapper->inventory_cb != NULL)
        {
            LOG(E, "Inventory callback has already been set for rule[%s:%d]! 'inventory' user function and inventory callback cannot be used together.",
                specification->name, specification->id);
            return RULE_ENGINE__INV_RULE_TYPE;
        }
        break;
    }
    case RULE_ENGINE__EXPRESSION_TYPE_INSTANCE_RESOLUTION_DYN_INST:  // fallthrough
    case RULE_ENGINE__EXPRESSION_TYPE_STANDARD:                      // fallthrough
    default:
        break;
    }

    return RULE_ENGINE__OK;
}

static int create_rule_expression(struct rule_engine__specification *specification, const char *const rule, struct rule_engine__sensitivity_list_parser *const sens_list)
{
    int status;

    specification->expr.rule = expr_create(rule, strlen(rule), (struct expr_var_list *)&specification->expr.vars, (const struct expr_func *const)&l_user_funcs, specification->id);
    if (specification->expr.rule == NULL)
    {
        LOG(E, "Invalid rule format[%s]!", rule);
        return RULE_ENGINE__INV_RULE_FORMAT;
    }

    status = bind_sens_list_vars_with_rule_list_vars(specification, sens_list);
    if (status != RULE_ENGINE__OK)
    {
        return status;
    }

    status = resolve_rule_expression_type(specification);
    if (status != RULE_ENGINE__OK)
    {
        return status;
    }

    status = resolve_rule_variable_types(specification, sens_list);
    if (status != RULE_ENGINE__OK)
    {
        return status;
    }

    resolve_rule_expression(specification);

    return RULE_ENGINE__OK;
}

static int parse_sens_list(struct rule_engine__sensitivity_list_parser *const sens_list, char *sens_list_str)
{
    char *token = NULL;
    uint32_t n_variables = 0;

    sens_list->mask = 0;
    sens_list->mask_type = RULE_ENGINE__MASK_CONSTRAINT_AND;
    sens_list->n_variables = 0;
    ZERO(sens_list->variables);

    // If no sensitivity list provided, return with empty list
    if (sens_list_str == NULL)
    {
        LOG(D, "No sensitivity list provided, using empty list");
        return RULE_ENGINE__OK;
    }

    // Divide parameters and mask from sensitivity list
    token = strtok_r(sens_list_str, ",", &sens_list_str);
    while (token)
    {
        char *token_name_str = NULL;
        char *token_value_str = NULL;

        token_name_str = strtok_r(token, "= ", &token_value_str);

        if (!token_name_str)
        {
            LOG(E, "No name of variable specified:[%s]!", token);
            return RULE_ENGINE__INV_RULE_FORMAT;
        }

        if (strings_equal(token_name_str, strlen(token_name_str), RULE_ENGINE__MASK, sizeof(RULE_ENGINE__MASK) - 1) == true)
        {
            char *mask_token = NULL;

            if (!token_value_str)
            {
                LOG(E, "No \"%s\" parameters assigned!", token_name_str);
                return RULE_ENGINE__INV_RULE_FORMAT;
            }

            while ((mask_token = strtok_r(NULL, " =[;]", &token_value_str)))
            {
                if (strings_equal(mask_token, strlen(mask_token), RULE_ENGINE__MASK_CONSTRAINT_OR_STR, sizeof(RULE_ENGINE__MASK_CONSTRAINT_OR_STR) - 1) == true)
                {
                    sens_list->mask_type = RULE_ENGINE__MASK_CONSTRAINT_OR;
                }
                else if (strings_equal(mask_token, strlen(mask_token), RULE_ENGINE__MASK_CONSTRAINT_AND_STR, sizeof(RULE_ENGINE__MASK_CONSTRAINT_AND_STR) - 1) == true)
                {
                    sens_list->mask_type = RULE_ENGINE__MASK_CONSTRAINT_AND;
                }
                else
                {
                    char *end_ptr;
                    uint32_t mask_value = strtol(mask_token, &end_ptr, 16);
                    if ((!mask_value) || (mask_token == end_ptr) || (*end_ptr != '\0'))
                    {
                        LOG(E, "Invalid \"mask\" parameter[%s]."
                               " Accepted format: mask=[<value>;<constraint>]",
                            mask_token);
                        return RULE_ENGINE__INV_RULE_FORMAT;
                    }
                    sens_list->mask = mask_value;
                }
            }
        }
        else
        {
            int32_t value = 0;
            char *value_str = NULL;

            if (n_variables >= ELEMENTS(sens_list->variables))
            {
                LOG(E, "Rule's sensitivity list depth exceeded!");
                return RULE_ENGINE__RULES_DEPTH;
            }

            if ((value_str = strtok_r(NULL, "= ", &token_value_str)))
            {
                // Check if this is a DDM2 parameter
                if (strncmp(value_str, RULE_ENGINE__DDM, sizeof(RULE_ENGINE__DDM) - 1) == 0)
                {
                    char *ddm_token = NULL;
                    char *tmp_value = value_str;

                    // Parse the value inside ddm(...)
                    if ((ddm_token = strtok_r(value_str, " ()", &tmp_value)))
                    {
                        if ((ddm_token = strtok_r(NULL, " ()", &tmp_value)))
                        {
                            char *endPtr = NULL;
                            value = strtol(ddm_token, &endPtr, 10);

                            if ((ddm_token == endPtr) || (*endPtr != '\0'))
                            {
                                LOG(E, "Invalid DDM2 parameter initialization: [%s]", ddm_token);
                                return RULE_ENGINE__INV_RULE_FORMAT;
                            }

                            sens_list->variables[n_variables].initialized = true;
                        }
                        else
                        {
                            LOG(W, "No valid initialization value found for DDM2 parameter: [%s]", value_str);
                        }
                    }
                    else
                    {
                    }
                    sens_list->variables[n_variables].ddm_create = RULE_ENGINE__VAR_TYPE_DDM2_CREATE_OWNED;
                }
                else
                {
                    // Parse a regular variable value
                    char *endPtr = NULL;
                    value = strtol(value_str, &endPtr, 10);

                    if ((value_str == endPtr) || (*endPtr != '\0'))
                    {
                        LOG(E, "Invalid variable initialization: [%s]", value_str);
                        return RULE_ENGINE__INV_RULE_FORMAT;
                    }

                    sens_list->variables[n_variables].initialized = true;
                }
            }
            else
            {
                // Handle variables without values
                LOG(D, "Variable [%s] has no initialization value. Defaulting to 0.", token_name_str);
                value = 0;
            }

            // Store the variable in the sensitivity list
            sens_list->variables[n_variables].name = token_name_str;
            sens_list->variables[n_variables++].value = value;
        }

        token = strtok_r(NULL, ",", &sens_list_str);  // Move to the next token
    }

    sens_list->n_variables = n_variables;

    return RULE_ENGINE__OK;
}

static int split_rule_and_sens_list(char *rule_str, char **rule, char **sens_list)
{
    char *sens_list_str = NULL;
    char *colon_pos = strchr(rule_str, ':');

    if (colon_pos == NULL)
    {
        // No colon found - the entire string is the rule
        *rule = rule_str;
        *sens_list = NULL;  // No sensitivity list
    }
    else
    {
        // Colon found - split into sensitivity list and rule
        // Sens list:" <-- :", Rule: " : -->"
        sens_list_str = strtok_r(rule_str, ":", rule);
        *sens_list = sens_list_str;
    }
    return RULE_ENGINE__OK;
}

static int create_rule(struct rule_engine__specification *specification, int rule_str_size)
{
    int status;
    char *rule = NULL;
    char rule_str[300];
    char *sens_list_str = NULL;
    struct rule_engine__sensitivity_list_parser sens_list;

    if (rule_str_size >= (int)sizeof(rule_str))
    {
        LOG(E, "Too large rule string for parsing");
    }
    copy_string_to_buffer(specification->expr.rule_string, rule_str, (size_t)MIN(sizeof(rule_str), (size_t)(rule_str_size + 1)));

    status = split_rule_and_sens_list(rule_str, &rule, &sens_list_str);
    if (status != RULE_ENGINE__OK)
    {
        return status;
    }

    status = parse_sens_list(&sens_list, sens_list_str);
    if (status != RULE_ENGINE__OK)
    {
        return status;
    }

    status = create_rule_expression(specification, rule, &sens_list);
    if (status != RULE_ENGINE__OK)
    {
        return status;
    }

    status = init_rule_variables(specification, &sens_list);
    if (status != RULE_ENGINE__OK)
    {
        return status;
    }

    status = create_rule_mask(specification, &sens_list);
    if (status != RULE_ENGINE__OK)
    {
        return status;
    }

    status = create_trigger_section(specification, &sens_list);
    if (status != RULE_ENGINE__OK)
    {
        return status;
    }

    status = setup_user_functions(specification);
    if (status != RULE_ENGINE__OK)
    {
        return status;
    }

    return RULE_ENGINE__OK;
}

static struct rule_engine__specification *create_new_rule_specification(rule_engine_inst_t *const p_rule_engine_inst, const struct rule_engine__rule *const rule, uint32_t *const rule_id)
{
    struct rule_engine__specification *specification = NULL;

    // keep the order of inserted rules
    *rule_id = sorted_container__occupied(p_rule_engine_inst->p_rule_engine_specifications);
    if (*rule_id != 0)
    {
        struct rule_engine__specification *last_rule = sorted_container__access(p_rule_engine_inst->p_rule_engine_specifications, *rule_id - 1);  // hash_key(rule->name);
        *rule_id = last_rule->id + 1;
    }

    specification = sorted_container__new(p_rule_engine_inst->p_rule_engine_specifications, *rule_id);
    if (!specification)
    {
        return NULL;
    }

    specification->execute = false;
    // Cannot know where the rule string originates from. Therefore we have to make a "persistent" copy
    //    specification->expr.rule_string = heap_caps_malloc_prefer(strlen(rule->rule)+1, 2, MALLOC_CAP_DEFAULT | MALLOC_CAP_SPIRAM, MALLOC_CAP_DEFAULT | MALLOC_CAP_INTERNAL);
    //    assert(specification->expr.rule_string != NULL);
    //    strcpy(specification->expr.rule_string, (rule->rule));
    // We don't need this string after create and parse
    specification->expr.rule_string = rule->rule;
    specification->id = *rule_id;
#ifdef RULE_ENGINE__PRINT_RULE_NAME_ENABLED
    copy_string_to_buffer(rule->name, specification->name, sizeof(specification->name));
#else
    specification->name = (char *)rule->name;
#endif
    return specification;
}

/**
 * @brief Sets up DDM2 parameter subscriptions and ownership for a rule specification
 *
 * Processes all static DDM2 variables in the rule's expression and configures them
 * for either subscription or ownership based on their sensitivity list declarations.
 *
 * @param specification The rule specification to process
 * @return RULE_ENGINE__OK on success, error code on failure
 */
static int setup_standard_rule_ddm2_parameters(const struct rule_engine__specification *const specification)
{
    // Only process DDM2 parameters if the rule expression type is standard, since the dynamic instance
    // and inventory resolution has different schemes for handling DDM2 parameters(will be handled through
    // dynamic_variables_subscription() functions instead).
    if (specification->expr.type.type == RULE_ENGINE__EXPRESSION_TYPE_STANDARD)
    {
        // Process all DDM2 variables (only static ones) in the rule's expression
        for (struct expr_var *var = specification->expr.vars.head; var; var = var->next)
        {
            if (expr_var_type(var) == RULE_ENGINE__VAR_TYPE_DDM2)
            {
                setup_ddm2_parameter(specification, var);
            }
        }
    }

    return RULE_ENGINE__OK;
}

/**
 * @brief Sets up inventory callback for inventory resolution rules
 *
 * Configures the inventory callback for rules that use the inventory() function
 * to resolve dynamic instances.
 *
 * @param specification The rule specification to check and configure
 * @return RULE_ENGINE__OK on success, error code on failure
 */
static int setup_inventory_callback(const struct rule_engine__specification *const specification)
{
    // Set up inventory callback for inventory resolution rules
    if (specification->expr.type.type == RULE_ENGINE__EXPRESSION_TYPE_INSTANCE_RESOLUTION_INV)
    {
        ddmw_t *ddmw_wrapper = specification->p_rule_engine_inst->wrapper;
        if (ddmw_wrapper->inventory_context_cb == NULL)
        {
            ddmw_get_inventory_with_context(ddmw_wrapper, resolution_of_dynamic_instances_inventory_cb, (void *)specification->p_rule_engine_inst);
        }
    }

    return RULE_ENGINE__OK;
}

/**
 * @brief Creates and configures a DDM2 parameter subscription or ownership
 *
 * @param specification The rule specification context
 * @param var The DDM2 variable to set up
 * @return RULE_ENGINE__OK on success, error code on failure
 */
static int setup_ddm2_parameter(const struct rule_engine__specification *specification, const struct expr_var *var)
{
    if ((expr_var_type(var) != RULE_ENGINE__VAR_TYPE_DDM2) && (expr_var_type(var) != RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC))
    {
        LOG(E, "Variable '%s' is not a DDM2 parameter!", var->name);
        return RULE_ENGINE__INV_VAR_TYPE;
    }

    // Create new or get already existing DDM2 item in subscription table
    ddmw_item_t *item = sorted_container__new(specification->p_rule_engine_inst->p_rule_engine_ddm2_subscription_table, expr_var_id(var));
    if (item == NULL)
    {
        LOG(E, "Failed to create subscription entry for parameter '%s'", var->name);
        return RULE_ENGINE__SUBS_DEPTH;
    }

    // Set up the parameter based on it's intended type
    ddmw_action_t desired_type = determine_ddm2_variable_action_type(var);
    if (desired_type == DDMW_ACTION_PUBLISH)
    {
        setup_ddm2_owned_parameter(specification, item, var);
    }
    else
    {
        setup_ddm2_subscribed_parameter(specification, item, var);
    }

    // Detect EVENT type for DDM2 parameters
    detect_and_setup_ddm2_event_parameter(item, var);

    return RULE_ENGINE__OK;
}

/**
 * @brief Determines the subscription type for a DDM2 parameter based on ddm_create flag
 *
 * @param var The expression variable to check
 * @return DDMW_ACTION_PUBLISH for owned parameters, DDMW_ACTION_SET for subscribed parameters
 */
static ddmw_action_t determine_ddm2_variable_action_type(const struct expr_var *var)
{
    if (expr_var_type_ddm_create(var) == RULE_ENGINE__VAR_TYPE_DDM2_CREATE_OWNED)
    {
        return DDMW_ACTION_PUBLISH;  // This rule owns/publishes this parameter
    }
    else if (expr_var_type_ddm_create(var) == RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED)
    {
        return DDMW_ACTION_SET;  // This rule subscribes to this parameter
    }
    else
    {
        return DDMW_ACTION_SET;  // Default to subscription if not specified(should never end up here)
    }
}

/**
 * @brief Gets the appropriate DDM2 type for serializing outbound parameter data
 *
 * Sets the DDM2 type to use when sending data based on the action type.
 * This determines how data should be marshaled for transmission:
 * - PUBLISH action: Returns out_type (for publishing owned parameter values)
 * - SET action: Returns in_type (for sending SET requests to subscribed parameters)
 *
 * Note: This function is for OUTBOUND operations only (sending data). It does not
 * apply to receiving/deserializing incoming data.
 *
 * @param ddm2_var The DDM2 variable to check
 * @param type Output parameter to receive the determined DDM2 type
 * @return 1 on success, 0 on failure
 */
static int get_ddm2_type_for_write_action(const struct expr_var *ddm2_var, DDM2_TYPE_ENUM *type)
{
    int parameter_index = ddm2_parameter_list_lookup(DDM2_PARAMETER_BASE_INSTANCE(expr_var_id(ddm2_var)));
    TRUE_CHECK_RETURN0(parameter_index != -1);

    ddmw_action_t parameter_action_type = determine_ddm2_variable_action_type(ddm2_var);
    if (parameter_action_type == DDMW_ACTION_PUBLISH)
    {
        // direction is PUBLISH, so we are interested in the output type
        *type = Ddm2_parameter_list_data[parameter_index].out_type;
    }
    else  // DDMW_ACTION_SET
    {
        // direction is SET, so we are interested in the input type
        *type = Ddm2_parameter_list_data[parameter_index].in_type;
    }

    return 1;
}

/**
 * @brief Sets up a DDM2 parameter for publishing (rule owns/publishes this parameter)
 *
 * Registers as publisher with DDMW_ACTION_PUBLISH type, sets availability flag
 * for AVL properties, and initializes with variable's current value.
 *
 * @param specification The rule specification
 * @param item The DDM2 wrapper item
 * @param var The DDM2 variable to configure
 */
static void setup_ddm2_owned_parameter(const struct rule_engine__specification *specification, ddmw_item_t *item, const struct expr_var *var)
{
    // Register and set up as publisher
    ddmw_add(specification->p_rule_engine_inst->wrapper, item, expr_var_id(var), DDM2_PARAMETER_INSTANCE_FIELD(expr_var_id(var)));
    ddmw_set_type(item, DDMW_ACTION_PUBLISH);

    // Set availability flag for availability properties
    if (DDM2_IS_AVAIL_PROPERTY(expr_var_id(var)))
    {
        ddmw_set_data(item, &One, sizeof(One));
    }

    // Set initial value from the variable
    ddmw_set_i32(item, expr_var_value(var));

    LOG(I, "Set up owned parameter '%s' (0x%08X) with initial value %d", var->name, expr_var_id(var), expr_var_value(var));
}

/**
 * @brief Sets up a DDM2 parameter for subscription
 *
 * Configures a DDM2 parameter to receive PUBLISH frames from the broker system.
 * For new items, initializes as subscription with DDMW_ACTION_SET type.
 * Respects existing ownership (won't modify DDMW_ACTION_PUBLISH items).
 * For trigger variables: sets initial values and subscribes for updates.
 *
 * @param specification The rule specification
 * @param item The DDM2 wrapper item (new or existing)
 * @param var The DDM2 variable to configure
 */
static void setup_ddm2_subscribed_parameter(const struct rule_engine__specification *specification, ddmw_item_t *item, const struct expr_var *var)
{
    bool item_was_new = (item->parameter == 0);  // Check if item was just created

    if (item_was_new)
    {
        // New item - set it up as a subscription
        ddmw_add(specification->p_rule_engine_inst->wrapper, item, expr_var_id(var), DDM2_PARAMETER_INSTANCE_FIELD(expr_var_id(var)));
        ddmw_set_type(item, DDMW_ACTION_SET);

        LOG(D, "Created new subscription for parameter '%s' (0x%08X)", var->name, expr_var_id(var));
    }
    else if (ddmw_get_type(item) != DDMW_ACTION_SET)
    {
        // Item exists but is not a subscription (it is owned by another rule)
        LOG(D, "Item '%s' (0x%08X) already exists but is not a subscription, changing type to SET", var->name, expr_var_id(var));
        return;  // Don't modify existing owned parameters, ownership is respected
    }

    // Handle trigger section variables (variables that can trigger the rule)
    const struct rule_engine__sensitivity_list *sens_var;
    const struct rule_engine__trigger_section *trigger_section = &specification->trigger;
    if ((sens_var = rule_engine_sens_list_contains_expr_var(trigger_section->variables, trigger_section->n_params, var)) != NULL)
    {
        // Set initial value if it was initialized in sensitivity list
        if (rule_engine_sens_list_variable_is_initialized(sens_var))
        {
            ddmw_set_i32(item, expr_var_value(var));
            LOG(D, "Set initial value %d for trigger parameter '%s'", expr_var_value(var), var->name);
        }

        // Avoid double subscriptions
        if (ddmw_is_subscribed(item) == false)
        {
            // Subscribe to get updates for trigger variables
            ddmw_subscribe(item);
            LOG(D, "Subscribed to trigger parameter '%s' (0x%08X)", var->name, expr_var_id(var));
        }
        else
        {
            LOG(D, "DDM2 trigger parameter instance '%s' already subscribed!", var->name);
        }
    }
}

/**
 * @brief Detects and configures EVENT type for DDM2 parameters
 *
 * This function checks if a DDM2 parameter should be configured as an event parameter
 * based on its parameter ID. Event parameters are sent immediately when set rather
 * than waiting for the normal publish cycle.
 *
 * NOTE: Generic detection of event parameters should be based on a dedicated flag/bit
 * in the 'Connectivity_Parameters_List', but such information is currently not available.
 *
 * @param item The DDM2 wrapper item to potentially configure as event parameter
 * @param var The DDM2 variable being processed
 * @return RULE_ENGINE__OK on success, error code on failure
 */
static int detect_and_setup_ddm2_event_parameter(ddmw_item_t *item, const struct expr_var *var)
{
    uint32_t parameter_base = DDM2_PARAMETER_BASE_INSTANCE(expr_var_id(var));

    switch (parameter_base)
    {
    case EVM0TRIG:
        // EVM0TRIG is an event trigger parameter - configure as event parameter
        ddmw_set_requires_immediate_processing(item, true);
        break;
    default:
        // Regular parameter - not an event parameter
        break;
    }

    return RULE_ENGINE__OK;
}

/**
 * @brief Removes a DDM2 parameter from the rule engine's subscription table.
 *
 * @param specification The rule specification context
 * @param var The DDM2 variable to remove
 * @return RULE_ENGINE__OK on success, error code on failure
 */
static int remove_ddm2_parameter(const struct rule_engine__specification *specification, const struct expr_var *var)
{
    if ((expr_var_type(var) != RULE_ENGINE__VAR_TYPE_DDM2) && (expr_var_type(var) != RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC))
    {
        LOG(E, "Variable '%s' is not a DDM2 parameter!", var->name);
        return RULE_ENGINE__INV_VAR_TYPE;
    }

    // Get the DDM2 item from the subscription table
    ddmw_item_t *item = sorted_container__access(specification->p_rule_engine_inst->p_rule_engine_ddm2_subscription_table, expr_var_id(var));
    if (item == NULL)
    {
        LOG(E, "No DDM2 item found for variable '%s' (0x%08X)", var->name, expr_var_id(var));
        return RULE_ENGINE__INV_PTR;
    }

    sorted_container__delete(specification->p_rule_engine_inst->p_rule_engine_ddm2_subscription_table, expr_var_id(var));
    ddmw_remove(specification->p_rule_engine_inst->wrapper, item);

    return RULE_ENGINE__OK;
}

static void rule_engine_resubscribe_rules_parameters(rule_engine_inst_t *const p_rule_engine_inst)
{
    ddmw_item_t *item = p_rule_engine_inst->wrapper->items;
    while (item)
    {
        // Resubscribe to all parameters in trigger sections
        if (ddmw_is_subscribed(item) && (ddmw_get_type(item) == DDMW_ACTION_SET))
        {
            ddmw_subscribe(item);
        }
        item = item->next;
    }
}

static void delete_rule_specification(rule_engine_inst_t *const p_rule_engine_inst, uint32_t rule_id)
{
    struct rule_engine__specification *rule = sorted_container__access(p_rule_engine_inst->p_rule_engine_specifications, rule_id);
    if (rule != NULL)
    {
        /* Delete rule expression and assosiated expession variables */
        expr_destroy(rule->expr.rule, &rule->expr.vars);
        /* Reset specification */
        memset(rule, 0, sizeof(struct rule_engine__specification));
    }

    sorted_container__delete(p_rule_engine_inst->p_rule_engine_specifications, rule_id);
}

static void delete_rule_specifications(rule_engine_inst_t *const p_rule_engine_inst)
{
    for (size_t i = 0; i < sorted_container__occupied(p_rule_engine_inst->p_rule_engine_specifications); ++i)
    {
        uint32_t rule_id;
        struct rule_engine__specification *rule;

        sorted_container__iterate(p_rule_engine_inst->p_rule_engine_specifications, i, (void **)&rule, &rule_id);
        if (rule)
        {
            /* Delete rule expression and assosiated expession variables */
            expr_destroy(rule->expr.rule, &rule->expr.vars);
            /* Reset specification */
            memset(rule, 0, sizeof(struct rule_engine__specification));
        }
    }
}

static void delete_rule_specifications_parameters(rule_engine_inst_t *const p_rule_engine_inst)
{
    for (size_t i = 0; i < sorted_container__occupied(p_rule_engine_inst->p_rule_engine_ddm2_subscription_table); i++)
    {
        uint32_t id;
        ddmw_item_t *item;

        sorted_container__iterate(p_rule_engine_inst->p_rule_engine_ddm2_subscription_table, i, (void **)&item, &id);
        ddmw_remove(p_rule_engine_inst->wrapper, item);
    }
}

static ddmw_item_t *rule_engine_get_activation_parameter(rule_engine_inst_t *const p_rule_engine_inst)
{
    return ddmw_find_item(p_rule_engine_inst->wrapper, RULE0ACT | DDM2_PARAMETER_INSTANCE(p_rule_engine_inst->instance));
}

static bool rule_engine_is_active(rule_engine_inst_t *const p_rule_engine_inst)
{
    return ddmw_get_i32(rule_engine_get_activation_parameter(p_rule_engine_inst));
}

static bool rule_engine_should_evaluate_rules(rule_engine_inst_t *const p_rule_engine_inst)
{
    bool should_evaluate_rules = false;

    ddmw_item_t *activation_parameter = rule_engine_get_activation_parameter(p_rule_engine_inst);
    /* Use 'modified' instaed of 'updated', as it
     * will be modifed for SET and PUBLISH frames */
    if (activation_parameter->modified == true)
    {
        /* Changing from Inactive to Active state.
         * Resubscribe to all parameters before you
         * request evaluating of the rules. */
        if (rule_engine_is_active(p_rule_engine_inst) == true)
        {
            rule_engine_resubscribe_rules_parameters(p_rule_engine_inst);
            should_evaluate_rules = true;
        }
        else
        {
            /* Changing from Active to Inactive state.
             * Request evaluating of the rules before
             * you go in inactive state. It allows triggering
             * of the rules dependent on RULE0ACT parmeter(if any)*/
            should_evaluate_rules = true;
        }
    }
    else
    {
        /* If in Active state, request evaluating of the rules */
        if (rule_engine_is_active(p_rule_engine_inst) == true)
        {
            should_evaluate_rules = true;
        }
        else
        {
            // Inactive state, do nothing.
        }
    }

    return should_evaluate_rules;
}

#if (RULE_ENGINE__TEST_VARIABLE_IDS != 0)
void test_variables_ids(struct rule_engine__specification *specification)
{
    for (size_t s = 0; s < sorted_container__occupied(specification->p_rule_engine_inst->p_rule_engine_specifications); s++)
    {
        uint32_t sspec_key;
        struct rule_engine__specification *existing_spec;

        sorted_container__iterate(specification->p_rule_engine_inst->p_rule_engine_specifications, s, (void **)&existing_spec, &sspec_key);

        if (specification != existing_spec)
        {
            for (struct expr_var *spec_var = specification->expr.vars.head; spec_var; spec_var = spec_var->next)
            {
                for (struct expr_var *existing_spec_var = existing_spec->expr.vars.head; existing_spec_var; existing_spec_var = existing_spec_var->next)
                {
                    // Test if any RULE_ENGINE__VAR_TYPE_GLOBAL/LOCAL variable has the ID
                    // as any RULE_ENGINE__VAR_TYPE_DDM2 parameter in the existisng rules
                    if (expr_var_id(spec_var) == expr_var_id(existing_spec_var))
                    {
                        // By type
                        if (expr_var_type(spec_var) != expr_var_type(existing_spec_var))
                        {
                            LOG(W, "Same ID generated for variables with different types: [%s][0x%X] type[%d] && [%s][0x%" PRIX32 "] type[%d]",
                                spec_var->name, spec_var->id, spec_var->type,
                                existing_spec_var->name, existing_spec_var->id, existing_spec_var->type);
                        }

                        // By name
                        if (strings_equal(spec_var->name, strlen(spec_var->name), existing_spec_var->name, strlen(existing_spec_var->name)) == false)
                        {
                            LOG(W, "Same ID generated for variables with different names: [%s][0x%X] && [%s][0x%" PRIX32 "]",
                                spec_var->name, spec_var->id,
                                existing_spec_var->name, existing_spec_var->id);
                        }
                    }
                }
            }
        }
    }

    // Test if any RULE_ENGINE__VAR_TYPE_GLOBAL/LOCAL has ID as any DDM2 parameter in ddm2 parameter list
    for (struct expr_var *spec_var = specification->expr.vars.head; spec_var; spec_var = spec_var->next)
    {
        if ((expr_var_type(spec_var) == RULE_ENGINE__VAR_TYPE_GLOBAL) ||
            (expr_var_type(spec_var) == RULE_ENGINE__VAR_TYPE_LOCAL))
        {
            if (ddm2_parameter_list_lookup(expr_var_id(spec_var)) != -1)
            {
                char parameter_name[64];
                size_t parameter_name_size = sizeof(parameter_name);

                LOG(W, "Global variable[%s] ID[0x%X] is the same as DDM2 parameter[%s]",
                    spec_var->name,
                    expr_var_id(spec_var),
                    ddm2_parameter_name(expr_var_id(spec_var), parameter_name, &parameter_name_size));
            }
        }
    }
}
#endif  // defined(RULE_ENGINE__TEST_VARIABLE_IDS)
static const char nullpointer[] = "";
int rule_engine__add_specification(rule_engine_inst_t *const p_rule_engine_inst, const struct rule_engine__rule *const rule)
{
    int status;
    uint32_t rule_id;
    struct rule_engine__specification *specification = NULL;

    TRUE_CHECK_RETURN0((rule != NULL));

    specification = create_new_rule_specification(p_rule_engine_inst, rule, &rule_id);
    if (specification == NULL)
    {
        LOG(E, "Rule[%s] specification not created, status[%d]", rule->name, RULE_ENGINE__RULES_DEPTH);
        return RULE_ENGINE__RULES_DEPTH;
    }
    specification->p_rule_engine_inst = p_rule_engine_inst;
    status = create_rule(specification, rule->size);
    if (status != RULE_ENGINE__OK)
    {
        LOG(E, "Rule[%s] not created, status[%d]", rule->name, status);
        delete_rule_specification(p_rule_engine_inst, rule_id);
        return status;
    }

    // Set up DDM2 parameter subscriptions and ownership
    status = setup_standard_rule_ddm2_parameters(specification);
    if (status != RULE_ENGINE__OK)
    {
        return status;
    }

    // Set up inventory callback if needed
    status = setup_inventory_callback(specification);
    if (status != RULE_ENGINE__OK)
    {
        return status;
    }

#if RULE_ENGINE__TEST_VARIABLE_IDS
    test_variables_ids(specification);
#endif  // RULE_ENGINE__TEST_VARIABLE_IDS

#ifndef RULE_ENGINE__PRINT_RULE_NAME_ENABLED
    specification->name = (char *)nullpointer;  // Cannot trust that memory anymore
#endif
    // Update value
    ddmw_item_t *num_item = ddmw_find_item(p_rule_engine_inst->wrapper, RULE0NUM | DDM2_PARAMETER_INSTANCE(p_rule_engine_inst->instance));
    if (num_item)
    {
        int32_t num = (int32_t)rule_engine_get_number_of_rules(p_rule_engine_inst);
        ddmw_set_i32(num_item, num);
    }
    return 1;
}

int rule_engine_initialize_instance(rule_engine_inst_t *p_rule_engine_inst, const rule_engine_inst_config_t *config)
{
    TRUE_CHECK_RETURN0((p_rule_engine_inst != NULL));
    TRUE_CHECK_RETURN0((p_rule_engine_inst->wrapper != NULL));
    TRUE_CHECK_RETURN0(p_rule_engine_inst->instance >= 0);

    p_rule_engine_inst->p_rule_engine_specifications = sorted_container__create(sizeof(struct rule_engine__specification), config->rule_engine_specification_elements);
    TRUE_CHECK_RETURN0((p_rule_engine_inst->p_rule_engine_specifications != NULL));
    p_rule_engine_inst->p_rule_engine_ddm2_subscription_table = sorted_container__create(sizeof(ddmw_item_t), config->rule_engine_ddm2_subscription_table_elements);
    TRUE_CHECK_RETURN0((p_rule_engine_inst->p_rule_engine_ddm2_subscription_table != NULL));
    p_rule_engine_inst->p_rule_engine_set_timers = sorted_container__create(sizeof(struct rule_engine__timer), config->rule_engine_set_timers_elements);
    TRUE_CHECK_RETURN0((p_rule_engine_inst->p_rule_engine_set_timers != NULL));
    p_rule_engine_inst->p_rule_engine_ddm_map = sorted_container__create(sizeof(uint32_t), config->rule_engine_ddm_map_elements);
    TRUE_CHECK_RETURN0((p_rule_engine_inst->p_rule_engine_ddm_map != NULL));

    ddmw_item_t *rules_instance_activate = sorted_container__new(
        p_rule_engine_inst->p_rule_engine_ddm2_subscription_table,
        RULE0ACT | DDM2_PARAMETER_INSTANCE(p_rule_engine_inst->instance));
    TRUE_CHECK_RETURN0((rules_instance_activate != NULL));

    ddmw_add(p_rule_engine_inst->wrapper, rules_instance_activate, RULE0ACT, p_rule_engine_inst->instance);
    ddmw_set_type(rules_instance_activate, DDMW_ACTION_PUBLISH);
    ddmw_set_i32(rules_instance_activate, 0);
    // Prevent triggering rule evaluation during initialization.
    // Setting modified=false ensures that the first iteration after rule loading
    // (where we subscribe to parameters) won't automatically trigger rule evaluation.
    // Rules will only be evaluated when actual parameter updates occur after initialization.
    rules_instance_activate->modified = false;

    p_rule_engine_inst->rules_valid = true;

    return 1;
}

/** @brief This function is called trough ddmw_process function whenever GW0INV is published by the broker */
static void resolution_of_dynamic_instances_inventory_cb(uint32_t parameter_class, void *context)
{
    // Resolve rule engine instance
    const rule_engine_inst_t *p_rule_engine_inst = (rule_engine_inst_t *)context;
    // Evaluate rule expressions inventory functions
    for (size_t s = 0; s < sorted_container__occupied(p_rule_engine_inst->p_rule_engine_specifications); s++)
    {
        uint32_t sspec_key;
        struct rule_engine__specification *specification;

        sorted_container__iterate(p_rule_engine_inst->p_rule_engine_specifications, s, (void **)&specification, &sspec_key);

        if (specification->expr.type.type == RULE_ENGINE__EXPRESSION_TYPE_INSTANCE_RESOLUTION_INV)
        {
            struct expr_var *gw0inv_local = expr_get_var_by_id_and_type(&specification->expr.vars, GW0INV, RULE_ENGINE__VAR_TYPE_LOCAL);
            TRUE_CHECK_RETURN(gw0inv_local != NULL);

            // Transfer inventory update to gw0inv local variable
            expr_set_var(gw0inv_local, parameter_class);
            // Translates to 'inventory()' user function
            expr_eval(specification->expr.rule);
        }
    }
}

static void resolution_of_dynamic_instances_dyn_instance(rule_engine_inst_t *const p_rule_engine_inst)
{
    bool has_ddm2_dynamic_parameter_been_updated = false;
    for (size_t s = 0; s < sorted_container__occupied(p_rule_engine_inst->p_rule_engine_specifications); s++)
    {
        uint32_t sspec_key;
        struct rule_engine__specification *specification;

        sorted_container__iterate(p_rule_engine_inst->p_rule_engine_specifications, s, (void **)&specification, &sspec_key);

        if (specification->expr.type.type == RULE_ENGINE__EXPRESSION_TYPE_INSTANCE_RESOLUTION_DYN_INST)
        {
            bool should_resolve = false;
            struct rule_engine__sensitivity_list *variable;

            TRUE_CHECK_RETURN((specification->trigger.n_params == 1));
            TRUE_CHECK_RETURN((variable = &specification->trigger.variables[0]) != NULL);  // DNY_INST has only one variable in trigger section i.e. dynamic variable

            // Check if the parameter is inserted in the DDM2 wrapper. If so, it means that
            // it's instance is resolved and update(PUBLISH) from the broker is received.
            ddmw_item_t *ddm2_dynamic_parameter_item = sorted_container__access(p_rule_engine_inst->p_rule_engine_ddm2_subscription_table, expr_var_id(variable->variable));
            if (ddm2_dynamic_parameter_item == NULL)
            {
                // If the parameter is not found, it means that the instance is not resolved yet or it was unresolved,
                // but another resolution rule might have triggered it so we can execute the resolution flow.
                if (trigger_rule(specification))
                {
                    should_resolve = true;
                    clear_trigger_section(specification);
                }
            }
            else
            {
                if (ddmw_is_updated(ddm2_dynamic_parameter_item) || has_ddm2_dynamic_parameter_been_updated)
                {
                    should_resolve = true;
                    has_ddm2_dynamic_parameter_been_updated = true;  // allows reoslution of multiple rules dependent
                                                                     // on same ddm2 dynamic parameter in one iteration
                }
            }

            if (should_resolve)
            {
                // Translates to 'dyn_instance()' user function
                expr_eval(specification->expr.rule);
            }
        }
    }
}

void rule_engine_task_process(rule_engine_inst_t *const p_rule_engine_inst, const DDMP2_FRAME *frame)
{
    ddmw_process(p_rule_engine_inst->wrapper, frame);

    resolution_of_dynamic_instances_dyn_instance(p_rule_engine_inst);

    if (rule_engine_should_evaluate_rules(p_rule_engine_inst) == true)
    {
        bool has_any_rule_trigger_section_updated = false;

        if (ddmw_is_generic_event_updated(p_rule_engine_inst->wrapper))
        {
            uint32_t event_id;
            ddmw_event_t event;

            event_id = ddmw_get_generic_event_id(p_rule_engine_inst->wrapper);
            event.data_size = ddmw_get_generic_event_data(p_rule_engine_inst->wrapper, event.data, sizeof(event.data));
            switch (event_id)
            {
            case RULE_ENGINE__TIMER_TRIGG_EVENT:
            {
                has_any_rule_trigger_section_updated = rule_engine__evaluate_timers(p_rule_engine_inst, event.data, event.data_size);
                break;
            }
            default:
                LOG(E, "Invalid generic event[%d]!", event_id);
                return;
            }
        }
        else
        {
            has_any_rule_trigger_section_updated = rule_engine__evaluate_ddm2_parameters(p_rule_engine_inst);
        }

        if (has_any_rule_trigger_section_updated)
        {
            rule_engine__evaluate_rules(p_rule_engine_inst);
        }
    }

    ddmw_process_publish(p_rule_engine_inst->wrapper);
}

void rule_engine_activate(rule_engine_inst_t *const p_rule_engine_inst, uint8_t activate)
{
    ddmw_item_t *rules_instance_activate = rule_engine_get_activation_parameter(p_rule_engine_inst);
    TRUE_CHECK_RETURN((rules_instance_activate != NULL));

    if (ddmw_get_i32(rules_instance_activate) != activate)
    {
        ddmw_set_i32(rules_instance_activate, activate);
        rules_instance_activate->updated = true;
    }
}

int rule_engine_get_number_of_rules(rule_engine_inst_t *const p_rule_engine_inst)
{
    return sorted_container__occupied(p_rule_engine_inst->p_rule_engine_specifications);
}

void rule_engine_delete_rules(rule_engine_inst_t *const p_rule_engine_inst)
{
    delete_rule_specifications_parameters(p_rule_engine_inst);
    delete_rule_specifications(p_rule_engine_inst);

    sorted_container__delete_all(p_rule_engine_inst->p_rule_engine_ddm2_subscription_table);
    sorted_container__destroy(p_rule_engine_inst->p_rule_engine_ddm2_subscription_table);
    sorted_container__delete_all(p_rule_engine_inst->p_rule_engine_ddm_map);
    sorted_container__destroy(p_rule_engine_inst->p_rule_engine_ddm_map);
    sorted_container__delete_all(p_rule_engine_inst->p_rule_engine_set_timers);
    sorted_container__destroy(p_rule_engine_inst->p_rule_engine_set_timers);
    sorted_container__delete_all(p_rule_engine_inst->p_rule_engine_specifications);
    sorted_container__destroy(p_rule_engine_inst->p_rule_engine_specifications);
}

void rule_engine_set_inventory_user_cb(rule_engine_inst_t *const p_rule_engine_inst, inventory_user_cb_t inventory_user_cb)
{
    if (p_rule_engine_inst != NULL)
    {
        if (p_rule_engine_inst->inventory_user_cb != NULL)
        {
            LOG(W, "Overwriting existing inventory user callback function pointer");
        }
        p_rule_engine_inst->inventory_user_cb = inventory_user_cb;
    }
}
