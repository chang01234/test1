/*****************************************************************************
 * \file       hal_cpu.h
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
#ifndef HAL_CPU_H_
#define HAL_CPU_H_

/*****************************************************************************
 * Includes
 *****************************************************************************/
#include "hal_types.h"

/*****************************************************************************
 * Public Definitions
 *****************************************************************************/
#define HALCPU_RESET_FLAG_NONE                0x00
#define HALCPU_RESET_FLAG_STAY_IN_BOOTLOADER  0x01
#define HALCPU_RESET_FLAG_FREEZE_OUTPUTS      0x02
#define HALCPU_RESET_FLAG_CHANGE_APPLICATION  0x04
#define HALCPU_RESET_FLAG_PARSE_XML           0x08

/*****************************************************************************
 * Public Types
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/
int64_t  hal_cpu_get_micros(void);
uint32_t hal_cpu_get_millis(void);
uint64_t hal_cpu_get_millis_u64(void);
void     hal_cpu_wait_us(uint32_t duration);
void     hal_cpu_enter_critical(void);
void     hal_cpu_leave_critical(void);
void     hal_cpu_reset(const uint8_t reason);
void     hal_cpu_refresh_watchdog(void);
void     hal_cpu_yield(void);

/*****************************************************************************/
#endif // HAL_CPU_H_
