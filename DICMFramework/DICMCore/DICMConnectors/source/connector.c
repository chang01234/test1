/*****************************************************************************
 * \file       connector.c
 * \brief      List of connectors used by the application
 * \copyright  Dometic Group
 *             This source file and the information contained in it are
 *             confidential and proprietary to Dometic Group
 *             The reproduction or disclosure, in whole or in part,
 *             to anyone outside of Dometic Group without the written
 *             approval of a Dometic Group officer under a Non-Disclosure
 *             Agreement is expressly prohibited.
 *
 *             All rights reserved
 *****************************************************************************/
#include "configuration.h"

#include "connector.h"

#include "freertos/ringbuf.h"
#include "hal_cpu.h"
#include "hal_mem.h"
#include <string.h>

#ifdef CONNECTOR_BLE
#include "connector_ble.h"
#endif

#ifdef CONNECTOR_WIFI
#include "connector_wifi.h"
#endif

#ifdef CONNECTOR_LIN_LOOPBACK
#include "connector_lin_loopback.h"
#endif

#ifdef CONNECTOR_PREM_RVC
#include "connector_prem_rvc.h"
#endif

#ifdef CONNECTOR_RVC
#include "connector_rvc.h"
#endif

#ifdef CONNECTOR_UART
#include "connector_uart.h"
#endif

#ifdef CONNECTOR_UART_DDM
#include "connector_uart_ddm.h"
#endif

#ifdef CONNECTOR_LIN_SERVER
#include "connector_lin_server.h"
#endif

#ifdef CONNECTOR_LINDEV
#include "connector_lindev.h"
#endif

#ifdef CONNECTOR_MQTT
#include "connector_mqtt.h"
#endif

#ifdef CONNECTOR_TLS
#include "connector_tls.h"
#endif

#ifdef CONNECTOR_TESLA
#include "connector_tesla.h"
#endif

#ifdef CONNECTOR_DEVSIM
#include "connector_devsim.h"
#endif

#ifdef CONNECTOR_RS485_MASTER
#include "connector_rs485_master.h"
#endif

#ifdef CONNECTOR_SYSTEM
#include "connector_system.h"
#endif

#ifdef CONNECTOR_RCS
#include "connector_rcs.h"
#endif

#ifdef CONNECTOR_MODEM
#include "connector_modem.h"
#endif

#ifdef CONNECTOR_PMIC
#include "connector_pmic.h"
#endif

#ifdef CONNECTOR_GNSS
#include "connector_gnss.h"
#endif

#ifdef CONNECTOR_HMI
#include "connector_hmi.h"
#endif

#ifdef CONNECTOR_BZ_SERVICE
#include "connector_bz_service.h"
#endif

#ifdef CONNECTOR_USM
#include "connector_usm.h"
#endif

#ifdef CONNECTOR_US_DICM
#include "connector_us_dicm.h"
#endif

#ifdef CONNECTOR_DDM_LOG
#include "connector_ddm_log.h"
#endif

#ifdef CONNECTOR_LIGHT
#include "connector_light.h"
#endif

#ifdef CONNECTOR_DEVICE_DISCOVERY
#include "connector_device_discovery.h"
#endif

#ifdef CONNECTOR_EVENT_NOTIFICATION
#include "connector_event_manager.h"
#include "connector_event_notification.h"
#endif

#ifdef CONNECTOR_CLIMATE_ZONE_FEATURE
#include "connector_climate_zone_feature.h"
#endif

#ifdef CONNECTOR_SMART_ECO_FEATURE
#include "connector_smart_eco_feature.h"
#endif

#ifdef CONNECTOR_FEATURE_MGR
#include "connector_feature_mgr.h"
#endif

#ifdef CONNECTOR_SUPERVISOR
#include "connector_supervisor.h"
#endif

#ifdef CONNECTOR_SYSAPPL_SERVICE
#include "connector_sysappl_service.h"
#endif

#ifdef CONNECTOR_ACCELEROMETER
#include "connector_accelerometer.h"
#endif

#ifdef CONNECTOR_IOT_MODEM
#include "connector_iot_modem.h"
#endif

#ifdef CONNECTOR_FULL_CLIMATE_RULE_ENGINE
#include "connector_fc_rule_engine.h"
#endif

#ifdef CONNECTOR_SMART_ECO_RULE_ENGINE
#include "connector_smart_eco_rule_engine.h"
#endif

#ifdef CONNECTOR_NFC
#include "connector_nfc.h"
#endif
#ifdef CONNECTOR_NMEA2K
#include "connector_nmea2k.h"
#endif

#ifdef CONNECTOR_BATT_MGMT_SERVICE
#include "connector_batt_mgmt_service.h"
#endif

#ifdef CONNECTOR_IC_MGMT_SERVICE
#include "connector_ic_mgmt_service.h"
#endif

#ifdef CONNECTOR_PROD_SNIFFER_SERVICE
#include "connector_prod_sniffer_service.h"
#endif

#if defined(CONNECTOR_SIMULATOR_SHAPE)
#include "connector_simulator_shape.h"
#endif

#if defined(CONNECTOR_SIMULATOR_SHARC)
#include "connector_simulator_sharc.h"
#endif

#if defined(CONNECTOR_SIMULATOR_INVENTILATE)
#include "connector_simulator_inventilate.h"
#endif

#if defined(CONNECTOR_SIMULATOR_CFX2)
#include "connector_simulator_cfxx.h"
#endif

#if defined(CONNECTOR_SIMULATOR_SHAPEX)
#include "connector_simulator_shapex.h"
#endif

#if defined(CONNECTOR_SIMULATOR_RULE)
#include "connector_simulator_rule.h"
#endif

#if defined(CONNECTOR_SIMULATOR_THUNDERSTORM)
#include "connector_simulator_thunderstorm.h"
#endif

#if defined(CONNECTOR_SIMULATOR_PROD_MBAT)
#include "connector_simulator_prod_mbat.h"
#endif

#if defined(CONNECTOR_UNITTEST)
#include "connector_unittest.h"
#endif
// Extern declare application connectors from application defined macro

#if defined(DICM_APPLICATION_CONNECTOR_EXTERN)
DICM_APPLICATION_CONNECTOR_EXTERN()
#endif

/*****************************************************************************
 * Local defines
 *****************************************************************************/
#define CONNECTOR_TASK_DURATION_WARN_MS        100
#define CONNECTOR_TASK_DURATION_ERROR_MS       250
#define CONNECTOR_TASK_OVERRUN_MS              2000
#define CONNECTOR_TASK_DURATION_CHECK_DELAY_MS 50

#define IS_CONNECTOR_TASK_BASED_CONNECTOR(connector) \
    (connector->process_event != NULL)
#ifndef CONNECTOR_TASK_CORE_ID
#ifndef CONFIG_IDF_TARGET_LINUX
#define CONNECTOR_TASK_CORE_ID tskNO_AFFINITY  // Default to run on both
#else
#define CONNECTOR_TASK_CORE_ID 0
#endif
#endif

/*****************************************************************************
 * Local functions
 *****************************************************************************/
static void connector_task(void *param);

/*****************************************************************************
 * Public variables
 *****************************************************************************/
//! \~ Broker provided ringbufferhandle
static RingbufHandle_t l_broker_ringbuffer_handle;
static CONNECTOR_PROCESS_EVENT l_broker_task;

//! \~ Broker connector list; defines enabled connectors
CONNECTOR *const enabled_connectors[] =
    {
#ifdef CONNECTOR_SYSTEM
        &connector_system,
#endif
#ifdef CONNECTOR_SYSAPPL_SERVICE
        &connector_sysappl_service,
#endif
#ifdef CONNECTOR_BLE
        &connector_ble,
#endif
#ifdef CONNECTOR_WIFI
        &connector_wifi,
#endif
#ifdef CONNECTOR_TLS
        &connector_tls,
#endif
#ifdef CONNECTOR_UART
        &connector_uart,
#endif
#ifdef CONNECTOR_UART_DDM
        &connector_uart_ddm,
#endif
#ifdef CONNECTOR_LIN_LOOPBACK
        &connector_lin_loopback,
#endif
#ifdef CONNECTOR_PREM_RVC
        &connector_prem_rvc,
#endif
#ifdef CONNECTOR_RVC
        &connector_rvc,
#endif
#ifdef CONNECTOR_LINDEV
        &connector_lindev,
#endif
#ifdef CONNECTOR_MQTT
        &connector_mqtt,
#endif
#ifdef CONNECTOR_LIN_SERVER
        &connector_lin_server,
#endif
#ifdef CONNECTOR_TESLA
        &connector_tesla,
#endif
#ifdef CONNECTOR_DEVSIM
        &connector_devsim,
#endif
#ifdef CONNECTOR_RS485_MASTER
        &connector_rs485_master,
#endif
#ifdef CONNECTOR_USM
        &connector_usm,
#endif
#ifdef CONNECTOR_US_DICM
        &connector_us_dicm,
#endif
#ifdef CONNECTOR_MODEM
        &connector_modem,
#endif
#ifdef CONNECTOR_RCS
        &connector_rcs,
#endif
#ifdef CONNECTOR_PMIC
        &connector_pmic,
#endif
#ifdef CONNECTOR_GNSS
        &connector_gnss,
#endif
#ifdef CONNECTOR_BZ_SERVICE
        &connector_bz_service,
#endif
#ifdef CONNECTOR_TES_SERVICE
        &connector_tes_service,
#endif
#ifdef CONNECTOR_ACCELEROMETER
        &connector_accelerometer,
#endif
#ifdef CONNECTOR_IOT_MODEM
        &connector_iot_modem,
#endif
#ifdef CONNECTOR_HMI
        &connector_hmi,
#endif
#ifdef CONNECTOR_DEVICE_DISCOVERY
        &connector_device_discovery,
#endif
#ifdef CONNECTOR_EVENT_NOTIFICATION
        &connector_event_manager,
        &connector_event_notification,
#endif
#ifdef CONNECTOR_DDM_LOG
        &connector_ddm_log,
#endif
#ifdef CONNECTOR_FULL_CLIMATE_RULE_ENGINE
        &connector_fc_rule_engine,
#endif
#ifdef CONNECTOR_SMART_ECO_RULE_ENGINE
        &connector_smart_eco_rule_engine,
#endif
#ifdef CONNECTOR_NFC
        &connector_nfc,
#endif
#ifdef CONNECTOR_NMEA2K
        &connector_nmea2k,
#endif
#ifdef CONNECTOR_BATT_MGMT_SERVICE
        &connector_batt_mgmt_service,
#endif
#ifdef CONNECTOR_IC_MGMT_SERVICE
        &connector_ic_mgmt_service,
#endif
#ifdef CONNECTOR_CLIMATE_ZONE_FEATURE
        &connector_climate_zone_feature,
#endif
#ifdef CONNECTOR_SMART_ECO_FEATURE
        &connector_smart_eco_feature,
#endif
#ifdef CONNECTOR_FEATURE_MGR
        &connector_feature_mgr,
#endif
#ifdef CONNECTOR_PROD_SNIFFER_SERVICE
        &connector_prod_sniffer_service,
#endif
#if defined(DICM_APPLICATION_CONNECTORS)
        DICM_APPLICATION_CONNECTORS()
#endif
#ifdef CONNECTOR_LIGHT
            & connector_light,
#endif
#ifdef CONNECTOR_SUPERVISOR
        &connector_supervisor,
#endif
/* Keep simulators connectors below this line. Operational connectors are above this line */
#if defined(CONNECTOR_SIMULATOR_SHAPE)
        &connector_simulator_shape,
#endif
#if defined(CONNECTOR_SIMULATOR_INVENTILATE)
        &connector_simulator_inventilate,
#endif
#if defined(CONNECTOR_SIMULATOR_SHARC_HEATER)
        &connector_simulator_sharc_heater,
#endif
#if defined(CONNECTOR_SIMULATOR_SHARC_CLIMATE_CONTROL)
        &connector_simulator_sharc_climate_control,
#endif
#if defined(CONNECTOR_SIMULATOR_SHARC_CLIMATE_CONTROL_SCHEDULER)
        &connector_simulator_sharc_climate_control_scheduler,
#endif
#if defined(CONNECTOR_SIMULATOR_CFX2)
        &connector_simulator_cfx2_mccc,
        &connector_simulator_cfx2_prod,
        &connector_simulator_cfx2_hmi,
#endif
#if defined(CONNECTOR_SIMULATOR_SHAPEX)
        &connector_simulator_shapex_ac,
        &connector_simulator_shapex_prod,
        &connector_simulator_shapex_dim_a,
        &connector_simulator_shapex_dim_b,
        &connector_simulator_shapex_hmi,
#endif
#if defined(CONNECTOR_SIMULATOR_RULE)
        &connector_simulator_rule,
#endif
#if defined(CONNECTOR_SIMULATOR_THUNDERSTORM)
        &connector_simulator_thunderstorm_hmi,
        &connector_simulator_thunderstorm_spir,
        &connector_simulator_thunderstorm_rfan,
#endif
#if defined(CONNECTOR_SIMULATOR_PROD_MBAT)
        &connector_simulator_mbat,
        &connector_simulator_mbat_prod,
#endif
#if defined(CONNECTOR_UNITTEST)
        &connector_unittest,
#endif
};

CONNECTOR_SLOT **connectors;                              //! \~ Broker connector list
CONNECTOR_SLOT *connector_storage;                        //! \~ Broker connector storage
int connector_count;                                      //! \~ Broker connector count
static StaticRingbuffer_t connector_task_ringbuffer;      //! \~ Connector task based connectors ringbuffer data structure
static RingbufHandle_t connector_task_ringbuffer_handle;  //! \~ Connector task based connectors ringbuffer

static TimerHandle_t connector_timer;
static void connector_timer_callback(TimerHandle_t xTimer);

#if defined(CONNECTOR_MONITOR_BROKER_RINGBUFFER) || defined(CONNECTOR_MONITOR_CONNECTORS_RINGBUFFER)
#include "sorted_container.h"
typedef struct CONNECTOR_MONITOR_RINGBUFFER
{
    const char *name;               //!< \~ name of the ringbuffer
    RingbufHandle_t buffer;         //!< \~ ringbuffer handle
    UBaseType_t size;               //!< \~ size of ringbuffer (bytes)
    uint8_t max_usage_percent;      //!< \~ max usage of ringbuffer in percentage
    BaseType_t min_free_space;      //!< \~ new minimum free space (bytes)
    UBaseType_t items_to_retrieve;  //!< \~ number of items waiting to be retrieved
    // Private
    UBaseType_t uxFree;  //!< \~ store free pointer position
} CONNECTOR_MONITOR_RINGBUFFER_T;

static void connector_monitor_ringbuffer(CONNECTOR_MONITOR_RINGBUFFER_T *monitor_ringbuf);
static void connector_monitor_ringbuffers_dump(uint32_t TickType_t);

// Broker ringbuffer's helper macros
#if defined(CONNECTOR_MONITOR_BROKER_RINGBUFFER)
static void connector_monitor_broker_ringbuffer(void);
#define MONITOR_BROKER_RINGBUFFERS_SLOT                                       (1)
#define MONITOR_ADD_BROKER_RINGBUFFER(ringbuff_name, ringbuff, ringbuff_size) MONITOR_ADD_RINGBUFFER(ringbuff_name, ringbuff, ringbuff_size)
#define MONITOR_BROKER_RINGBUFFER()                                           connector_monitor_broker_ringbuffer()
#else  //! CONNECTOR_MONITOR_BROKER_RINGBUFFER
#define MONITOR_ADD_BROKER_RINGBUFFER(ringbuff_name, ringbuff, ringbuff_size)
#define MONITOR_BROKER_RINGBUFFERS_SLOT (0)
#define MONITOR_BROKER_RINGBUFFER()
#endif  // CONNECTOR_MONITOR_BROKER_RINGBUFFER

// Connector ringbuffer's helper macros
#if defined(CONNECTOR_MONITOR_CONNECTORS_RINGBUFFER)
static void connector_monitor_connectors_ringbuffer(uint32_t connector_id);
#define MONITOR_CONNECTORS_RINGBUFFERS_SLOT                                      (ELEMENTS(enabled_connectors))
#define MONITOR_ADD_CONNECTOR_RINGBUFFER(ringbuff_name, ringbuff, ringbuff_size) MONITOR_ADD_RINGBUFFER(ringbuff_name, ringbuff, ringbuff_size)
#define MONITOR_CONNECTOR_RINGBUFFER(connector_id)                               connector_monitor_connectors_ringbuffer(connector_id)
#else  //! CONNECTOR_MONITOR_CONNECTORS_RINGBUFFER
#define MONITOR_CONNECTORS_RINGBUFFERS_SLOT (0)
#define MONITOR_ADD_CONNECTOR_RINGBUFFER(ringbuff_name, ringbuff, ringbuff_size)
#define MONITOR_CONNECTOR_RINGBUFFER(connector_id)
#endif  // CONNECTOR_MONITOR_CONNECTORS_RINGBUFFER

// Helper macros
#define MONITOR_RINGBUFFER_DUMP(time_elapsed) connector_monitor_ringbuffers_dump(time_elapsed)

#define MONITOR_RINGBUFFER_PRINT(level, mointor_ringbuf)         \
    LOG(level, "'%s' ringbuf:\n\
			Size of buffer: %u\n\
			Minimum free buffer size: %u\n\
			Maximum buffer usage: %u%%\n\
			Maximum items waiting to be retrieved observed: %u", \
        mointor_ringbuf->name,                                   \
        mointor_ringbuf->size,                                   \
        mointor_ringbuf->min_free_space,                         \
        mointor_ringbuf->max_usage_percent,                      \
        mointor_ringbuf->items_to_retrieve);

// Allocate memory for enabled connectors and the broker. connector_task based connectors will refer to same
// ringbuffer("connector_task"), although we allocate space for each connector seperatly.
#define NUM_OF_RINGBUFFERS_TO_MONITOR (MONITOR_BROKER_RINGBUFFERS_SLOT + MONITOR_CONNECTORS_RINGBUFFERS_SLOT)
SORTED_CONTAINER__DECLARE_EXTRAM(monitor_ringbuffs_container, NUM_OF_RINGBUFFERS_TO_MONITOR, CONNECTOR_MONITOR_RINGBUFFER_T);

#define MONITOR_INIT_RINGBUFFER(ringbuff_monitor_handle, ringbuff_name, ringbuff, ringbuff_size) \
    (ringbuff_monitor_handle)->name = ringbuff_name;                                             \
    (ringbuff_monitor_handle)->buffer = ringbuff;                                                \
    (ringbuff_monitor_handle)->size = ringbuff_size;                                             \
    (ringbuff_monitor_handle)->min_free_space = ringbuff_size;                                   \
    (ringbuff_monitor_handle)->max_usage_percent = 0;                                            \
    (ringbuff_monitor_handle)->items_to_retrieve = 0;

#define MONITOR_ADD_RINGBUFFER(ringbuff_name, ringbuff, ringbuff_size)                                               \
    CONNECTOR_MONITOR_RINGBUFFER_T *monitor_ringbuffer =                                                             \
        ((CONNECTOR_MONITOR_RINGBUFFER_T *)sorted_container__new(&monitor_ringbuffs_container, (uint32_t)ringbuff)); \
    MONITOR_INIT_RINGBUFFER(monitor_ringbuffer, ringbuff_name, ringbuff, ringbuff_size)

#else  //!(defined(CONNECTOR_MONITOR_BROKER_RINGBUFFER) || defined(CONNECTOR_MONITOR_CONNECTORS_RINGBUFFER))
#define MONITOR_RINGBUFFER_DUMP(time_elapsed)
#define MONITOR_BROKER_RINGBUFFER()
#define MONITOR_CONNECTOR_RINGBUFFER(connector_id)
#define MONITOR_ADD_BROKER_RINGBUFFER(ringbuff_name, ringbuff, ringbuff_size)
#define MONITOR_ADD_CONNECTOR_RINGBUFFER(ringbuff_name, ringbuff, ringbuff_size)
#endif  // defined(CONNECTOR_MONITOR_BROKER_RINGBUFFER) || defined(CONNECTOR_MONITOR_CONNECTORS_RINGBUFFER)

/*! \brief Create and send DDMP2 frame to a ring buffer
    \param control Control byte
    \param parameter DDMP2 parameter ID
    \param value Payload to add to frame
    \param value_size Size of payload
    \param connector Connector to tag frame with
    \param timeout How long to block waiting for access to ring buffer
    \param ring_buffer Ring buffer to send frame to
    \return TRUE if successful, a NOP frame bay be sent in case of failure
 */
static int connector_send_frame(const DDMP2_CONTROL_ENUM control, const uint32_t parameter, const void *const value, const uint8_t value_size, const uint8_t connector, const TickType_t timeout, const RingbufHandle_t ring_buffer)
{
    if (value_size)
    {
        TRUE_CHECK_RETURN0(value != NULL);
    }

    DDMP2_FRAME frame;
    int create_frame_result;

    switch (control)
    {
    case DDMP2_CONTROL_PUBLISH:
        TRUE_CHECK(create_frame_result = ddmp2_create_publish(&frame, parameter, value, value_size, connector));
        break;
    case DDMP2_CONTROL_SET:
        TRUE_CHECK(create_frame_result = ddmp2_create_set(&frame, parameter, value, value_size, connector));
        break;
    case DDMP2_CONTROL_SUBSCRIBE:
        TRUE_CHECK(create_frame_result = ddmp2_create_subscribe(&frame, parameter, connector));
        break;
    case DDMP2_CONTROL_UNSUBSCRIBE:
        TRUE_CHECK(create_frame_result = ddmp2_create_unsubscribe(&frame, parameter, connector));
        break;
    case DDMP2_CONTROL_NOP:
        TRUE_CHECK(create_frame_result = ddmp2_create_nop(&frame, connector));
        break;
    case DDMP2_CONTROL_REG:
        TRUE_CHECK(create_frame_result = ddmp2_create_reg(&frame, parameter, connector));
        break;
    case DDMP2_CONTROL_MESSAGE:
        TRUE_CHECK(create_frame_result = ddmp2_create_message(&frame, (uint8_t)parameter, connector));
        break;
    case DDMP2_CONTROL_MULTIBROKER:
        if (value_size != sizeof(uint64_t))
        {
            LOG(E, "Bad value size: %d", value_size);
            return 0;
        }
        TRUE_CHECK(create_frame_result = ddmp2_create_multibroker(&frame, parameter, *((uint64_t *)value), connector));
        break;
    case DDMP2_CONTROL_GENERIC:
        TRUE_CHECK(create_frame_result = ddmp2_create_generic(&frame, parameter, value, value_size, connector));
        break;
    default:
        LOG(E, "Bad control byte: 0x%x", control);
        return 0;
    }

    MONITOR_CONNECTOR_RINGBUFFER(connector);

    if (!create_frame_result)
    {
        return 0;
    }

    BaseType_t send_result;

    TRUE_CHECK(send_result = xRingbufferSend(ring_buffer, &frame, frame.frame_size + DDMP2_METADATA_SIZE, timeout));

    return send_result == pdTRUE;
}

/*! \brief Create and send DDMP2 frame to broker via ring buffer
    \param control Control byte
    \param parameter DDMP2 parameter ID
    \param value Payload to add to frame
    \param value_size Size of payload
    \param source_connector Connector ID to tag frame with
    \param timeout How long to block waiting for access to ring buffer
    \return TRUE if successful, a NOP frame may be sent in case of failure
 */
int connector_send_frame_to_broker(const DDMP2_CONTROL_ENUM control, const uint32_t parameter, const void *value, const uint8_t value_size, const uint8_t source_connector, const TickType_t timeout)
{
    return connector_send_frame(control, parameter, value, value_size, source_connector, timeout, l_broker_ringbuffer_handle);
}

/*! \brief Create and send DDMP2 frame to connector via ring buffer
    \param control Control byte
    \param parameter DDMP2 parameter ID
    \param value Payload to add to frame
    \param value_size Size of payload
    \param destination_connector Connector to tag frame with and to send to
    \param timeout How long to block waiting for access to ring buffer
    \return TRUE if successful, a NOP frame bay be sent in case of failure
 */
int connector_send_frame_to_connector(const DDMP2_CONTROL_ENUM control, const uint32_t parameter, const void *const value, const uint8_t value_size, const uint8_t destination_connector, const TickType_t timeout)
{
    return connector_send_frame(control, parameter, value, value_size, destination_connector, timeout, connectors[destination_connector]->to_connector);
}

/*! \brief Forward frame to destination connector
    \param pframe Pointer to DDMP2 frame
    \param destination_connector ID of the connector that should receive the frame
    \return pdTRUE if succeeded
 */
int connector_forward_frame_to_connector(const DDMP2_FRAME *pframe, const int destination_connector)
{
    const DDMP2_FRAME forward_frame =
        {
            .frame = pframe->frame,
            .frame_size = pframe->frame_size,
            .source_connector = pframe->source_connector,
            .destination_connector = destination_connector,
        };

    MONITOR_CONNECTOR_RINGBUFFER(destination_connector);
    return xRingbufferSend(connectors[destination_connector]->to_connector, &forward_frame, forward_frame.frame_size + DDMP2_METADATA_SIZE, BROKER_SEND_TIMEOUT);
}

/*! \brief Forward frame to broker
    \param pframe Pointer to DDMP2 frame
    \return pdTRUE if succeeded
 */
int connector_forward_frame_to_broker(const DDMP2_FRAME *const pframe)
{
    MONITOR_BROKER_RINGBUFFER();
    return xRingbufferSend(l_broker_ringbuffer_handle, pframe, pframe->frame_size + DDMP2_METADATA_SIZE, portMAX_DELAY);
}

int connectors_enabled(void)
{
    return ELEMENTS(enabled_connectors);
}

/**
 * \brief Overide this function in DICMApplication to disable any connectors depending on application requirements.
 */
__attribute__((weak)) void disable_connectors(void)
{
    LOG(I, "Running all connectors");
}

//! \brief Create ring buffer for connector_task based connectors
static RingbufHandle_t connector_task_ringbuffer_create(void)
{
    int connector_task_based_count = 0;
    void *connectors_ringbuffer_storage = NULL;
    StaticRingbuffer_t *connectors_ringbuffer = NULL;
    RingbufHandle_t connectors_ringbuffer_handle = NULL;

    // Determine number of connectors processed by connector_task
    for (int connector = 0; connector < (int)ELEMENTS(enabled_connectors); connector++)
    {
        // connector_task based connectors
        if (IS_CONNECTOR_TASK_BASED_CONNECTOR(enabled_connectors[connector]))
        {
            connector_task_based_count += 1;
        }
    }

    // allocate memory for connector_task based connectors
    if (connector_task_based_count > 0)
    {
        connectors_ringbuffer = &connector_task_ringbuffer;
        TRUE_CHECK_RETURN0((connectors_ringbuffer_storage = hal_mem_malloc_prefer(connector_task_based_count * TO_CONNECTOR_RINGBUFFER_SIZE, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM)) != NULL);
        TRUE_CHECK((connectors_ringbuffer_handle = xRingbufferCreateStatic(connector_task_based_count * TO_CONNECTOR_RINGBUFFER_SIZE, RINGBUF_TYPE_NOSPLIT, connectors_ringbuffer_storage, connectors_ringbuffer)) != NULL);

        MONITOR_ADD_CONNECTOR_RINGBUFFER("connector_task", connectors_ringbuffer_handle, connector_task_based_count * TO_CONNECTOR_RINGBUFFER_SIZE);
    }

    return connectors_ringbuffer_handle;
}

/*****************************************************************************
 * Function:    install_connectors
 * Description: Install and initialize the connectors
 *****************************************************************************/
void install_connectors(const RingbufHandle_t broker_ringbuffer_handle, CONNECTOR_PROCESS_EVENT extra_task)
{
    // Disable any connectors first
    disable_connectors();
    MONITOR_ADD_BROKER_RINGBUFFER("broker", broker_ringbuffer_handle, TO_BROKER_RINGBUFFER_SIZE);

    for (int connector = 0; connector < (int)ELEMENTS(enabled_connectors); connector++)  // determine number of needed connector slots
    {
        if (!enabled_connectors[connector]->disabled)
        {
            connector_count += 1 + enabled_connectors[connector]->data_lines;
        }
    }

    connectors = hal_mem_malloc_prefer(connector_count * sizeof(connectors[0]), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);  // Allocate memory for connector list (pointers)
    assert(connectors != NULL);
    connector_storage = hal_mem_malloc_prefer(connector_count * sizeof(connector_storage[0]), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);  // Allocated memory for connector storage (structs)
    assert(connector_storage != NULL);
    l_broker_ringbuffer_handle = broker_ringbuffer_handle;
    l_broker_task = extra_task;

    connector_task_ringbuffer_handle = connector_task_ringbuffer_create();

    int connector_id = 0;

    for (int connector = 0; connector < (int)ELEMENTS(enabled_connectors); connector++)
    {
        if (enabled_connectors[connector]->disabled)
        {
            // Skip this connector
            LOG(I, "Skip installing (disabled) %s", enabled_connectors[connector]->name);
            continue;
        }
        if (IS_CONNECTOR_TASK_BASED_CONNECTOR(enabled_connectors[connector]))  // all task based connectors share the same ringbuffer
        {
            enabled_connectors[connector]->to_connector = connector_task_ringbuffer_handle;
        }
        else  // normal connector, create ringbuffer
        {
            void *ringbuffer_storage;
            TRUE_CHECK((ringbuffer_storage = hal_mem_malloc_prefer(TO_CONNECTOR_RINGBUFFER_SIZE, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM)) != NULL);
            TRUE_CHECK((enabled_connectors[connector]->to_connector = xRingbufferCreateStatic(TO_CONNECTOR_RINGBUFFER_SIZE, RINGBUF_TYPE_NOSPLIT, ringbuffer_storage, &connector_storage[connector].ringbuffer)) != NULL);
            MONITOR_ADD_CONNECTOR_RINGBUFFER(enabled_connectors[connector]->name, enabled_connectors[connector]->to_connector, TO_CONNECTOR_RINGBUFFER_SIZE);
        }

        enabled_connectors[connector]->to_broker = l_broker_ringbuffer_handle;  // provide connector with broker ringbuffer handle
        enabled_connectors[connector]->connector_id = connector_id;             // and its connector id

        const int Slots = enabled_connectors[connector]->data_lines;  // get requested number of data lines from connector

        for (int slot = 0; slot <= Slots; slot++, connector_id++)  // install connector slots
        {
            LOG(I, "Installing %s:%d (%d)", enabled_connectors[connector]->name, slot, connector_id);

            connectors[connector_id] = &connector_storage[connector_id];  // copy connector data to storage
            connectors[connector_id]->sub_connector_id = slot;
            connectors[connector_id]->to_broker = enabled_connectors[connector]->to_broker;
            connectors[connector_id]->to_connector = enabled_connectors[connector]->to_connector;
            connectors[connector_id]->name = enabled_connectors[connector]->name;
            connectors[connector_id]->initialize = enabled_connectors[connector]->initialize;
            connectors[connector_id]->task_init = enabled_connectors[connector]->task_init;
            connectors[connector_id]->process_event = enabled_connectors[connector]->process_event;
            connectors[connector_id]->connector_id = (int)connector_id;
            connectors[connector_id]->task_timer = 0;

            if (slot)  // this is a data line
            {
                connectors[connector_id]->task_init = NULL;  // data lines do not have tasks
                connectors[connector_id]->process_event = NULL;
            }
        }

        LOG(I, "Initializing %s", enabled_connectors[connector]->name);
        TRUE_CHECK(enabled_connectors[connector]->initialize());  // initialize connector
    }

    // Create default task handler (core 1), if at least one
    // non-task based connector is enabled in enabled_connectors list
    if (connector_task_ringbuffer_handle != NULL)
    {
        TRUE_CHECK(xTaskCreatePinnedToCore(connector_task, "connectors", xTASK_STACK_DEFAULT, NULL, xTASK_PRIORITY_NORMAL, NULL, CONNECTOR_TASK_CORE_ID));
    }
    else
    {
        LOG(I, "Non-task based connectors not enabled. \"connectors\" task is not created.");
    }

    // Broker extra task timer
    TRUE_CHECK((connector_timer = xTimerCreate(NULL, pdMS_TO_TICKS(5000), pdTRUE, NULL, connector_timer_callback)) != NULL);
    TRUE_CHECK(xTimerStart(connector_timer, portMAX_DELAY));
}

/*****************************************************************************
 * Function:    connector_task
 * Description: Main task
 *****************************************************************************/
static void connector_task(void *param)
{
    // Init connectors
    for (int i = 0; i < connector_count; i++)
    {
        if (connectors[i]->task_init != NULL)
        {
            connectors[i]->task_init();
        }
    }

    // Run tasks
    uint32_t now = hal_cpu_get_millis();
    uint32_t startup_task_delay = now;
    uint32_t duration_max = 0;

    while (1)
    {
        DDMP2_FRAME *pframe;
        size_t frame_size;

        // Block on connectors queue
        TRUE_CHECK((pframe = xRingbufferReceive(connector_task_ringbuffer_handle, &frame_size, portMAX_DELAY)) != NULL);
        if (pframe)
        {
            // Destination connector installed
            if (pframe->destination_connector < connector_count)
            {
                if (pframe->destination_connector >= connectors[pframe->destination_connector]->sub_connector_id)
                {
                    // Get base connector id. If frame is sent towards any data line, it should be processed by the connector itself
                    uint32_t connector_id = pframe->destination_connector - connectors[pframe->destination_connector]->sub_connector_id;

                    // Run connector
                    if (IS_CONNECTOR_TASK_BASED_CONNECTOR(connectors[connector_id]))
                    {
                        // Update time
                        now = hal_cpu_get_millis();
                        connectors[pframe->destination_connector]->task_timer = now;

                        // Proces connector event
                        connectors[connector_id]->process_event(pframe);

                        now = hal_cpu_get_millis();

                        // Calculate duration
                        uint32_t duration = now - connectors[pframe->destination_connector]->task_timer;
                        if (startup_task_delay >= CONNECTOR_TASK_DURATION_CHECK_DELAY_MS)
                        {
                            if (duration > duration_max)
                            {
                                if (duration >= CONNECTOR_TASK_DURATION_WARN_MS)
                                {
                                    LOG(I, "Connector task new maximum duration: %d ms", duration);
                                }
                                duration_max = duration;
                            }
                            if (duration >= CONNECTOR_TASK_DURATION_WARN_MS && duration < CONNECTOR_TASK_DURATION_ERROR_MS)
                            {
                                LOG(W, "Connector task duration (%s (%u): %d ms)", connectors[pframe->destination_connector]->name, pframe->destination_connector, duration);
                            }
                            else if (duration >= CONNECTOR_TASK_DURATION_ERROR_MS)
                            {
                                LOG(E, "Connector task duration (%s (%u): %d ms)", connectors[pframe->destination_connector]->name, pframe->destination_connector, duration);
                            }
                        }
                        else
                        {
                            startup_task_delay = now;
                        }

                        if (duration >= CONNECTOR_TASK_OVERRUN_MS)
                        {
                            LOG(E, "Connector task overrun (%s:%d: %d ms)", connectors[pframe->destination_connector]->name, connectors[pframe->destination_connector]->sub_connector_id, duration);
                        }
                    }
                }
                else
                {
                    LOG(E, "Invalid connector's data line received (%s:%d > %d)", connectors[pframe->destination_connector]->name, connectors[pframe->destination_connector]->sub_connector_id, pframe->destination_connector);
                }
            }
            else
            {
                LOG(E, "Connector with id(%" PRIu8 ") is not installed", pframe->destination_connector);
            }
        }

        vRingbufferReturnItem(connector_task_ringbuffer_handle, pframe);
    }
}

#ifdef CONNECTOR_MONITOR_BROKER_RINGBUFFER
static void connector_monitor_broker_ringbuffer(void)
{
    CONNECTOR_MONITOR_RINGBUFFER_T *monitor_broker_ringbuf = sorted_container__access(&monitor_ringbuffs_container, (uint32_t)l_broker_ringbuffer_handle);
    connector_monitor_ringbuffer(monitor_broker_ringbuf);
}
#endif  // CONNECTOR_MONITOR_BROKER_RINGBUFFER

#ifdef CONNECTOR_MONITOR_CONNECTORS_RINGBUFFER
static void connector_monitor_connectors_ringbuffer(uint32_t connector_id)
{
    CONNECTOR_SLOT *connector = connectors[connector_id];
    CONNECTOR_MONITOR_RINGBUFFER_T *monitor_connector_ringbuf = sorted_container__access(&monitor_ringbuffs_container, (uint32_t)connector->to_connector);
    connector_monitor_ringbuffer(monitor_connector_ringbuf);
}
#endif  // CONNECTOR_MONITOR_CONNECTORS_RINGBUFFER

#if defined(CONNECTOR_MONITOR_BROKER_RINGBUFFER) || defined(CONNECTOR_MONITOR_CONNECTORS_RINGBUFFER)
static void connector_monitor_ringbuffer(CONNECTOR_MONITOR_RINGBUFFER_T *monitor_ringbuf)
{
    UBaseType_t uxFree = 0;
    UBaseType_t uxAcquire = 0;
    UBaseType_t uxItemsWaiting = 0;
    BaseType_t xfree;

    // Check ringbuffers
    vRingbufferGetInfo(monitor_ringbuf->buffer, &uxFree, NULL, NULL, &uxAcquire, &uxItemsWaiting);
    xfree = uxFree - uxAcquire;
    if (xfree <= 0)
    {
        xfree += monitor_ringbuf->size;
    }

    if (xfree < monitor_ringbuf->min_free_space)
    {
        monitor_ringbuf->min_free_space = xfree;
        monitor_ringbuf->items_to_retrieve = uxItemsWaiting;

        float usage_percentage = ((float)(monitor_ringbuf->size - xfree) / monitor_ringbuf->size);
        monitor_ringbuf->max_usage_percent = (uint8_t)(usage_percentage * 100);

        monitor_ringbuf->uxFree = uxFree;
    }
}

static void connector_monitor_ringbuffers_dump(TickType_t time_elapsed)
{
    static TickType_t time_elapsed_10s = 0;

    time_elapsed_10s += time_elapsed;
    if (time_elapsed_10s >= pdMS_TO_TICKS(10000))
    {
        LOG(D, "*** Dump ringbuffers runtime statistics ***");
        for (size_t ringbuf_n = 0; ringbuf_n < sorted_container__occupied(&monitor_ringbuffs_container); ++ringbuf_n)
        {
            uint32_t ringbuf_key;
            CONNECTOR_MONITOR_RINGBUFFER_T *monitor_ringbuf;

            sorted_container__iterate(&monitor_ringbuffs_container, ringbuf_n, (void **)&monitor_ringbuf, &ringbuf_key);

            if (monitor_ringbuf->max_usage_percent >= 80)
            {
                MONITOR_RINGBUFFER_PRINT(E, monitor_ringbuf);
            }
            else if (monitor_ringbuf->max_usage_percent >= 50 && monitor_ringbuf->max_usage_percent < 80)
            {
                MONITOR_RINGBUFFER_PRINT(W, monitor_ringbuf);
            }
            else
            {
                MONITOR_RINGBUFFER_PRINT(D, monitor_ringbuf);
            }
        }

        time_elapsed_10s = 0;
    }
}
#endif  // defined(CONNECTOR_MONITOR_BROKER_RINGBUFFER) || defined(CONNECTOR_MONITOR_CONNECTORS_RINGBUFFER)

static void connector_timer_callback(TimerHandle_t xTimer)
{
    l_broker_task(NULL);

    MONITOR_RINGBUFFER_DUMP(xTimerGetPeriod(xTimer));
}
