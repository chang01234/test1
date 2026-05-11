/**
 * \file        ddm2_parameter_list_mock.hpp
 * \date        2024-09-12
 * \author      (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 *
 * \brief       Mock of ddm2_parameter_list component.
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

#ifndef DDM2_PARAMETER_LIST_MOCK_HPP_
#define DDM2_PARAMETER_LIST_MOCK_HPP_

#include <gmock/gmock.h>
#include <gtest/gtest.h>

extern "C" {
#include "ddm2_parameter_list.h"
}
using ::testing::Test;
struct Iddm2_parameter_list;
extern Iddm2_parameter_list *ddm2_parameter_list_mock_ptr;

struct Iddm2_parameter_list
{
    Iddm2_parameter_list() { ddm2_parameter_list_mock_ptr = this; }
    virtual ~Iddm2_parameter_list() { ddm2_parameter_list_mock_ptr = NULL; };
    virtual int ddm2_parameter_list_lookup(const uint32_t) = 0;
};

struct ddm2_parameter_list_mock : public Iddm2_parameter_list
{
    MOCK_METHOD(int, ddm2_parameter_list_lookup, (const uint32_t));
};

#endif /* DDM2_PARAMETER_LIST_MOCK_HPP_ */
