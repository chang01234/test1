/*
 * @file production.c
 *
 *  Created on: 15 feb. 2024
 *      Author: Andlun
 */

#include <stdint.h>
#include <string.h>

#include "configuration.h"

#include "broker.h"
#include "connector.h"
#include "ddm2.h"
#include "gateway.h"
#include "production.h"
#ifdef CONNECTOR_BLE
#include "ble_peripheral.h"
#endif
#include "ddm2_parameter_list.h"
#include "esp_err.h"

#ifndef DICM_PRODUCTION_NAME
#define DICM_PRODUCTION_NAME ""
#endif

#ifndef DICM_PRODUCTION_DESCRIPTION
#define DICM_PRODUCTION_DESCRIPTION "DICM product description"
#endif

#ifndef DICM_PRODUCTION_MODEL
#define DICM_PRODUCTION_MODEL "Generic DICM Model"
#endif

#ifndef DICM_PRODUCTION_MANUFACTURER
#define DICM_PRODUCTION_MANUFACTURER "Dometic"
#endif

#define MAX_PROD0_NAME_LENGTH  31
#define MAX_PROD0_DESC_LENGTH  127
#define MAX_PROD0_MDL_LENGTH   127
#define MAX_PROD0_MANUF_LENGTH 127
#define MAX_PROD0_CLIST_SIZE   10

EXT_RAM_ATTR uint8_t prod0name[MAX_PROD0_NAME_LENGTH + 1];           // string + NULL
static EXT_RAM_ATTR uint8_t prod0desc[MAX_PROD0_DESC_LENGTH + 1];    // string + NULL
static EXT_RAM_ATTR uint8_t prod0mdl[MAX_PROD0_MDL_LENGTH + 1];      // string + NULL
static EXT_RAM_ATTR uint8_t prod0manuf[MAX_PROD0_MANUF_LENGTH + 1];  // string + NULL
static EXT_RAM_ATTR uint32_t prod0clist[MAX_PROD0_CLIST_SIZE];       // List of class instances
static EXT_RAM_ATTR int32_t prod0reset;
static uint8_t num_of_classes;

void production_init(void)
{
    // Make sure instance 0 is registered
    int instance = broker_request_instance_n(PROD0);
    ASSERT(instance == 0);

    size_t length = MAX_PROD0_NAME_LENGTH;
    if (ESP_OK != config_get_str("prod0name", (char *)prod0name, &length))
    {
        memset(prod0name, 0, sizeof(prod0name));
        strcpy((char *)prod0name, DICM_PRODUCTION_NAME);
        config_set_str("prod0name", (char *)prod0name);
    }
    length = MAX_PROD0_DESC_LENGTH;
    if (ESP_OK != config_get_str("prod0desc", (char *)prod0desc, &length))
    {
        memset(prod0desc, 0, sizeof(prod0desc));
        strcpy((char *)prod0desc, DICM_PRODUCTION_DESCRIPTION);
        config_set_str("prod0desc", (char *)prod0desc);
    }
    length = MAX_PROD0_MDL_LENGTH;
    if (ESP_OK != config_get_str("prod0mdl", (char *)prod0mdl, &length))
    {
        memset(prod0mdl, 0, sizeof(prod0mdl));
        strcpy((char *)prod0mdl, DICM_PRODUCTION_MODEL);
        config_set_str("prod0mdl", (char *)prod0mdl);
    }
    length = MAX_PROD0_MANUF_LENGTH;
    if (ESP_OK != config_get_str("prod0manuf", (char *)prod0manuf, &length))
    {
        memset(prod0manuf, 0, sizeof(prod0manuf));
        strcpy((char *)prod0manuf, DICM_PRODUCTION_MANUFACTURER);
        config_set_str("prod0manuf", (char *)prod0manuf);
    }
    num_of_classes = 0;
    memset((void *)prod0clist, 0, sizeof(prod0clist));
    prod0clist[num_of_classes++] = GW0;
    prod0reset = PROD0RESET_IDLE;
}

void production_handle_subscribe(const DDMP2_FRAME *const pframe)
{
    switch (pframe->frame.subscribe.parameter)
    {
    case PROD0NAME:
        gateway_reply(pframe, prod0name, strlen((char *)prod0name));
        break;
    case PROD0DESCRIPTION:
        gateway_reply(pframe, prod0desc, strlen((char *)prod0desc));
        break;
    case PROD0MDL:
        gateway_reply(pframe, prod0mdl, strlen((char *)prod0mdl));
        break;
    case PROD0SKU:
        gateway_reply(pframe, gw0sku, strlen((char *)gw0sku));
        break;
    case PROD0SN:
        gateway_reply(pframe, gw0dsn, strlen((char *)gw0dsn));
        break;
    case PROD0PNC:
        gateway_reply(pframe, gw0pnc, strlen((char *)gw0pnc));
        break;
    case PROD0FWVER:
        gateway_reply(pframe, device_information.firmware_version, strlen(device_information.firmware_version));
        break;
    case PROD0HWVER:
        gateway_reply(pframe, HARDWARE_STRING, strlen(HARDWARE_STRING));
        break;
    case PROD0CLIST:
        gateway_reply(pframe, prod0clist, num_of_classes * sizeof(prod0clist[0]));
        break;
    case PROD0MANUF:
        gateway_reply(pframe, prod0manuf, strlen((char *)prod0manuf));
        break;
    case PROD0RESET:
        gateway_publish_int32(pframe->frame.set.parameter, prod0reset);
        break;
    case PROD0FWID:
        gateway_reply(pframe, FIRMWARE_BUILD_ID, strlen(FIRMWARE_BUILD_ID));
    default:
        break;
    }
}

void production_handle_set(const DDMP2_FRAME *const pframe)
{
    switch (pframe->frame.set.parameter)
    {
    case PROD0NAME:
        gateway_accept_and_publish_string(pframe, prod0name, "prod0name", sizeof(prod0name));
        if (strlen((char *)prod0name) != 0)
        {
#ifdef CONNECTOR_BLE
            ble_peripheral_dicm_update_advertising_name((const char *)prod0name);
#endif
        }
        break;
    case PROD0DESCRIPTION:
        gateway_accept_and_publish_string(pframe, prod0desc, "prod0desc", sizeof(prod0desc));
        break;
    case PROD0MDL:
        gateway_accept_and_publish_string(pframe, prod0mdl, "prod0mdl", sizeof(prod0mdl));
        break;
    case PROD0SKU:
        gateway_accept_and_publish_string(pframe, gw0sku, "gw0sku", sizeof(gw0sku));
        break;
    case PROD0SN:
        gateway_accept_and_publish_string(pframe, gw0dsn, "gw0dsn", sizeof(gw0dsn));
        break;
    case PROD0PNC:
        gateway_accept_and_publish_string(pframe, gw0pnc, "gw0pnc", sizeof(gw0pnc));
        break;
    case PROD0MANUF:
        gateway_accept_and_publish_string(pframe, prod0manuf, "prod0manuf", sizeof(prod0manuf));
        break;
    case PROD0CLIST:
    {
        UPDLINKEDCLASS_T *p_update_clist = (UPDLINKEDCLASS_T *)pframe->frame.set.value.raw;
        bool add_class = true;  // Default is true
        // Check length of value to see if extra param is used to add or remove the param. If this is missing, adding is assumed.
        if (ddmp2_value_size(pframe) > sizeof(UPDLINKEDCLASS_T))
        {
            add_class = (bool)p_update_clist->update[0];
        }
        if (add_class)
        {
            if (num_of_classes >= MAX_PROD0_CLIST_SIZE)
            {
                // Cannot add more, since static allocation
                LOG(E, "Cannot add more classes to prod0clist");
            }
            else
            {
                // Avoid duplicates
                bool found_duplicate = false;
                for (uint8_t i = 0; i < num_of_classes; ++i)
                {
                    if (prod0clist[i] == p_update_clist->updclass)
                    {
                        found_duplicate = true;
                        break;
                    }
                }
                if (!found_duplicate)
                {
                    prod0clist[num_of_classes] = p_update_clist->updclass;
                    num_of_classes++;
                }
            }
        }
        else
        {
            // Find which one to remove
            for (uint8_t i = 0; i < num_of_classes; ++i)
            {
                if (prod0clist[i] == p_update_clist->updclass)
                {
                    // Remove this entry
                    memmove(&prod0clist[i], &prod0clist[i + 1], sizeof(prod0clist[0] * (num_of_classes - i - 1)));
                    --num_of_classes;
                    break;
                }
            }
        }
        gateway_publish(PROD0CLIST, prod0clist, num_of_classes * sizeof(prod0clist[0]));
        break;
    }
    case PROD0RESET:
        switch (pframe->frame.set.value.int32)
        {
        case PROD0RESET_RESTART:
        {
            // Request a DICM restart
            int32_t reset = GW0CUPD_RESTART;
            prod0reset = PROD0RESET_RESTART;
            connector_send_frame_to_broker(DDMP2_CONTROL_SET, GW0CUPD, &reset, sizeof(reset), pframe->source_connector, portMAX_DELAY);
            break;
        }
        case PROD0RESET_RESET_TO_DEFAULT_SETTINGS:
        // fallthrough
        case PROD0RESET_RESET_TO_FACTORY_SETTINGS:
        {
            // Request a DICM factory reset
            int32_t reset = GW0CUPD_FACTORY_RESET;
            prod0reset = pframe->frame.set.value.int32;
            connector_send_frame_to_broker(DDMP2_CONTROL_SET, GW0CUPD, &reset, sizeof(reset), pframe->source_connector, portMAX_DELAY);
            break;
        }
        case PROD0RESET_CLEAR_FAULTS:
            // Not supported
            prod0reset = PROD0RESET_NOT_SUPPORTED;
            gateway_publish_int32(PROD0RESET, prod0reset);
            prod0reset = PROD0RESET_IDLE;
            break;
        default:
            // Do nothing
            break;
        }
        gateway_publish_int32(PROD0RESET, prod0reset);
        break;
    case PROD0IND:
        // Add common GW/DICM indicate function here
        break;

    default:
        break;
    }
}
