/**
 * \file        sorted_list64_mock.cpp
 * \date        2025-04-29
 * \author      (BB) Borjan Bozhinovski (borjan.bozhinovski@qinshift.com)
 *
 * \brief       sorted_list64 mock implementation
 *
 * Implementation of sorted_list64 mocks.
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

/* Implements */
#include "sorted_list64_mock.hpp"

/* Depends */
#include <cassert>

Isorted_list64 *sorted_list64_mock_ptr;

SORTED_LIST64 *sorted_list64_create(const size_t count)
{
    assert(sorted_list64_mock_ptr);
    return sorted_list64_mock_ptr->sorted_list64_create(count);
}

void sorted_list64_destroy(SORTED_LIST64 *sorted_list)
{
    assert(sorted_list64_mock_ptr);
    sorted_list64_mock_ptr->sorted_list64_destroy(sorted_list);
}

int sorted_list64_key_search(int *const position, const SORTED_LIST64 *const sorted_list, const SORTED_LIST64_KEY_TYPE key)
{
    assert(sorted_list64_mock_ptr);
    return sorted_list64_mock_ptr->sorted_list64_key_search(position, sorted_list, key);
}

SORTED_LIST64_RETURN_VALUE sorted_list64_remove_entry(SORTED_LIST64 *const sorted_list, const int position)
{
    assert(sorted_list64_mock_ptr);
    return sorted_list64_mock_ptr->sorted_list64_remove_entry(sorted_list, position);
}

SORTED_LIST64_RETURN_VALUE sorted_list64_clear(SORTED_LIST64 *const sortlist)
{
    assert(sorted_list64_mock_ptr);
    return sorted_list64_mock_ptr->sorted_list64_clear(sortlist);
}

SORTED_LIST64_RETURN_VALUE sorted_list64_get_keys(SORTED_LIST64_KEY_TYPE *const keys, int *key_count, SORTED_LIST64 *const sorted_list, const SORTED_LIST64_VALUE_TYPE value, const int remove)
{
    assert(sorted_list64_mock_ptr);
    return sorted_list64_mock_ptr->sorted_list64_get_keys(keys, key_count, sorted_list, value, remove);
}

SORTED_LIST64_RETURN_VALUE sorted_list64_remove_value(SORTED_LIST64 *const sorted_list, const SORTED_LIST64_VALUE_TYPE value)
{
    assert(sorted_list64_mock_ptr);
    return sorted_list64_mock_ptr->sorted_list64_remove_value(sorted_list, value);
}

SORTED_LIST64_RETURN_VALUE sorted_list64_unique_get(SORTED_LIST64_VALUE_TYPE *const value, SORTED_LIST64 *const sorted_list, const SORTED_LIST64_KEY_TYPE key, const int remove)
{
    assert(sorted_list64_mock_ptr);
    return sorted_list64_mock_ptr->sorted_list64_unique_get(value, sorted_list, key, remove);
}

SORTED_LIST64_RETURN_VALUE sorted_list64_unique_remove(SORTED_LIST64 *const sorted_list, const SORTED_LIST64_KEY_TYPE key)
{
    assert(sorted_list64_mock_ptr);
    return sorted_list64_mock_ptr->sorted_list64_unique_remove(sorted_list, key);
}

SORTED_LIST64_RETURN_VALUE sorted_list64_unique_add(SORTED_LIST64 *const sorted_list, const SORTED_LIST64_KEY_TYPE key, const SORTED_LIST64_VALUE_TYPE value)
{
    assert(sorted_list64_mock_ptr);
    return sorted_list64_mock_ptr->sorted_list64_unique_add(sorted_list, key, value);
}

SORTED_LIST64_RETURN_VALUE sorted_list64_single_get(SORTED_LIST64_VALUE_TYPE *const values, int *value_count, SORTED_LIST64 *const sorted_list, const SORTED_LIST64_KEY_TYPE key, const int remove)
{
    assert(sorted_list64_mock_ptr);
    return sorted_list64_mock_ptr->sorted_list64_single_get(values, value_count, sorted_list, key, remove);
}

SORTED_LIST64_RETURN_VALUE sorted_list64_single_remove(SORTED_LIST64 *const sorted_list, const SORTED_LIST64_KEY_TYPE key)
{
    assert(sorted_list64_mock_ptr);
    return sorted_list64_mock_ptr->sorted_list64_single_remove(sorted_list, key);
}

SORTED_LIST64_RETURN_VALUE sorted_list64_single_add(SORTED_LIST64 *const sorted_list, const SORTED_LIST64_KEY_TYPE key, const SORTED_LIST64_VALUE_TYPE value)
{
    assert(sorted_list64_mock_ptr);
    return sorted_list64_mock_ptr->sorted_list64_single_add(sorted_list, key, value);
}

SORTED_LIST64_RETURN_VALUE sorted_list64_multiple_get(SORTED_LIST64_VALUE_TYPE *const values, int *value_count, SORTED_LIST64 *const sorted_list, const SORTED_LIST64_KEY_TYPE key, const int remove)
{
    assert(sorted_list64_mock_ptr);
    return sorted_list64_mock_ptr->sorted_list64_multiple_get(values, value_count, sorted_list, key, remove);
}

SORTED_LIST64_RETURN_VALUE sorted_list64_multiple_remove(SORTED_LIST64 *const sorted_list, const SORTED_LIST64_KEY_TYPE key)
{
    assert(sorted_list64_mock_ptr);
    return sorted_list64_mock_ptr->sorted_list64_multiple_remove(sorted_list, key);
}

SORTED_LIST64_RETURN_VALUE sorted_list64_multiple_add(SORTED_LIST64 *const sorted_list, const SORTED_LIST64_KEY_TYPE key, const SORTED_LIST64_VALUE_TYPE value)
{
    assert(sorted_list64_mock_ptr);
    return sorted_list64_mock_ptr->sorted_list64_multiple_add(sorted_list, key, value);
}

SORTED_LIST64_RETURN_VALUE sorted_list64_multiple_get_count(int *const value_count, SORTED_LIST64 *const sorted_list, const SORTED_LIST64_KEY_TYPE key, const SORTED_LIST64_VALUE_TYPE value, const int remove)
{
        assert(sorted_list64_mock_ptr);
    return sorted_list64_mock_ptr->sorted_list64_multiple_get_count(value_count, sorted_list, key, value, remove);
}
