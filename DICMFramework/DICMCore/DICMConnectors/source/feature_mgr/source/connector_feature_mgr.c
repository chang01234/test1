/*! \file
 *  \brief Feature mgr service
 *
 *  Service is serve query request from any clients requesting in a feature exists or not
 */

#include <stdint.h>
#include <string.h>

#include "broker.h"
#include "configuration.h"
#include "connector.h"
#include "connector_feature_mgr.h"
#include "ddm2.h"
#include "ddm2_parameter_list.h"
#include "feature_database.h"
#include "hal_mem.h"
#include "inventory_handler.h"

static int initialize_connector(void);
static void inventory_handler_cb(void *argument, uint32_t device_class_instance, bool is_available);
static void connector_process_task(void *param);
static void handle_set(DDMP2_FRAME *p_frame);

static inventory_handler_t inventory_handler;   // inventory handler handling inventory updates
DECLARE_SORTED_LIST_EXTRAM(inventory_list, 1);  // sorted list for inventory handler

CONNECTOR connector_feature_mgr = {
    .name = "Feature mgr",               // connector name
    .initialize = initialize_connector,  // connector initialize function
};

static int initialize_connector(void)
{
    int32_t err = -1;
    int32_t ret = 1;

    err = feat_db_init();
    if (err != ESP_OK)
    {
        ret = 0;
    }

    // Register groupmgr class
    uint32_t device_class = GROUPMGR0;
    int instance = broker_register_instance(&device_class, connector_feature_mgr.connector_id);
    if (instance < 0)
    {
        LOG(E, "Failed to register GROUPMGR0");
        ret = 0;
    }

    inventory_handler_init(&inventory_handler, &inventory_list, inventory_handler_cb, NULL);  // initialize inventory handler
    inventory_handler_add_any(&inventory_handler, PROD0);
    inventory_handler_start(&inventory_handler, &connector_feature_mgr);
    TRUE_CHECK(xTaskCreate(connector_process_task, connector_feature_mgr.name, 4096, NULL, xTASK_PRIORITY_NORMAL, NULL));
    return ret;
}

static void inventory_handler_cb(void *argument, uint32_t device_class_instance, bool is_available)
{
}

static void connector_process_task(void *param)
{
    DDMP2_FRAME *p_frame;
    size_t frame_size;

    while (1)
    {
        p_frame = xRingbufferReceive(connector_feature_mgr.to_connector, &frame_size, portMAX_DELAY);  // retrieve frame from broker
        TRUE_CHECK(p_frame != NULL);

        if ((p_frame == NULL) || (frame_size == 0))
        {
            continue;
        }

        switch (p_frame->frame.control)  // handle frame according to its control byte
        {
        case DDMP2_CONTROL_SET:
            handle_set(p_frame);
            break;
        case DDMP2_CONTROL_SUBSCRIBE:
            // Nothing to return
            break;
        default:
            break;
        }

        vRingbufferReturnItem(connector_feature_mgr.to_connector, p_frame);  // return frame to ring buffer
    }
}

static void handle_set(DDMP2_FRAME *p_frame)
{
    size_t size;
    int32_t data_arr[20];
    size = 20;
    switch (p_frame->frame.set.parameter)
    {
    case GROUPMGR0REQTYPE:
    {
        GROUP0TYPE_ENUM type = p_frame->frame.set.value.int32;
        switch (type)
        {
        case GROUP0TYPE_CLIMATEZONE:
            // fallthrough
        case GROUP0TYPE_SMARTECO:
            // fallthrough
        case GROUP0TYPE_MOBILECOOLER:
            // fallthrough
        case GROUP0TYPE_CLIMATECONTROL:
            // fallthrough
        case GROUP0TYPE_POWERSYSTEM:
        {
            feat_db_get_all_enabled_instances_of_type(data_arr, &size, type);
            if (size > 0)
            {
                GROUPCLASSES_T *groups = hal_mem_malloc_prefer(size * sizeof(struct GROUPS), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
                memset(groups, 0, size * sizeof(struct GROUPS));
                // We found some enabled classes
                for (size_t i = 0; i < size; ++i)
                {
                    size_t name_size = sizeof(struct GROUPS) - offsetof(struct GROUPS, name);
                    groups->groups[i].class_inst = GROUP0 | DDM2_PARAMETER_INSTANCE(data_arr[i]);
                    feat_db_read_cache(FEAT_DB_FIELD_NAME, data_arr[i], &groups->groups[i].name[0], &name_size);
                }
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.set.parameter, groups, size * sizeof(struct GROUPS), connector_feature_mgr.connector_id, portMAX_DELAY);
                hal_mem_free(groups);
            }
            else
            {
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.set.parameter, NULL, 0, connector_feature_mgr.connector_id, portMAX_DELAY);
            }
            break;
        }
        case GROUP0TYPE_GENERIC:
            break;
        }
        break;
    }
    default:
        break;
    }
}
