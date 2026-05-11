/**
 * \file        ddm_store.c
 * \date        2021-10-08
 * \author      (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 * \author      (BB) Borjan Bozhinovski (ext-borjan.bozhinovski@dometic.com)
 * \brief       DDM store implementation
 *
 * Implementation of DDM store.
 *
 * \li          2021-10-08  (NR) Initial implementation
 * \li          2022-07-07  (NR) Add ddm_store__capacity
 * \li          2022-07-08  (NR) Add ddm_store__init_with_empty_entries
 * \li          2024-05-15  (BB) Handle parameters of type string with no initialized value
 * \li          2024-06-26  (NR) Added instance handling
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

#include <string.h>

#include "ddm2.h"
#include "ddm2_parameter_list.h"
#include "ddm_store.h"
#include "hal_mem.h"
#include "iGeneralDefinitions.h"

void ddm_store__init(struct ddm_store *ddm_store, SORTED_CONTAINER *containers)
{
    ddm_store->containers = containers;
}

void ddm_store__terminate(struct ddm_store *ddm_store)
{
    ddm_store__delete_all(ddm_store);
    /* Make the ddm_store structure invalid */
    ddm_store->containers = NULL;
}

struct ddm_store *ddm_store__create(size_t nbr_of_entries)
{
    struct ddm_store *ddm_store;
    SORTED_CONTAINER *sorted_container;

    sorted_container = sorted_container__create(sizeof(ddm_entry_t), nbr_of_entries);
    if (sorted_container == NULL)
    {
        return NULL;
    }
    ddm_store = hal_mem_malloc_prefer(sizeof(*ddm_store), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    if (ddm_store == NULL)
    {
        sorted_container__destroy(sorted_container);
        return NULL;
    }
    ddm_store__init(ddm_store, sorted_container);
    return ddm_store;
}

void ddm_store__destroy(struct ddm_store *ddm_store)
{
    SORTED_CONTAINER *sorted_container = ddm_store->containers;

    ddm_store__terminate(ddm_store);
    hal_mem_free(ddm_store);
    sorted_container__destroy(sorted_container);
}

int ddm_store__create_entries(
    struct ddm_store *ddm_store,
    const struct ddm_store_ddm *entries,
    size_t entries_size,
    uint32_t instance)
{
    int retval = 0;

    for (size_t i = 0u; i < entries_size; i++)
    {
        ddm_entry_t *ddm_entry;

        ddm_entry = ddm_store__new_entry(ddm_store, entries[i].ddm_parameter | DDM2_PARAMETER_INSTANCE(instance));
        if (ddm_entry == NULL)
        {
            retval = -1;
            break;
        }
    }
    return retval;
}

int ddm_store__load_entries(
    struct ddm_store *ddm_store,
    const struct ddm_store_ddm *entries,
    size_t entries_size,
    uint32_t instance)
{
    int retval = 0;

    for (size_t i = 0u; i < entries_size; i++)
    {
        ddm_entry_t *ddm_entry;
        bool has_changed;

        ddm_entry = ddm_store__new_entry(ddm_store, entries[i].ddm_parameter | DDM2_PARAMETER_INSTANCE(instance));
        if (ddm_entry != NULL)
        {
            has_changed = false;
            switch (entries[i].value.type)
            {
            case DDM2_TYPE_INT32_T:
                has_changed = ddm_entry__set__value_i32(ddm_entry, entries[i].value.storage.i32);
                break;
            case DDM2_TYPE_UINT32_T:
                has_changed = ddm_entry__set__value_u32(ddm_entry, entries[i].value.storage.u32);
                break;
            case DDM2_TYPE_STRING:
                if (entries[i].value.storage.str == NULL)
                {
                    has_changed = ddm_entry__set__value_str(ddm_entry, "", 0);
                }
                else
                {
                    has_changed = ddm_entry__set__value_str(
                        ddm_entry,
                        entries[i].value.storage.str,
                        strlen(entries[i].value.storage.str));
                }
                break;
            case DDM2_TYPE_STRUCT:
                has_changed = ddm_entry__set__value_struct(
                    ddm_entry,
                    entries[i].value.storage.structure,
                    entries[i].value.size);
                break;
            case DDM2_TYPE_OTHER:
                has_changed = ddm_entry__set__value_other(
                    ddm_entry,
                    entries[i].value.storage.other,
                    entries[i].value.size);
                break;

            case DDM2_TYPE_NONE:
            case DDM2_TYPE_JUMBO:
            case DDM2_TYPE_VOID:
            case DDM2_TYPE_COUNT:
                break;
            default:
                retval = -1;  // The type is not supported, return error
                break;
            }
            ddm_entry__set__has_changed_conditionally(ddm_entry, has_changed);
        }
        else
        {
            retval = -1;  // Unable to create new entry, return error
            break;
        }
    }
    return retval;
}

int ddm_store__copy_entries(struct ddm_store *ddm_store, const struct ddm_store *from_ddm_store)
{
    int retval = 0;
    size_t source_occupied = sorted_container__occupied(from_ddm_store->containers);
    for (size_t i = 0u; i < source_occupied; i++)
    {
        ddm_entry_t *source_ddm_entry;
        ddm_entry_t *destination_ddm_entry;
        uint32_t dummy_key;

        /* NOTE: (casting)
         * The function sorted_container__iterate was written as a generic helper
         * function. This module operates on concrete data type therefore it
         * need to operate on ddm_entry_t type. The casting is needed to accomodate
         * this.
         */
        sorted_container__iterate(
            /* sorted container */ from_ddm_store->containers,
            /* index */ i,
            /* data */ (void **)&source_ddm_entry,
            /* key */ &dummy_key);
        destination_ddm_entry = ddm_store__new_entry(ddm_store, ddm_entry__parameter_id(source_ddm_entry));
        if (destination_ddm_entry == NULL)
        {
            retval = -1;  // Unable to create new entry in DDM store, return error
            break;
        }
        ddm_entry__copy(destination_ddm_entry, source_ddm_entry);
    }
    return retval;
}

ddm_entry_t *ddm_store__new_entry(struct ddm_store *ddm_store, uint32_t ddm_parameter)
{
    ddm_entry_t *entry;

    entry = sorted_container__new(ddm_store->containers, ddm_parameter);
    if (entry != NULL)
    {
        int status;

        status = ddm_entry__init(entry, ddm_parameter);
        if (status < 0)
        {
            sorted_container__delete(ddm_store->containers, ddm_parameter);
            entry = NULL;
        }
    }
    return entry;
}

ddm_entry_t *ddm_store__access(struct ddm_store *ddm_store, uint32_t ddm_parameter)
{
    return sorted_container__access(ddm_store->containers, ddm_parameter);
}

void ddm_store__delete_entry(struct ddm_store *ddm_store, uint32_t ddm_parameter)
{
    ddm_entry_t *entry = ddm_store__access(ddm_store, ddm_parameter);
    if (entry != NULL)
    {
        ddm_entry__terminate(entry);
        sorted_container__delete(ddm_store->containers, ddm_parameter);
    }
}

void ddm_store__delete_all(ddm_store_t *ddm_store)
{
    size_t occupied = sorted_container__occupied(ddm_store->containers);
    for (size_t i = 0u; i < occupied; i++)
    {
        ddm_entry_t *ddm_entry;
        uint32_t dummy_key;

        sorted_container__iterate(
            ddm_store->containers,
            i,
            (void **)&ddm_entry,
            &dummy_key);
        ddm_entry__terminate(ddm_entry);
    }
    sorted_container__delete_all(ddm_store->containers);
}

void ddm_store__delete_by_class(ddm_store_t *ddm_store, uint32_t ddm_parameter_class)
{
    size_t i = 0;
    while (i < ddm_store__occupied(ddm_store))
    {
        ddm_entry_t *entry;
        uint32_t entry_parameter;

        ddm_store__iterate(ddm_store, i, &entry);
        entry_parameter = ddm_entry__parameter_id(entry);
        if (ddm_parameter_class == DDM2_PARAMETER_CLASS(entry_parameter))
        {
            ddm_store__delete_entry(ddm_store, entry_parameter);
        }
        else
        {
            // Proceed with next DDM entry only if the current one does not match the criteria
            i++;
        }
    }
}

void ddm_store__delete_by_class_instance(ddm_store_t *ddm_store, uint32_t ddm_parameter_class_instance)
{
	size_t i = 0;
	while (i < ddm_store__occupied(ddm_store))
	{
		ddm_entry_t * entry;
		uint32_t entry_parameter;

		ddm_store__iterate(ddm_store, i, &entry);
		entry_parameter = ddm_entry__parameter_id(entry);
		if (ddm_parameter_class_instance == DDM2_PARAMETER_CLASS_INSTANCE(entry_parameter))
		{
			ddm_store__delete_entry(ddm_store, entry_parameter);
		}
		else
		{
			// Proceed with next DDM entry only if the current one does not match the criteria
			i++;
		}
	}
}

size_t ddm_store__occupied(const struct ddm_store *ddm_store)
{
    return sorted_container__occupied(ddm_store->containers);
}

void ddm_store__iterate(
    struct ddm_store *ddm_store,
    uint32_t index,
    ddm_entry_t **entry)
{
    ASSERT(index < ddm_store__occupied(ddm_store));
    uint32_t key;

    /* NOTE: (casting)
     * The function sorted_container__iterate was written as a generic helper
     * function. This module operates on concrete data type therefeore it
     * accepts struct ddm_store__entry. The casting is needed to accomodate
     * this
     */
    sorted_container__iterate(
        /* sorted container */ ddm_store->containers,
        /* index */ index,
        /* data */ (void **)entry,
        /* key */ &key);
}

void ddm_store__iterate_const(
    const struct ddm_store *ddm_store,
    uint32_t index,
    ddm_entry_t **entry)
{
    ASSERT(index < ddm_store__occupied(ddm_store));
    uint32_t key;

    /* NOTE: (casting)
     * The function sorted_container__iterate was written as a generic helper
     * function. This module operates on concrete data type therefeore it
     * accepts struct ddm_store__entry. The casting is needed to accomodate
     * this
     */
    sorted_container__iterate(
        /* sorted container */ ddm_store->containers,
        /* index */ index,
        /* data */ (void **)entry,
        /* key */ &key);
}

void ddm_store__map(ddm_store_t *ddm_store, ddm_store__map_fn_t *map_fn, void *map_context)
{
    size_t occupied = ddm_store__occupied(ddm_store);

    for (size_t i = 0u; i < occupied; i++)
    {
        ddm_entry_t *entry;

        ddm_store__iterate(ddm_store, i, &entry);
        map_fn(entry, map_context);
    }
}

void ddm_store__filter_then_map(ddm_store_t *ddm_store, ddm_store__condition_fn_t *condition_fn, const void *condition_context, ddm_store__map_fn_t *map_fn, void *map_context)
{
    size_t occupied = ddm_store__occupied(ddm_store);

    for (size_t i = 0u; i < occupied; i++)
    {
        ddm_entry_t *entry;

        ddm_store__iterate(ddm_store, i, &entry);
        if (condition_fn(entry, condition_context))
        {
            map_fn(entry, map_context);
        }
    }
}

bool ddm_store__condition_by_parameter_any_instance(const ddm_entry_t *entry, const void *condition_context)
{
    uint32_t parameter = *(const uint32_t *)condition_context;
    if (DDM2_PARAMETER_BASE_INSTANCE(ddm_entry__parameter_id(entry)) == DDM2_PARAMETER_BASE_INSTANCE(parameter))
    {
        return true;
    }
    else
    {
        return false;
    }
}