/*
 * date_time.c
 *
 *  Created on: 29 aug. 2025
 *      Author: Andlun
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "configuration.h"
#include "connector.h"
#include "date_time.h"
#include "rvc_to_ddm.h"

#include "broker.h"
#include "ddm2.h"

#include "HALCAN.h"
#include "MsgCAN.h"
#include "NMEA2K.h"
#include "RVCDGN.h"

#if defined(RVC_CONFIG_BUS_TIME_USE) || defined(RVC_CONFIG_BUS_TIME_KEEP)
static EXT_RAM_ATTR RVCDGN_zDGN_131070 l_131070_dgn;
static EXT_RAM_ATTR RVCDGN_zDGN_131071 l_131071_dgn;
static EXT_RAM_ATTR uint8_t l_connector_id;

void date_time_init(uint8_t connector_id)
{
    l_connector_id = connector_id;
    uint32_t class = RVCTIME0;
    broker_register_instance(&class, connector_id);

    l_131071_dgn.u8Day = l_131070_dgn.u8Day = NMEA2K_UINT8_NO_DATA;
    l_131071_dgn.u8DayOfWeek = l_131070_dgn.u8DayOfWeek = NMEA2K_UINT8_NO_DATA;
    l_131071_dgn.u8Hour = l_131070_dgn.u8Hour = NMEA2K_UINT8_NO_DATA;
    l_131071_dgn.u8Minute = l_131070_dgn.u8Minute = NMEA2K_UINT8_NO_DATA;
    l_131071_dgn.u8Month = l_131070_dgn.u8Month = NMEA2K_UINT8_NO_DATA;
    l_131071_dgn.u8Second = l_131070_dgn.u8Second = NMEA2K_UINT8_NO_DATA;
    l_131071_dgn.u8TimeZone = l_131070_dgn.u8TimeZone = NMEA2K_UINT8_NO_DATA;
    l_131071_dgn.u8Year = l_131070_dgn.u8Year = NMEA2K_UINT8_NO_DATA;
}

/**
 * @brief RVCTIME0 class handler function
 *
 * @param dgn Transmit dgn
 * @param p_frame Received DDM2P frame
 */
void handleRVCTIME0(uint32_t dgn, DDMP2_FRAME *p_frame)
{
    uint8_t instance = DDM2_PARAMETER_INSTANCE_FIELD(p_frame->frame.set.parameter);
    int32_t value;
#ifdef RVC_CONFIG_BUS_TIME_USE
    RVCDGN_zDGN_131071 *receive_frame = &l_131071_dgn;
    RVCDGN_zDGN_131070 *set_frame = &l_131070_dgn;
#endif
#ifdef RVC_CONFIG_BUS_TIME_KEEP
    RVCDGN_zDGN_131070 *receive_frame = &l_131070_dgn;
    RVCDGN_zDGN_131071 *set_frame = &l_131071_dgn;
#endif
    if (p_frame->frame.control == DDMP2_CONTROL_SUBSCRIBE)
    {
        // Return last received TIME status
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.subscribe.parameter))
        {
        case RVCTIME0YEAR:
            value = receive_frame->u8Year;
            if (value != NMEA2K_UINT8_NO_DATA)
            {
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCTIME0MONTH:
            value = receive_frame->u8Month;
            if (value != NMEA2K_UINT8_NO_DATA)
            {
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCTIME0DAY:
            value = receive_frame->u8Day;
            if (value != NMEA2K_UINT8_NO_DATA)
            {
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCTIME0DAYOFWEEK:
            value = receive_frame->u8DayOfWeek;
            if (value != NMEA2K_UINT8_NO_DATA)
            {
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCTIME0HOUR:
            value = receive_frame->u8Hour;
            if (value != NMEA2K_UINT8_NO_DATA)
            {
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCTIME0MIN:
            value = receive_frame->u8Minute;
            if (value != NMEA2K_UINT8_NO_DATA)
            {
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCTIME0SEC:
            value = receive_frame->u8Second;
            if (value != NMEA2K_UINT8_NO_DATA)
            {
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        case RVCTIME0TZ:
            value = receive_frame->u8TimeZone;
            if (value != NMEA2K_UINT8_NO_DATA)
            {
                convert_rvc_to_ddm_system_value(p_frame->frame.subscribe.parameter, &value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, p_frame->frame.subscribe.parameter, &value, sizeof(value), l_connector_id, (TickType_t)portMAX_DELAY);
            }
            break;
        default:
            break;
        }
    }
    else
    {
        value = p_frame->frame.set.value.int32;
        if (convert_ddm_system_value_to_rvc_value(p_frame->frame.set.parameter, &value, true))
        {
            switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.set.parameter))
            {
            case RVCTIME0YEAR:
                set_frame->u8Year = value;
                break;
            case RVCTIME0MONTH:
                set_frame->u8Month = value;
                break;
            case RVCTIME0DAY:
                set_frame->u8Day = value;
                break;
            case RVCTIME0DAYOFWEEK:
                set_frame->u8DayOfWeek = value;
                break;
            case RVCTIME0HOUR:
                set_frame->u8Hour = value;
                break;
            case RVCTIME0MIN:
                set_frame->u8Minute = value;
                break;
            case RVCTIME0SEC:
                set_frame->u8Second = value;
                break;
            case RVCTIME0TZ:
                set_frame->u8TimeZone = value;
                break;

            case RVCTIME0SYNC:
                // Set the command frame
                NMEA2K_SetTxRequest(BSP_CAN_RVC, dgn, instance);
                break;
            default:
                break;
            }
        }
    }
}

#ifdef RVC_CONFIG_BUS_TIME_USE
/**
 * @brief Process received RVC DGN 131071 (Date time status)
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive131071Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    (void)size;
    RVCDGN_zDGN_131071 zDGN;
    int32_t value;
    bool updated_data = false;
    RVCDGN_DGN_131071_Extract(&zDGN, p_data);

    // Compare with last data
    if ((zDGN.u8Year != NMEA2K_UINT8_NO_DATA) && (zDGN.u8Year != l_131071_dgn.u8Year))
    {
        updated_data = true;
        value = l_131071_dgn.u8Year = zDGN.u8Year;
        convert_rvc_to_ddm_system_value(RVCTIME0YEAR, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTIME0YEAR, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u8Month != NMEA2K_UINT8_NO_DATA) && (zDGN.u8Month != l_131071_dgn.u8Month))
    {
        updated_data = true;
        value = l_131071_dgn.u8Month = zDGN.u8Month;
        convert_rvc_to_ddm_system_value(RVCTIME0MONTH, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTIME0MONTH, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u8Day != NMEA2K_UINT8_NO_DATA) && (zDGN.u8Day != l_131071_dgn.u8Day))
    {
        updated_data = true;
        value = l_131071_dgn.u8Day = zDGN.u8Day;
        convert_rvc_to_ddm_system_value(RVCTIME0DAY, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTIME0DAY, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u8DayOfWeek != NMEA2K_UINT8_NO_DATA) && (zDGN.u8DayOfWeek != l_131071_dgn.u8DayOfWeek))
    {
        l_131071_dgn.u8DayOfWeek = zDGN.u8DayOfWeek;
    }
    if ((zDGN.u8TimeZone != NMEA2K_UINT8_NO_DATA) && (zDGN.u8TimeZone != l_131071_dgn.u8TimeZone))
    {
        updated_data = true;
        value = l_131071_dgn.u8TimeZone = zDGN.u8TimeZone;
        convert_rvc_to_ddm_system_value(RVCTIME0TZ, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTIME0TZ, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u8Hour != NMEA2K_UINT8_NO_DATA) && (zDGN.u8Hour != l_131071_dgn.u8Hour))
    {
        updated_data = true;
        value = l_131071_dgn.u8Hour = zDGN.u8Hour;
        convert_rvc_to_ddm_system_value(RVCTIME0HOUR, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTIME0HOUR, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u8Minute != NMEA2K_UINT8_NO_DATA) && (zDGN.u8Minute != l_131071_dgn.u8Minute))
    {
        updated_data = true;
        value = l_131071_dgn.u8Minute = zDGN.u8Minute;
        convert_rvc_to_ddm_system_value(RVCTIME0MIN, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTIME0MIN, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u8Second != NMEA2K_UINT8_NO_DATA) && (zDGN.u8Second != l_131071_dgn.u8Second))
    {
        updated_data = true;
        value = l_131071_dgn.u8Second = zDGN.u8Second;
        convert_rvc_to_ddm_system_value(RVCTIME0SEC, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTIME0SEC, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (true == updated_data)
    {
        value = 1;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTIME0SYNC, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    return true;
}

/**
 * @brief Prepare a Date Time Command frame
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit131070Dgn(uint8_t instance, uint8_t *p_data)
{
    (void)instance;
    // Stuff message data
    RVCDGN_DGN_131070_Stuff(p_data, &l_131070_dgn);
    LOG(D, " Date Time Command");
    return true;
}
#endif

#ifdef RVC_CONFIG_BUS_TIME_KEEP
/**
 * @brief Date Time status received
 *
 * @param p_data DGN data pointer
 * @return true if message has been handled
 */
bool receive131070Dgn(uint8_t *p_data, uint8_t sa, size_t size)
{
    (void)size;
    RVCDGN_zDGN_131070 zDGN;
    int32_t value;
    bool updated_data = false;
    RVCDGN_DGN_131070_Extract(&zDGN, p_data);

    // Compare with last data
    // Compare with last data
    if ((zDGN.u8Year != NMEA2K_UINT8_NO_DATA) && (zDGN.u8Year != l_131070_dgn.u8Year))
    {
        updated_data = true;
        value = l_131070_dgn.u8Year = zDGN.u8Year;
        convert_rvc_to_ddm_system_value(RVCTIME0YEAR, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTIME0YEAR, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u8Month != NMEA2K_UINT8_NO_DATA) && (zDGN.u8Month != l_131070_dgn.u8Month))
    {
        updated_data = true;
        value = l_131070_dgn.u8Month = zDGN.u8Month;
        convert_rvc_to_ddm_system_value(RVCTIME0MONTH, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTIME0MONTH, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u8Day != NMEA2K_UINT8_NO_DATA) && (zDGN.u8Day != l_131070_dgn.u8Day))
    {
        updated_data = true;
        value = l_131070_dgn.u8Day = zDGN.u8Day;
        convert_rvc_to_ddm_system_value(RVCTIME0DAY, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTIME0DAY, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u8DayOfWeek != NMEA2K_UINT8_NO_DATA) && (zDGN.u8DayOfWeek != l_131070_dgn.u8DayOfWeek))
    {
        l_131070_dgn.u8DayOfWeek = zDGN.u8DayOfWeek;
    }
    if ((zDGN.u8TimeZone != NMEA2K_UINT8_NO_DATA) && (zDGN.u8TimeZone != l_131070_dgn.u8TimeZone))
    {
        updated_data = true;
        value = l_131070_dgn.u8TimeZone = zDGN.u8TimeZone;
        convert_rvc_to_ddm_system_value(RVCTIME0TZ, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTIME0TZ, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u8Hour != NMEA2K_UINT8_NO_DATA) && (zDGN.u8Hour != l_131070_dgn.u8Hour))
    {
        updated_data = true;
        value = l_131070_dgn.u8Hour = zDGN.u8Hour;
        convert_rvc_to_ddm_system_value(RVCTIME0HOUR, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTIME0HOUR, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u8Minute != NMEA2K_UINT8_NO_DATA) && (zDGN.u8Minute != l_131070_dgn.u8Minute))
    {
        updated_data = true;
        value = l_131070_dgn.u8Minute = zDGN.u8Minute;
        convert_rvc_to_ddm_system_value(RVCTIME0MIN, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTIME0MIN, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if ((zDGN.u8Second != NMEA2K_UINT8_NO_DATA) && (zDGN.u8Second != l_131070_dgn.u8Second))
    {
        updated_data = true;
        value = l_131070_dgn.u8Second = zDGN.u8Second;
        convert_rvc_to_ddm_system_value(RVCTIME0SEC, &value);
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTIME0SEC, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    if (true == updated_data)
    {
        value = 1;
        connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, RVCTIME0SYNC, &value, 4, l_connector_id, (TickType_t)portMAX_DELAY);
    }
    return true;
}

/**
 * @brief Prepare a Date Time Command frame
 *
 * @param instance Class instance
 * @param p_data Pointer to transmit buffer to fill
 * @return true if message is to be transmitted
 */
bool transmit131071Dgn(uint8_t instance, uint8_t *p_data)
{
    (void)instance;
    // Stuff message data
    RVCDGN_DGN_131071_Stuff(p_data, &l_131071_dgn);
    return true;
}
#endif

#endif
