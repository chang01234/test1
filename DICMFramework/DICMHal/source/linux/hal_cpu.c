/*****************************************************************************
 * \file       hal_cpu.c
 * \brief      CPU Hardware Abstraction
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
#include "hal_cpu.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#include "freertos/FreeRTOS.h"
#pragma GCC diagnostic pop

#include "freertos/semphr.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"
#include "esp_system.h"

/*****************************************************************************
 * Private Functions
 *****************************************************************************/

/*****************************************************************************
 * Private variables
 *****************************************************************************/
static struct
{
    bool              init;
    StaticSemaphore_t storage;
    SemaphoreHandle_t handle;    
} critical;

/*****************************************************************************
 * \name   hal_cpu_get_micros
 * \brief  Return number of microseconds
 * \return Milliseconds (64 bits)
 *****************************************************************************/
int64_t hal_cpu_get_micros(void)
{
    return esp_timer_get_time();
}

/*****************************************************************************
 * \name   hal_cpu_get_millis
 * \brief  Return number of milliseconds
 * \return Milliseconds (32 bits)
 *****************************************************************************/
uint32_t hal_cpu_get_millis(void)
{
	int64_t micro;
	// Will use mocked version for now
    micro  = esp_timer_get_time();
    micro /= 1000;
    return (uint32_t)micro;
}

/*****************************************************************************
 * \name   hal_cpu_wait_us
 * \brief  Wait for specified microseconds
 *****************************************************************************/
void hal_cpu_wait_us(uint32_t duration)
{
    esp_rom_delay_us(duration);
}

/*****************************************************************************
 * \name   hal_cpu_enter_critical
 * \brief  Enter critical section
 *****************************************************************************/
void hal_cpu_enter_critical(void)
{
    // Auto-create
    if (critical.init == false)
    {
        critical.handle = xSemaphoreCreateRecursiveMutexStatic(&critical.storage);
        critical.init   = true;
    }

    // Acquire
    xSemaphoreTakeRecursive(critical.handle, portMAX_DELAY);
}

/*****************************************************************************
 * \name   hal_cpu_leave_critical
 * \brief  Leave critical section
 *****************************************************************************/
void hal_cpu_leave_critical(void)
{
    // Release
    xSemaphoreGiveRecursive(critical.handle);
}

/*****************************************************************************
 * \name   hal_cpu_reset
 * \brief  reset CPU
 *****************************************************************************/
void hal_cpu_reset(const uint8_t reason)
{
    (void)reason;
    esp_restart();
}

/*****************************************************************************
 * \name   hal_cpu_refresh_watchdog
 * \brief  refresh watchdog
 *****************************************************************************/
void hal_cpu_refresh_watchdog(void)
{
}

/*****************************************************************************
 * \name   hal_cpu_yield
 * \brief  yield to other core
 *****************************************************************************/
void hal_cpu_yield(void)
{
    taskYIELD( );
}
