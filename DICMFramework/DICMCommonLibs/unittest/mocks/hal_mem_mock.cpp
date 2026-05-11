/**
 * \file        hal_mem_mock.cpp
 * \date        2024-09-12
 * \author      (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 *
 * \brief       hal_mem mock implementation
 *
 * Implementation of hal_mem mocks.
 *
 * \li          2024-09-12  (NR) Initial implementation
 *
 * \copyright   Dometic Group
 *              This source file and the information contained in it are
 *              confidential and proprietary to Dometic Group
 *              The reproduction or disclosure, in whole or in part,
 *              to anyone outside of Dometic Group without the written
 *              approval of a Dometic Group officer under a Non-Disclosure
 *              Agreement is expressly prohibited.
 *
 *              All rights reserved
 */

/* Implements */
#include "hal_mem_mock.hpp"

/* Depends */
#include <cassert>

Ihal_mem * hal_mem_mock_ptr;

void hal_mem_free(void *memory)
{
    assert(hal_mem_mock_ptr);
    hal_mem_mock_ptr->hal_mem_free(memory);
}

void *hal_mem_malloc_prefer(size_t size, int opt1, int opt2)
{
    assert(hal_mem_mock_ptr);
    return hal_mem_mock_ptr->hal_mem_malloc_prefer(size, opt1, opt2);
}

void *hal_mem_realloc_prefer(void *mem, size_t size, int opt1, int opt2)
{
    assert(hal_mem_mock_ptr);
    return hal_mem_mock_ptr->hal_mem_realloc_prefer(mem, size, opt1, opt2);
}
