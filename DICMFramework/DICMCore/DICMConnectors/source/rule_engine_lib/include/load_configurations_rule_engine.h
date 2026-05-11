/*
 * load_configurations_rule_engine.h
 *
 *  Created on: 31 maj 2023
 *      Author: Andlun
 */

#ifndef LOAD_CONFIGURATIONS_RULE_ENGINE_H_
#define LOAD_CONFIGURATIONS_RULE_ENGINE_H_

#include "load_configurations.h"
#include "rule_engine.h"

/**
 * @brief DDM2 Rule Engine add specification
 *
 *  It will add the specification in preallocated sorted containter
 *  and subscribe the connector to the parameters listed in the
 *  \ref rule_engine__sensitivity_list
 *
 * @param p_rule_engine_inst Pointer to rule engine instance
 * @param config Pointer to configuration object
 * @return 1 - Configuration successfully loaded, 0 - Not loaded
 */
int load_rule_engine_configuration(rule_engine_inst_t *const p_rule_engine_inst, const struct load_configurations__configuration *config);

#endif /* LOAD_CONFIGURATIONS_RULE_ENGINE_H_ */
