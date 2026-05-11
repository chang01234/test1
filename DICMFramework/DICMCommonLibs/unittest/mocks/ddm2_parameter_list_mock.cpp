/**
 * \file        ddm2_parameter_list_mock.cpp
 * \date        2024-09-12
 * \author      (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 *
 * \brief       ddm2_parameter_list mock implementation
 *
 * Implementation of ddm2_parameter_list mocks.
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
#include "ddm2_parameter_list_mock.hpp"

/* Depends */
#include <cassert>

Iddm2_parameter_list * ddm2_parameter_list_mock_ptr;

int ddm2_parameter_list_lookup(const uint32_t parameter)
{
    assert(ddm2_parameter_list_mock_ptr);
    return ddm2_parameter_list_mock_ptr->ddm2_parameter_list_lookup(parameter);
}