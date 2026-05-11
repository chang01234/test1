/**
 * \file        load_configurations_configuration.h
 * \date        2022-09-27
 * \author      (BB) Borjan Bozhinovski <borjan.bozhinovski@seavus.com>
 * \brief       Configuration header file for loading configurations.
 *
 * This header contains common defines and macros for loading configurations.
 *
 * \li          2022-09-27 (BB) Initial implementation
 *
 * \copyright   Dometic Group
 *              This source file and the information contained in it are
 *              confidential and proprietary to Dometic Group
 *              The reproduction or disclosure, in whole or in part,
 *              to anyone outside of Dometic Group without the written
 *              approval of a Dometic Group officer under a Non-Disclosure
 *              Agreement is expressly prohibited.
 *
 *              All rights reserved
 */

#ifndef LOAD_CONFIGURATIONS_CONFIGURATION_H_
#define LOAD_CONFIGURATIONS_CONFIGURATION_H_

#include "configuration.h"

#if defined(LOAD_CONFIGURATIONS)

// List all defined Rule Engine Configurations '||'
#if defined(LOAD_CONFIGURATIONS__FC_RULE_ENGINE_CONFIGURATION)
#include "load_configurations_fc_rule_engine.h"
#if !defined(CONNECTOR_FULL_CLIMATE_RULE_ENGINE)
#error "Rule Engine connector not enabled!"
#endif //defined(CONNECTOR_FULL_CLIMATE_RULE_ENGINE)
#endif //defined(LOAD_CONFIGURATIONS__FC_RULE_ENGINE_CONFIGURATION)

#if defined(LOAD_CONFIGURATIONS__SUPERVISOR_RULE_ENGINE_CONFIGURATION)
#include "load_configurations_supervisor.h"
#if !defined(CONNECTOR_SUPERVISOR)
#error "Rule Engine connector not enabled!"
#endif //defined(CONNECTOR_SUPERVISOR)
#endif //defined(LOAD_CONFIGURATIONS__SUPERVISOR_RULE_ENGINE_CONFIGURATION)

#if defined(LOAD_CONFIGURATIONS__SMART_ECO_RULE_ENGINE_CONFIGURATION)
#include "load_configurations_smart_eco_rule_engine.h"
#if !defined(CONNECTOR_SMART_ECO_RULE_ENGINE)
#error "Rule Engine connector not enabled!"
#endif //defined(CONNECTOR_SMART_ECO_RULE_ENGINE)
#endif //defined(LOAD_CONFIGURATIONS__SMART_ECO_RULE_ENGINE_CONFIGURATION)

#if defined(LOAD_CONFIGURATIONS__DDM_LOG_CONFIGURATION)
#include "load_configurations_ddm_log.h"
#if !defined(CONNECTOR_DDM_LOG)
#error "DDM Log connector not enabled!"
#endif //defined(CONNECTOR_DDM_LOG)
#endif //defined(LOAD_CONFIGURATIONS__DDM_LOG_CONFIGURATION)

#if defined(LOAD_CONFIGURATIONS__EVENT_NOTIFICATION_CONFIGURATION)
#include "load_configurations_event_notification.h"
#if !defined(CONNECTOR_EVENT_NOTIFICATION)
#error "Event notification connector not enabled!"
#endif //defined(CONNECTOR_EVENT_NOTIFICATION)
#endif //defined(LOAD_CONFIGURATIONS__EVENT_NOTIFICATION_CONFIGURATION)

// List all defined descriptors '&&'
#if !defined(LOAD_CONFIGURATIONS__DESCRIPTOR_FC_RULE_ENGINE) && !defined(LOAD_CONFIGURATIONS__DESCRIPTOR_SUPERVISOR_RULE_ENGINE) \
    && !defined(LOAD_CONFIGURATIONS__DESCRIPTOR_SMART_ECO_RULE_ENGINE) && !defined(LOAD_CONFIGURATIONS__DESCRIPTOR_DDM_LOG)
#warning "Configurations not specified. Launching with no default rules. Please define the desired configurations in the configuration.h file."
#endif//!defined(LOAD_CONFIGURATIONS__DESCRIPTOR_FC_RULE_ENGINE) && !defined(LOAD_CONFIGURATIONS__DESCRIPTOR_SUPERVISOR)
        // && !defined(LOAD_CONFIGURATIONS__DESCRIPTOR_SMART_ECO_RULE_ENGINE) && !defined(LOAD_CONFIGURATIONS__DESCRIPTOR_DDM_LOG)


#endif //defined(LOAD_CONFIGURATIONS)
#endif //LOAD_CONFIGURATIONS_CONFIGURATION_H_
