/*! \file   connector_nmea2k.h
    \author Felix Qin
    \brief  Support the product series of Dometic Marine.
*/

#include "connector_nmea2k.h"
#include "HALCAN.h"
#include "configuration.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

/**********************************************************
 * Macro definition
 *********************************************************/
#define DEVICE_TWAI_RX GPIO_NUM_7  // CAN_R

/**********************************************************
 * Public variables
 *********************************************************/
TaskHandle_t can_txrx_task_handle = NULL;  //!< TX RX task handle for task notifications from ISR
static void rx_isr_handler(void *args);

/**********************************************************
 * Function definition
 *********************************************************/

/**
 * @brief This function is used to process the nmea2k event.
 * @param *Parameter: A pointer variable.
 * @return void.
 */
static void connector_nmea2k_process_task(void *Parameter)
{
    LOG(I, "Connector_nmea2k_process_task started!");

    while (1)
    {

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

static void IRAM_ATTR rx_isr_handler(void *args)
{
    (void)args;
    BaseType_t higher_priority_task_woken = pdFALSE;

    gpio_intr_disable(DEVICE_TWAI_RX);
#if defined(CAN_RS_PIN_NUM)
    CAN_RS(CAN_RS_HIGH_SPEED);
#endif
    /* Wakeup can_txrx_task. */
    if (can_txrx_task_handle != NULL)
    {
        vTaskNotifyGiveFromISR(can_txrx_task_handle, &higher_priority_task_woken);
    }

    if (higher_priority_task_woken == pdTRUE)
    {
        portYIELD_FROM_ISR();
    }
}

/**
 * @brief This function is used to create the nmea2k main task.
 *
 * @param Parameter: void.
 * @return 1.
 */
static int connector_nmea2k_init(void)
{
    LOG(D, "Connector_nmea2k_init!");

    if (HALCAN_Start(BSP_CAN_NMEA2K))
    {
        LOG(D, "CAN started");
    }
    else
    {
        LOG(E, "Failed to install CAN driver");
    }

    TRUE_CHECK(xTaskCreate(connector_nmea2k_process_task, (char *)connector_nmea2k.name, 4096, NULL, xTASK_PRIORITY_NORMAL, &can_txrx_task_handle));

    TRUE_CHECK(gpio_isr_handler_add(DEVICE_TWAI_RX, rx_isr_handler, NULL) == ESP_OK);
    TRUE_CHECK(gpio_set_intr_type(DEVICE_TWAI_RX, GPIO_INTR_NEGEDGE) == ESP_OK);
    TRUE_CHECK(gpio_intr_enable(DEVICE_TWAI_RX) == ESP_OK);

    return 1;
}

CONNECTOR connector_nmea2k = {
    .name = "NMEA2K connector",
    .initialize = connector_nmea2k_init,
};
