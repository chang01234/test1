/**
 * \file        connector_sysappl_service.c
 * \date        2024-03-14
 * \author      Kire Janev (kire.janev@dometic.com)
 * \brief       System application service that is the owner of the SYSAPPL class,
 * 
 * \li          2024-03-14 (KJ) Initial implementation
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

#include "connector_sysappl_service.h"

#include "connector.h"
#include "ddm_wrapper.h"
#include "ddm2_parameter_list.h"
#include "dicm_framework_config.h"
#include "iGeneralDefinitions.h"

//! \~ DDM Wrapper parameters
typedef struct _connector_sysappl_ddm2_parameters
{
    /* owned paramaters */
    ddmw_item_t sysappl0smarteco; 

    /* parameters that service is subscribed to*/
    ddmw_item_t prod0clist;
} connector_sysappl_ddm2_parameters_t;

//! \~ DDM Wrapper data
typedef struct
{
    ddmw_t ddm; // create ddmw object
    // create parameters structure
    connector_sysappl_ddm2_parameters_t ddm2_parameters;
} connector_sysappl_t;

static EXT_RAM_ATTR connector_sysappl_t connector_sysappl;

static void connector_sysappl_service_process_task(const DDMP2_FRAME * frame);

static void connector_sysappl_service_process_task(const DDMP2_FRAME * frame)
{
    // process the frame received from broker and update the corresponding parameter
    ddmw_process(&connector_sysappl.ddm, frame);

    ddmw_save_if_updated_i32(&connector_sysappl.ddm2_parameters.sysappl0smarteco, "SYSAPPL", 0, "SECO");
    // process all parameters and publish those that have been updated
    ddmw_process_publish(&connector_sysappl.ddm);
}

static int connector_sysappl_init(void)
{
    ddmw_init(&connector_sysappl.ddm, &connector_sysappl_service);

    int32_t instance = ddmw_register(&connector_sysappl.ddm, SYSAPPL0);
    assert(instance == 0);
    connector_sysappl_ddm2_parameters_t *connector_sysappl_instance = &connector_sysappl.ddm2_parameters;

    ddmw_add(&connector_sysappl.ddm, &connector_sysappl_instance->sysappl0smarteco, SYSAPPL0SMARTECO, instance);
    ddmw_load_i32(&connector_sysappl_instance->sysappl0smarteco, "SYSAPPL", instance, "SECO", 0);

    ddmw_add(&connector_sysappl.ddm, &connector_sysappl_instance->prod0clist, PROD0CLIST, 0);
    ddmw_set_type(&connector_sysappl_instance->prod0clist, DDMW_ACTION_SET);
    ddmw_send_set_i32(&connector_sysappl.ddm, PROD0CLIST, instance, SYSAPPL0);

    return 1;
}

CONNECTOR connector_sysappl_service =
{
    .name = "System application service connector",
    .initialize = connector_sysappl_init,
    .process_event = connector_sysappl_service_process_task,
};