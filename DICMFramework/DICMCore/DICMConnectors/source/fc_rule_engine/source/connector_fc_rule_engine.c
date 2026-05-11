/*! \file connector_fc_rule_engine.c
    \author Borjan Bozhinovski
    \brief Full climate Rule engine implementation.

    This connector maintains a list of rules that will combine DDM2 parameters to
    generate events that will generate a publish of other DDM2 parameter
*/
#include <string.h>

#include "configuration.h"

#include "connector_fc_rule_engine.h"
#include "ddm2.h"
#include "ddm_entry.h"
#include "ddm_store.h"
#include "ddm_wrapper.h"
#include "freertos/task.h"
#include "load_configurations_rule_engine.h"
#include "utils.h"

/**********************************************************
 * Private types
 *********************************************************/
typedef struct
{
    // DDM
    ddmw_item_t name;  // Name
    ddmw_item_t num;   // Number
} rule_engine_param_t;

typedef struct
{
    // DDM wrapper
    ddmw_t ddm;
    rule_engine_param_t items;

    rule_engine_inst_t rule_engine_inst;
} connector_fc_rule_engine_t;

static EXT_RAM_ATTR connector_fc_rule_engine_t fc_rule_engine;

/**********************************************************
 * Private functions
 *********************************************************/
static int connector_fc_rule_engine_init(void);
static void connector_fc_rule_engine_task(const DDMP2_FRAME *frame);

/**********************************************************
 * Public variables
 *********************************************************/
CONNECTOR connector_fc_rule_engine =
    {
        .name = "Full climate (Rule engine) connector",
        .initialize = connector_fc_rule_engine_init,
        .process_event = connector_fc_rule_engine_task,
};

/**********************************************************
 * Implementation
 *********************************************************/
static int connector_fc_rule_engine_init(void)
{
    rule_engine_inst_config_t rule_engine_inst_config =
        {
            .rule_engine_specification_elements = RULE_ENGINE__MAX_NUMBER_OF_RULES,
            .rule_engine_ddm2_subscription_table_elements = RULE_ENGINE__DDM2_SUBSCRIPTION_DEPTH,
            .rule_engine_set_timers_elements = RULE_ENGINE__MAX_NUMBER_OF_TIMERS,
            .rule_engine_ddm_map_elements = RULE_ENGINE__DDM2_MAP_SIZE,
        };

    memset(&fc_rule_engine, 0, sizeof(fc_rule_engine));

    ddmw_init(&fc_rule_engine.ddm, &connector_fc_rule_engine);

    fc_rule_engine.rule_engine_inst.wrapper = &fc_rule_engine.ddm;
    rule_engine_initialize_instance(&fc_rule_engine.rule_engine_inst, &rule_engine_inst_config);
    rule_engine_activate(&fc_rule_engine.rule_engine_inst, 1);

    // Register Rule Engine
    int instance = ddmw_register(&fc_rule_engine.ddm, RULE0);
    ddmw_add(&fc_rule_engine.ddm, &fc_rule_engine.items.name, RULE0NAME, instance);
    ddmw_set_str(&fc_rule_engine.items.name, "Full climate rules");
    ddmw_add(&fc_rule_engine.ddm, &fc_rule_engine.items.num, RULE0NUM, instance);
    ddmw_set_data(&fc_rule_engine.items.num, &Zero, sizeof(Zero));

    return 1;
}

static void connector_fc_rule_engine_task(const DDMP2_FRAME *frame)
{
    rule_engine_task_process(&fc_rule_engine.rule_engine_inst, frame);
}

int connector_fc_rule_engine_load_configurations(const struct load_configurations__configuration *config)
{
    return load_rule_engine_configuration(&fc_rule_engine.rule_engine_inst, config);
}

/** \} */
