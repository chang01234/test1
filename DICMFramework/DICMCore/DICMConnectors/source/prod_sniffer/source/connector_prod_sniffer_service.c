/*! \file
 *  \brief PROD sniffer service
 *
 *  The service is responsible for tracking/sniffing PROD class instances that are not registered in
 *  the DICM system through the product database by using the product database API, but by calling
 *  the broker API for registration instead.
 *  When the availability/unavailability of a new PROD class instance is detected in the inventory
 *  handler, it is registered in the product database without any additional information by calling
 *  specific API. The service then subscribes to the PROD class parameters of interest and when the
 *  parameters are published by the external owner, the service updates the product database with the
 *  published values.
 *
 */
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <sys/queue.h>

#include "broker.h"
#include "configuration.h"
#include "connector.h"
#include "connector_prod_sniffer_service.h"
#include "ddm2.h"
#include "ddm2_parameter_list.h"
#include "freertos/FreeRTOS.h"
#include "hal_mem.h"
#include "iGeneralDefinitions.h"
#include "inventory_handler.h"
#include "product_database.h"

#define INVENTORY_HANDLER_STORAGE_SIZE 1

static int initialize_connector(void);
static void inventory_handler_cb(void *argument, uint32_t device_class_instance, bool is_available);
static void connector_process_task(void *Parameter);

DECLARE_SORTED_LIST_EXTRAM(l_inventory_list, INVENTORY_HANDLER_STORAGE_SIZE);

static EXT_RAM_ATTR inventory_handler_t l_ih;

CONNECTOR connector_prod_sniffer_service = {
    .name = "Prod sniffer service",      // connector name
    .initialize = initialize_connector,  // connector initialize function
};

static int initialize_connector(void)  // initialize connector
{
    // Init inventory handler
    inventory_handler_init(&l_ih, &l_inventory_list, inventory_handler_cb, NULL);
    int32_t err = -1;
    int32_t ret = 1;
    err = ProdDBInit();
    if (err == ESP_OK)
    {
        TRUE_CHECK(xTaskCreate(connector_process_task, connector_prod_sniffer_service.name, 3584, 0, xTASK_PRIORITY_NORMAL, NULL));
    }
    else
    {
        LOG(E, "Product database init returned error %d", err);
        ret = 0;
    }
    return ret;
}

static void inventory_handler_cb(void *argument, uint32_t device_class_instance, bool is_available)
{
    // Detect if any new PRODs are created
    // If so, subscribe to specific parameters
    if (is_available)
    {
        if (DDMP2_INVENTORY_CLASS(device_class_instance) == PROD0)
        {
            if (DDM2_PARAMETER_INSTANCE_FIELD(device_class_instance) == 0)
            {
                // Ignore PROD0
            }
            else
            {
                // Check whether the product is already registered in the product database and subscribe to the parameters
                if (!ProdDBProdClassNodeExists(DDM2_PARAMETER_INSTANCE_FIELD(device_class_instance)))
                {
                    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, device_class_instance | DDM2_PARAMETER_PROPERTY_FIELD(PROD0UID), NULL, 0, connector_prod_sniffer_service.connector_id, portMAX_DELAY);
                    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, device_class_instance | DDM2_PARAMETER_PROPERTY_FIELD(PROD0PROP), NULL, 0, connector_prod_sniffer_service.connector_id, portMAX_DELAY);
                    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, device_class_instance | DDM2_PARAMETER_PROPERTY_FIELD(PROD0CLIST), NULL, 0, connector_prod_sniffer_service.connector_id, portMAX_DELAY);
                    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, device_class_instance | DDM2_PARAMETER_PROPERTY_FIELD(PROD0MDL), NULL, 0, connector_prod_sniffer_service.connector_id, portMAX_DELAY);
                    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, device_class_instance | DDM2_PARAMETER_PROPERTY_FIELD(PROD0NAME), NULL, 0, connector_prod_sniffer_service.connector_id, portMAX_DELAY);
                    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, device_class_instance | DDM2_PARAMETER_PROPERTY_FIELD(PROD0SN), NULL, 0, connector_prod_sniffer_service.connector_id, portMAX_DELAY);
                    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, device_class_instance | DDM2_PARAMETER_PROPERTY_FIELD(PROD0FWVER), NULL, 0, connector_prod_sniffer_service.connector_id, portMAX_DELAY);
                    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, device_class_instance | DDM2_PARAMETER_PROPERTY_FIELD(PROD0HWVER), NULL, 0, connector_prod_sniffer_service.connector_id, portMAX_DELAY);
                    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, device_class_instance | DDM2_PARAMETER_PROPERTY_FIELD(PROD0MANUF), NULL, 0, connector_prod_sniffer_service.connector_id, portMAX_DELAY);
                    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, device_class_instance | DDM2_PARAMETER_PROPERTY_FIELD(PROD0EAN), NULL, 0, connector_prod_sniffer_service.connector_id, portMAX_DELAY);
                    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, device_class_instance | DDM2_PARAMETER_PROPERTY_FIELD(PROD0PNC), NULL, 0, connector_prod_sniffer_service.connector_id, portMAX_DELAY);
                    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, device_class_instance | DDM2_PARAMETER_PROPERTY_FIELD(PROD0SKU), NULL, 0, connector_prod_sniffer_service.connector_id, portMAX_DELAY);
                    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, device_class_instance | DDM2_PARAMETER_PROPERTY_FIELD(PROD0DESCRIPTION), NULL, 0, connector_prod_sniffer_service.connector_id, portMAX_DELAY);
                }
            }
        }
    }
    else
    {
        if (DDMP2_INVENTORY_CLASS(device_class_instance) == PROD0)
        {
            if (DDM2_PARAMETER_INSTANCE_FIELD(device_class_instance) == 0)
            {
                // Ignore PROD0
            }
            else
            {
                // Remove the PROD class instance from the product database only if the connector ID is not set and DDM inst is set
                // If the connector ID is not set and DDM inst is set, it means that the PROD class instance is externally owned
                uint8_t conn_id = INVALID_CONNECTOR_ID;
                size_t conn_id_size = sizeof(conn_id);
                int32_t ddm_inst = INVALID_DDM_INSTANCE;
                size_t ddm_inst_size = sizeof(ddm_inst);
                ProdDBReadCache(FIELD_DDM_INST, DDM2_PARAMETER_INSTANCE_FIELD(device_class_instance), &ddm_inst, &ddm_inst_size);
                ProdDBReadCache(FIELD_CONN_ID, DDM2_PARAMETER_INSTANCE_FIELD(device_class_instance), &conn_id, &conn_id_size);
                if (((conn_id_size != 0) && (conn_id == INVALID_CONNECTOR_ID)) && ((ddm_inst_size != 0) && (ddm_inst != INVALID_DDM_INSTANCE)))
                {
                    ProdDBProdClassNodeDelete(DDM2_PARAMETER_INSTANCE_FIELD(device_class_instance));
                    LOG(D, "A product PROD%d has been removed", DDM2_PARAMETER_INSTANCE_FIELD(device_class_instance));
                    // Unsubscribe from the parameters
                    connector_send_frame_to_broker(DDMP2_CONTROL_UNSUBSCRIBE, device_class_instance | DDM2_PARAMETER_PROPERTY_FIELD(PROD0UID), NULL, 0, connector_prod_sniffer_service.connector_id, portMAX_DELAY);
                    connector_send_frame_to_broker(DDMP2_CONTROL_UNSUBSCRIBE, device_class_instance | DDM2_PARAMETER_PROPERTY_FIELD(PROD0PROP), NULL, 0, connector_prod_sniffer_service.connector_id, portMAX_DELAY);
                    connector_send_frame_to_broker(DDMP2_CONTROL_UNSUBSCRIBE, device_class_instance | DDM2_PARAMETER_PROPERTY_FIELD(PROD0CLIST), NULL, 0, connector_prod_sniffer_service.connector_id, portMAX_DELAY);
                    connector_send_frame_to_broker(DDMP2_CONTROL_UNSUBSCRIBE, device_class_instance | DDM2_PARAMETER_PROPERTY_FIELD(PROD0MDL), NULL, 0, connector_prod_sniffer_service.connector_id, portMAX_DELAY);
                    connector_send_frame_to_broker(DDMP2_CONTROL_UNSUBSCRIBE, device_class_instance | DDM2_PARAMETER_PROPERTY_FIELD(PROD0NAME), NULL, 0, connector_prod_sniffer_service.connector_id, portMAX_DELAY);
                    connector_send_frame_to_broker(DDMP2_CONTROL_UNSUBSCRIBE, device_class_instance | DDM2_PARAMETER_PROPERTY_FIELD(PROD0SN), NULL, 0, connector_prod_sniffer_service.connector_id, portMAX_DELAY);
                    connector_send_frame_to_broker(DDMP2_CONTROL_UNSUBSCRIBE, device_class_instance | DDM2_PARAMETER_PROPERTY_FIELD(PROD0FWVER), NULL, 0, connector_prod_sniffer_service.connector_id, portMAX_DELAY);
                    connector_send_frame_to_broker(DDMP2_CONTROL_UNSUBSCRIBE, device_class_instance | DDM2_PARAMETER_PROPERTY_FIELD(PROD0HWVER), NULL, 0, connector_prod_sniffer_service.connector_id, portMAX_DELAY);
                    connector_send_frame_to_broker(DDMP2_CONTROL_UNSUBSCRIBE, device_class_instance | DDM2_PARAMETER_PROPERTY_FIELD(PROD0MANUF), NULL, 0, connector_prod_sniffer_service.connector_id, portMAX_DELAY);
                    connector_send_frame_to_broker(DDMP2_CONTROL_UNSUBSCRIBE, device_class_instance | DDM2_PARAMETER_PROPERTY_FIELD(PROD0EAN), NULL, 0, connector_prod_sniffer_service.connector_id, portMAX_DELAY);
                    connector_send_frame_to_broker(DDMP2_CONTROL_UNSUBSCRIBE, device_class_instance | DDM2_PARAMETER_PROPERTY_FIELD(PROD0PNC), NULL, 0, connector_prod_sniffer_service.connector_id, portMAX_DELAY);
                    connector_send_frame_to_broker(DDMP2_CONTROL_UNSUBSCRIBE, device_class_instance | DDM2_PARAMETER_PROPERTY_FIELD(PROD0SKU), NULL, 0, connector_prod_sniffer_service.connector_id, portMAX_DELAY);
                    connector_send_frame_to_broker(DDMP2_CONTROL_UNSUBSCRIBE, device_class_instance | DDM2_PARAMETER_PROPERTY_FIELD(PROD0DESCRIPTION), NULL, 0, connector_prod_sniffer_service.connector_id, portMAX_DELAY);
                }
            }
        }
    }
}

static void connector_process_task(void *Parameter)
{
    size_t frame_size = 0;
    DDMP2_FRAME *pframe;

    int32_t ret = inventory_handler_add_any(&l_ih, PROD0);  // Add to inventory handler
    assert(ret == 0);
    ret = inventory_handler_start(&l_ih, &connector_prod_sniffer_service);
    assert(ret == 0);

    while (1)
    {
        frame_size = 0;
        pframe = (DDMP2_FRAME *)xRingbufferReceive(connector_prod_sniffer_service.to_connector, &frame_size, portMAX_DELAY);
        TRUE_CHECK(NULL != pframe);
        if ((frame_size != 0) && (NULL != pframe))
        {
            if (!inventory_handler_update(&l_ih, pframe))
            {
                switch (pframe->frame.control)
                {
                case DDMP2_CONTROL_PUBLISH:
                {
                    // Handler subscribed parameters
                    switch (DDM2_PARAMETER_BASE_INSTANCE(pframe->frame.publish.parameter))
                    {
                    case PROD0UID:
                    {
                        char prodXuid[DICM_UID_KEY_STR_LEN];
                        memset(prodXuid, 0, DICM_UID_KEY_STR_LEN);
                        ddmp2_extract_string_from_frame(pframe, prodXuid, DICM_UID_KEY_STR_LEN);
                        int32_t ret = ProdDBVirtualProdClassNodeCreate(prodXuid, DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.publish.parameter));
                        if ((ret < 0) && (ret != PROD_DB_ERR_PROD_CLASS_ALREADY_EXISTS))
                        {
                            LOG(E, "Failed to create PROD class node for UID %s with error %d", prodXuid, ret);
                            // Unsubscribe from the parameters
                            connector_send_frame_to_broker(DDMP2_CONTROL_UNSUBSCRIBE, DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.publish.parameter) | DDM2_PARAMETER_PROPERTY_FIELD(PROD0UID), NULL, 0, connector_prod_sniffer_service.connector_id, portMAX_DELAY);
                            connector_send_frame_to_broker(DDMP2_CONTROL_UNSUBSCRIBE, DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.publish.parameter) | DDM2_PARAMETER_PROPERTY_FIELD(PROD0PROP), NULL, 0, connector_prod_sniffer_service.connector_id, portMAX_DELAY);
                            connector_send_frame_to_broker(DDMP2_CONTROL_UNSUBSCRIBE, DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.publish.parameter) | DDM2_PARAMETER_PROPERTY_FIELD(PROD0CLIST), NULL, 0, connector_prod_sniffer_service.connector_id, portMAX_DELAY);
                            connector_send_frame_to_broker(DDMP2_CONTROL_UNSUBSCRIBE, DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.publish.parameter) | DDM2_PARAMETER_PROPERTY_FIELD(PROD0MDL), NULL, 0, connector_prod_sniffer_service.connector_id, portMAX_DELAY);
                            connector_send_frame_to_broker(DDMP2_CONTROL_UNSUBSCRIBE, DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.publish.parameter) | DDM2_PARAMETER_PROPERTY_FIELD(PROD0NAME), NULL, 0, connector_prod_sniffer_service.connector_id, portMAX_DELAY);
                            connector_send_frame_to_broker(DDMP2_CONTROL_UNSUBSCRIBE, DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.publish.parameter) | DDM2_PARAMETER_PROPERTY_FIELD(PROD0SN), NULL, 0, connector_prod_sniffer_service.connector_id, portMAX_DELAY);
                            connector_send_frame_to_broker(DDMP2_CONTROL_UNSUBSCRIBE, DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.publish.parameter) | DDM2_PARAMETER_PROPERTY_FIELD(PROD0FWVER), NULL, 0, connector_prod_sniffer_service.connector_id, portMAX_DELAY);
                            connector_send_frame_to_broker(DDMP2_CONTROL_UNSUBSCRIBE, DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.publish.parameter) | DDM2_PARAMETER_PROPERTY_FIELD(PROD0HWVER), NULL, 0, connector_prod_sniffer_service.connector_id, portMAX_DELAY);
                            connector_send_frame_to_broker(DDMP2_CONTROL_UNSUBSCRIBE, DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.publish.parameter) | DDM2_PARAMETER_PROPERTY_FIELD(PROD0MANUF), NULL, 0, connector_prod_sniffer_service.connector_id, portMAX_DELAY);
                            connector_send_frame_to_broker(DDMP2_CONTROL_UNSUBSCRIBE, DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.publish.parameter) | DDM2_PARAMETER_PROPERTY_FIELD(PROD0EAN), NULL, 0, connector_prod_sniffer_service.connector_id, portMAX_DELAY);
                            connector_send_frame_to_broker(DDMP2_CONTROL_UNSUBSCRIBE, DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.publish.parameter) | DDM2_PARAMETER_PROPERTY_FIELD(PROD0PNC), NULL, 0, connector_prod_sniffer_service.connector_id, portMAX_DELAY);
                            connector_send_frame_to_broker(DDMP2_CONTROL_UNSUBSCRIBE, DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.publish.parameter) | DDM2_PARAMETER_PROPERTY_FIELD(PROD0SKU), NULL, 0, connector_prod_sniffer_service.connector_id, portMAX_DELAY);
                            connector_send_frame_to_broker(DDMP2_CONTROL_UNSUBSCRIBE, DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.publish.parameter) | DDM2_PARAMETER_PROPERTY_FIELD(PROD0DESCRIPTION), NULL, 0, connector_prod_sniffer_service.connector_id, portMAX_DELAY);
                        }
                        else if (ret == ESP_OK)
                        {
                            LOG(D, "Prod class created for UID %s", prodXuid);
                        }
                        else
                        {
                            LOG(D, "Prod class already exists for UID %s", prodXuid);
                        }
                    }
                    break;
                    case PROD0PROP:
                    {
                        PROD0PROP_T *p_propprop = (PROD0PROP_T *)pframe->frame.publish.value.raw;
                        prodxprop_type_t type = {0};
                        type.data = p_propprop->type;
                        uint8_t sa = p_propprop->addr;
                        uint8_t inst = p_propprop->inst;
                        ProdDBVirtualUpdateCache((const void *)&inst, sizeof(uint8_t), FIELD_PROP_INST, DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.publish.parameter));
                        ProdDBVirtualUpdateCache((const void *)&type, sizeof(type), FIELD_PROP_TYPE, DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.publish.parameter));
                        ProdDBVirtualUpdateCache((const void *)&sa, sizeof(uint8_t), FIELD_PROP_SA, DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.publish.parameter));
                        // Calculate number of classes in the data
                        size_t num_classes = (ddmp2_value_size(pframe) - sizeof(PROD0PROP_T)) / sizeof(uint32_t);
                        for (size_t i = 0; i < num_classes; i++)
                        {
                            uint32_t class_instance = p_propprop->classes[i];
                            ProdDBVirtualUpdateCache((const void *)&class_instance, sizeof(uint32_t), FIELD_PROP_CLASS, DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.publish.parameter));
                        }
                    }
                    break;
                    case PROD0FWVER:
                    {
                        char version_string[PROD_DB_MAX_FIELD_SIZE];
                        ddmp2_extract_string_from_frame(pframe, version_string, PROD_DB_MAX_FIELD_SIZE);
                        ProdDBVirtualUpdateCache(version_string, strlen(version_string), FIELD_FWVER, DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.publish.parameter));
                    }
                    break;
                    case PROD0NAME:
                    {
                        char prodXname[PROD_DB_MAX_FIELD_SIZE];
                        ddmp2_extract_string_from_frame(pframe, prodXname, PROD_DB_MAX_FIELD_SIZE);
                        ProdDBVirtualUpdateCache(prodXname, strlen(prodXname), FIELD_NAME, DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.publish.parameter));
                    }
                    break;
                    case PROD0SN:
                    {
                        char prodXsn[PROD_DB_MAX_FIELD_SIZE];
                        ddmp2_extract_string_from_frame(pframe, prodXsn, PROD_DB_MAX_FIELD_SIZE);
                        ProdDBVirtualUpdateCache(prodXsn, strlen(prodXsn), FIELD_SN, DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.publish.parameter));
                    }
                    break;
                    case PROD0MDL:
                    {
                        char prodXmdl[PROD_DB_MAX_FIELD_SIZE];
                        ddmp2_extract_string_from_frame(pframe, prodXmdl, PROD_DB_MAX_FIELD_SIZE);
                        ProdDBVirtualUpdateCache(prodXmdl, strlen(prodXmdl), FIELD_MDL, DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.publish.parameter));
                    }
                    break;
                    case PROD0HWVER:
                    {
                        char prodXhwver[PROD_DB_MAX_FIELD_SIZE];
                        ddmp2_extract_string_from_frame(pframe, prodXhwver, PROD_DB_MAX_FIELD_SIZE);
                        ProdDBVirtualUpdateCache(prodXhwver, strlen(prodXhwver), FIELD_HWVER, DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.publish.parameter));
                    }
                    break;
                    case PROD0EAN:
                    {
                        char prodXean[PROD_DB_MAX_FIELD_SIZE];
                        ddmp2_extract_string_from_frame(pframe, prodXean, PROD_DB_MAX_FIELD_SIZE);
                        ProdDBVirtualUpdateCache(prodXean, strlen(prodXean), FIELD_EAN, DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.publish.parameter));
                    }
                    break;
                    case PROD0PNC:
                    {
                        char prodXpnc[PROD_DB_MAX_FIELD_SIZE];
                        ddmp2_extract_string_from_frame(pframe, prodXpnc, PROD_DB_MAX_FIELD_SIZE);
                        ProdDBVirtualUpdateCache(prodXpnc, strlen(prodXpnc), FIELD_PNC, DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.publish.parameter));
                    }
                    break;
                    case PROD0SKU:
                    {
                        char prodXsku[PROD_DB_MAX_FIELD_SIZE];
                        ddmp2_extract_string_from_frame(pframe, prodXsku, PROD_DB_MAX_FIELD_SIZE);
                        ProdDBVirtualUpdateCache(prodXsku, strlen(prodXsku), FIELD_SKU, DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.publish.parameter));
                    }
                    break;
                    case PROD0DESCRIPTION:
                    {
                        char prodXdescription[PROD_DB_MAX_FIELD_SIZE];
                        ddmp2_extract_string_from_frame(pframe, prodXdescription, PROD_DB_MAX_FIELD_SIZE);
                        ProdDBVirtualUpdateCache(prodXdescription, strlen(prodXdescription), FIELD_DESC, DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.publish.parameter));
                    }
                    break;
                    case PROD0MANUF:
                    {
                        char prodXmanuf[PROD_DB_MAX_FIELD_SIZE];
                        ddmp2_extract_string_from_frame(pframe, prodXmanuf, PROD_DB_MAX_FIELD_SIZE);
                        ProdDBVirtualUpdateCache(prodXmanuf, strlen(prodXmanuf), FIELD_MANUF, DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.publish.parameter));
                    }
                    break;
                    case PROD0CLIST:
                    {
                        ProdDBVirtualUpdateCache(pframe->frame.publish.value.raw, ddmp2_value_size(pframe), FIELD_CLIST, DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.publish.parameter));
                    }
                    break;
                    case PROD0FWID:
                    {
                        ProdDBVirtualUpdateCache(pframe->frame.publish.value.raw, ddmp2_value_size(pframe), FIELD_FWID, DDM2_PARAMETER_INSTANCE_FIELD(pframe->frame.publish.parameter));
                        break;
                    }
                    default:
                        break;
                    }
                }
                break;
                default:
                    LOG(W, "Unexpected frame control type %d", pframe->frame.control);
                    break;
                }
            }
            vRingbufferReturnItem(connector_prod_sniffer_service.to_connector, pframe);
        }
    }
}
