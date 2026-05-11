/*! \file rule_engine.c
    \brief Rule engine implementation

    Before initializing the rule engine instance, the user has to allocate and create the following sorted_containers:

    SORTED_CONTAINER__DECLARE_EXTRAM(rule_engine_ddm2_subscription_table, RULE_ENGINE__DDM2_SUBSCRIPTION_DEPTH, ddmw_item_t);
    SORTED_CONTAINER__DECLARE_EXTRAM(rule_engine_specifications, RULE_ENGINE__MAX_NUMBER_OF_RULES, struct rule_engine__specification);
    SORTED_CONTAINER__DECLARE_EXTRAM(rule_engine_set_parameters, RULE_ENGINE__DDM2_SET_PARAMS_DEPTH, struct rule_engine__set_parameter);
    SORTED_CONTAINER__DECLARE_EXTRAM(rule_engine_set_timers, RULE_ENGINE__MAX_NUMBER_OF_TIMERS, struct rule_engine__timer);

    And also set the corresponding pointers in the rule engine instance.

    \author Borjan Bozhinovski, Andreas Lundeen
*/
#ifndef RULE_ENGINE_H_
#define RULE_ENGINE_H_

#include "configuration.h"

#include <stddef.h>
#include <stdint.h>

#include "connector.h"
#include "ddm2.h"
#include "ddm_wrapper.h"
#include "sorted_container.h"

/**
 * @brief Possible to be overridden globally (in config) for each instance to reduce memory consumption for small set of rules
 *
 * Override by defining in dicm_framework_config.h
 *
 */
#ifndef CONFIG_RULE_ENGINE__MAX_NUMBER_OF_VARS_PER_RULE
#define RULE_ENGINE__MAX_NUMBER_OF_VARS_PER_RULE 16
#else
#define RULE_ENGINE__MAX_NUMBER_OF_VARS_PER_RULE CONFIG_RULE_ENGINE__MAX_NUMBER_OF_VARS_PER_RULE

#endif
/**
 * @brief Possible to be overridden locally (smaller) for each instance to reduce memory consumption for small set of rules.
 *
 * Override by defining before including this file in module instance implementation.
 */
#ifndef RULE_ENGINE__MAX_NUMBER_OF_RULES
#define RULE_ENGINE__MAX_NUMBER_OF_RULES 200
#endif

/**
 * @brief Possible to be overridden locally (smaller) for each instance to reduce memory consumption for small set of rules
 *
 * Override by defining before including this file in module instance implementation.
 */
#ifndef RULE_ENGINE__DDM2_MAP_SIZE
#define RULE_ENGINE__DDM2_MAP_SIZE 50
#endif

#define RULE_ENGINE__DDM2_SUBSCRIPTION_DEPTH (RULE_ENGINE__MAX_NUMBER_OF_VARS_PER_RULE * RULE_ENGINE__MAX_NUMBER_OF_RULES)

#define RULE_ENGINE__VAR_NAME_LEN  32
#define RULE_ENGINE__RULE_NAME_LEN 32

#define RULE_ENGINE__MAX_NUMBER_OF_TIMERS 20

/** @brief
 *  Callback function pointer for GW0INV events.
 *  This function is called when a GW0INV event occurs, allowing the user to handle
 *  the event in a way to detected if the instance of interest is avaialbe in the system
 *  It should be used for instance resolution(mainly for group/feature classes).
 *
 *  If used, it should be defined in the connectors code and set using `rule_engine_set_inventory_user_cb`.
 *
 *  @param rule_id          The unique identifier of the rule that triggered the event
 *  @param ddm2_class       The DDM2 class for which the inventory event was generated (class|instance|avl)
 *  @param current_instance The currently resolved instance for this rule (DDMP2_INVALID_INSTANCE if unresolved)
 *  @return                 Return value: DDMP2_INVALID_INSTANCE to unresolve, otherwise desired instance value
 */
typedef int (*inventory_user_cb_t)(uint32_t rule_id, uint32_t ddm2_class, uint32_t current_instance);

/**
 * @brief Rule engine instance configuration
 *
 * This structure defines the configuration parameters for a rule engine instance.
 * It specifies the number of elements for various sorted containers used by the rule engine,
 * allowing customization of memory allocation based on the specific needs of the application(instance).
 */
typedef struct rule_engine_inst_config
{
    uint16_t rule_engine_specification_elements;           /**< Number of rule specifications(p_rule_engine_specifications). */
    uint16_t rule_engine_ddm2_subscription_table_elements; /**< Number of DDM2 subscription table elements(p_rule_engine_ddm2_subscription_table). */
    uint8_t rule_engine_set_timers_elements;               /**< Number of timers(p_rule_engine_set_timers). */
    uint8_t rule_engine_ddm_map_elements;                  /**< Number of DDM2 class instance mappings(p_rule_engine_ddm_map). */
} rule_engine_inst_config_t;

/**
 * @brief Rule Engine instance definition
 *
 * This structure holds all information about the instantiated rule engine instance, including
 * memory containers and other data structures required by the library.
 * The structure is allocated and initialized by the `rule_engine_initialize_instance` function.
 * Users only need to provide a `rule_engine_inst_config_t` with initialized values to specify
 * the depth (number of elements) for each container.
 */
typedef struct rule_engine_inst
{
    ddmw_t *wrapper;                                                /**< Pointer to the DDM2 wrapper object */
    int instance;                                                   /**< Instance of RULE0 class in wrapper object */
    struct sorted_container *p_rule_engine_specifications;          /**< Pointer to storage of all rule engine specifications */
    struct sorted_container *p_rule_engine_ddm2_subscription_table; /**< Pointer to storage of subscribed DDM2 parameters required by rules. */
    struct sorted_container *p_rule_engine_set_timers;              /**< Pointer to storage of rule engine timers */
    struct sorted_container *p_rule_engine_ddm_map;                 /**< Pointer to storage of ddm2 class instances maps */
    bool rules_valid;                                               /*!< \~ True if timers are created successfully and ddm2 parameters are
                                                                    successfully added by all of the executed rules into the sorted container
                                                                    */
    inventory_user_cb_t inventory_user_cb;                          /**< Callback function pointer for rule engine events */
} rule_engine_inst_t;

/**
 * @brief Checks the instance and initialized internal instance variables
 *
 * @param[in,out] p_rule_engine_inst Instance to be initialized
 * @return 1 if successful, 0 if failed - not possible to use instance
 */
int rule_engine_initialize_instance(rule_engine_inst_t *p_rule_engine_inst, const rule_engine_inst_config_t *config);

/**
 * @brief Process the rule engine task
 *
 * @param[in,out] p_rule_engine_inst Instance to be processed
 * @param[in] frame Pointer to the DDM2 frame
 */
void rule_engine_task_process(rule_engine_inst_t *const p_rule_engine_inst, const DDMP2_FRAME *frame);

/**
 * @brief Returns the number of rules added to this instance
 *
 * @param p_rule_engine_inst instance
 * @return number of rules
 */
int rule_engine_get_number_of_rules(rule_engine_inst_t *const p_rule_engine_inst);

/**
 * @brief Activate/deactivate rule engine instance
 *
 * @param p_rule_engine_inst instance
 * @param activate activation status: Activate(1) / Deactivate(0)
 */
void rule_engine_activate(rule_engine_inst_t *const p_rule_engine_inst, uint8_t activate);

/**
 * @brief Inventory callback function for GW0INV events
 *
 * @param p_rule_engine_inst instance
 * @param inventory_user_cb callback function to be called on GW0INV events
 */
void rule_engine_set_inventory_user_cb(rule_engine_inst_t *const p_rule_engine_inst, inventory_user_cb_t inventory_user_cb);

/**
 * @brief Delete rules
 *
 * @param p_rule_engine_inst instance
 */
void rule_engine_delete_rules(rule_engine_inst_t *const p_rule_engine_inst);

#endif  // RULE_ENGINE_H_
