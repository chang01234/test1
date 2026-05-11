/**
 * \file        load_configurations_ddm_log.h
 *
 */

#ifndef LOAD_CONFIGURATIONS__DDM_LOG_H
#define LOAD_CONFIGURATIONS__DDM_LOG_H
#include "load_configurations.h"

#define LOAD_CONFIGURATIONS__DESCRIPTOR_DDM_LOG
#define LOAD_CONFIGURATIONS__DESCRIPTOR_DDM_LOG_NAME ddm_log__descriptor

extern const struct load_configurations__descriptor ddm_log__descriptor;

int connector_ddm_log_load_configurations(const struct load_configurations__configuration *config);
#endif //LOAD_CONFIGURATIONS__DDM_LOG_H
