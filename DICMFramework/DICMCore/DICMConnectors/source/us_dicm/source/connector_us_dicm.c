/*****************************************************************************
 * \file       connector_us_dicm.c
 * \brief      Connector Service DICM ESP32 update service
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
// System include
#include <stdint.h>
#include <string.h>

// IDF include
#include "cJSON.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"

// Framework include
#include "configuration.h"

#include "broker.h"
#include "connector_us_dicm.h"
#include "crc32.h"
#include "ddm2_parameter_list.h"
#include "fsm.h"
#include "hal_cpu.h"
#ifdef CONNECTOR_US_HMI
#include "hmi_data.h"
#endif
#include "update_service_types.h"

/*****************************************************************************
 * Private defines
 ****************************************************************************/
#define US_EXTENDED_LOGS 0

#if US_EXTENDED_LOGS
#define US_LOG(level, format, ...) LOG(level, format, ##__VA_ARGS__)
#else
#define US_LOG(level, format, ...) ((void)0)
#endif  // USM_EXTENDED_LOGS
#ifndef CONNECTOR_US_DICM_US_STATE_EXTENDED_LOGS
#define CONNECTOR_US_DICM_US_STATE_EXTENDED_LOGS 0
#endif
#if CONNECTOR_US_DICM_US_STATE_EXTENDED_LOGS
#define US_STATE_LOG(level, format, ...) LOG(level, format, ##__VA_ARGS__)
#else
#define US_STATE_LOG(level, format, ...) ((void)0)
#endif  // CONNECTOR_US_DICM_US_STATE_EXTENDED_LOGS

// Possible to override in config
#ifndef CONNECTOR_US_DICM_TASK_STACK_SIZE
#define CONNECTOR_US_DICM_TASK_STACK_SIZE (3840)
#endif
#ifndef CONNECTOR_US_DICM_STORAGE_SIZE
#define CONNECTOR_US_DICM_STORAGE_SIZE (1024 * 10)
#endif
// #define IGNORE_OTA_CALL

#define OTA_FILE_TYPE_DICM     (0)
#define OTA_FILE_TYPE_HMI_DATA (1)
#define OTA_FILE_TYPE_FS_DATA  (2)

#define PROGRESS_START           (0)
#define PROGRESS_HALF_COMPLETE   (50)
#define PROGRESS_NEARLY_COMPLETE (99)
#define PROGRESS_COMPLETE        (100)

#ifdef CONNECTOR_US_HMI
#define HMI_DATA_ID "-HMI"
static const char firmware_hmi_id_name[] = FIRMWARE_BUILD_ID HMI_DATA_ID;
#endif
#ifdef CONNECTOR_US_FS
#define FS_DATA_ID "-FS"
static const char firmware_fs_id_name[] = FIRMWARE_BUILD_ID FS_DATA_ID;
#endif
static const char firmware_id_name[] = FIRMWARE_BUILD_ID;
static EXT_RAM_ATTR char fwid_list[DDMP2_MAX_VALUE_SIZE];

// Structure defining which files are supported by the update service
static const struct
{
    const char *file_name;
    const char *(*get_version)(void);
    uint8_t type;
} l_ota_files[] = {
#ifdef CONNECTOR_US_HMI
    {
        .file_name = firmware_hmi_id_name,
        .get_version = hmi_data_get_version,
        .type = OTA_FILE_TYPE_HMI_DATA,
    },
#endif
#ifdef CONNECTOR_US_FS
    {
        .file_name = firmware_fs_id_name,
        .get_version = gateway_get_file_system_partition_version,
        .type = OTA_FILE_TYPE_FS_DATA,
    },
#endif
    {
        .file_name = firmware_id_name,
        .get_version = gateway_get_firmware_version,
        .type = OTA_FILE_TYPE_DICM,
    },
};

/*****************************************************************************
 * Private types
 ****************************************************************************/
typedef enum
{
    CRC_STATE_INIT,
    CRC_STATE_CALC
} crc_state_t;

typedef struct
{
    esp_ota_handle_t handle;
    const esp_partition_t *update_partition;
#ifdef CONNECTOR_US_HMI
    uint8_t nvs_partition;
#endif
    uint32_t size_cnt;
    const char *requested_file;
    uint16_t current_version;
    uint32_t dl_size;            // Download size
    uint32_t dl_crc;             // Download CRC
    int16_t block_byte_cnt;      // Counter for how many bytes that is received in the block
    int32_t transfer_block_sz;   // The current transfer block size defined by manager
    int32_t service_block_size;  // Own block size that service can handle
#if defined(CONNECTOR_US_HMI) || defined(CONNECTOR_US_FS)
    int32_t partition_bytes_written;  // Own block size that service can handle
#endif
    int instance;
    int client;
    crc32_t block_crc;
    const void *storage;
    const void *flash_pointer;
    uint32_t storage_size;
    uint8_t ota_file_index;
} us_ota_handle_t;

static EXT_RAM_ATTR us_ota_handle_t l_us;
static const uint32_t l_sub_list[] = {USM0DD, USM0TBS, USM0STAT};
static int32_t service_status = 0;  // TODO: Make to enum
static fsm_t my_fsm;

// Local fsm functions
static void my_state_idle(fsm_t *const p_sm, const fsm_event_t *p_event);
static void my_state_begin(fsm_t *const p_sm, const fsm_event_t *p_event);
static void my_state_in_progress(fsm_t *const p_sm, const fsm_event_t *p_event);
static void my_state_wait_complete(fsm_t *const p_sm, const fsm_event_t *p_event);
static void my_state_flash_complete(fsm_t *const p_sm, const fsm_event_t *p_event);
static void start_subscriptions(uint8_t instance);
static uint32_t get_event_type(const DDMP2_FRAME *const p_frame);
static int handle_data_set_new(const DDMP2_FRAME *const pframe);
static int handle_download_done(DDMP2_FRAME *const pframe);
static esp_err_t connector_us_dicm_prepare_ota(void);
static esp_err_t connector_us_dicm_ota_write(size_t sz, const uint8_t *data);
static esp_err_t connector_us_dicm_ota_end(void);
static esp_err_t connector_us_dicm_ota_abort(void);

static uint32_t get_event_type(const DDMP2_FRAME *const p_frame)
{
    uint32_t retvalue = FSM_UNDEFINED_EVENT;

    switch (p_frame->frame.control)
    {
    case DDMP2_CONTROL_PUBLISH:
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.publish.parameter))
        {
        case GW0INV:
        {
            uint32_t entry;
            uint8_t available;
            size_t sz = ddmp2_value_size(p_frame);

            for (size_t i = 0; i < sz; i += 4)
            {
                entry = *((uint32_t *)&p_frame->frame.publish.value.raw[i]);
                available = DDM2_PARAMETER_PROPERTY_FIELD(entry);

                if (available && (DDM2_PARAMETER_CLASS(entry) == USM0AVL))
                {
                    // Start suscription of Update manager class
                    start_subscriptions(DDM2_PARAMETER_INSTANCE_FIELD(entry));
                }
            }
            break;
        }
        case USM0DD:
        {
            retvalue = USMDD_PUB_EVENT;
            break;
        }
        case USM0TBS:
        {
            retvalue = USMTBS_PUB_EVENT;
            break;
        }
        case USM0STAT:
        {
            retvalue = USMSTAT_PUB_EVENT;
            break;
        }
        default:
        {
            LOG(W, "Unhandled PUB parameter! 0x%x", DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.publish.parameter));
            break;
        }
        }
        break;
    }
    case DDMP2_CONTROL_SET:
    {
        switch (DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.set.parameter))
        {
        case US0DATA:
        {
            retvalue = USDATA_SET_EVENT;
            break;
        }
        case US0BTR:
        {
            retvalue = USBTR_SET_EVENT;
            break;
        }
        default:
        {
            LOG(W, "Unhandled SET parameter! 0x%x", DDM2_PARAMETER_BASE_INSTANCE(p_frame->frame.set.parameter));
            break;
        }
        }
        break;
    }
    default:
    {
        break;
    }
    }
    return retvalue;
}

static void my_state_idle(fsm_t *p_sm, const fsm_event_t *p_event)
{
    switch (p_event->id)
    {
    case FSM_ENTRY_EVENT:
    {
        US_STATE_LOG(I, "%s", "ENTRY_EVENT");
        break;
    }
    case FSM_EXIT_EVENT:
    {
        US_STATE_LOG(I, "%s", "EXIT_EVENT");
        break;
    }
    case USMDD_PUB_EVENT:
    {
        US_STATE_LOG(I, "%s", "USMDD_PUB_EVENT");
        if (handle_download_done((DDMP2_FRAME *const)p_event->p_data) != 0)
        {
            // Clear size counter
            l_us.size_cnt = 0;
            // Clear block size counter
            l_us.block_byte_cnt = 0;
            fsm_state_change(p_sm, FSM_STATE_HANDLER(my_state_begin));
        }
        break;
    }
    default:
    {
        US_STATE_LOG(W, "unhandled event: %d", p_event->id);
        break;
    }
    }
}

static void my_state_begin(fsm_t *p_sm, const fsm_event_t *p_event)
{
    switch (p_event->id)
    {
    case FSM_ENTRY_EVENT:
    {
        US_STATE_LOG(I, "%s", "ENTRY_EVENT");
        l_us.block_crc = crc32_init();

        if (connector_us_dicm_prepare_ota() != ESP_OK)
        {
            us_data_read_ack_t ack;
            ZERO_CHECK(connector_us_dicm_ota_abort());
            ack.result = (uint8_t)US_ACK_RESULT_ABORT;
            ack.crc = 0x11223344u;
            LOG(W, "Could not erase %d", US_ACK_RESULT_ABORT);
            TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                                      US0DATA | DDM2_PARAMETER_INSTANCE(l_us.instance),
                                                      &ack,
                                                      sizeof(ack),
                                                      connector_us_dicm.connector_id,
                                                      portMAX_DELAY));
            fsm_state_change(p_sm, FSM_STATE_HANDLER(my_state_idle));
        }
        break;
    }
    case FSM_EXIT_EVENT:
    {
        US_STATE_LOG(I, "%s", "EXIT_EVENT");
        break;
    }
    case USMTBS_PUB_EVENT:
    {
        US_STATE_LOG(I, "%s: %d", "USMTBS_PUB_EVENT", ((DDMP2_FRAME *const)p_event->p_data)->frame.publish.value.int32);
        l_us.transfer_block_sz = ((DDMP2_FRAME *const)p_event->p_data)->frame.publish.value.int32;
        if (l_us.transfer_block_sz < l_us.service_block_size)
        {
            // Use manager block size as that is less
            l_us.service_block_size = l_us.transfer_block_sz;
            LOG(I, "Using manager block size of %d", l_us.service_block_size);
        }
        fsm_state_change(p_sm, FSM_STATE_HANDLER(my_state_in_progress));
        break;
    }
    case USMSTAT_PUB_EVENT:
    {
        US_STATE_LOG(I, "%s: %d", "USMSTAT_PUB_EVENT", ((DDMP2_FRAME *const)p_event->p_data)->frame.publish.value.int32);
        // If error goto idle
        if ((((DDMP2_FRAME *const)p_event->p_data)->frame.publish.value.int32) != 0)
        {
            // Something went wrong
            fsm_state_change(p_sm, FSM_STATE_HANDLER(my_state_idle));
        }
        break;
    }
    default:
    {
        US_STATE_LOG(W, "unhandled event: %d", p_event->id);
        break;
    }
    }
}

static void my_state_in_progress(fsm_t *const p_sm, const fsm_event_t *p_event)
{
    switch (p_event->id)
    {
    case FSM_ENTRY_EVENT:
    {
        US_STATE_LOG(I, "%s", "ENTRY_EVENT");
        break;
    }
    case FSM_EXIT_EVENT:
    {
        US_STATE_LOG(I, "%s", "EXIT_EVENT");
        break;
    }
    case USDATA_SET_EVENT:
    {
        int ret_val;
        // US_STATE_LOG(I, "%s", "USDATA_SET_EVENT");
        ret_val = handle_data_set_new((DDMP2_FRAME *const)p_event->p_data);
        switch (ret_val)
        {
        case 1:
        {
            fsm_state_change(p_sm, FSM_STATE_HANDLER(my_state_idle));
            break;
        }
        case 2:
        {
            fsm_state_change(p_sm, FSM_STATE_HANDLER(my_state_wait_complete));
            break;
        }
        default:
        {
            // Do nothing
            break;
        }
        }
        break;
    }
    case USBTR_SET_EVENT:
    {
        // US_STATE_LOG(I, "%s: %d", "USBTR_SET_EVENT", ((DDMP2_FRAME * const)p_event->p_data)->frame.set.value.int32);
        int32_t result = US_ACK_RESULT_OK;
        if (((DDMP2_FRAME *const)p_event->p_data)->frame.set.value.int32 == (int32_t)US_ACK_RESULT_OK)
        {
            US_LOG(I, "Block transferred OK!");

            // Done flashing, publish BTR with result = 0 = ok
        }
        else
        {
            LOG(E, "Block transfered Failure!");
            // No point in trying to resend as flashing has already been done.
            result = US_ACK_RESULT_ABORT;
        }
        TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                                  US0BTR | DDM2_PARAMETER_INSTANCE(l_us.instance),
                                                  &result,
                                                  sizeof(result),
                                                  connector_us_dicm.connector_id,
                                                  portMAX_DELAY));
        break;
    }
    case USMSTAT_PUB_EVENT:
    {
        US_STATE_LOG(I, "%s: %d", "USMSTAT_PUB_EVENT", ((DDMP2_FRAME *const)p_event->p_data)->frame.publish.value.int32);
        // If error goto idle
        if ((((DDMP2_FRAME *const)p_event->p_data)->frame.publish.value.int32) != 0)
        {
            // Something went wrong
            ZERO_CHECK(connector_us_dicm_ota_abort());
            fsm_state_change(p_sm, FSM_STATE_HANDLER(my_state_idle));
        }
        break;
    }
    default:
    {
        US_STATE_LOG(W, "unhandled event: %d", p_event->id);
        break;
    }
    }
}

static void my_state_wait_complete(fsm_t *const p_sm, const fsm_event_t *p_event)
{
    switch (p_event->id)
    {
    case FSM_ENTRY_EVENT:
    {
        US_STATE_LOG(I, "%s", "ENTRY_EVENT");
        break;
    }
    case FSM_EXIT_EVENT:
    {
        US_STATE_LOG(I, "%s", "EXIT_EVENT");
        break;
    }
    case USBTR_SET_EVENT:
    {
        int32_t result = US_ACK_RESULT_OK;

        US_STATE_LOG(I, "%s: %d", "USBTR_SET_EVENT", ((DDMP2_FRAME *const)p_event->p_data)->frame.set.value.int32);

        if (((DDMP2_FRAME *const)p_event->p_data)->frame.set.value.int32 == (int32_t)US_ACK_RESULT_OK)
        {
            US_LOG(I, "Block transfered OK!");

            // Done receiving data, publish BTR with result = 0 = ok
            TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                                      US0BTR | DDM2_PARAMETER_INSTANCE(l_us.instance),
                                                      &result,
                                                      sizeof(result),
                                                      connector_us_dicm.connector_id,
                                                      portMAX_DELAY));

            fsm_state_change(p_sm, FSM_STATE_HANDLER(my_state_flash_complete));
        }
        else
        {
            LOG(E, "Block transfered Failure!");
            // No point in trying to resend as flashing has already been done.
            result = US_ACK_RESULT_ABORT;

            // Done flashing, publish BTR with result = 0 = ok
            TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                                      US0BTR | DDM2_PARAMETER_INSTANCE(l_us.instance),
                                                      &result,
                                                      sizeof(int32_t),
                                                      connector_us_dicm.connector_id,
                                                      portMAX_DELAY));

            fsm_state_change(p_sm, FSM_STATE_HANDLER(my_state_idle));
        }
        break;
    }
    case USMSTAT_PUB_EVENT:
    {
        US_STATE_LOG(I, "%s: %d", "USMSTAT_PUB_EVENT", ((DDMP2_FRAME *const)p_event->p_data)->frame.publish.value.int32);
        // If error goto idle
        if ((((DDMP2_FRAME *const)p_event->p_data)->frame.publish.value.int32) != 0)
        {
            // Something went wrong
            ZERO_CHECK(connector_us_dicm_ota_abort());
            fsm_state_change(p_sm, FSM_STATE_HANDLER(my_state_idle));
        }
        break;
    }
    default:
    {
        US_STATE_LOG(W, "unhandled event: %d", p_event->id);
        break;
    }
    }
}

static void my_state_flash_complete(fsm_t *const p_sm, const fsm_event_t *p_event)
{
    switch (p_event->id)
    {
    case FSM_ENTRY_EVENT:
    {
        US_STATE_LOG(I, "%s", "ENTRY_EVENT");
        esp_err_t err;
        us_data_read_ack_t ack;
        int32_t result = US_ACK_RESULT_OK;
        USM0STATE_ENUM state = USM0STATE_EXTENDED_STATE;

        // Set the state to extended state
        TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_SET,
                                                  USM0STATE,
                                                  &state,
                                                  sizeof(int32_t),
                                                  connector_us_dicm.connector_id,
                                                  portMAX_DELAY));

        ZERO_CHECK(err = connector_us_dicm_ota_end());
        if (err == ESP_OK)
        {
            result = US_ACK_RESULT_OK_RESTART;
        }
        else
        {
            result = US_ACK_RESULT_ABORT;
        }

        // Done with flash procedures, publish status through US0DATA
        ack.result = (uint8_t)result;
        ack.crc = l_us.dl_crc;
        TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                                  US0DATA | DDM2_PARAMETER_INSTANCE(l_us.instance),
                                                  &ack,
                                                  sizeof(ack),
                                                  connector_us_dicm.connector_id,
                                                  portMAX_DELAY));

        if (err != ESP_OK)
        {
            ZERO_CHECK(connector_us_dicm_ota_abort());
        }

        fsm_state_change(p_sm, FSM_STATE_HANDLER(my_state_idle));

        break;
    }
    case FSM_EXIT_EVENT:
    {
        US_STATE_LOG(I, "%s", "EXIT_EVENT");
        break;
    }
    default:
    {
        US_STATE_LOG(W, "unhandled event: %d", p_event->id);
        break;
    }
    }
}

static void start_subscriptions(uint8_t instance)
{
    for (int i = 0; i < (int)(sizeof(l_sub_list) / sizeof(uint32_t)); i++)
    {
        TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                                  l_sub_list[i] | DDM2_PARAMETER_INSTANCE(instance),
                                                  NULL,
                                                  0,
                                                  connector_us_dicm.connector_id,
                                                  portMAX_DELAY));
    }
}

static esp_err_t connector_us_dicm_prepare_ota(void)
{
    esp_err_t err = ESP_OK;
    if (l_ota_files[l_us.ota_file_index].type == OTA_FILE_TYPE_DICM)
    {
        LOG(I, "Starting OTA...");
        l_us.update_partition = esp_ota_get_next_update_partition(NULL);
        if (l_us.update_partition == NULL)
        {
            LOG(E, "Passive OTA partition not found");
            err = ESP_FAIL;
        }
        else
        {
            LOG(I, "Writing to partition subtype %d at offset 0x%x", l_us.update_partition->subtype, l_us.update_partition->address);

            ZERO_CHECK(err = esp_ota_begin(l_us.update_partition, OTA_SIZE_UNKNOWN, &l_us.handle));
        }
    }
#ifdef CONNECTOR_US_HMI
    else if (l_ota_files[l_us.ota_file_index].type == OTA_FILE_TYPE_HMI_DATA)
    {
        uint8_t l_partition = 0;
        LOG(I, "Starting OTA of HMI...");
        l_partition = hmi_data_get_next_partition_index();
        l_us.partition_bytes_written = 0;

        LOG(I, "Erase and flash HMI_DATA partition %d", l_partition);
        l_us.nvs_partition = l_partition;

        l_us.update_partition = hmi_data_get_partition(l_partition);
        if (l_us.update_partition == NULL)
        {
            LOG(E, "OTA HMI partition not found");
            err = ESP_FAIL;
        }
        else
        {
            LOG(I, "Writing to partition subtype %d at offset 0x%x", l_us.update_partition->subtype, l_us.update_partition->address);
            ZERO_CHECK(err = esp_partition_erase_range(l_us.update_partition, 0, l_us.update_partition->size));
        }
    }
#endif
#ifdef CONNECTOR_US_FS
    else if (l_ota_files[l_us.ota_file_index].type == OTA_FILE_TYPE_FS_DATA)
    {
        LOG(I, "Starting OTA of FS partition...");
        l_us.partition_bytes_written = 0;

        l_us.update_partition = gateway_file_system_get_unused_partition();
        if (l_us.update_partition == NULL)
        {
            LOG(E, "OTA FS partition not found");
            err = ESP_FAIL;
        }
        else
        {
            LOG(I, "Erase and flash FS partition %s", l_us.update_partition->label);
            LOG(I, "Writing to partition subtype %d at offset 0x%x", l_us.update_partition->subtype, l_us.update_partition->address);
            ZERO_CHECK(err = esp_partition_erase_range(l_us.update_partition, 0, l_us.update_partition->size));
        }
    }
#endif

    if (err != ESP_OK)
    {
        l_us.handle = 0;
    }
    return err;
}

static esp_err_t connector_us_dicm_ota_write(size_t sz, const uint8_t *data)
{
    esp_err_t err = ESP_OK;
#ifndef IGNORE_OTA_CALL
    if (l_ota_files[l_us.ota_file_index].type == OTA_FILE_TYPE_DICM)
    {
        err = esp_ota_write(l_us.handle, data, sz);
    }
#if defined(CONNECTOR_US_HMI) || defined(CONNECTOR_US_FS)
    else if ((l_ota_files[l_us.ota_file_index].type == OTA_FILE_TYPE_HMI_DATA) || (l_ota_files[l_us.ota_file_index].type == OTA_FILE_TYPE_FS_DATA))
    {
        if (l_us.update_partition->encrypted)
        {
            // Make sure we have 16 bytes alignement
            if (sz % 16)
            {
                sz += (16 - (sz % 16));
            }
        }
        ZERO_CHECK(err = esp_partition_write(l_us.update_partition, l_us.partition_bytes_written, data, sz));
        l_us.partition_bytes_written += sz;
    }
#endif

#endif
    return err;
}

static esp_err_t connector_us_dicm_ota_abort(void)
{
    if (l_ota_files[l_us.ota_file_index].type == OTA_FILE_TYPE_DICM)
    {
        return esp_ota_abort(l_us.handle);
    }
    else
    {
        return ESP_OK;
    }
}

static void us_dicm_publish_progress(int32_t progress)
{
    TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                              US0PROG | DDM2_PARAMETER_INSTANCE(l_us.instance),
                                              &progress,
                                              sizeof(progress),
                                              connector_us_dicm.connector_id,
                                              portMAX_DELAY));
}

static esp_err_t connector_us_dicm_ota_end(void)
{
#ifndef IGNORE_OTA_CALL
    esp_err_t err = ESP_OK;

    LOG(I, "OTA ending....(%s)", l_ota_files[l_us.ota_file_index].file_name);

    us_dicm_publish_progress(PROGRESS_START);

    if (l_ota_files[l_us.ota_file_index].type == OTA_FILE_TYPE_DICM)
    {
        if ((err = esp_ota_end(l_us.handle)) == ESP_OK)
        {
            us_dicm_publish_progress(PROGRESS_HALF_COMPLETE);
            if ((err = esp_ota_set_boot_partition(l_us.update_partition)) != ESP_OK)
            {
                LOG(E, "Failed to set boot partition");
            }
            else
            {
                // Restart here if client. Restart is otherwise handled by manager.
                if (l_us.client)
                {
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    hal_cpu_reset(HALCPU_RESET_FLAG_NONE);
                }
            }
        }
        else
        {
            LOG(E, "Failed to end OTA");
        }
        us_dicm_publish_progress(PROGRESS_NEARLY_COMPLETE);
    }
#ifdef CONNECTOR_US_HMI
    else if (l_ota_files[l_us.ota_file_index].type == OTA_FILE_TYPE_HMI_DATA)
    {
        LOG(I, "Store HMI partition to use after reboot: %d", l_us.nvs_partition);
        // Switch part in nvs
        us_dicm_publish_progress(PROGRESS_HALF_COMPLETE);
        if (false == hmi_data_save_partition_index(l_us.nvs_partition))
        {
            LOG(E, "Failed to set HMI DATA partition (%d)", l_us.nvs_partition);
            LOG(E, "Failed to end OTA HMI");
        }
        else
        {
            // Restart here if client. Restart is otherwise handled by manager.
            if (l_us.client)
            {
                vTaskDelay(pdMS_TO_TICKS(1000));
                hal_cpu_reset(HALCPU_RESET_FLAG_NONE);
            }
        }
        us_dicm_publish_progress(PROGRESS_NEARLY_COMPLETE);
    }
#endif
#ifdef CONNECTOR_US_FS
    else if (l_ota_files[l_us.ota_file_index].type == OTA_FILE_TYPE_FS_DATA)
    {
        LOG(I, "Store next partition to use after reboot: %s", l_us.update_partition->label);
        // Switch part in nvs
        us_dicm_publish_progress(PROGRESS_HALF_COMPLETE);
        if (false == gateway_file_system_save_next_partition())
        {
            LOG(E, "Failed to end OTA FS");
        }
        else
        {
            // Restart here if client. Restart is otherwise handled by manager.
            if (l_us.client)
            {
                vTaskDelay(pdMS_TO_TICKS(1000));
                hal_cpu_reset(HALCPU_RESET_FLAG_NONE);
            }
        }
        us_dicm_publish_progress(PROGRESS_NEARLY_COMPLETE);
    }
#endif
    us_dicm_publish_progress(PROGRESS_COMPLETE);

    return err;
#else
    us_dicm_publish_progress(PROGRESS_COMPLETE);
    return ESP_OK;
#endif
}

static uint32_t connector_us_dicm_calc_crc32(const int last, const size_t sz, const void *const data)
{
    static crc32_t crc;
    static crc_state_t crc_state = CRC_STATE_INIT;

    switch (crc_state)
    {
    case CRC_STATE_INIT:
    {
        crc = crc32_init();
        crc_state = CRC_STATE_CALC;
    }
    // fallthrough
    case CRC_STATE_CALC:
    {
        if (sz)
        {
            crc = crc32_update(crc, data, sz);
        }
        if (last)
        {
            crc = crc32_finalize(crc);
            crc_state = CRC_STATE_INIT;
        }
        break;
    }
    default:
        break;
    }

    return crc;
}

static int handle_download_done(DDMP2_FRAME *const pframe)
{
    int retvalue = 0;
    us_dd_download_done_t *dd;
    us_rrq_read_request_t *rrq;
    size_t data_size = ddmp2_value_size(pframe);

    ESP_LOG_BUFFER_HEXDUMP("DD", pframe->frame.publish.value.raw, data_size, ESP_LOG_INFO);

    // Examine which download that is done
    dd = (us_dd_download_done_t *)pframe->frame.publish.value.raw;
    // Zero terminate string as received data is not zero terminated
    char fwid_string[data_size];
    memset(fwid_string, 0, data_size);
    memmove(fwid_string, dd->fwid, data_size - offsetof(us_dd_download_done_t, fwid));

    l_us.dl_size = dd->size;
    l_us.dl_crc = dd->crc;

    // New download, reset service blocksize to default
    l_us.service_block_size = CONNECTOR_US_DICM_STORAGE_SIZE;

    // Each bit in service means service instance, max 32 services
    // Check if this is for this service
    for (int i = 0; i < (int)ELEMENTS(l_ota_files); i++)
    {
        if ((dd->service & (1 << (l_us.instance & 0x1f))) && (strcmp(fwid_string, l_ota_files[i].file_name) == 0))
        {
            l_us.ota_file_index = i;
            rrq = heap_caps_malloc_prefer(sizeof(us_rrq_read_request_t) + strlen(l_ota_files[i].file_name), 2, MALLOC_CAP_DEFAULT | MALLOC_CAP_SPIRAM, MALLOC_CAP_DEFAULT | MALLOC_CAP_INTERNAL);

            rrq->version = 2;                                                // Supported version
            rrq->block_size = (uint16_t)(l_us.service_block_size & 0xFFFF);  // Requested block size
            rrq->frame_size = DDMP2_MAX_VALUE_SIZE;
            strcpy(rrq->transfer_info, l_ota_files[i].file_name);
            TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                                      US0RRQ | DDM2_PARAMETER_INSTANCE(l_us.instance),
                                                      rrq,
                                                      sizeof(us_rrq_read_request_t) + strlen(l_ota_files[i].file_name) - 1,
                                                      connector_us_dicm.connector_id,
                                                      (TickType_t)portMAX_DELAY));

            free(rrq);

            // Restart CRC calculation
            (void)connector_us_dicm_calc_crc32(1, 0, NULL);

            retvalue = 1;
            break;
        }
    }
    return retvalue;
}

static int handle_data_set_new(const DDMP2_FRAME *const pframe)
{
    int retvalue = 0;
    us_data_frame_t *p = (us_data_frame_t *)pframe->frame.set.value.raw;
    size_t data_size = ddmp2_value_size(pframe);
    us_data_read_ack_t ack;
    uint32_t crc_calc;
    esp_err_t err = ESP_OK;

    memcpy((void *)((uint8_t *)l_us.storage + l_us.block_byte_cnt), p->data, data_size);

    // Update total received bytes
    l_us.size_cnt += data_size;

    // Update block received bytes
    l_us.block_byte_cnt += data_size;

    US_LOG(I, "Handled=%d, of total=%d", l_us.size_cnt, l_us.dl_size);

    l_us.block_crc = crc32_update(l_us.block_crc, p->data, data_size);
    US_LOG(I, "CRC for len=%x, crc=0x%x", ddmp2_value_size(pframe), l_us.block_crc);
    // Do crc calculation
    connector_us_dicm_calc_crc32(0, data_size, p->data);

    if ((l_us.block_byte_cnt == l_us.service_block_size) || (l_us.dl_size == l_us.size_cnt))
    {
        // Block complete. Should send ack back
        // Allocate DRAM pointer if not already done.
        static int16_t flash_size = MIN(CONNECTOR_US_DICM_STORAGE_SIZE, 4096);
        if (l_us.flash_pointer == NULL)
        {
            switch (flash_size)
            {
            case 4096:
                LOG(D, "Trying internal 4096");
                l_us.flash_pointer = heap_caps_malloc(4096, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
                if (l_us.flash_pointer != NULL)
                {
                    break;
                }
                /* fallthrough */
            case 2048:
                LOG(D, "Trying internal 2048");
                flash_size = 2048;
                l_us.flash_pointer = heap_caps_malloc(2048, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
                if (l_us.flash_pointer != NULL)
                {
                    break;
                }
                /* fallthrough */
            case 1024:
                flash_size = 1024;
                LOG(D, "Trying internal 1024");
                l_us.flash_pointer = heap_caps_malloc(1024, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
                if (l_us.flash_pointer == NULL)
                {
                    LOG(D, "trying SPIRAM 4096");
                    flash_size = 4096;
                    l_us.flash_pointer = heap_caps_malloc(4096, MALLOC_CAP_DEFAULT | MALLOC_CAP_SPIRAM);
                }
                ASSERT(l_us.flash_pointer != NULL);
                assert(l_us.flash_pointer != NULL);
                break;
            default:
                assert(NULL);
                break;
            }
        }
        // Write block to flash
        uint32_t bytes = 0;
        uint32_t total_written = 0;
        while (0 < l_us.block_byte_cnt)
        {
            // Copy to internal ram for performance
            bytes = MIN(flash_size, l_us.block_byte_cnt);
            l_us.block_byte_cnt -= bytes;
            memcpy((void *)l_us.flash_pointer, (const void *)((uint8_t *)l_us.storage + total_written), bytes);
            total_written += bytes;
            ZERO_CHECK(err = connector_us_dicm_ota_write(bytes, l_us.flash_pointer));
            if (err != ESP_OK)
            {
                ZERO_CHECK(connector_us_dicm_ota_abort());
                ack.result = (uint8_t)US_ACK_RESULT_ABORT;
                ack.crc = 0x11223344u;
                LOG(W, "Could not write. Bad ACK %d", US_ACK_RESULT_ABORT);
                TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                                          US0DATA | DDM2_PARAMETER_INSTANCE(l_us.instance),
                                                          &ack,
                                                          sizeof(ack),
                                                          connector_us_dicm.connector_id,
                                                          portMAX_DELAY));
                return 1;
            }
        }

        // Finalize block crc
        l_us.block_crc = crc32_finalize(l_us.block_crc);

        // Publish ack to manager
        ack.result = (uint8_t)US_ACK_RESULT_OK;
        ack.crc = l_us.block_crc;
        if (l_us.dl_size == l_us.size_cnt)
        {
            // Finalize crc calculation
            crc_calc = connector_us_dicm_calc_crc32(1, 0, NULL);
            // Special treatment at end up update
            if (crc_calc == l_us.dl_crc)
            {
                // End OTA
                US_LOG(I, "OTA success! Restart required. Final block crc=0x%x", l_us.block_crc);
                retvalue = 2;  // Wait for last confirm
            }
            else
            {
                LOG(E, "OTA failed! Wrong CRC. Calculated by FW: 0x%x, received from app: 0x%x", crc_calc, l_us.dl_crc);
                // Wrong CRC, abort OTA
                // Abort manager
                // Publish ack to manager
                ack.result = (uint8_t)US_ACK_RESULT_ABORT;
                ack.crc = crc_calc;
                // Abort ESP OTA
                ZERO_CHECK(connector_us_dicm_ota_abort());
                retvalue = 1;
            }
        }
        // Send
        TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                                  US0DATA | DDM2_PARAMETER_INSTANCE(l_us.instance),
                                                  &ack,
                                                  sizeof(ack),
                                                  connector_us_dicm.connector_id,
                                                  portMAX_DELAY));
        l_us.block_crc = crc32_init();
        // Reset block byte counter
        l_us.block_byte_cnt = 0;
    }
    return retvalue;
}

static void handle_subscribe(const DDMP2_FRAME *const pframe)
{
    switch (DDM2_PARAMETER_BASE_INSTANCE(pframe->frame.subscribe.parameter))
    {
    case US0LIST:
    {
        uint8_t size_of_fwids = 0;

        memset(fwid_list, 0, sizeof(fwid_list));

        for (int i = 0; i < (int)ELEMENTS(l_ota_files); i++)
        {
            size_of_fwids += strlen(l_ota_files[i].file_name) + strlen("_") + strlen(l_ota_files[i].get_version());
            if ((ELEMENTS(l_ota_files) > 1) && ((i + 1) < (int)(ELEMENTS(l_ota_files))))
            {
                // Add separation in case of more than 1 file.
                size_of_fwids += strlen(";");
            }
        }
        size_of_fwids++;  // +1 for null terminator

        if (size_of_fwids > sizeof(fwid_list))
        {
            LOG(E, "FW ID storage overflow (capacity: %d bytes, required: %d bytes).", sizeof(fwid_list), size_of_fwids);
            return;
        }

        for (int i = 0; i < (int)ELEMENTS(l_ota_files); i++)
        {
            // list of "<fwid>_<version>" strings separated with ";"
            strcat(fwid_list, l_ota_files[i].file_name);
            strcat(fwid_list, "_");
            strcat(fwid_list, l_ota_files[i].get_version());
            if ((ELEMENTS(l_ota_files) > 1) && ((i + 1) < (int)(ELEMENTS(l_ota_files))))
            {
                // Add separation in case of more than 1 file.
                strcat(fwid_list, ";");
            }
        }
        TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                                  US0LIST | DDM2_PARAMETER_INSTANCE(l_us.instance),
                                                  fwid_list,
                                                  strlen(fwid_list),
                                                  connector_us_dicm.connector_id,
                                                  portMAX_DELAY));
        break;
    }
    case US0STAT:
    {
        TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                                  US0STAT | DDM2_PARAMETER_INSTANCE(l_us.instance),
                                                  &service_status,
                                                  sizeof(service_status),
                                                  connector_us_dicm.connector_id,
                                                  portMAX_DELAY));
        break;
    }
    case US0DATA:
    case US0BTR:
    case US0RRQ:
        // Do nothing
        break;
    default:
    {
        LOG(W, "Subscription for 0x%x is not handled yet", pframe->frame.subscribe.parameter);
        break;
    }
    }
}

static void us_ddm_process_task(void *parameter)
{
    DDMP2_FRAME l_frame;
    DDMP2_FRAME *pframe;
    fsm_event_t m_event;
    size_t frame_size;
    for (;;)
    {
        frame_size = 0;
        TRUE_CHECK(pframe = xRingbufferReceive(connector_us_dicm.to_connector, &frame_size, portMAX_DELAY));
        if (frame_size != 0)
        {
            memcpy(&l_frame, pframe, frame_size);
        }
        vRingbufferReturnItem(connector_us_dicm.to_connector, pframe);
        pframe = &l_frame;
        m_event.p_data = (void *)pframe;
        m_event.id = get_event_type(pframe);
        if (m_event.id != FSM_UNDEFINED_EVENT)
        {
            fsm_state_dispatch(&my_fsm, &m_event);
        }
        else
        {
            switch (pframe->frame.control)
            {
            case DDMP2_CONTROL_PUBLISH:
            // fallthrough
            case DDMP2_CONTROL_SET:
            {
                break;
            }
            case DDMP2_CONTROL_SUBSCRIBE:
            {
                handle_subscribe(pframe);
                break;
            }
            default:
            {
                LOG(E, "USM received UNHANDLED frame %02x from broker!", pframe->frame.control);
                break;
            }
            }
        }
    }
}

static int initialize_us_dicm(void)
{
    memset(&l_us, 0, sizeof(l_us));

    fsm_initialize(&my_fsm, (fsm_state_fcn_t)my_state_idle);

    l_us.instance = -1;
    l_us.storage_size = CONNECTOR_US_DICM_STORAGE_SIZE;
    l_us.storage = heap_caps_malloc_prefer(CONNECTOR_US_DICM_STORAGE_SIZE, 2, MALLOC_CAP_DEFAULT | MALLOC_CAP_SPIRAM, MALLOC_CAP_DEFAULT | MALLOC_CAP_INTERNAL);
    if (l_us.storage == NULL)
    {
        LOG(E, "Failed to allocate l_us.storage");
        return 0;
    }
    l_us.service_block_size = CONNECTOR_US_DICM_STORAGE_SIZE;

    // Register
    uint32_t device_class = US0;
    l_us.instance = broker_register_instance((uint32_t *const)&device_class, connector_us_dicm.connector_id);
    if (l_us.instance == -1)
    {
        LOG(E, "Failed to allocate instance!");
        return 0;
    }
    TRUE_CHECK(xTaskCreate(us_ddm_process_task, "us_dicm", CONNECTOR_US_DICM_TASK_STACK_SIZE, NULL, xTASK_PRIORITY_ABOVE_NORMAL, NULL));

    // Subscribe to inventory
    TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                              GW0INV,
                                              NULL,
                                              0,
                                              connector_us_dicm.connector_id,
                                              portMAX_DELAY));

    LOG(I, "US DICM initialized");
    return 1;
}

// Connector structure
CONNECTOR connector_us_dicm = {
    .name = "US DICM connector",
    .initialize = initialize_us_dicm,
};
