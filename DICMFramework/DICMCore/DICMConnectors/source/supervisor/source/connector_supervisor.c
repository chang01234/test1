/*
 * supervisor.c
 *
 *  Created on: 31 maj 2023
 *      Author: Andlun
 */

#define DDMW_ITEM_DATA_CAPACITY 60
#include <string.h>

#include "configuration.h"

#include "connector_supervisor.h"
#ifdef CONNECTOR_SUPERVISOR_MAX_NUMBER_OF_RULES
#define RULE_ENGINE__MAX_NUMBER_OF_RULES (CONNECTOR_SUPERVISOR_MAX_NUMBER_OF_RULES)
#endif
#define RULE_ENGINE__DDM2_SET_PARAMS_DEPTH 10
#define RULE_ENGINE__DDM2_MAP_SIZE         2  // Number of "created" instances
#include "load_configurations_rule_engine.h"
#include "rule_engine.h"

#include "ddm_wrapper.h"
#include "utils.h"

/**********************************************************
 * Private types
 *********************************************************/
typedef struct
{
    // DDM
    ddmw_item_t rulename;  // Name
    ddmw_item_t num;       // Number

} supervisor_rule_engine_param_t;

typedef struct
{
    // DDM wrapper
    ddmw_t ddm;
    supervisor_rule_engine_param_t items;

    rule_engine_inst_t rule_engine_inst;
} connector_supervisor_rule_engine_t;

static EXT_RAM_ATTR connector_supervisor_rule_engine_t supervisor_rule_engine;

/**********************************************************
 * Private functions
 *********************************************************/
static int connector_supervisor_init(void);
static void connector_supervisor_task(const DDMP2_FRAME *frame);

/**********************************************************
 * Public variables
 *********************************************************/
CONNECTOR connector_supervisor =
    {
        .name = "Supervisor (Rule engine) connector",
        .initialize = connector_supervisor_init,
        .process_event = connector_supervisor_task,
};

/**********************************************************
 * Implementation
 *********************************************************/
static int connector_supervisor_init(void)
{
    rule_engine_inst_config_t rule_engine_inst_config =
        {
            .rule_engine_specification_elements = RULE_ENGINE__MAX_NUMBER_OF_RULES,
            .rule_engine_ddm2_subscription_table_elements = RULE_ENGINE__DDM2_SUBSCRIPTION_DEPTH,
            .rule_engine_set_timers_elements = RULE_ENGINE__MAX_NUMBER_OF_TIMERS,
            .rule_engine_ddm_map_elements = RULE_ENGINE__DDM2_MAP_SIZE,
        };

    memset(&supervisor_rule_engine, 0, sizeof(supervisor_rule_engine));

    ddmw_init_size(&supervisor_rule_engine.ddm, &connector_supervisor, DDMW_ITEM_DATA_CAPACITY);

    // Register Rule Engine instance
    int instance = ddmw_register(&supervisor_rule_engine.ddm, RULE0);
    ddmw_add(&supervisor_rule_engine.ddm, &supervisor_rule_engine.items.rulename, RULE0NAME, instance);
    ddmw_set_str(&supervisor_rule_engine.items.rulename, "Supervisor rules");
    ddmw_add(&supervisor_rule_engine.ddm, &supervisor_rule_engine.items.num, RULE0NUM, instance);
    ddmw_set_data(&supervisor_rule_engine.items.num, &Zero, sizeof(Zero));

    supervisor_rule_engine.rule_engine_inst.instance = instance;
    supervisor_rule_engine.rule_engine_inst.wrapper = &supervisor_rule_engine.ddm;
    rule_engine_initialize_instance(&supervisor_rule_engine.rule_engine_inst, &rule_engine_inst_config);
    rule_engine_activate(&supervisor_rule_engine.rule_engine_inst, 1);

    return 1;
}

static void connector_supervisor_task(const DDMP2_FRAME *frame)
{
    rule_engine_task_process(&supervisor_rule_engine.rule_engine_inst, frame);
}

int connector_supervisor_load_configurations(const struct load_configurations__configuration *config)
{
    // Adding rules
    int load_status = load_rule_engine_configuration(&supervisor_rule_engine.rule_engine_inst, config);
    if (load_status == 0)
    {
        ddmw_set_str(&supervisor_rule_engine.items.rulename, config->static_config->configuration_name);
    }
    return load_status;
}
