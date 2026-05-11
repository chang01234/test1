/**
 * \file        load_configurations_supervisor.h
 * 
 */

#ifndef LOAD_CONFIGURATIONS__SUPERVISOR_H
#define LOAD_CONFIGURATIONS__SUPERVISOR_H
#include "load_configurations.h"

#define LOAD_CONFIGURATIONS__DESCRIPTOR_SUPERVISOR_RULE_ENGINE
#define LOAD_CONFIGURATIONS__DESCRIPTOR_SUPERVISOR_RULE_ENGINE_NAME supervisor__descriptor

extern const struct load_configurations__descriptor supervisor__descriptor;

int connector_supervisor_load_configurations(const struct load_configurations__configuration *config);
#endif //LOAD_CONFIGURATIONS__SUPERVISOR_H
