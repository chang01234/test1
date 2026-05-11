/**
 * \file        ddm_entry_mock.cpp
 * \date        2024-09-12
 * \author      (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 *
 * \brief       ddm_entry mock implementation
 *
 * Implementation of ddm_entry mocks.
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
#include "ddm_entry_mock.hpp"

/* Depends */
#include <cassert>

Iddm_entry *ddm_entry_mock_ptr;

int ddm_entry__init(struct ddm_entry *entry, uint32_t ddm_parameter)
{
    assert(ddm_entry_mock_ptr);
    return ddm_entry_mock_ptr->ddm_entry__init(entry, ddm_parameter);
}

int ddm_entry__init_copy(struct ddm_entry *entry, const struct ddm_entry *from)
{
    assert(ddm_entry_mock_ptr);
    return ddm_entry_mock_ptr->ddm_entry__init_copy(entry, from);
}

void ddm_entry__terminate(struct ddm_entry *entry)
{
    assert(ddm_entry_mock_ptr);
    ddm_entry_mock_ptr->ddm_entry__terminate(entry);
}

uint32_t ddm_entry__parameter_id(const struct ddm_entry *const entry)
{
    assert(ddm_entry_mock_ptr);
    return ddm_entry_mock_ptr->ddm_entry__parameter_id(entry);
}

DDM2_TYPE_ENUM ddm_entry__out_type(const struct ddm_entry *const entry)
{
    assert(ddm_entry_mock_ptr);
    return ddm_entry_mock_ptr->ddm_entry__out_type(entry);
}

DDM2_UNIT_ENUM ddm_entry__out_unit(const struct ddm_entry *const entry)
{
    assert(ddm_entry_mock_ptr);
    return ddm_entry_mock_ptr->ddm_entry__out_unit(entry);
}

DDM2_TYPE_ENUM ddm_entry__in_type(const struct ddm_entry *const entry)
{
    assert(ddm_entry_mock_ptr);
    return ddm_entry_mock_ptr->ddm_entry__in_type(entry);
}

DDM2_UNIT_ENUM ddm_entry__in_unit(const struct ddm_entry *const entry)
{
    assert(ddm_entry_mock_ptr);
    return ddm_entry_mock_ptr->ddm_entry__in_unit(entry);
}

bool ddm_entry__is_on_cloud(const struct ddm_entry *const entry)
{
    assert(ddm_entry_mock_ptr);
    return ddm_entry_mock_ptr->ddm_entry__is_on_cloud(entry);
}

const char *ddm_entry__device_class(const struct ddm_entry *const entry)
{
    assert(ddm_entry_mock_ptr);
    return ddm_entry_mock_ptr->ddm_entry__device_class(entry);
}

const char *ddm_entry__property(const struct ddm_entry *const entry)
{
    assert(ddm_entry_mock_ptr);
    return ddm_entry_mock_ptr->ddm_entry__property(entry);
}

bool ddm_entry__delete__value(struct ddm_entry *entry)
{
    assert(ddm_entry_mock_ptr);
    return ddm_entry_mock_ptr->ddm_entry__delete__value(entry);
}

bool ddm_entry__set__value(struct ddm_entry *entry, const void *data, size_t data_size)
{
    assert(ddm_entry_mock_ptr);
    return ddm_entry_mock_ptr->ddm_entry__set__value(entry, data, data_size);
}

bool ddm_entry__set__value_i32(struct ddm_entry *entry, int32_t i32)
{
    assert(ddm_entry_mock_ptr);
    return ddm_entry_mock_ptr->ddm_entry__set__value_i32(entry, i32);
}

bool ddm_entry__set__value_u32(struct ddm_entry *entry, uint32_t u32)
{
    assert(ddm_entry_mock_ptr);
    return ddm_entry_mock_ptr->ddm_entry__set__value_u32(entry, u32);
}

bool ddm_entry__set__value_str(
    struct ddm_entry *entry,
    const char *new_str,
    size_t new_str_len)
{
    assert(ddm_entry_mock_ptr);
    return ddm_entry_mock_ptr->ddm_entry__set__value_str(entry, new_str, new_str_len);
}

bool ddm_entry__set__value_other(
    struct ddm_entry *entry,
    const void *new_other,
    size_t new_other_size)
{
    assert(ddm_entry_mock_ptr);
    return ddm_entry_mock_ptr->ddm_entry__set__value_other(entry, new_other, new_other_size);
}

bool ddm_entry__set__value_struct(
    struct ddm_entry *entry,
    const void *new_struct,
    size_t new_struct_size)
{
    assert(ddm_entry_mock_ptr);
    return ddm_entry_mock_ptr->ddm_entry__set__value_struct(entry, new_struct, new_struct_size);
}

void ddm_entry__value(const struct ddm_entry *entry, const void **data, size_t *data_size)
{
    assert(ddm_entry_mock_ptr);
    ddm_entry_mock_ptr->ddm_entry__value(entry, data, data_size);
}

int32_t ddm_entry__value_i32(const struct ddm_entry *entry)
{
    assert(ddm_entry_mock_ptr);
    return ddm_entry_mock_ptr->ddm_entry__value_i32(entry);
}

uint32_t ddm_entry__value_u32(const struct ddm_entry *entry)
{
    assert(ddm_entry_mock_ptr);
    return ddm_entry_mock_ptr->ddm_entry__value_u32(entry);
}

const char *ddm_entry__value_str(const struct ddm_entry *entry)
{
    assert(ddm_entry_mock_ptr);
    return ddm_entry_mock_ptr->ddm_entry__value_str(entry);
}

const void *ddm_entry__value_other(const struct ddm_entry *entry)
{
    assert(ddm_entry_mock_ptr);
    return ddm_entry_mock_ptr->ddm_entry__value_other(entry);
}

const void *ddm_entry__value_struct(const struct ddm_entry *entry)
{
    assert(ddm_entry_mock_ptr);
    return ddm_entry_mock_ptr->ddm_entry__value_struct(entry);
}

bool ddm_entry__write__value(struct ddm_entry *entry, const void *data, size_t data_size)
{
    assert(ddm_entry_mock_ptr);
    return ddm_entry_mock_ptr->ddm_entry__write__value(entry, data, data_size);
}

void ddm_entry__read__value(const struct ddm_entry *entry, const void **data, size_t *data_size)
{
    assert(ddm_entry_mock_ptr);
    ddm_entry_mock_ptr->ddm_entry__read__value(entry, data, data_size);
}

bool ddm_entry__copy__value(struct ddm_entry *entry, const struct ddm_entry *from)
{
    assert(ddm_entry_mock_ptr);
    return ddm_entry_mock_ptr->ddm_entry__copy__value(entry, from);
}

void ddm_entry__copy(struct ddm_entry *entry, const struct ddm_entry *from)
{
    assert(ddm_entry_mock_ptr);
    ddm_entry_mock_ptr->ddm_entry__copy(entry, from);
}
