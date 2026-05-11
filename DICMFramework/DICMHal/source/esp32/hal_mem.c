/*****************************************************************************
 * \file       hal_mem.h
 * \brief      Memory Hardware Abstraction Layer
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
#include <stdint.h>
#include "hal_mem.h"
#include "esp_idf_version.h"
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include "esp_memory_utils.h"
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#include "soc/soc_memory_types.h"
#pragma GCC diagnostic pop
#endif

#include "esp_heap_caps.h"

void *hal_mem_malloc(size_t size, HAL_MEM_TYPE mem_type)
{
	uint32_t caps;

	switch (mem_type)
	{
	case HAL_MEM_INTERNAL_RAM:
		caps = MALLOC_CAP_DEFAULT | MALLOC_CAP_INTERNAL;
		break;
	case HAL_MEM_SPIRAM:
	default:
		caps = MALLOC_CAP_DEFAULT | MALLOC_CAP_SPIRAM;
		break;
	}

	return heap_caps_malloc(size, caps);
}

void *hal_mem_realloc(void *ptr, size_t size, HAL_MEM_TYPE mem_type)
{
    uint32_t caps;

    switch (mem_type)
    {
    case HAL_MEM_INTERNAL_RAM:
        caps = MALLOC_CAP_DEFAULT | MALLOC_CAP_INTERNAL;
        break;
    case HAL_MEM_SPIRAM:
    default:
        caps = MALLOC_CAP_DEFAULT | MALLOC_CAP_SPIRAM;
        break;
    }

    return heap_caps_realloc(ptr, size, caps);
}

void *hal_mem_malloc_prefer(size_t size, HAL_MEM_TYPE mem_type, HAL_MEM_TYPE mem_type2)
{
    uint32_t caps, caps2;
    switch (mem_type)
    {
    case HAL_MEM_INTERNAL_RAM:
        caps = MALLOC_CAP_DEFAULT | MALLOC_CAP_INTERNAL;
        break;
    case HAL_MEM_SPIRAM:
    default:
        caps = MALLOC_CAP_DEFAULT | MALLOC_CAP_SPIRAM;
        break;
    }
    switch (mem_type2)
    {
    case HAL_MEM_INTERNAL_RAM:
        caps2 = MALLOC_CAP_DEFAULT | MALLOC_CAP_INTERNAL;
        break;
    case HAL_MEM_SPIRAM:
    default:
        caps2 = MALLOC_CAP_DEFAULT | MALLOC_CAP_SPIRAM;
        break;
    }
    return heap_caps_malloc_prefer(size, 2, caps, caps2);
}

void *hal_mem_realloc_prefer(void *ptr, size_t size, HAL_MEM_TYPE mem_type, HAL_MEM_TYPE mem_type2)
{
    uint32_t caps, caps2;
    switch (mem_type)
    {
    case HAL_MEM_INTERNAL_RAM:
        caps = MALLOC_CAP_DEFAULT | MALLOC_CAP_INTERNAL;
        break;
    case HAL_MEM_SPIRAM:
    default:
        caps = MALLOC_CAP_DEFAULT | MALLOC_CAP_SPIRAM;
        break;
    }
    switch (mem_type2)
    {
    case HAL_MEM_INTERNAL_RAM:
        caps2 = MALLOC_CAP_DEFAULT | MALLOC_CAP_INTERNAL;
        break;
    case HAL_MEM_SPIRAM:
    default:
        caps2 = MALLOC_CAP_DEFAULT | MALLOC_CAP_SPIRAM;
        break;
    }
    return heap_caps_realloc_prefer(ptr, size, 2, caps, caps2);
}

bool hal_mem_is_flash_ptr(void *ptr)
{
	return esp_ptr_in_drom(ptr);
}

void hal_mem_free(void *ptr)
{
    heap_caps_free(ptr);
}
