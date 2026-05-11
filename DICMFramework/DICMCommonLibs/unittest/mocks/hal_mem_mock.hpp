/**
 * \file        hal_mem_mock.hpp
 * \date        2024-09-12
 * \author      (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 *
 * \brief       Mock of hal_mem component.
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

#ifndef HAL_MEM_MOCK_HPP_
#define HAL_MEM_MOCK_HPP_

#include <gmock/gmock.h>
#include <gtest/gtest.h>

extern "C" {
#include "hal_mem.h"
}
using ::testing::Test;
struct Ihal_mem;                    // Forward declaration
extern Ihal_mem *hal_mem_mock_ptr;  // Mock context pointer

struct Ihal_mem
{
    Ihal_mem() { hal_mem_mock_ptr = this; }
    virtual ~Ihal_mem() { hal_mem_mock_ptr = NULL; }
    virtual void hal_mem_free(void *mem) = 0;
    virtual void *hal_mem_malloc_prefer(size_t, int, int) = 0;
    virtual void *hal_mem_realloc_prefer(void *, size_t, int, int) = 0;
};

struct hal_mem_mock : public Ihal_mem
{
    MOCK_METHOD(void, hal_mem_free, (void *));
    MOCK_METHOD(void *, hal_mem_malloc_prefer, (size_t, int, int));
    MOCK_METHOD(void *, hal_mem_realloc_prefer, (void *, size_t, int, int));
};

#endif /* HAL_MEM_MOCK_HPP_ */
