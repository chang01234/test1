/**
 * \file        ddm_entry_mock.hpp
 * \date        2024-09-12
 * \author      (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 *
 * \brief       Mock of ddm_entry component.
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

#ifndef DDM_ENTRY_MOCK_HPP_
#define DDM_ENTRY_MOCK_HPP_

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "ddm_entry.h"

struct Iddm_entry;                      // Forward declaration
extern Iddm_entry *ddm_entry_mock_ptr;  // Mock context pointer

struct Iddm_entry
{
    Iddm_entry() { ddm_entry_mock_ptr = this; }
    virtual ~Iddm_entry() { ddm_entry_mock_ptr = nullptr; }
    virtual int ddm_entry__init(ddm_entry_t *, uint32_t) = 0;
    virtual int ddm_entry__init_copy(ddm_entry_t *, const ddm_entry_t *) = 0;
    virtual void ddm_entry__terminate(ddm_entry_t *) = 0;
    virtual uint32_t ddm_entry__parameter_id(const ddm_entry_t *const) = 0;
    virtual DDM2_TYPE_ENUM ddm_entry__out_type(const ddm_entry_t *const) = 0;
    virtual DDM2_UNIT_ENUM ddm_entry__out_unit(const ddm_entry_t *const) = 0;
    virtual DDM2_TYPE_ENUM ddm_entry__in_type(const ddm_entry_t *const) = 0;
    virtual DDM2_UNIT_ENUM ddm_entry__in_unit(const ddm_entry_t *const) = 0;
    virtual bool ddm_entry__is_on_cloud(const ddm_entry_t *const) = 0;
    virtual const char *ddm_entry__device_class(const ddm_entry_t *const) = 0;
    virtual const char *ddm_entry__property(const ddm_entry_t *const) = 0;
    virtual bool ddm_entry__delete__value(ddm_entry_t *) = 0;
    virtual bool ddm_entry__write__value(ddm_entry_t *, const void *, size_t) = 0;
    virtual void ddm_entry__read__value(const ddm_entry_t *, const void **, size_t *) = 0;
    virtual bool ddm_entry__copy__value(ddm_entry_t *, const ddm_entry_t *) = 0;
    virtual void ddm_entry__copy(ddm_entry_t *, const ddm_entry_t *) = 0;
    virtual bool ddm_entry__set__value(ddm_entry_t *, const void *, size_t) = 0;
    virtual bool ddm_entry__set__value_i32(ddm_entry_t *, int32_t) = 0;
    virtual bool ddm_entry__set__value_u32(ddm_entry_t *, uint32_t) = 0;
    virtual bool ddm_entry__set__value_str(ddm_entry_t *, const char *, size_t) = 0;
    virtual bool ddm_entry__set__value_other(ddm_entry_t *, const void *, size_t) = 0;
    virtual bool ddm_entry__set__value_struct(ddm_entry_t *, const void *, size_t) = 0;
    virtual void ddm_entry__value(const ddm_entry_t *, const void **, size_t *) = 0;
    virtual int32_t ddm_entry__value_i32(const ddm_entry_t *) = 0;
    virtual uint32_t ddm_entry__value_u32(const ddm_entry_t *) = 0;
    virtual const char *ddm_entry__value_str(const ddm_entry_t *) = 0;
    virtual const void *ddm_entry__value_other(const ddm_entry_t *) = 0;
    virtual const void *ddm_entry__value_struct(const ddm_entry_t *) = 0;
};

struct ddm_entry_mock : public Iddm_entry
{
    MOCK_METHOD(int, ddm_entry__init, (ddm_entry_t *, uint32_t));
    MOCK_METHOD(int, ddm_entry__init_copy, (ddm_entry_t *, const ddm_entry_t *));
    MOCK_METHOD(void, ddm_entry__terminate, (ddm_entry_t *));
    MOCK_METHOD(uint32_t, ddm_entry__parameter_id, (const ddm_entry_t *const));
    MOCK_METHOD(DDM2_TYPE_ENUM, ddm_entry__out_type, (const ddm_entry_t *const));
    MOCK_METHOD(DDM2_UNIT_ENUM, ddm_entry__out_unit, (const ddm_entry_t *const));
    MOCK_METHOD(DDM2_TYPE_ENUM, ddm_entry__in_type, (const ddm_entry_t *const));
    MOCK_METHOD(DDM2_UNIT_ENUM, ddm_entry__in_unit, (const ddm_entry_t *const));
    MOCK_METHOD(bool, ddm_entry__is_on_cloud, (const ddm_entry_t *const));
    MOCK_METHOD(const char *, ddm_entry__device_class, (const ddm_entry_t *const));
    MOCK_METHOD(const char *, ddm_entry__property, (const ddm_entry_t *const));
    MOCK_METHOD(bool, ddm_entry__delete__value, (ddm_entry_t *));
    MOCK_METHOD(bool, ddm_entry__write__value, (ddm_entry_t *, const void *, size_t));
    MOCK_METHOD(void, ddm_entry__read__value, (const ddm_entry_t *, const void **, size_t *));
    MOCK_METHOD(bool, ddm_entry__copy__value, (ddm_entry_t *, const ddm_entry_t *));
    MOCK_METHOD(void, ddm_entry__copy, (ddm_entry_t *, const ddm_entry_t *));
    MOCK_METHOD(bool, ddm_entry__set__value, (ddm_entry_t *, const void *, size_t));
    MOCK_METHOD(bool, ddm_entry__set__value_i32, (ddm_entry_t *, int32_t));
    MOCK_METHOD(bool, ddm_entry__set__value_u32, (ddm_entry_t *, uint32_t));
    MOCK_METHOD(bool, ddm_entry__set__value_str, (ddm_entry_t *, const char *, size_t));
    MOCK_METHOD(bool, ddm_entry__set__value_other, (ddm_entry_t *, const void *, size_t));
    MOCK_METHOD(bool, ddm_entry__set__value_struct, (ddm_entry_t *, const void *, size_t));
    MOCK_METHOD(void, ddm_entry__value, (const ddm_entry_t *, const void **, size_t *));
    MOCK_METHOD(int32_t, ddm_entry__value_i32, (const ddm_entry_t *));
    MOCK_METHOD(uint32_t, ddm_entry__value_u32, (const ddm_entry_t *));
    MOCK_METHOD(const char *, ddm_entry__value_str, (const ddm_entry_t *));
    MOCK_METHOD(const void *, ddm_entry__value_other, (const ddm_entry_t *));
    MOCK_METHOD(const void *, ddm_entry__value_struct, (const ddm_entry_t *));
};

#endif /* DDM_ENTRY_MOCK_HPP_ */
