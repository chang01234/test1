/**
 * \file        sorted_list_mock.hpp
 * \date        2024-09-12
 * \author      (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 *
 * \brief       Mock of sorted_list component.
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

#ifndef SORTED_LIST_MOCK_HPP_
#define SORTED_LIST_MOCK_HPP_

#include <gtest/gtest.h>
#include <gmock/gmock.h>

extern "C" {
#include "sorted_list.h"
}

struct Isorted_list;
extern Isorted_list *sorted_list_mock_ptr;

struct Isorted_list
{
    Isorted_list() { sorted_list_mock_ptr = this; }
    virtual ~Isorted_list() { sorted_list_mock_ptr = NULL; }
    virtual SORTED_LIST *sorted_list_create(const size_t) = 0;
    virtual void sorted_list_destroy(SORTED_LIST *) = 0;
    virtual int sorted_list_key_search(int *const , const SORTED_LIST *const, const SORTED_LIST_KEY_TYPE) = 0;
    virtual SORTED_LIST_RETURN_VALUE sorted_list_remove_entry(SORTED_LIST *const, const int) = 0;
    virtual SORTED_LIST_RETURN_VALUE sorted_list_clear(SORTED_LIST *const) = 0;
    virtual SORTED_LIST_RETURN_VALUE sorted_list_get_keys(SORTED_LIST_KEY_TYPE *const, int *, SORTED_LIST *const, const SORTED_LIST_VALUE_TYPE, const int) = 0;
    virtual SORTED_LIST_RETURN_VALUE sorted_list_remove_value(SORTED_LIST *const, const SORTED_LIST_VALUE_TYPE) = 0;
    virtual SORTED_LIST_RETURN_VALUE sorted_list_unique_get(SORTED_LIST_VALUE_TYPE *const, SORTED_LIST *const, const SORTED_LIST_KEY_TYPE, const int) = 0;
    virtual SORTED_LIST_RETURN_VALUE sorted_list_unique_remove(SORTED_LIST *const, const SORTED_LIST_KEY_TYPE) = 0;
    virtual SORTED_LIST_RETURN_VALUE sorted_list_unique_add(SORTED_LIST *const, const SORTED_LIST_KEY_TYPE, const SORTED_LIST_VALUE_TYPE) = 0;
    virtual SORTED_LIST_RETURN_VALUE sorted_list_single_get(SORTED_LIST_VALUE_TYPE *const, int *, SORTED_LIST *const, const SORTED_LIST_KEY_TYPE, const int) = 0;
    virtual SORTED_LIST_RETURN_VALUE sorted_list_single_remove(SORTED_LIST *const, const SORTED_LIST_KEY_TYPE) = 0;
    virtual SORTED_LIST_RETURN_VALUE sorted_list_single_add(SORTED_LIST *const, const SORTED_LIST_KEY_TYPE, const SORTED_LIST_VALUE_TYPE) = 0;
    virtual SORTED_LIST_RETURN_VALUE sorted_list_multiple_get(SORTED_LIST_VALUE_TYPE *const, int *, SORTED_LIST *const, const SORTED_LIST_KEY_TYPE, const int) = 0;
    virtual SORTED_LIST_RETURN_VALUE sorted_list_multiple_remove(SORTED_LIST *const, const SORTED_LIST_KEY_TYPE) = 0;
    virtual SORTED_LIST_RETURN_VALUE sorted_list_multiple_add(SORTED_LIST *const, const SORTED_LIST_KEY_TYPE, const SORTED_LIST_VALUE_TYPE) = 0;
    virtual SORTED_LIST_RETURN_VALUE sorted_list_multiple_get_count(int *const, SORTED_LIST *const, const SORTED_LIST_KEY_TYPE, const SORTED_LIST_VALUE_TYPE, const int) = 0;
};

struct sorted_list_mock : public Isorted_list
{
    MOCK_METHOD(SORTED_LIST *, sorted_list_create, (const size_t));
    MOCK_METHOD(void, sorted_list_destroy, (SORTED_LIST *));
    MOCK_METHOD(int, sorted_list_key_search, (int *const, const SORTED_LIST *const, const SORTED_LIST_KEY_TYPE));
    MOCK_METHOD(SORTED_LIST_RETURN_VALUE, sorted_list_remove_entry, (SORTED_LIST *const, const int));
    MOCK_METHOD(SORTED_LIST_RETURN_VALUE, sorted_list_clear, (SORTED_LIST *const));
    MOCK_METHOD(SORTED_LIST_RETURN_VALUE, sorted_list_get_keys, (SORTED_LIST_KEY_TYPE *const, int *, SORTED_LIST *const, const SORTED_LIST_VALUE_TYPE, const int));
    MOCK_METHOD(SORTED_LIST_RETURN_VALUE, sorted_list_remove_value, (SORTED_LIST *const, const SORTED_LIST_VALUE_TYPE));
    MOCK_METHOD(SORTED_LIST_RETURN_VALUE, sorted_list_unique_get, (SORTED_LIST_VALUE_TYPE *const, SORTED_LIST *const, const SORTED_LIST_KEY_TYPE, const int));
    MOCK_METHOD(SORTED_LIST_RETURN_VALUE, sorted_list_unique_remove, (SORTED_LIST *const, const SORTED_LIST_KEY_TYPE));
    MOCK_METHOD(SORTED_LIST_RETURN_VALUE, sorted_list_unique_add, (SORTED_LIST *const, const SORTED_LIST_KEY_TYPE, const SORTED_LIST_VALUE_TYPE));
    MOCK_METHOD(SORTED_LIST_RETURN_VALUE, sorted_list_single_get, (SORTED_LIST_VALUE_TYPE *const, int *, SORTED_LIST *const, const SORTED_LIST_KEY_TYPE, const int));
    MOCK_METHOD(SORTED_LIST_RETURN_VALUE, sorted_list_single_remove, (SORTED_LIST *const, const SORTED_LIST_KEY_TYPE));
    MOCK_METHOD(SORTED_LIST_RETURN_VALUE, sorted_list_single_add, (SORTED_LIST *const, const SORTED_LIST_KEY_TYPE, const SORTED_LIST_VALUE_TYPE));
    MOCK_METHOD(SORTED_LIST_RETURN_VALUE, sorted_list_multiple_get, (SORTED_LIST_VALUE_TYPE *const, int *, SORTED_LIST *const, const SORTED_LIST_KEY_TYPE, const int));
    MOCK_METHOD(SORTED_LIST_RETURN_VALUE, sorted_list_multiple_remove, (SORTED_LIST *const, const SORTED_LIST_KEY_TYPE));
    MOCK_METHOD(SORTED_LIST_RETURN_VALUE, sorted_list_multiple_add, (SORTED_LIST *const, const SORTED_LIST_KEY_TYPE, const SORTED_LIST_VALUE_TYPE value));
    MOCK_METHOD(SORTED_LIST_RETURN_VALUE, sorted_list_multiple_get_count, (int *const, SORTED_LIST *const, const SORTED_LIST_KEY_TYPE, const SORTED_LIST_VALUE_TYPE, const int));
};

#endif /* SORTED_LIST_MOCK_HPP_ */