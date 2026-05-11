/**
 * \file        sorted_list64_mock.hpp
 * \date        2025-04-29
 * \author      (BB) Borjan Bozhinovski (borjan.bozhinovski@qinshift.com)
 *
 * \brief       Mock of sorted_list component.
 *
 * \li          2025-04-29  (BB) Initial implementation
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

#ifndef SORTED_LIST64_MOCK_HPP_
#define SORTED_LIST64_MOCK_HPP_

#include <gtest/gtest.h>
#include <gmock/gmock.h>

extern "C" {
#include "sorted_list64.h"
}

struct Isorted_list64;
extern Isorted_list64 *sorted_list64_mock_ptr;

struct Isorted_list64
{
    Isorted_list64() { sorted_list64_mock_ptr = this; }
    virtual ~Isorted_list64() { sorted_list64_mock_ptr = NULL; }
    virtual SORTED_LIST64 *sorted_list64_create(const size_t) = 0;
    virtual void sorted_list64_destroy(SORTED_LIST64 *) = 0;
    virtual int sorted_list64_key_search(int *const , const SORTED_LIST64 *const, const SORTED_LIST64_KEY_TYPE) = 0;
    virtual SORTED_LIST64_RETURN_VALUE sorted_list64_remove_entry(SORTED_LIST64 *const, const int) = 0;
    virtual SORTED_LIST64_RETURN_VALUE sorted_list64_clear(SORTED_LIST64 *const) = 0;
    virtual SORTED_LIST64_RETURN_VALUE sorted_list64_get_keys(SORTED_LIST64_KEY_TYPE *const, int *, SORTED_LIST64 *const, const SORTED_LIST64_VALUE_TYPE, const int) = 0;
    virtual SORTED_LIST64_RETURN_VALUE sorted_list64_remove_value(SORTED_LIST64 *const, const SORTED_LIST64_VALUE_TYPE) = 0;
    virtual SORTED_LIST64_RETURN_VALUE sorted_list64_unique_get(SORTED_LIST64_VALUE_TYPE *const, SORTED_LIST64 *const, const SORTED_LIST64_KEY_TYPE, const int) = 0;
    virtual SORTED_LIST64_RETURN_VALUE sorted_list64_unique_remove(SORTED_LIST64 *const, const SORTED_LIST64_KEY_TYPE) = 0;
    virtual SORTED_LIST64_RETURN_VALUE sorted_list64_unique_add(SORTED_LIST64 *const, const SORTED_LIST64_KEY_TYPE, const SORTED_LIST64_VALUE_TYPE) = 0;
    virtual SORTED_LIST64_RETURN_VALUE sorted_list64_single_get(SORTED_LIST64_VALUE_TYPE *const, int *, SORTED_LIST64 *const, const SORTED_LIST64_KEY_TYPE, const int) = 0;
    virtual SORTED_LIST64_RETURN_VALUE sorted_list64_single_remove(SORTED_LIST64 *const, const SORTED_LIST64_KEY_TYPE) = 0;
    virtual SORTED_LIST64_RETURN_VALUE sorted_list64_single_add(SORTED_LIST64 *const, const SORTED_LIST64_KEY_TYPE, const SORTED_LIST64_VALUE_TYPE) = 0;
    virtual SORTED_LIST64_RETURN_VALUE sorted_list64_multiple_get(SORTED_LIST64_VALUE_TYPE *const, int *, SORTED_LIST64 *const, const SORTED_LIST64_KEY_TYPE, const int) = 0;
    virtual SORTED_LIST64_RETURN_VALUE sorted_list64_multiple_remove(SORTED_LIST64 *const, const SORTED_LIST64_KEY_TYPE) = 0;
    virtual SORTED_LIST64_RETURN_VALUE sorted_list64_multiple_add(SORTED_LIST64 *const, const SORTED_LIST64_KEY_TYPE, const SORTED_LIST64_VALUE_TYPE) = 0;
    virtual SORTED_LIST64_RETURN_VALUE sorted_list64_multiple_get_count(int *const, SORTED_LIST64 *const, const SORTED_LIST64_KEY_TYPE, const SORTED_LIST64_VALUE_TYPE, const int) = 0;
};

struct sorted_list64_mock : public Isorted_list64
{
    MOCK_METHOD(SORTED_LIST64 *, sorted_list64_create, (const size_t));
    MOCK_METHOD(void, sorted_list64_destroy, (SORTED_LIST64 *));
    MOCK_METHOD(int, sorted_list64_key_search, (int *const, const SORTED_LIST64 *const, const SORTED_LIST64_KEY_TYPE));
    MOCK_METHOD(SORTED_LIST64_RETURN_VALUE, sorted_list64_remove_entry, (SORTED_LIST64 *const, const int));
    MOCK_METHOD(SORTED_LIST64_RETURN_VALUE, sorted_list64_clear, (SORTED_LIST64 *const));
    MOCK_METHOD(SORTED_LIST64_RETURN_VALUE, sorted_list64_get_keys, (SORTED_LIST64_KEY_TYPE *const, int *, SORTED_LIST64 *const, const SORTED_LIST64_VALUE_TYPE, const int));
    MOCK_METHOD(SORTED_LIST64_RETURN_VALUE, sorted_list64_remove_value, (SORTED_LIST64 *const, const SORTED_LIST64_VALUE_TYPE));
    MOCK_METHOD(SORTED_LIST64_RETURN_VALUE, sorted_list64_unique_get, (SORTED_LIST64_VALUE_TYPE *const, SORTED_LIST64 *const, const SORTED_LIST64_KEY_TYPE, const int));
    MOCK_METHOD(SORTED_LIST64_RETURN_VALUE, sorted_list64_unique_remove, (SORTED_LIST64 *const, const SORTED_LIST64_KEY_TYPE));
    MOCK_METHOD(SORTED_LIST64_RETURN_VALUE, sorted_list64_unique_add, (SORTED_LIST64 *const, const SORTED_LIST64_KEY_TYPE, const SORTED_LIST64_VALUE_TYPE));
    MOCK_METHOD(SORTED_LIST64_RETURN_VALUE, sorted_list64_single_get, (SORTED_LIST64_VALUE_TYPE *const, int *, SORTED_LIST64 *const, const SORTED_LIST64_KEY_TYPE, const int));
    MOCK_METHOD(SORTED_LIST64_RETURN_VALUE, sorted_list64_single_remove, (SORTED_LIST64 *const, const SORTED_LIST64_KEY_TYPE));
    MOCK_METHOD(SORTED_LIST64_RETURN_VALUE, sorted_list64_single_add, (SORTED_LIST64 *const, const SORTED_LIST64_KEY_TYPE, const SORTED_LIST64_VALUE_TYPE));
    MOCK_METHOD(SORTED_LIST64_RETURN_VALUE, sorted_list64_multiple_get, (SORTED_LIST64_VALUE_TYPE *const, int *, SORTED_LIST64 *const, const SORTED_LIST64_KEY_TYPE, const int));
    MOCK_METHOD(SORTED_LIST64_RETURN_VALUE, sorted_list64_multiple_remove, (SORTED_LIST64 *const, const SORTED_LIST64_KEY_TYPE));
    MOCK_METHOD(SORTED_LIST64_RETURN_VALUE, sorted_list64_multiple_add, (SORTED_LIST64 *const, const SORTED_LIST64_KEY_TYPE, const SORTED_LIST64_VALUE_TYPE value));
    MOCK_METHOD(SORTED_LIST64_RETURN_VALUE, sorted_list64_multiple_get_count, (int *const, SORTED_LIST64 *const, const SORTED_LIST64_KEY_TYPE, const SORTED_LIST64_VALUE_TYPE, const int));
};

#endif /* SORTED_LIST64_MOCK_HPP_ */