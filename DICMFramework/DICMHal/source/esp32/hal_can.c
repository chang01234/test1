/*****************************************************************************
 * \file       hal_can.h
 * \brief      CAN Hardware Abstraction
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

/*****************************************************************************
 * Includes
 *****************************************************************************/
#include <string.h>
#include "hal_can.h"
#include "dicm_framework_config.h"
#include "iGeneralDefinitions.h"
#ifdef DEVICE_TWAI
#include "driver/mytwai.h"
#else
#include "driver/twai.h"
#endif
#ifdef DEVICE_MCP2518FD
#include "drv_mcp2518fd.h"
#include "drv_mcp2518fd_api.h"
#endif
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#ifdef EXT_RAM_ATTR
#undef EXT_RAM_ATTR
#define EXT_RAM_ATTR EXT_RAM_BSS_ATTR
#endif
#endif

// Drivers
#define HAL_CAN_VERBOSE_LOG 0

/*****************************************************************************
 * Private Variables
 *****************************************************************************/
static HAL_CAN_STATUS hal_can_status[HAL_CAN_DEVICE_DIM];

/*****************************************************************************
 * Private defines
 *****************************************************************************/
#define MCP2518_FIFO_SIZE  64

/*****************************************************************************
 * Private Functions
 *****************************************************************************/
 #ifdef DEVICE_TWAI
static EXT_RAM_ATTR twai_general_config_t ConfigGeneral;
static hal_err_t hal_can_twai_init(HAL_CAN_BAUD baud);
static hal_err_t hal_can_twai_read( hal_can_msg_t* pMsg);
static hal_err_t hal_can_twai_write(hal_can_msg_t* pMsg);
#endif

#ifdef DEVICE_MCP2518FD
static struct
{
    bool            init;
    RingbufHandle_t fifo_read;
    RingbufHandle_t fifo_write;
} mcp2518fd = {0};
static hal_err_t hal_can_mcp2518fd_init(HAL_CAN_BAUD baud);
static hal_err_t hal_can_mcp2518fd_read( hal_can_msg_t* pMsg);
static hal_err_t hal_can_mcp2518fd_write(hal_can_msg_t* pMsg);
static void      hal_can_mcp2518fd_thread(void *pvParameter);
#endif

/*****************************************************************************
 * \name   hal_can_init
 * \brief  Initialize CAN devices
 * \return Error code (success = HAL_E_OK)
 *****************************************************************************/
hal_err_t hal_can_init(HAL_CAN_DEVICE device, HAL_CAN_BAUD baud)
{
	// Init
	hal_err_t Result = HAL_E_DEVICE;

 	// TWAI
    #ifdef DEVICE_TWAI
    if (device == HAL_CAN_DEVICE_TWAI)
    {
        Result = hal_can_twai_init(baud);
        hal_can_status[HAL_CAN_DEVICE_TWAI] =
          (Result == HAL_E_OK) ? HAL_CAN_SYNCHRONIZED : HAL_CAN_NOT_INITIALIZED;
    }
    #endif

    // MCP2518FD
    #ifdef DEVICE_MCP2518FD
    if (device == HAL_CAN_DEVICE_MCP2518)
    {    
        Result = hal_can_mcp2518fd_init(baud);
        if (mcp2518fd.init == false)
        {            
            // Initialize            
            mcp2518fd.fifo_read  = xRingbufferCreate(MCP2518_FIFO_SIZE*sizeof(hal_can_msg_t), RINGBUF_TYPE_NOSPLIT) ;
            mcp2518fd.fifo_write = xRingbufferCreate(MCP2518_FIFO_SIZE*sizeof(hal_can_msg_t), RINGBUF_TYPE_NOSPLIT);
            mcp2518fd.init       = true;

            // Start read/write thread
            TRUE_CHECK(xTaskCreate(hal_can_mcp2518fd_thread, "mcp2518fd", 3072, NULL, xTASK_PRIORITY_HIGH, NULL));
        }
        hal_can_status[HAL_CAN_DEVICE_MCP2518] =
          (Result == HAL_E_OK) ? HAL_CAN_SYNCHRONIZED : HAL_CAN_NOT_INITIALIZED;
    }
    #endif

	return Result;
}

/*****************************************************************************
 * \name   hal_can_deinit
 * \brief  Deinitilize CAN device
 * \return Error code (success = HAL_E_OK)
 *****************************************************************************/
hal_err_t hal_can_deinit(HAL_CAN_DEVICE device)
{
	// Init
	hal_err_t Result = HAL_E_DEVICE;

 	// TWAI
    #ifdef DEVICE_TWAI
    if (device == HAL_CAN_DEVICE_TWAI)
    {
        Result = twai_driver_uninstall();
    }
    #endif

    // MCP2518FD
    #ifdef DEVICE_MCP2518FD
    if (device == HAL_CAN_DEVICE_MCP2518)
    {    
        // Not supported
        Result = HAL_E_OK; 
    }
    #endif

	return Result;
}

/*****************************************************************************
 * \name   hal_can_recover
 * \brief  Recover from bus-off condition
 * \return Error code (success = HAL_E_OK)
 *****************************************************************************/
hal_err_t hal_can_recover(HAL_CAN_DEVICE device)
{
	// Init
	hal_err_t result = HAL_E_NOT_SUPPORTED;

    #ifdef DEVICE_TWAI        
    if (device == HAL_CAN_DEVICE_TWAI)
    {
        if (hal_can_status[HAL_CAN_DEVICE_TWAI] == HAL_CAN_ERROR_PASSIVE)
        {          
            LOG(I, "Initiating TWAI(CAN) recovery...");
            twai_start();
            twai_initiate_recovery();
            hal_can_status[HAL_CAN_DEVICE_TWAI] = HAL_CAN_INITIALIZED;
            result = HAL_E_OK;
        }
    }
    #endif
    return result;
}

/**
 * @brief Resumes driver (PM) and possible start
 *
 * @param device device to resume
 * @param bStart if TRUE also start driver
 * @return HAL_E_OK if successful
 */
hal_err_t hal_can_resume(HAL_CAN_DEVICE device, bool bStart)
{

    if (HAL_CAN_DEVICE_TWAI == device)
    {
        if (bStart)
        {
            twai_start();
        }
#ifdef DEVICE_TWAI
        twai_driver_resume();
#endif
    }
    return HAL_E_OK;
}

/*****************************************************************************
 * \name   hal_can_pause
 * \brief  Pause operation
 * \return Error code (success = HAL_E_OK)
 *****************************************************************************/
/**
 * @brief Pause driver and possibly also stop
 *
 * @param device device to resume
 * @param bStart if TRUE also start driver
 * @return HAL_E_OK if successful
 */
hal_err_t hal_can_pause(HAL_CAN_DEVICE device, bool bStop)
{
    if (HAL_CAN_DEVICE_TWAI == device)
    {
#ifdef DEVICE_TWAI
        twai_driver_pause();
#endif
        if (bStop)
        {
            twai_stop();
        }
    }
    return HAL_E_OK;
}
/**
 * @brief Starts the twai device
 *
 * @param device device to start
 * @return HAL_E_OK if successful
 */
hal_err_t hal_can_start(HAL_CAN_DEVICE device)
{
    if (HAL_CAN_DEVICE_TWAI == device)
    {
        twai_start();
    }
    return HAL_E_OK;
}

/**
 * @brief Stops the twai device
 *
 * @param device device to stop
 * @return HAL_E_OK if successful
 */
hal_err_t hal_can_stop(HAL_CAN_DEVICE device)
{
    if (HAL_CAN_DEVICE_TWAI == device)
    {
        twai_stop();
    }
    return HAL_E_OK;
}

/*****************************************************************************
 * \name   hal_can_get_stat
 * \brief  get CAN interface simple status
 * \return Error code (success = HAL_E_OK)
 *****************************************************************************/
HAL_CAN_STATUS hal_can_get_stat(HAL_CAN_DEVICE device)
{
    HAL_CAN_STATUS retval = HAL_CAN_NOT_INITIALIZED;
    if (device < HAL_CAN_DEVICE_DIM)
    {
        retval = hal_can_status[device];
    }
    return retval;
}

/*****************************************************************************
 * \name   hal_can_get_info
 * \brief
 * \return Error code (success = HAL_E_OK)
 *****************************************************************************/
hal_err_t hal_can_get_info( HAL_CAN_DEVICE device, hal_can_info_t* info)
{
    // Init
    hal_err_t Result = HAL_E_DEVICE;
    memset(info, 0, sizeof(hal_can_info_t));
    if (device < HAL_CAN_DEVICE_DIM)
    {
        info->eStatus = hal_can_status[device];
    }

 	// TWAI
    #ifdef DEVICE_TWAI
    if (device == HAL_CAN_DEVICE_TWAI)
    {
        twai_status_info_t status;
        Result = twai_get_status_info(&status);
        if (Result == ESP_OK)
        {
            info->queue.rx_used     = status.msgs_to_rx;
            info->queue.rx_capacity = ConfigGeneral.rx_queue_len;
            info->queue.rx_overflow = status.rx_missed_count;
            info->queue.tx_used     = status.msgs_to_tx;
            info->queue.tx_capacity = ConfigGeneral.tx_queue_len;            
            info->status.bus_off    = (status.state == TWAI_STATE_BUS_OFF);
        }
    }
    #endif

    // MCP2518FD
    #ifdef DEVICE_MCP2518FD
    if (device == HAL_CAN_DEVICE_MCP2518)
    {
        // todo
        info->queue.rx_used     = 0;
        info->queue.rx_capacity = MCP2518_FIFO_SIZE;
        info->queue.rx_overflow = 0;
        info->queue.tx_used     = 0;
        info->queue.tx_capacity = MCP2518_FIFO_SIZE;            
        info->status.bus_off    = false;
        Result = HAL_E_OK;
    }
    #endif

    return Result;
}

/*****************************************************************************
 * \name   hal_can_read
 * \brief  Read a single message
 * \return Error code (success = HAL_E_OK)
 *****************************************************************************/
hal_err_t hal_can_read( HAL_CAN_DEVICE device, hal_can_msg_t* msg)
{
	// Init
	hal_err_t result = HAL_E_DEVICE;
 	
     // TWAI
    #ifdef DEVICE_TWAI
    if (device == HAL_CAN_DEVICE_TWAI)
    {
        result = hal_can_twai_read(msg);
    }
    #endif

    // MCP2518FD
    #ifdef DEVICE_MCP2518FD
    if (device == HAL_CAN_DEVICE_MCP2518)
    {
        size_t size;
        void* item = xRingbufferReceive(mcp2518fd.fifo_read, &size, 0);
        if (item != NULL)
        {
            memcpy(msg, item, MIN(size, sizeof(hal_can_msg_t)));
            vRingbufferReturnItem(mcp2518fd.fifo_read, item);
            result = HAL_E_OK;
        }
        else
        {
            result = HAL_E_EMPTY;
        }
    }
    #endif
	return result;
}

/*****************************************************************************
 * \name   hal_can_write
 * \brief  Write a single message
 * \return Error code (success = HAL_E_OK)
 *****************************************************************************/
hal_err_t hal_can_write(HAL_CAN_DEVICE device, hal_can_msg_t* msg)
{
	// Init
	hal_err_t result = HAL_E_DEVICE;

    // TWAI
    #ifdef DEVICE_TWAI
    if (device == HAL_CAN_DEVICE_TWAI)
    {
        result = hal_can_twai_write(msg);
    }
    #endif

    // MCP2518FD
    #ifdef DEVICE_MCP2518FD
    if (device == HAL_CAN_DEVICE_MCP2518)
    {
        void *item;
        if (xRingbufferSendAcquire(mcp2518fd.fifo_write, &item, sizeof(hal_can_msg_t), 0) == pdTRUE)
        {
            memcpy(item, msg, sizeof(hal_can_msg_t));
            xRingbufferSendComplete(mcp2518fd.fifo_write, item);
            result = HAL_E_OK;
        }
        else
        {
            result = HAL_E_FULL;
        }
    }
    #endif
	return result;
}

/*****************************************************************************
 * \name   hal_can_twai_init
 * \brief  Initialize TWAI devices
 * \return Error code (success = HAL_E_OK)
 *****************************************************************************/
#ifdef DEVICE_TWAI
static hal_err_t hal_can_twai_init(HAL_CAN_BAUD baud)
{
    // Default configs       
    const twai_general_config_t ConfigDefault    = TWAI_GENERAL_CONFIG_DEFAULT(DEVICE_TWAI_TX, DEVICE_TWAI_RX, TWAI_MODE_NORMAL);  
    const twai_timing_config_t  ConfigTiming250  = TWAI_TIMING_CONFIG_250KBITS();
    const twai_timing_config_t  ConfigTiming500  = TWAI_TIMING_CONFIG_500KBITS();
    const twai_timing_config_t  ConfigTiming1000 = TWAI_TIMING_CONFIG_1MBITS();
    const twai_filter_config_t  ConfigFilter     = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    hal_err_t Result = HAL_E_OK;

    // General
    ConfigGeneral = ConfigDefault;  
    ConfigGeneral.tx_queue_len = 16;
    ConfigGeneral.rx_queue_len = 64;
    
    // Set baud rate
    twai_timing_config_t ConfigTiming;
    switch (baud)
    {
        case HAL_CAN_BAUD_250K: ConfigTiming = ConfigTiming250;  break;
        case HAL_CAN_BAUD_500K: ConfigTiming = ConfigTiming500;  break;
        case HAL_CAN_BAUD_1M:   ConfigTiming = ConfigTiming1000; break;
        default:                ConfigTiming = ConfigTiming250;  break;
    }

    // Start driver
    Result = twai_driver_install(&ConfigGeneral, &ConfigTiming, &ConfigFilter);
    if (Result == ESP_OK)
    {
        Result = twai_start(); 
    }

    // Enable
    DEVICE_TWAI_EN(1);

	return Result;
}
#endif

/*****************************************************************************
 * \name   hal_can_read_twai
 * \brief  Read message from twai
 *****************************************************************************/
#ifdef DEVICE_TWAI
static hal_err_t hal_can_twai_read(hal_can_msg_t* msg)
{
    hal_err_t Result = HAL_E_EMPTY;

	// Receive and do not block
    twai_message_t message;
    if (twai_receive(&message, (TickType_t)0) == ESP_OK)
    {
        msg->id         = message.identifier;
        msg->length     = message.data_length_code;
        memcpy(&msg->data[0], &message.data[0], 8);
        if (message.flags & TWAI_MSG_FLAG_EXTD)
        {
            // Message is in Extended Format
            msg->id_type = HAL_CAN_ID_EXT;
        }
        else
        {
            msg->id_type = HAL_CAN_ID_STD;
        }
        Result = HAL_E_OK;

        if (HAL_CAN_VERBOSE_LOG)
        {
            LOG(I, "RX: id=0x%08x", message.identifier);
        }
    }
    return Result;
}
#endif

/*****************************************************************************
 * \name   hal_can_twai_write
 * \brief  Read message to twai
 *****************************************************************************/
#ifdef DEVICE_TWAI
static hal_err_t hal_can_twai_write(hal_can_msg_t* msg)
{
    // Build message
    twai_message_t message = {0};
    message.identifier       = msg->id;
    message.data_length_code = msg->length;
    memcpy(&message.data[0], &msg->data[0], 8);
    if (msg->id_type == HAL_CAN_ID_EXT)
        message.flags |= TWAI_MSG_FLAG_EXTD;

    // Send
    esp_err_t result = twai_transmit(&message, pdMS_TO_TICKS(10));
    if (result != ESP_OK)
    {
        if (hal_can_status[HAL_CAN_DEVICE_TWAI] != HAL_CAN_ERROR_PASSIVE)
        {
            if (result == ESP_ERR_INVALID_STATE)
            {
                LOG(E, "twai_transmit invalid state");
                hal_can_status[HAL_CAN_DEVICE_TWAI] = HAL_CAN_ERROR_PASSIVE;
            }
            else
            {
                LOG(E, "twai_transmit failed (%d)", result);
            }
        }
    }
    else 
    {
        // Success
        if (HAL_CAN_VERBOSE_LOG)
        {
            LOG(I, "twai_transmit id=0x%08x OK", msg->id);
        }
        hal_can_status[HAL_CAN_DEVICE_TWAI] = HAL_CAN_SYNCHRONIZED;
    }
    return result;
}
#endif

/*****************************************************************************
 * \name   hal_can_mcp2518fd_init
 * \brief  Initialize MCP2518FD 
 * \return Error code (success = HAL_E_OK)
 *****************************************************************************/
#ifdef DEVICE_MCP2518FD
static hal_err_t hal_can_mcp2518fd_init(HAL_CAN_BAUD baud)
{
    hal_err_t result;
    CAN_TX_CONFIG txCfg = {.can_tx_fifo = CAN_FIFO_CH2, .payload_size = CAN_PLSIZE_8, .fifo_size = 31, .tx_priority = 1};
    CAN_RX_CONFIG rxCfg = {.can_rx_fifo = CAN_FIFO_CH1, .payload_size = CAN_PLSIZE_8, .fifo_size = 31};
    CAN_FILTER_MASK_CONFIG fltMaskConfig = CAN_FILTER_CONFIG_ACCEPT_ALL();
    CAN_TIMING_CONFIG timeCfg = {.sys_clk = CAN_SYSCLK_20M, .ssp_mode = CAN_SSP_MODE_AUTO, .bit_time = CAN_250K_4M};      // TBD: configure accroding to hello world project temporily: CAN_TIMING_CONFIG_250KBITS()    {.brp = 16, .tseg_1 = 15, .tseg_2 = 4, .sjw = 3, .triple_sampling = false}
    fltMaskConfig.filterId = CAN_FILTER0;        

    // Set baud rate
    switch (baud)
    {
        case HAL_CAN_BAUD_250K: timeCfg.bit_time = CAN_250K_4M;  break;
        case HAL_CAN_BAUD_500K: timeCfg.bit_time = CAN_500K_4M;  break;
        case HAL_CAN_BAUD_1M:   timeCfg.bit_time = CAN_1000K_4M; break;
    }

    // Init device
    result = hal_spi_master_acquire(HAL_SPI_HOST2);
    if (result == HAL_E_OK)
    {
        DRV_MCP2518FD_Init(spi3, txCfg, rxCfg, fltMaskConfig, timeCfg); 
        hal_spi_master_release(HAL_SPI_HOST2);

        DEVICE_MCP2518FD_EN(1);
    }
    return result;
}
#endif

/*****************************************************************************
 * \name   hal_can_mcp2518fd_read
 * \brief  Read message from MCP2518FD
 *****************************************************************************/
#ifdef DEVICE_MCP2518FD
static hal_err_t hal_can_mcp2518fd_read(hal_can_msg_t* msg)
{
    uint32_t u32Id = 0; 
    bool bExt = 0; 
    uint8_t u8Dlc = 0; 
    uint8_t dataBuff[MCP2518FD_MAX_DATA_BYTES] = {0}; 
    hal_err_t Result;

    Result = hal_spi_master_acquire(HAL_SPI_HOST2);
    if (Result == HAL_E_OK)
    {
        if (DRV_MCP2518FD_Read(&u32Id, &bExt, &u8Dlc, dataBuff) == ESP_OK)
        {
            msg->id = u32Id;
            msg->length = u8Dlc;
            memcpy(msg->data, dataBuff, msg->length);
            if (bExt)
            {
                msg->id_type = HAL_CAN_ID_EXT;
            }
            else
            {
                msg->id_type = HAL_CAN_ID_STD;
            }
    
            Result = HAL_E_OK;
        }
        else
        {
            Result = HAL_E_EMPTY;
        }
        hal_spi_master_release(HAL_SPI_HOST2);
    }
    return Result;
}
#endif

/*****************************************************************************
 * \name   hal_can_mcp2518fd_write
 * \brief  Write message to MCP2518FD
 *****************************************************************************/
#ifdef DEVICE_MCP2518FD
static hal_err_t hal_can_mcp2518fd_write(hal_can_msg_t* msg)
{
    bool bExt = 1; 
    hal_err_t Result;

    if (msg->id_type == HAL_CAN_ID_EXT)
    {
        bExt = 1;
    }
    else
    {
        bExt = 0;
    }

    Result = hal_spi_master_acquire(HAL_SPI_HOST2);
    if (Result == HAL_E_OK)
    {
        if (DRV_MCP2518FD_Write(msg->id, bExt, msg->length, msg->data) == ESP_OK)
        {
            Result = HAL_E_OK;
        }
        else
        {
            Result = HAL_E_FULL;
        }
        hal_spi_master_release(HAL_SPI_HOST2);

    }

    return Result;
}
#endif

/*****************************************************************************
 * \name   hal_can_mcp2518fd_thread
 * \brief  MCP2518FD thread
 *****************************************************************************/
#ifdef DEVICE_MCP2518FD
void hal_can_mcp2518fd_thread(void *pvParameter)
{
    hal_can_msg_t msg;
    void *item;
    size_t size;
    while (1)
    {
        // Process up to 32 transactions
        for (int i=0; i<32; i++)
        {
            // Something to write?
            item = xRingbufferReceive(mcp2518fd.fifo_write, &size, 0);
            if (item != NULL)
            {
                // Extract
                memcpy(&msg, item, MIN(size, sizeof(msg)));
                vRingbufferReturnItem(mcp2518fd.fifo_write, item);

                // Write
                hal_can_mcp2518fd_write(&msg);
            }       

            // Something to read?                 
            else if (hal_can_mcp2518fd_read(&msg) == ESP_OK)
            {                
                if (xRingbufferSendAcquire(mcp2518fd.fifo_read, &item, sizeof(hal_can_msg_t), 0) == pdTRUE)
                {
                    // Stuff
                    memcpy(item, &msg, sizeof(msg));
                    xRingbufferSendComplete(mcp2518fd.fifo_read, item);
                }
            }

            else
            {
                // Nothing to do... break and wait
                break;
            }
        }

        // Wait minimum time
        vTaskDelay(1);
    }
}
#endif

/*****************************************************************************
 * \name   hal_can_get_is_bus_off
 * \brief  Return buss off state
 *****************************************************************************/
bool hal_can_get_is_bus_off(HAL_CAN_DEVICE device)
{
    hal_can_info_t info = {0};
    hal_can_get_info(device, &info);
    return info.status.bus_off;
}

/*****************************************************************************
 * \name   hal_can_get_tx_free
 * \brief  Return tx queue free size
 *****************************************************************************/
uint16_t hal_can_get_tx_free(HAL_CAN_DEVICE device)
{
    hal_can_info_t info = {0};
    hal_can_get_info(device, &info);
    return info.queue.tx_capacity - info.queue.tx_used;
}
