/*****************************************************************************
 * \file       hal_mem
 * \brief      Define memory functions for hal layer
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
#ifndef HAL_MEM_H_
#define HAL_MEM_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
 * Includes
 *****************************************************************************/
#include <stddef.h>
#include <stdbool.h>

/*****************************************************************************
 * Public Definitions
 *****************************************************************************/

/*****************************************************************************
 * Public Types
 *****************************************************************************/
typedef enum
{
	HAL_MEM_INTERNAL_RAM,
	HAL_MEM_SPIRAM,
} HAL_MEM_TYPE;

/*****************************************************************************
 * Public prototypes
 *****************************************************************************/
void *hal_mem_malloc(size_t size, HAL_MEM_TYPE mem_type);
void *hal_mem_realloc(void *ptr, size_t size, HAL_MEM_TYPE mem_type);
void *hal_mem_malloc_prefer(size_t size, HAL_MEM_TYPE mem_type, HAL_MEM_TYPE mem_type2);
void *hal_mem_realloc_prefer(void *ptr, size_t size, HAL_MEM_TYPE mem_type, HAL_MEM_TYPE mem_type2);
bool hal_mem_is_flash_ptr(void *ptr);
void hal_mem_free(void *ptr);

#ifdef __cplusplus
}
#endif

//----------------------------------------------------------------------------
#endif // HAL_MEM_H_
