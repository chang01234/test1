/**
 * \file        load_configurations.c
 * \date        2022-09-27
 * \author      (BB) Borjan Bozhinovski <borjan.bozhinovski@seavus.com>
 * \brief       Configurations loader.
 *
 * Loads custom configurations located in non-volatile memory, that are not consisted
 * of DDM2 parameters. Each descriptor has it's own implementation of the loading functionality
 * that should populate the intended connector with specific data.
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

#include "configuration.h"
#ifdef LOAD_CONFIGURATIONS

#include "load_configurations.h"

#include "ddm2.h"
#include "ddm2_parameter_list.h"

#include "load_configurations_configuration.h"

static const struct load_configurations load_configurations__descriptors[] = {
#if defined(LOAD_CONFIGURATIONS__DESCRIPTOR_SMART_ECO_RULE_ENGINE)
    {
        &LOAD_CONFIGURATIONS__DESCRIPTOR_SMART_ECO_RULE_ENGINE_NAME,
    },
#endif
#if defined(LOAD_CONFIGURATIONS__DESCRIPTOR_FC_RULE_ENGINE)
    {
        &LOAD_CONFIGURATIONS__DESCRIPTOR_FC_RULE_ENGINE_NAME,
    },
#endif
#if defined(LOAD_CONFIGURATIONS__DESCRIPTOR_SUPERVISOR_RULE_ENGINE)
    {
        &LOAD_CONFIGURATIONS__DESCRIPTOR_SUPERVISOR_RULE_ENGINE_NAME,
    },
#endif
#if defined(LOAD_CONFIGURATIONS__DESCRIPTOR_DDM_LOG)
    {
        &LOAD_CONFIGURATIONS__DESCRIPTOR_DDM_LOG_NAME,
    },
#endif
#if defined(LOAD_CONFIGURATIONS__DESCRIPTOR_EVENT_NOTIFICATION)
    {
        &LOAD_CONFIGURATIONS__DESCRIPTOR_EVENT_NOTIFICATION_NAME,
    }
#endif
};

void load_configurations(void)
{
    LOG(I, "Loading %d configuration descriptors", ELEMENTS(load_configurations__descriptors));
    for (unsigned int dscr = 0; dscr < ELEMENTS(load_configurations__descriptors); ++dscr)
    {
        const struct load_configurations__descriptor *descriptor = load_configurations__descriptors[dscr].descriptors;

        // Load custom configurations, that are not consisted of DDM2 parameters
        if (descriptor->custom_configurations_size > 0)
        {
            LOG(I, "Loading [%s]'s configurations", descriptor->descriptor_name);
            if (descriptor->load_custom_configurations != NULL)
            {
                for (int config = 0, configurations = descriptor->custom_configurations_size; configurations > 0; ++config, --configurations)
                {
                    const struct load_configurations__configuration *configuration = &descriptor->custom_configurations[config];
                    if (configuration)
                    {
                        LOG(I, "  Loading configuration[%s]", configuration->static_config->configuration_name);
                        int is_loaded = descriptor->load_custom_configurations(configuration);
                        if (is_loaded != 0)
                        {
                            LOG(E, "  Failed to load configuration!");
                        }
                        else
                        {
                            LOG(I, "  Loading configuration[%s] done", configuration->static_config->configuration_name);
                        }
                    }
                    else
                    {
                        LOG(E, "  Invalid configuration!");
                    }
                }
            }
            else
            {
                LOG(W, "  Loading function not specified!");
            }
        }
    }

    // TODO: Load configurations consisted of DDM2 parameters.
    // DDM2 parameters have to be set by some connector, and cannot be loaded
    // and set manually trough 'board_initialization()' function as it is done
    // for the custom configurations.
}
#endif
