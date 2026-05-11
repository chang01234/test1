/*****************************************************************************
 * \file       hal_types
 * \brief      Define common types for hal layer
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
#ifndef HAL_TYPES_H_
#define HAL_TYPES_H_

/*****************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*****************************************************************************
 * Public Definitions
 *****************************************************************************/
#define HAL_E_OK        (0)
#define HAL_E_CODE(x)   (0 - (x))

// Generic HAL errors
#define HAL_E_FAIL          HAL_E_CODE(1)   // Generic fail
#define HAL_E_BUSY          HAL_E_CODE(2)   // Device busy
#define HAL_E_EMPTY         HAL_E_CODE(3)   // Queue is empty / data not available
#define HAL_E_FULL          HAL_E_CODE(4)   // Queue is full
#define HAL_E_DEVICE        HAL_E_CODE(5)   // Device not present
#define HAL_E_PARAM         HAL_E_CODE(6)   // Invalid parameter
#define HAL_E_COMM          HAL_E_CODE(7)   // Generic communication error
#define HAL_E_NOT_SET       HAL_E_CODE(8)   // Value/register not set
#define HAL_E_NOT_SUPPORTED HAL_E_CODE(9)   // Service not supported

/*****************************************************************************
 * Public Types
 *****************************************************************************/
typedef int hal_err_t;

//------------------------------------------------------------------------------
#ifndef NULL_PTR
    #define NULL_PTR   ((void*)0)
#endif

#ifndef FALSE
    #define FALSE           0
#endif

#ifndef TRUE
    #define TRUE            1
#endif

//------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#endif // HAL_TYPES_H_
