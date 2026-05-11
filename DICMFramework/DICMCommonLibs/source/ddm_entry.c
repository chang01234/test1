/**
 * \file        ddm_entry.c
 * \date        2021-10-08
 * \author      (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 * \author      (AL) Andreas Lundeen (andreas.lundeen@dometic.com)
 * \author      (AD) Alvin Difuntorum (alvin.difuntorum@dometic.com)
 * \author      (BB) Borjan Bozhinovski (ext-borjan.bozhinovski@dometic.com)
 *
 * \brief       DDM entry implementation
 *
 * Implementation of DDM entry.
 *
 * \li          2021-10-08  (NR) Initial implementation
 * \li          2022-07-07  (NR) Added ddm_entry__value, ddm_entry__set__value
 * \li          2024-04-26  (AL) Re-worked to use hal_mem APIs
 * \li          2024-04-25  (AD) Add unittests for the DICMCommonLibs
 * \li          2024-05-15  (BB) Handle empty strings in ddm_entry/ddm_store library
 * \li          2024-06-26  (NR) Add instance handling
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

#include "ddm_entry.h"

#include "ddm2.h"
#include "hal_mem.h"
#include "iGeneralDefinitions.h"
#include <string.h>

static const int32_t *ddm_entry__value_ref_i32(const struct ddm_entry *entry)
{
    ASSERT((ddm_entry__out_type(entry) == DDM2_TYPE_INT32_T) ||
           (ddm_entry__in_type(entry) == DDM2_TYPE_INT32_T));
    return &entry->p__value.storage.i32;
}

static const uint32_t *ddm_entry__value_ref_u32(const struct ddm_entry *entry)
{
    ASSERT((ddm_entry__out_type(entry) == DDM2_TYPE_UINT32_T) ||
           (ddm_entry__in_type(entry) == DDM2_TYPE_UINT32_T))
    return &entry->p__value.storage.u32;
}

int ddm_entry__init(struct ddm_entry *entry, uint32_t ddm_parameter)
{
    int index;

    index = ddm2_parameter_list_lookup(ddm_parameter);
    if (index == -1)
    {
        return -1;
    }
    memset(entry, 0, sizeof(*entry));  // p__internal_flags, p__flags are cleared
    entry->p__value.size = 0u;
    entry->p__value.type = DDM2_TYPE_NONE;
    entry->p__index = (uint32_t)index;
    entry->p__instance = DDM2_PARAMETER_INSTANCE_FIELD(ddm_parameter);
    return 0;
}

int ddm_entry__init_copy(struct ddm_entry *entry, const struct ddm_entry *from)
{
    memset(entry, 0, sizeof(*entry));
    entry->p__index = from->p__index;
    entry->p__instance = from->p__instance;
    entry->p__internal_flags = from->p__internal_flags;
    entry->p__flags = from->p__flags;
    /* Prepare value by marking it as NONE type */
    entry->p__value.size = 0u;
    entry->p__value.type = DDM2_TYPE_NONE;
    /* Copy value data */
    (void)ddm_entry__copy__value(entry, from);
    return 0;
}

void ddm_entry__terminate(ddm_entry_t *entry)
{
    ddm_entry__delete__value(entry);
}

uint32_t ddm_entry__parameter_id(const struct ddm_entry *const entry)
{
    return Ddm2_parameter_list_data[entry->p__index].parameter | DDM2_PARAMETER_INSTANCE(entry->p__instance);
}

DDM2_TYPE_ENUM ddm_entry__out_type(const struct ddm_entry *const entry)
{
    return Ddm2_parameter_list_data[entry->p__index].out_type;
}

DDM2_UNIT_ENUM ddm_entry__out_unit(const struct ddm_entry *const entry)
{
    return Ddm2_parameter_list_data[entry->p__index].out_unit;
}

DDM2_TYPE_ENUM ddm_entry__in_type(const struct ddm_entry *const entry)
{
    return Ddm2_parameter_list_data[entry->p__index].in_type;
}

DDM2_UNIT_ENUM ddm_entry__in_unit(const struct ddm_entry *const entry)
{
    return Ddm2_parameter_list_data[entry->p__index].in_unit;
}

bool ddm_entry__is_on_cloud(const struct ddm_entry *const entry)
{
    return !!Ddm2_parameter_list_data[entry->p__index].cloud;
}

const char *ddm_entry__device_class(const struct ddm_entry *const entry)
{
    return Ddm2_parameter_list_data[entry->p__index].device_class;
}

const char *ddm_entry__property(const struct ddm_entry *const entry)
{
    return Ddm2_parameter_list_data[entry->p__index].property;
}

bool ddm_entry__delete__value(struct ddm_entry *entry)
{
    switch (entry->p__value.type)
    {
    case DDM2_TYPE_STRING:
        hal_mem_free(entry->p__value.storage.str);
        break;
    case DDM2_TYPE_OTHER:
        hal_mem_free(entry->p__value.storage.other);
        break;
    case DDM2_TYPE_STRUCT:
        hal_mem_free(entry->p__value.storage.structure);
        break;
    case DDM2_TYPE_NONE:
        return false;
    default:
        break;
    }
    memset(&entry->p__value.storage, 0, sizeof(entry->p__value.storage));
    entry->p__value.size = 0u;
    entry->p__value.type = DDM2_TYPE_NONE;
    entry->p__flags = 0u;
    entry->p__internal_flags = 0u;
    return true;
}

bool ddm_entry__set__value(ddm_entry_t *entry, const void *data, size_t data_size)
{
    bool has_changed;
    DDM2_TYPE_ENUM set_type = ddm_entry__out_type(entry);
    switch (set_type)
    {
    case DDM2_TYPE_INT32_T:
        TRUE_CHECK_RETURNX(false, data_size == sizeof(int32_t));
        has_changed = ddm_entry__set__value_i32(entry, *(int32_t *)data);
        break;
    case DDM2_TYPE_UINT32_T:
        TRUE_CHECK_RETURNX(false, data_size == sizeof(uint32_t));
        has_changed = ddm_entry__set__value_u32(entry, *(uint32_t *)data);
        break;
    case DDM2_TYPE_STRING:
        has_changed = ddm_entry__set__value_str(entry, data, data_size);
        break;
    case DDM2_TYPE_NONE:
    case DDM2_TYPE_VOID:
        LOG(W,
            "setting NONE/VOID input type is forbidden: %s0%s(0x%x)",
            ddm_entry__device_class(entry), ddm_entry__property(entry),
            ddm_entry__parameter_id(entry));
        has_changed = false;
        break;
    case DDM2_TYPE_OTHER:
        has_changed = ddm_entry__set__value_other(entry, data, data_size);
        break;
    case DDM2_TYPE_STRUCT:
        has_changed = ddm_entry__set__value_struct(entry, data, data_size);
        break;
    case DDM2_TYPE_JUMBO:
    case DDM2_TYPE_COUNT:
    default:
        LOG(W,
            "setting JUMBO/COUNT input type is not supported: %s0%s(0x%x)",
            ddm_entry__device_class(entry), ddm_entry__property(entry),
            ddm_entry__parameter_id(entry));
        has_changed = false;
        break;
    }
    return has_changed;
}

bool ddm_entry__set__value_i32(struct ddm_entry *entry, int32_t i32)
{
    bool has_changed = false;

    if (!((ddm_entry__out_type(entry) == DDM2_TYPE_INT32_T) ||
          (ddm_entry__in_type(entry) == DDM2_TYPE_INT32_T)))
    {
        LOG(W,
            "setting int32 for wrong DDM: %s0%s(0x%x) ",
            ddm_entry__device_class(entry), ddm_entry__property(entry),
            ddm_entry__parameter_id(entry));
        return false;
    }
    if (entry->p__value.type != DDM2_TYPE_INT32_T)
    {
        ddm_entry__delete__value(entry);
        entry->p__value.size = sizeof(entry->p__value.storage.i32);
        entry->p__value.type = DDM2_TYPE_INT32_T;
        entry->p__value.storage.i32 = 0;
        has_changed = true;
    }
    if (entry->p__value.storage.i32 != i32)
    {
        entry->p__value.storage.i32 = i32;
        has_changed = true;
    }
    ddm_entry__set__has_value(entry, true);
    return has_changed;
}

bool ddm_entry__set__value_u32(struct ddm_entry *entry, uint32_t u32)
{
    bool has_changed = false;

    if (!((ddm_entry__out_type(entry) == DDM2_TYPE_UINT32_T) ||
          (ddm_entry__in_type(entry) == DDM2_TYPE_UINT32_T)))
    {
        LOG(W,
            "setting uint32 for wrong DDM: %s0%s(0x%x) ",
            ddm_entry__device_class(entry), ddm_entry__property(entry),
            ddm_entry__parameter_id(entry));
        return false;
    }
    if (entry->p__value.type != DDM2_TYPE_UINT32_T)
    {
        ddm_entry__delete__value(entry);
        entry->p__value.size = sizeof(entry->p__value.storage.u32);
        entry->p__value.type = DDM2_TYPE_UINT32_T;
        entry->p__value.storage.u32 = 0;
        has_changed = true;
    }
    if (entry->p__value.storage.u32 != u32)
    {
        entry->p__value.storage.u32 = u32;
        has_changed = true;
    }
    ddm_entry__set__has_value(entry, true);
    return has_changed;
}

/*
 * |  new_str          |  p__value.str   |  p__value.type  |  Note
 * |-------------------|-----------------|-----------------|---------------------------------------------------------------------|
 * |  !NULL            |  !NULL          |  STRING         |  realloc (conditionally), has_changed (conditionally)
 * |  NULL             |  !NULL          |  STRING         |  free, has_changed
 * |  !NULL            |  NULL           |  STRING         |  malloc, has_changed
 * |  NULL             |  NULL           |  STRING         |
 * |  !NULL            |  !NULL          |  NULL           |  Not applicable/possible
 * |  NULL             |  !NULL          |  NULL           |  Not applicable/possible
 * |  !NULL            |  NULL           |  NULL           |  Set type to STRING, malloc, has_changed
 * |  NULL             |  NULL           |  NULL           |  Set type to STRING, has_changed
 */
bool ddm_entry__set__value_str(
    struct ddm_entry *entry,
    const char *new_str,
    size_t new_str_len)
{
    bool has_changed = false;

    TRUE_CHECK_RETURNX(false, (ddm_entry__out_type(entry) == DDM2_TYPE_STRING) ||
                                  (ddm_entry__in_type(entry) == DDM2_TYPE_STRING));

    /* Was the value inserted before? */
    if (entry->p__value.type == DDM2_TYPE_STRING)
    {
        /* |  !NULL            |  !NULL          |  STRING        |  realloc (conditionally), has_changed (conditionally) */
        if ((new_str != NULL) && (entry->p__value.storage.str != NULL))
        {
            if (new_str_len != entry->p__value.size)
            {
                /* New value is not the same size as old value storage:
                 * - reallocate old value storage and set pending has_changed
                 *
                 * NOTE: Add 1 space for null character since we are storing null terminated strings.
                 */
                entry->p__value.storage.str = hal_mem_realloc_prefer(
                    entry->p__value.storage.str,
                    new_str_len + 1,
                    HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
                TRUE_CHECK_RETURNX(false, entry->p__value.storage.str != NULL);
                has_changed = true;
            }
            else
            {
                /* new_str_len and p__value.size have the same value */
                has_changed = strncmp(entry->p__value.storage.str, new_str, new_str_len) != 0;
            }
        }
        /* |  NULL             |  !NULL          |  STRING        |  free, has_changed */
        else if ((new_str == NULL) && (entry->p__value.storage.str != NULL))
        {
            hal_mem_free(entry->p__value.storage.str);
            entry->p__value.storage.str = NULL;
            has_changed = true;
        }
        /* |  !NULL            |  NULL           |  STRING        |  malloc, has_changed */
        else if ((new_str != NULL) && (entry->p__value.storage.str == NULL))
        {
            /* NOTE: Add 1 space for null character since we are storing null terminated strings. */
            entry->p__value.storage.str = hal_mem_malloc_prefer(
                new_str_len + 1,
                HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
            TRUE_CHECK_RETURNX(false, entry->p__value.storage.str != NULL);
            has_changed = true;
        }
    }
    else
    {
        ddm_entry__delete__value(entry);
        /* The argument new_str can be NULL pointer. When this happens do not do any allocation. */
        if (new_str != NULL)
        {
            /* NOTE: Add 1 space for null character since we are storing null terminated strings. */
            entry->p__value.storage.str = hal_mem_malloc_prefer(
                new_str_len + 1,
                HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
            TRUE_CHECK_RETURNX(false, entry->p__value.storage.str != NULL);
        }
        /* ddm_entry storage was set to NULL by ddm_entry__delete__value function */
        entry->p__value.type = DDM2_TYPE_STRING;
        has_changed = true;
    }

    if ((has_changed) && (entry->p__value.storage.str != NULL))
    {
        /* There was a change detected, copy new value into ddm_entry. */
        if (new_str_len != 0)
        {
            strncpy(entry->p__value.storage.str, new_str, new_str_len);
        }
        entry->p__value.storage.str[new_str_len] = '\0';
    }
    entry->p__value.size = new_str_len;
    ddm_entry__set__has_value(entry, true);

    return has_changed;
}

/*
 * |  new_other_size  |  p__value.size  |  p__value.type  |  Note
 * |-------------------|-----------------|-----------------|---------------------------------------------------------------------|
 * |  !0               |  !0             |  OTHER          |  realloc (conditionally), has_changed (conditionally)
 * |  0                |  !0             |  OTHER          |  free, has_changed
 * |  !0               |  0              |  OTHER          |  malloc, has_changed
 * |  0                |  0              |  OTHER          |
 * |  !0               |  !0             |  NULL           |  Not applicable/possible
 * |  0                |  !0             |  NULL           |  Not applicable/possible
 * |  !0               |  0              |  NULL           |  Set type to OTHER, malloc, has_changed
 * |  0                |  0              |  NULL           |  Set type to OTHER, has_changed
 */
bool ddm_entry__set__value_other(
    struct ddm_entry *entry,
    const void *new_other,
    size_t new_other_size)
{
    bool has_changed = false;

    TRUE_CHECK_RETURNX(false, (ddm_entry__out_type(entry) == DDM2_TYPE_OTHER) ||
                                  (ddm_entry__in_type(entry) == DDM2_TYPE_OTHER));

    /* Was the value inserted before? */
    if (entry->p__value.type == DDM2_TYPE_OTHER)
    {
        /* |  !0               |  !0             |  OTHER         |  realloc (conditionally), has_changed (conditionally) */
        if ((new_other_size != 0) && (entry->p__value.size != 0))
        {
            if (new_other_size != entry->p__value.size)
            {
                /* New value is not the same size as old value storage:
                 * - reallocate old value storage and set pending has_changed
                 */
                entry->p__value.storage.other = hal_mem_realloc_prefer(
                    entry->p__value.storage.other,
                    new_other_size,
                    HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
                TRUE_CHECK_RETURNX(false, entry->p__value.storage.other != NULL);
                has_changed = true;
            }
            else
            {
                /* new_other_size and p__value.size have the same value */
                has_changed = memcmp(entry->p__value.storage.other, new_other, new_other_size) != 0;
            }
        }
        /* |  0                |  !0             |  OTHER         |  free, has_changed */
        else if ((new_other_size == 0) && (entry->p__value.size != 0))
        {
            hal_mem_free(entry->p__value.storage.other);
            entry->p__value.storage.other = NULL;
            has_changed = true;
        }
        /* |  !0               |  0              |  OTHER         |  malloc, has_changed */
        else if ((new_other_size != 0) && (entry->p__value.size == 0))
        {
            entry->p__value.storage.other = hal_mem_malloc_prefer(
                new_other_size,
                HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
            TRUE_CHECK_RETURNX(false, entry->p__value.storage.other != NULL);
            has_changed = true;
        }
    }
    else
    {
        ddm_entry__delete__value(entry);
        /* The argument new_other_size can be zero. When this happens do not do any allocation. */
        if (new_other_size != 0)
        {
            entry->p__value.storage.other = hal_mem_malloc_prefer(
                new_other_size,
                HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
            TRUE_CHECK_RETURNX(false, entry->p__value.storage.other != NULL);
        }
        /* entry storage was set to NULL by ddm_entry__delete__value function */
        entry->p__value.type = DDM2_TYPE_OTHER;
        has_changed = true;
    }

    if ((has_changed) && (new_other_size != 0))
    {
        /* There was a change detected, copy new value into entry. */
        memmove(entry->p__value.storage.other, new_other, new_other_size);
    }
    entry->p__value.size = new_other_size;
    ddm_entry__set__has_value(entry, true);

    return has_changed;
}

/*
 * |  new_struct_size  |  p__value.size  |  p__value.type  |  Note
 * |-------------------|-----------------|-----------------|---------------------------------------------------------------------|
 * |  !0               |  !0             |  STRUCT         |  realloc (conditionally), has_changed (conditionally)
 * |  0                |  !0             |  STRUCT         |  free, has_changed
 * |  !0               |  0              |  STRUCT         |  malloc, has_changed
 * |  0                |  0              |  STRUCT         |
 * |  !0               |  !0             |  NULL           |  Not applicable/possible
 * |  0                |  !0             |  NULL           |  Not applicable/possible
 * |  !0               |  0              |  NULL           |  Set type to STRUCT, malloc, has_changed
 * |  0                |  0              |  NULL           |  Set type to STRUCT, has_changed
 */
bool ddm_entry__set__value_struct(
    struct ddm_entry *entry,
    const void *new_struct,
    size_t new_struct_size)
{
    bool has_changed = false;

    TRUE_CHECK_RETURNX(false, (ddm_entry__out_type(entry) == DDM2_TYPE_STRUCT) ||
                                  (ddm_entry__in_type(entry) == DDM2_TYPE_STRUCT));

    /* Was the value inserted before? */
    if (entry->p__value.type == DDM2_TYPE_STRUCT)
    {
        /* |  !0               |  !0             |  STRUCT         |  realloc (conditionally), has_changed (conditionally) */
        if ((new_struct_size != 0) && (entry->p__value.size != 0))
        {
            if (new_struct_size != entry->p__value.size)
            {
                /* New value is not the same size as old value storage:
                 * - reallocate old value storage and set pending has_changed
                 */
                entry->p__value.storage.structure = hal_mem_realloc_prefer(
                    entry->p__value.storage.structure,
                    new_struct_size,
                    HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
                TRUE_CHECK_RETURNX(false, entry->p__value.storage.structure != NULL);
                has_changed = true;
            }
            else
            {
                /* new_struct_size and p__value.size have the same value */
                has_changed = memcmp(entry->p__value.storage.structure, new_struct, new_struct_size) != 0;
            }
        }
        /* |  0                |  !0             |  STRUCT         |  free, has_changed */
        else if ((new_struct_size == 0) && (entry->p__value.size != 0))
        {
            hal_mem_free(entry->p__value.storage.structure);
            entry->p__value.storage.structure = NULL;
            has_changed = true;
        }
        /* |  !0               |  0              |  STRUCT         |  malloc, has_changed */
        else if ((new_struct_size != 0) && (entry->p__value.size == 0))
        {
            entry->p__value.storage.structure = hal_mem_malloc_prefer(
                new_struct_size,
                HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
            TRUE_CHECK_RETURNX(false, entry->p__value.storage.structure != NULL);
            has_changed = true;
        }
    }
    else
    {
        ddm_entry__delete__value(entry);
        /* The argument new_struct_size can be zero. When this happens do not do any allocation. */
        if (new_struct_size != 0)
        {
            entry->p__value.storage.structure = hal_mem_malloc_prefer(
                new_struct_size,
                HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
            TRUE_CHECK_RETURNX(false, entry->p__value.storage.structure != NULL);
        }
        /* entry storage was set to NULL by ddm_entry__delete__value function */
        entry->p__value.type = DDM2_TYPE_STRUCT;
        has_changed = true;
    }

    if ((has_changed) && (new_struct_size != 0))
    {
        /* There was a change detected, copy new value into entry. */
        memmove(entry->p__value.storage.structure, new_struct, new_struct_size);
    }
    entry->p__value.size = new_struct_size;
    ddm_entry__set__has_value(entry, true);

    return has_changed;
}

void ddm_entry__value(const struct ddm_entry *entry, const void **data, size_t *data_size)
{
    DDM2_TYPE_ENUM set_type = ddm_entry__in_type(entry);

    switch (set_type)
    {
    case DDM2_TYPE_INT32_T:
        *(const int32_t **)data = ddm_entry__value_ref_i32(entry);
        *data_size = ddm_entry__value_size(entry);
        break;
    case DDM2_TYPE_UINT32_T:
        *(const uint32_t **)data = ddm_entry__value_ref_u32(entry);
        *data_size = ddm_entry__value_size(entry);
        break;
    case DDM2_TYPE_STRING:
    {
        const char *str;

        str = ddm_entry__value_str(entry);
        *(const char **)data = str;
        *data_size = ddm_entry__value_size(entry);
    }
    break;
    case DDM2_TYPE_OTHER:
        *data = ddm_entry__value_other(entry);
        *data_size = ddm_entry__value_size(entry);
        break;
    case DDM2_TYPE_STRUCT:
        *data = ddm_entry__value_struct(entry);
        *data_size = ddm_entry__value_size(entry);
        break;
    case DDM2_TYPE_NONE:
    case DDM2_TYPE_JUMBO:
    case DDM2_TYPE_VOID:
    case DDM2_TYPE_COUNT:
        LOG(W,
            "getting value from NONE/JUMBO/COUNT/VOID input type is not supported: %s0%s(0x%x)",
            ddm_entry__device_class(entry), ddm_entry__property(entry),
            ddm_entry__parameter_id(entry));
        *data_size = ddm_entry__value_size(entry);
        break;
    }
}

int32_t ddm_entry__value_i32(const struct ddm_entry *entry)
{
    return *ddm_entry__value_ref_i32(entry);
}

uint32_t ddm_entry__value_u32(const struct ddm_entry *entry)
{
    return *ddm_entry__value_ref_u32(entry);
}

const char *ddm_entry__value_str(const struct ddm_entry *entry)
{
    TRUE_CHECK_RETURNX(NULL, (ddm_entry__out_type(entry) == DDM2_TYPE_STRING) ||
                                 (ddm_entry__in_type(entry) == DDM2_TYPE_STRING));
    return entry->p__value.storage.str;
}

const void *ddm_entry__value_other(const struct ddm_entry *entry)
{
    TRUE_CHECK_RETURNX(NULL, (ddm_entry__out_type(entry) == DDM2_TYPE_OTHER) ||
                                 (ddm_entry__in_type(entry) == DDM2_TYPE_OTHER));
    return entry->p__value.storage.other;
}

const void *ddm_entry__value_struct(const struct ddm_entry *entry)
{
    TRUE_CHECK_RETURNX(NULL, (ddm_entry__out_type(entry) == DDM2_TYPE_STRUCT) ||
                                 (ddm_entry__in_type(entry) == DDM2_TYPE_STRUCT));
    return entry->p__value.storage.structure;
}

bool ddm_entry__write__value(ddm_entry_t *entry, const void *data, size_t data_size)
{
    bool has_changed;

    switch (ddm_entry__in_type(entry))
    {
    case DDM2_TYPE_INT32_T:
        TRUE_CHECK_RETURNX(false, data_size == sizeof(int32_t));
        has_changed = ddm_entry__set__value_i32(entry, *(int32_t *)data);
        break;
    case DDM2_TYPE_UINT32_T:
        TRUE_CHECK_RETURNX(false, data_size == sizeof(uint32_t));
        has_changed = ddm_entry__set__value_u32(entry, *(uint32_t *)data);
        break;
    case DDM2_TYPE_STRING:
        has_changed = ddm_entry__set__value_str(entry, data, data_size);
        break;
    case DDM2_TYPE_NONE:
    case DDM2_TYPE_VOID:
        LOG(W,
            "writing NONE/VOID input type is forbidden: %s0%s(0x%x)",
            ddm_entry__device_class(entry), ddm_entry__property(entry),
            ddm_entry__parameter_id(entry));
        has_changed = false;
        break;
    case DDM2_TYPE_OTHER:
        has_changed = ddm_entry__set__value_other(entry, data, data_size);
        break;
    case DDM2_TYPE_STRUCT:
        has_changed = ddm_entry__set__value_struct(entry, data, data_size);
        break;
    case DDM2_TYPE_JUMBO:
    case DDM2_TYPE_COUNT:
    default:
        LOG(W,
            "writing JUMBO/COUNT input type is not supported: %s0%s(0x%x)",
            ddm_entry__device_class(entry), ddm_entry__property(entry),
            ddm_entry__parameter_id(entry));
        has_changed = false;
        break;
    }
    return has_changed;
}

void ddm_entry__read__value(const struct ddm_entry *entry, const void **data, size_t *data_size)
{
    switch (ddm_entry__out_type(entry))
    {
    case DDM2_TYPE_INT32_T:
        *(const int32_t **)data = ddm_entry__value_ref_i32(entry);
        *data_size = ddm_entry__value_size(entry);
        break;
    case DDM2_TYPE_UINT32_T:
        *(const uint32_t **)data = ddm_entry__value_ref_u32(entry);
        *data_size = ddm_entry__value_size(entry);
        break;
    case DDM2_TYPE_STRING:
    {
        const char *str;

        str = ddm_entry__value_str(entry);
        *(const char **)data = str;
        *data_size = ddm_entry__value_size(entry);
    }
    break;
    case DDM2_TYPE_OTHER:
        *data = ddm_entry__value_other(entry);
        *data_size = ddm_entry__value_size(entry);
        break;
    case DDM2_TYPE_STRUCT:
        *data = ddm_entry__value_struct(entry);
        *data_size = ddm_entry__value_size(entry);
        break;
    case DDM2_TYPE_NONE:
        *data = NULL;
        *data_size = 0;
        break;
    case DDM2_TYPE_JUMBO:
    case DDM2_TYPE_VOID:
    case DDM2_TYPE_COUNT:
        *data_size = ddm_entry__value_size(entry);
        LOG(W, "read_value JUNBO/VOID/COUNT: data: %p, data_size: %u", *data, *data_size);
        break;
    }
}

bool ddm_entry__copy__value(struct ddm_entry *entry, const struct ddm_entry *from)
{
    bool has_changed;

    switch (from->p__value.type)
    {
    case DDM2_TYPE_INT32_T:
        has_changed = ddm_entry__set__value_i32(entry, ddm_entry__value_i32(from));
        break;
    case DDM2_TYPE_UINT32_T:
        has_changed = ddm_entry__set__value_u32(entry, ddm_entry__value_u32(from));
        break;
    case DDM2_TYPE_STRING:
    {
        const char *source;
        size_t source_size;

        source = ddm_entry__value_str(from);
        source_size = ddm_entry__value_size(from);
        has_changed = ddm_entry__set__value_str(entry, source, source_size);
        break;
    }
    case DDM2_TYPE_OTHER:
    {
        const void *source;
        size_t source_size;

        source = ddm_entry__value_other(from);
        source_size = ddm_entry__value_size(from);
        has_changed = ddm_entry__set__value_other(entry, source, source_size);
        break;
    }
    case DDM2_TYPE_STRUCT:
    {
        const void *source;
        size_t source_size;

        source = ddm_entry__value_struct(from);
        source_size = ddm_entry__value_size(from);
        has_changed = ddm_entry__set__value_struct(entry, source, source_size);
        break;
    }
    case DDM2_TYPE_NONE:
    case DDM2_TYPE_JUMBO:
    case DDM2_TYPE_VOID:
    case DDM2_TYPE_COUNT:
    default:
        has_changed = false;
        break;
    }
    return has_changed;
}

void ddm_entry__copy(struct ddm_entry *entry, const struct ddm_entry *from)
{
    /* Delete value if anything is stored */
    ddm_entry__delete__value(entry);
    /* Change type, parameter_id and all flags */
    entry->p__index = from->p__index;
    entry->p__instance = from->p__instance;
    entry->p__flags = from->p__flags;
    /* Copy the value */
    ddm_entry__copy__value(entry, from);
    entry->p__internal_flags = from->p__internal_flags;  // Apply flags from original entry after copying
}
