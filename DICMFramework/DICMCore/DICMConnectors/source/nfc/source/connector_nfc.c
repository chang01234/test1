/*! \file   connector_nfc.c
    \author Felix Qin
    \brief  NFC (Help quickly pair Mobile Phone with BLE/WIFI modules).
            NFC Binding Configuration Protocol:
            https://onedometic.atlassian.net/wiki/spaces/GC/pages/3480911874/NFC
*/
#include <string.h>
#include "hal_i2c_master.h"
#include "driver/i2c.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "crc16.h"
#include "configuration.h"
#include "iGeneralDefinitions.h"
#include "nfc_ndef.h"
#include "lib_ndef.h"
#include "connector_nfc.h"

/**********************************************************
 * Macro definition
 *********************************************************/
#define RES_PASS                   0
#define RES_FAIL                   -1
typedef int32_t error_type;
#define NFC_EVENT_QUEUE_LENGTH     10
#define EEPROM_READ_DELAY_MS       pdMS_TO_TICKS(1000)
#define I2C_MASTER_NUM             I2C_NUM_0
#define I2C_SLAVE_ADDR             0x57  // 7'b1010_111
#define I2C_WRITE_INTERVAL_MS      10    // multiples of 10
#define NFC_DEV_INFO_LENGTH        72    // 16(name) + 18(sku) + 32(pnc) + 6(ble_mac)
#define EEPROM_16BIT_DEFAULT_VALUE 0x0000
#define BLE_BOND                   0x0001
#define WIFI_BOND                  0x0002
#define EEPROM_ADDR_LEN            2
#define EEPROM_WRITE_DATA_MAX_LEN  16
#define EEPROM_WRITE_BUFF_SIZE     18  // Max: address: 2 bytes + data: 16 bytes
#define IOS_UNIVERSAL_LINK         "dometic.com"
#define ANDROID_APP_NAME           "com.dometic.outdoor"  // Climate App
#define DYNAMIC_LOCK_PARAM_VALUE   0x3F //Dynamic area: 0x0040 to 0x0xBF(block 0x10 to 0x6F)
#define DYNAMIC_LOCK_PARAM_LENGTH  1

typedef struct device_info_fields
{
    uint8_t name[16];                     //!< \~ Ble boardcast name, use 10 bytes
    uint8_t sku[MAX_SKU_LENGTH + 1];      //!< \~ 18 bytes, string + NULL
    uint8_t pnc[ONBOARDING_STRING_SIZE];  //!< \~ 32 bytes
    uint8_t ble_mac[6];                   //!< \~ MAC address, 6 bytes
} device_info_fields_t;

typedef struct device_info_eeprom
{
    union device_info_eeprom_view
    {
        uint8_t bytes[NFC_DEV_INFO_LENGTH];  //!< Plain array view of raw frame data
        device_info_fields_t fields;         //!< Structured view with data fields raw frame data
    } view;                                  //!< Different views of the same data
} device_info_eeprom_t;

/*!
 * @brief FM11NT082C device structure
 */
typedef struct __fm11nt082c_dev
{
    /*! Device Id */
    uint8_t dev_id;
    /*! slave address */
    uint8_t slave_address;
} fm11nt082c_dev_t;

/*!
 * @brief NFC task main fsm
 */
typedef enum
{
    NFC_READ, /*!< read new data from NFC RFID*/
} nfc_event_type_t;

typedef struct
{
    nfc_event_type_t type; /*!< NFC event type */
} nfc_event_t;

/*!
 * @brief eeprom_data array index
 */
typedef enum
{
    NDEF = 0,         // NDEF records
    COMMAND,          // command
    SSID,             // wifi SSID
    PASSWORD,         // wifi Password
    DYNAMIC_LOCK,     // D-lock(dynamic locking) area parameter
    DATA_COUNT        // Total, used to calculate the array size
} EepromInfoIndex;

/*!
 * @brief Address and length of device information or command stored in EEPROM
 */
typedef struct
{
    uint8_t start_addr[EEPROM_ADDR_LEN];
    uint16_t data_len;
} eeprom_data_t;

/**********************************************************
 * Public variables
 *********************************************************/
static fm11nt082c_dev_t fm11nt082c_dev_inst;
static EXT_RAM_ATTR QueueHandle_t nfc_event_queue;           //!< \~ Handles to NFC event queue
static EXT_RAM_ATTR StaticQueue_t nfc_event_queue_instance;  //!< Queue structure instance
static EXT_RAM_ATTR uint8_t pucQueueStorage[NFC_EVENT_QUEUE_LENGTH * sizeof(QueueHandle_t)];
static TimerHandle_t nfc_irq_timer = NULL;
static device_info_eeprom_t nfc_device_info;
static EXT_RAM_ATTR uint8_t eeprom_read_buff[NDEF_RECORD_MAX_SIZE];
static EXT_RAM_ATTR uint8_t ndef_record_buffer [NDEF_RECORD_MAX_SIZE];

static const eeprom_data_t eeprom_data[DATA_COUNT] = {
    {{0x00, 0x10}, 432},   // NDEF records, 0x0010 to 0x01BF(432 bytes)
    {{0x01, 0xC0}, 4},     // command
    {{0x01, 0xC4}, 32},    // wifi SSID
    {{0x01, 0xE4}, 64},    // wifi Password
    {{0x03, 0xC8}, 1},     // NFC block:0xE2, D-lock(dynamic locking) area parameter.
};

/**
 * @brief NFC IRQ gpio/pin isr handler.
 * @param Parameter: Unused.
 * @return void.
 */
void nfc_irq_gpio_isr_handler(int device, int port, int pin)
{
    if (xTimerIsTimerActive(nfc_irq_timer) == pdTRUE)
    {
        TRUE_CHECK(xTimerReset(nfc_irq_timer, portMAX_DELAY));
    }
    else
    {
        TRUE_CHECK(xTimerStart(nfc_irq_timer, portMAX_DELAY));
    }
}

/**
 *  @brief NFC IRQ timer callback function.
 * @param xTimer: Unused.
 * @return void.
 */
static void nfc_irq_timer_cb(TimerHandle_t xTimer)
{
    nfc_event_t event;
    event.type = NFC_READ;
    xQueueSend(nfc_event_queue, &event, portMAX_DELAY);
}

/**
 * @brief Create NFC IRQ timer.
   Usually the app interacting with NFC will do one or more read and write operations,
   which will trigger multiple interrupt signals. We only need to deal with the last one.
 * @param Parameter: Unused.
 * @return void.
*/
static void create_nfc_irq_timer(void *Parameter)
{
    TRUE_CHECK(nfc_irq_timer = xTimerCreate("nfc_irq_timer", EEPROM_READ_DELAY_MS, pdFALSE, NULL, nfc_irq_timer_cb));  // once timer
}

/*! \brief Initialize FM11NT082C IC
 *  \param void
 *  \return void
 */
static void fm11nt082c_init(void)
{
    /* Set the device ID */
    fm11nt082c_dev_inst.dev_id = I2C_MASTER_NUM;
    /* Set the slave address */
    fm11nt082c_dev_inst.slave_address = I2C_SLAVE_ADDR;
}

/*! \brief  This function provides write functionality to the NFC IC FM11NT082C.
 *  \param data	    Data to write to the eeprom.
 *  \param len      Length of bytes to be written.
 *
 *  \return 0 if successful. Any other value otherwise.
 */
static error_type fm11nt082c_write(uint8_t *data, uint8_t len)
{
    error_type result = RES_FAIL;
    uint8_t retry_count = 0;

    if ((len > EEPROM_WRITE_BUFF_SIZE) || (len == 0))
    {
        result = RES_FAIL;  // Data to long or to less for transfer buffer or no data.
    }

    do
    {
        result = hal_i2c_master_acquire(fm11nt082c_dev_inst.dev_id);
        if (result == HAL_E_OK)
        {
            result = hal_i2c_master_write(fm11nt082c_dev_inst.dev_id, fm11nt082c_dev_inst.slave_address, data, len);
            hal_i2c_master_release(fm11nt082c_dev_inst.dev_id);
            vTaskDelay(I2C_WRITE_INTERVAL_MS / portTICK_PERIOD_MS);
        }
    } while ((++retry_count <= 5) && (result != RES_PASS));

    return result;
}

/*! \brief  This function provides read functionality to the NFC IC FM11NT082C.
 *  \param reg_addr	Eeprom address to read the value from.
 *  \param reg_data	pointer to the data which has been read from sensor.
 *  \param len      Length of bytes to be read.
 *
 *  \return 0 if successful. Any other value otherwise.
 */
static error_type fm11nt082c_read(const uint8_t *start_addr, uint8_t addr_len, uint8_t *const data, uint8_t len)
{
    error_type result = RES_FAIL;
    uint8_t retry_count = 0;

    do
    {
        result = hal_i2c_master_acquire(fm11nt082c_dev_inst.dev_id);
        if (result == HAL_E_OK)
        {
            result = hal_i2c_master_writeread(fm11nt082c_dev_inst.dev_id, fm11nt082c_dev_inst.slave_address, start_addr, addr_len, data, len);
            hal_i2c_master_release(fm11nt082c_dev_inst.dev_id);
        }
    } while ((++retry_count <= 5) && (result != RES_PASS));

    return result;
}

/**
 * @brief Write data to EEPROM using I2C.
 *
 * This function writes a specified number of bytes to an EEPROM device over the I2C bus.
 * The function supports writing data in chunks, as EEPROMs typically have a page size limit.
 * The address and data are passed as parameters, and the function ensures proper handling
 * of the I2C communication.
 *
 * @param[in] addr   Pointer to a 2-byte array specifying the EEPROM memory address to write to.
 *                   The address is typically represented as {high_byte, low_byte}.
 * @param[in] data   Pointer to the data buffer containing the bytes to be written to the EEPROM.
 * @param[in] length The number of bytes to write. The function handles writing in chunks
 *                   if the length exceeds the EEPROM's page size (e.g., 16 bytes).
 *
 * @note The function assumes that the I2C bus and EEPROM device are properly initialized
 *       before calling this function. Ensure that the `addr` and `data` pointers point to
 *       valid memory regions.
 *
 * @attention This function is placed in external RAM using the `EXT_RAM_ATTR` attribute.
 *            Ensure that the external RAM is properly configured and accessible.
 *
 * @return None
 */
static error_type write_to_eeprom(const uint8_t *addr, const uint8_t *data, uint8_t length)
{
    uint8_t write_buff[EEPROM_WRITE_BUFF_SIZE] = {0};
    uint16_t addr_16bit = 0;  // EEPROM user data area address:0x0010 - 0x0384
    uint8_t addr_buff[EEPROM_ADDR_LEN] = {0};
    uint8_t data_offset = 0;
    uint8_t remaining = 0;
    error_type result = RES_FAIL;

    if ((addr == NULL) || (data == NULL))
    {
        return RES_FAIL;
    }

    addr_16bit = (*addr << 8) | *(addr + 1);
    LOG(D, "addr_16bit: %04X", addr_16bit);

    for (uint8_t i = 0; i < length / EEPROM_WRITE_DATA_MAX_LEN; i++)
    {
        addr_buff[0] = (addr_16bit & 0xFF00) >> 8;
        addr_buff[1] = addr_16bit & 0x00FF;
        memcpy(write_buff, addr_buff, EEPROM_ADDR_LEN);  // The first two bytes are the EEPROM address
        memcpy(write_buff + EEPROM_ADDR_LEN, data + data_offset, EEPROM_WRITE_DATA_MAX_LEN);
        data_offset += EEPROM_WRITE_DATA_MAX_LEN;
        addr_16bit += EEPROM_WRITE_DATA_MAX_LEN;
        result = fm11nt082c_write(write_buff, EEPROM_WRITE_BUFF_SIZE);
        for (uint8_t j = 0; j < EEPROM_WRITE_BUFF_SIZE; j++)
        {
            LOG(D, "write i: %d %02X", i, write_buff[j]);
        }
    }

    remaining = length % EEPROM_WRITE_DATA_MAX_LEN;
    if (remaining > 0)
    {
        addr_buff[0] = (addr_16bit & 0xFF00) >> 8;
        addr_buff[1] = addr_16bit & 0x00FF;

        memcpy(write_buff, addr_buff, EEPROM_ADDR_LEN);  // The first two bytes are the EEPROM address
        memcpy(write_buff + EEPROM_ADDR_LEN, data + data_offset, remaining);
        result = fm11nt082c_write(write_buff, remaining + EEPROM_ADDR_LEN);
        for (uint8_t j = 0; j < remaining + 2; j++)
        {
            LOG(D, "**write %02X", write_buff[j]);
        }
    }

    return result;
}

/**
 * @brief This function init device info. Fill the nfc_device_info structure.
 * @param Parameter: void.
 * @return void.
 */
static void init_device_info(void)
{
    ZERO_CHECK(esp_read_mac(nfc_device_info.view.fields.ble_mac, ESP_MAC_BT));  // get ble MAC address
    memcpy(nfc_device_info.view.fields.name, device_information.default_name, strlen(device_information.default_name));
    memcpy(nfc_device_info.view.fields.sku, gw0sku, strlen((char *)gw0sku));
    memcpy(nfc_device_info.view.fields.pnc, gw0pnc, strlen((char *)gw0pnc));
}

/**
 * @brief This function store device info into NFC Chip eeprom and uses NDEF text record format.
 * @param Parameter: void.
 * @return void.
 */
static void store_dev_info_into_eeprom(void)
{
    error_type result = RES_FAIL;
    uint16_t ndef_record_len = 0;
    char text_string[NFC_DEV_INFO_LENGTH] = {0};

    sprintf(text_string, "\"name\":\"%s\",\"sku\":\"%s\",\"pnc\":\"%s\"", nfc_device_info.view.fields.name, nfc_device_info.view.fields.sku, nfc_device_info.view.fields.pnc);
    LOG(D, "%s", text_string);
    ndef_record_len += NDEF_HEAD_LENGTH;
    memset(ndef_record_buffer, 0x00, NDEF_RECORD_MAX_SIZE);
    uint16_t ndef_text_len = generate_text_ndef_record(text_string, &ndef_record_buffer[ndef_record_len]);
    ndef_record_len += ndef_text_len;

    uint16_t ndef_ble_len = generate_ble_oob_ndef_record(nfc_device_info.view.fields.ble_mac, &ndef_record_buffer[ndef_record_len]);
    ndef_record_len += ndef_ble_len;

    const char *uri = IOS_UNIVERSAL_LINK;
    uint16_t ndef_uri_len = generate_uri_ndef_record(uri, &ndef_record_buffer[ndef_record_len]);
    ndef_record_len += ndef_uri_len;

    const char *app_name = ANDROID_APP_NAME;
    uint16_t ndef_android_aar_len = generate_android_aar_ndef_record(app_name, &ndef_record_buffer[ndef_record_len]);
    ndef_record_len += ndef_android_aar_len;
    
    ndef_record_buffer[0] = NDEF_START;                          // start
    ndef_record_buffer[1] = ndef_record_len - NDEF_HEAD_LENGTH;  // message length
    ndef_record_buffer[ndef_record_len] = NDEF_END;              // end
    ndef_record_len++;

    memset(eeprom_read_buff, 0x00, NDEF_RECORD_MAX_SIZE);
    result = fm11nt082c_read(eeprom_data[NDEF].start_addr, EEPROM_ADDR_LEN, eeprom_read_buff, ndef_record_len);
    if (result != RES_PASS)
    {
        LOG(E, "fm11nt082c_read error: %d", result);
    }

    if (memcmp(eeprom_read_buff, ndef_record_buffer, ndef_record_len) == 0) //same data, don't need to write
    {
        LOG(D, "NFC NDEF records have been stored and don't need to be written this time.");
        return;
    }

    result = write_to_eeprom(eeprom_data[NDEF].start_addr, ndef_record_buffer, ndef_record_len);
    if (result != RES_PASS)
    {
        LOG(E, "write_to_eeprom error: %d", result);
    }

    return;
}

/**
 * @brief This function lock NDEF records data in eeprom. Static lock(default lock) and dynamic lock together.
 *        Static lock area(default lock): 0x0000 to 0x0057.
 *        Dynamic area: 0x0040 to 0x0xBF(block 0x10 to 0x6F).
 * @param Parameter: void.
 * @return void.
 */
static void lock_ndef_data_in_eeprom(void)
{
    error_type result = RES_FAIL;
    uint8_t write_lock_param_buff [DYNAMIC_LOCK_PARAM_LENGTH] = {DYNAMIC_LOCK_PARAM_VALUE};
    uint8_t read_lock_param_buff [DYNAMIC_LOCK_PARAM_LENGTH] = {0};

    result = fm11nt082c_read(eeprom_data[DYNAMIC_LOCK].start_addr, EEPROM_ADDR_LEN, read_lock_param_buff, DYNAMIC_LOCK_PARAM_LENGTH);
    if (result != RES_PASS)
    {
        LOG(E, "fm11nt082c_read error: %d", result);
    }

    if (memcmp(read_lock_param_buff, write_lock_param_buff, DYNAMIC_LOCK_PARAM_LENGTH) == 0) //same data, don't need to write
    {
        LOG(D, "Dynamic lock paramrter has been set and don't need to be written this time.");
        return;
    }

    result = write_to_eeprom(eeprom_data[DYNAMIC_LOCK].start_addr, write_lock_param_buff, DYNAMIC_LOCK_PARAM_LENGTH);
    if (result != RES_PASS)
    {
        LOG(E, "write_to_eeprom error: %d", result);
    }

    return;
}

/**
 * @brief This function is used to process the NFC event.
 * @param *Parameter: A pointer variable.
 * @return void.
 */
static void connector_nfc_process_task(void *Parameter)
{
    error_type result = RES_FAIL;
   	nfc_event_t event;
    uint16_t nfc_command = EEPROM_16BIT_DEFAULT_VALUE;
    uint16_t crc16_check_code = EEPROM_16BIT_DEFAULT_VALUE;

    init_device_info();
    store_dev_info_into_eeprom();
    lock_ndef_data_in_eeprom();

    while (1)
    {
        TRUE_CHECK(xQueueReceive(nfc_event_queue, (void *)&event, (TickType_t)portMAX_DELAY));
        switch (event.type)
        {
        case NFC_READ:
            LOG(D, "Receive the NFC read EEPROM data command!");
            memset(eeprom_read_buff, 0, eeprom_data[COMMAND].data_len);
            result = fm11nt082c_read(eeprom_data[COMMAND].start_addr, EEPROM_ADDR_LEN, eeprom_read_buff, eeprom_data[COMMAND].data_len);
            if (result != RES_PASS)
            {
                LOG(E, "fm11nt082c_read error: %d", result);
            }

            nfc_command = (eeprom_read_buff[0] << 8) | eeprom_read_buff[1];
            LOG(D, "read nfc_command: %04X", nfc_command);
            if (nfc_command != EEPROM_16BIT_DEFAULT_VALUE)
            {
                crc16_check_code = crc16_get(eeprom_read_buff, 2);
                if (crc16_check_code == ((eeprom_read_buff[2] << 8) | eeprom_read_buff[3]))
                {
                    switch (nfc_command)
                    {
                    case BLE_BOND:
                        int32_t pairing_on = BT0PAIR_IN_PAIRING_MODE_ON;
                        TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_SET, BT0PAIR, (void *) &pairing_on, sizeof(pairing_on), 0, portMAX_DELAY));
                        break;
                    case WIFI_BOND:
                        /* code */
                        break;
                    default:
                        break;
                    }
                }
                else
                {
                    LOG(E, "crc16 check code Error! CRC16: %04X - rec: %04X", crc16_check_code, (eeprom_read_buff[2] << 8) | eeprom_read_buff[3]);
                }
                //Clear: write to default value 0x00
                memset(eeprom_read_buff, 0x00, eeprom_data[COMMAND].data_len);
                result = write_to_eeprom(eeprom_data[COMMAND].start_addr, eeprom_read_buff, eeprom_data[COMMAND].data_len);
                if (result != RES_PASS)
                {
                    LOG(E, "write_to_eeprom error: %d", result);
                }

            }
            break;  // end case NFC_READ:
        default:
            LOG(W, "Receive NFC event.type is Error type!");
            break;
        }
    }
}

/**
 * @brief This function is used to create the NFC event queue  and the NFC main task.
 *
 * @param Parameter: void.
 * @return 1.
 */
static int connector_nfc_init(void)
{
    LOG(D, "Connector_nfc_init!");

    // Init FM11NT082C device
    fm11nt082c_init();

    nfc_event_queue = xQueueCreateStatic(NFC_EVENT_QUEUE_LENGTH, sizeof(nfc_event_t), pucQueueStorage, &nfc_event_queue_instance);
    if (nfc_event_queue == NULL)
    {
        LOG(E, "Create nfc_event_queue fail!");
    }

    create_nfc_irq_timer(NULL);
    TRUE_CHECK(xTaskCreate(connector_nfc_process_task, (char *)connector_nfc.name, 3072, NULL, xTASK_PRIORITY_NORMAL, NULL));

    return 1;
}

CONNECTOR connector_nfc =
{
    .name = "NFC connector",
    .initialize = connector_nfc_init,
};
