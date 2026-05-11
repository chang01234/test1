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

#include "esp_heap_caps.h"

#include <cstdint>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <limits>
#include <sstream>
#include <vector>

bool roMem(uintptr_t addr) {
    auto f = std::ifstream("/proc/self/maps");
    while (f) {
        uintptr_t start;
        uintptr_t stop;
        char c;
        if ((f >> std::hex >> start >> c >> stop >> c >> c) && start <= addr && addr < stop) {
        	//printf("start: %p end: %p char: %c result: %d\n", start, stop, c, c != 'w');
            return (bool)(c != 'w');
        }
    	//printf("start: %p end: %p char: %c\n", start, stop, c);
        f.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    printf("NO matching memory found!\n");
    return false;
}

template <typename T> bool roMem(T* c) {
    return roMem(reinterpret_cast<uintptr_t>(c));
}

extern "C" void *hal_mem_malloc(size_t size, HAL_MEM_TYPE mem_type)
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

extern "C" void *hal_mem_malloc_prefer(size_t size, HAL_MEM_TYPE mem_type, HAL_MEM_TYPE mem_type2)
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

extern "C" bool hal_mem_is_flash_ptr(void *ptr)
{
	//printf("Testing pointer: %p\n", ptr);
	return (bool)roMem(ptr);
}

extern "C" void hal_mem_free(void *ptr)
{
    heap_caps_free(ptr);
}
