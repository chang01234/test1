/*! \file connector_smart_eco_rule_engine.c
    \author Borjan Bozhinovski
    \brief Smart Eco Rule engine implementation.

    This connector maintains a list of rules that will combine DDM2 parameters to
    generate events that will generate a publish of other DDM2 parameter
*/
#include <string.h>

#include "configuration.h"

#include "connector_smart_eco_rule_engine.h"
#include "ddm2.h"
#include "ddm_entry.h"
#include "ddm_store.h"
#include "ddm_wrapper.h"
#include "freertos/task.h"
#include "load_configurations_rule_engine.h"
#include "load_configurations_smart_eco_rule_engine.h"
#include "utils.h"

/**********************************************************
 * Private types
 *********************************************************/
typedef struct
{
    // Owned DDM parameters
    ddmw_item_t name;  // Name
    ddmw_item_t num;   // Number of rules
} rule_engine_param_t;

typedef struct
{
    // DDM wrapper
    ddmw_t ddm;
    rule_engine_param_t items;

    rule_engine_inst_t rule_engine_inst;
} connector_smart_eco_rule_engine_t;

static EXT_RAM_ATTR connector_smart_eco_rule_engine_t smart_eco_rule_engine;

/**********************************************************
 * Private functions
 *********************************************************/
static int connector_smart_eco_rule_engine_init(void);
static void connector_smart_eco_rule_engine_task(const DDMP2_FRAME *frame);
static int connector_smart_eco_rule_engine_load_configurations(void);
static bool connector_smart_eco_rule_engine_register(void);
static void connector_smart_eco_rule_engine_inventory_callback(uint32_t parameter);
static int connector_smart_eco_rule_engine_init_tables(int instance);

/**********************************************************
 * Public variables
 *********************************************************/
CONNECTOR connector_smart_eco_rule_engine =
    {
        .name = "Smart Eco (Rule engine) connector",
        .initialize = connector_smart_eco_rule_engine_init,
        .process_event = connector_smart_eco_rule_engine_task,
};

/**********************************************************
 * Implementation
 *********************************************************/
/**
 * @brief This is called for any change in inventory
 *
 * @param parameter
 */
static void connector_smart_eco_rule_engine_inventory_callback(uint32_t parameter)
{
    if (SYSAPPL0 == DDMP2_INVENTORY_CLASS_INSTANCE(parameter))
    {
        if (DDMP2_INVENTORY_AVL(parameter))
        {
            rule_engine_activate(&smart_eco_rule_engine.rule_engine_inst, 1);
        }
        else
        {
            rule_engine_activate(&smart_eco_rule_engine.rule_engine_inst, 0);
        }
    }
}

static bool connector_smart_eco_rule_engine_register(void)
{
    bool is_registration_successful = false;

    int instance = ddmw_register(&smart_eco_rule_engine.ddm, RULE0);
    if (instance != -1)
    {
        if (connector_smart_eco_rule_engine_init_tables(instance) == 1)
        {
            if (connector_smart_eco_rule_engine_load_configurations() == 0)
            {
                is_registration_successful = true;

                ddmw_add(&smart_eco_rule_engine.ddm, &smart_eco_rule_engine.items.name, RULE0NAME, instance);
                ddmw_set_str(&smart_eco_rule_engine.items.name, "Smart Eco rules");
                ddmw_add(&smart_eco_rule_engine.ddm, &smart_eco_rule_engine.items.num, RULE0NUM, instance);
                ddmw_set_i32(&smart_eco_rule_engine.items.num, rule_engine_get_number_of_rules(&smart_eco_rule_engine.rule_engine_inst));
            }
            else
            {
                LOG(E, "Invalid Smart Eco configuration!");
            }
        }
        else
        {
            LOG(E, "Invalid Smart Eco tables initialzation!");
        }
    }

    return is_registration_successful;
}

static void connector_smart_eco_rule_engine_task(const DDMP2_FRAME *frame)
{
    rule_engine_task_process(&smart_eco_rule_engine.rule_engine_inst, frame);
}

static int connector_smart_eco_rule_engine_init_tables(int instance)
{
    // Initialization of Rule Engine tables
    rule_engine_inst_config_t rule_engine_inst_config =
        {
            .rule_engine_specification_elements = RULE_ENGINE__MAX_NUMBER_OF_RULES,
            .rule_engine_ddm2_subscription_table_elements = RULE_ENGINE__DDM2_SUBSCRIPTION_DEPTH,
            .rule_engine_set_timers_elements = RULE_ENGINE__MAX_NUMBER_OF_TIMERS,
            .rule_engine_ddm_map_elements = RULE_ENGINE__DDM2_MAP_SIZE,
        };
    smart_eco_rule_engine.rule_engine_inst.wrapper = &smart_eco_rule_engine.ddm;
    smart_eco_rule_engine.rule_engine_inst.instance = instance;
    return rule_engine_initialize_instance(&smart_eco_rule_engine.rule_engine_inst, &rule_engine_inst_config);
}

static int connector_smart_eco_rule_engine_init(void)
{
    memset(&smart_eco_rule_engine, 0, sizeof(smart_eco_rule_engine));

    ddmw_init(&smart_eco_rule_engine.ddm, &connector_smart_eco_rule_engine);
    ddmw_get_inventory(&smart_eco_rule_engine.ddm, connector_smart_eco_rule_engine_inventory_callback);

    connector_smart_eco_rule_engine_register();

    return 1;
}

static int connector_smart_eco_rule_engine_load_configurations(void)
{
    return load_rule_engine_configuration(&smart_eco_rule_engine.rule_engine_inst, smart_eco_rule_engine__descriptor.custom_configurations);
}
/** \} */
