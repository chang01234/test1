/*! \file sorted_list_ptr.c
\brief Sorted list implementation

    GENERATED FILE, DO NOT EDIT!
    Exported from "sorted_list.c.template", please see sorted_list.h.template for instructions
*/

#include "sorted_list_ptr.h"

#include <string.h>

#ifdef IDF_VER
#include "hal_mem.h"
#include "iGeneralDefinitions.h"
#else
#include <stdlib.h>
#define TRUE_CHECK_RETURNX(x, result)      \
    do                                     \
    {                                      \
        int errchk_result = (int)(result); \
        if (!errchk_result)                \
        {                                  \
            return (x);                    \
        }                                  \
    } while (0);  // verify that result is true, exit function with result `x` if not

#endif  // IDF_VER

#define ARGUMENT_CHECK(x) TRUE_CHECK_RETURNX(SORTED_LIST_PTR_INVALID_ARGUMENT, (x))

// STATIC

/*! \brief Compare an entry in a sorted list to a candidate entry
    \param search_entry Pointer to current entry in sorted list
    \param insert_key Candidate key to compare to list entry
    \param insert_value Candidate value to compare to list entry
    \retval SORTED_LIST_PTR_COMPARE_RESULT_GREATER Entry comparison result >
    \retval SORTED_LIST_PTR_COMPARE_RESULT_EQUAL Entry comparison result <=
    \retval SORTED_LIST_PTR_COMPARE_RESULT_LESS Entry comparison result <
 */
static SORTED_LIST_PTR_RETURN_VALUE sorted_list_ptr_compare_entries(const SORTED_LIST_PTR_ENTRY *const search_entry, const SORTED_LIST_PTR_KEY_TYPE insert_key, const SORTED_LIST_PTR_VALUE_TYPE insert_value)
{
    int equal, greater_than;

    equal = (search_entry->key == insert_key);

    if (equal)  // key is equal, value is tiebreaker
    {
        greater_than = search_entry->value > insert_value;
        equal = (search_entry->value == insert_value);
    }
    else
    {
        greater_than = search_entry->key > insert_key;
    }

    if (greater_than)
    {
        return SORTED_LIST_PTR_COMPARE_RESULT_GREATER;  // false in both cases
    }

    if (equal)
    {
        return SORTED_LIST_PTR_COMPARE_RESULT_EQUAL;  // true in rightmost case
    }

    return SORTED_LIST_PTR_COMPARE_RESULT_LESS;  // true in both cases
}

/*! \brief Search for a key/value pair in the sorted list
    \param[out] position First position where pair was found
    \param sorted_list Sorted list to search
    \param key Key to search for
    \param value Value to search for
    \retval false Key/value pair was not found in list, position contains where it should have been
    \retval true Key/value pair was found in list, position contains first match
 */
static int sorted_list_ptr_entry_search(int *const position, const SORTED_LIST_PTR *const sorted_list, const SORTED_LIST_PTR_KEY_TYPE key, const SORTED_LIST_PTR_VALUE_TYPE value)
{
    ARGUMENT_CHECK(sorted_list != NULL);

    int imin = 0;
    int imid;
    int imax = (int)sorted_list->entry_count;

    while (imin < imax)
    {
        imid = (imin + imax) >> 1;
        if (sorted_list_ptr_compare_entries(&sorted_list->pdata[imid], key, value) == SORTED_LIST_PTR_COMPARE_RESULT_LESS)  // leftmost search (<)
        {
            imin = imid + 1;
        }
        else
        {
            imax = imid;
        }
    }

    *position = imin;

    return (imin < sorted_list->entry_count) && (sorted_list->pdata[imin].key == key) && (sorted_list->pdata[imin].value == value);
}

/*! \brief Inserts a new entry into a sorted list
    \param sorted_list Sorted list to insert to
    \param key Key of entry to insert
    \param value Value of entry to insert
    \param position Position in list to insert to
    \retval SORTED_LIST_PTR_FAIL Sorted list was full
    \retval SORTED_LIST_PTR_ENTRY_INSERTED Entry was inserted
 */
static SORTED_LIST_PTR_RETURN_VALUE sorted_list_ptr_insert_entry(SORTED_LIST_PTR *const sorted_list, const SORTED_LIST_PTR_KEY_TYPE key, const SORTED_LIST_PTR_VALUE_TYPE value, const int position)
{
    ARGUMENT_CHECK(position <= sorted_list->entry_count);  // within range of current data?
    ARGUMENT_CHECK(position >= 0);

    if (sorted_list->entry_count >= sorted_list->capacity)  // full?
    {
        return SORTED_LIST_PTR_FAIL;
    }

    const size_t Move_size = sizeof(SORTED_LIST_PTR_ENTRY) * (sorted_list->entry_count - (size_t)position);

    memmove(&sorted_list->pdata[position + 1], &sorted_list->pdata[position], Move_size);

    sorted_list->pdata[position].key = key;
    sorted_list->pdata[position].value = value;
    sorted_list->entry_count++;

    return SORTED_LIST_PTR_ENTRY_INSERTED;
}

/*! \brief Overwrites an old entry in sorted list
    \param sorted_list Sorted list to modify
    \param key New key
    \param value New value
    \param position Position in list to overwrite
    \retval SORTED_LIST_PTR_ENTRY_UPDATED Entry was updated
    \retval SORTED_LIST_PTR_NO_CHANGE Entry already existed
 */
static SORTED_LIST_PTR_RETURN_VALUE sorted_list_ptr_overwrite_entry(const SORTED_LIST_PTR *const sorted_list, const SORTED_LIST_PTR_KEY_TYPE key, const SORTED_LIST_PTR_VALUE_TYPE value, const int position)
{
    ARGUMENT_CHECK(position >= 0);

    SORTED_LIST_PTR_VALUE_TYPE old_value = sorted_list->pdata[position].value;

    sorted_list->pdata[position].key = key;
    sorted_list->pdata[position].value = value;

    return (value == old_value) ? SORTED_LIST_PTR_NO_CHANGE : SORTED_LIST_PTR_ENTRY_UPDATED;
}

// COMMON

/*! \brief Create (allocate) sorted list with @a count number of elements
    \param count Number of entries to allocate in sorted list
    \retval pointer to allocated and initialized sorted list or NULL if allocation failed.
 */
SORTED_LIST_PTR *sorted_list_ptr_create(const size_t count)
{
    SORTED_LIST_PTR_ENTRY *sorted_list_entry;
    SORTED_LIST_PTR *sorted_list;
    size_t sorted_list_entry_size = sizeof(SORTED_LIST_PTR_ENTRY) * count;

#ifdef IDF_VER
    sorted_list_entry = hal_mem_malloc_prefer(sorted_list_entry_size, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
#else
    sorted_list_entry = malloc(sorted_list_entry_size);
#endif  // IDF_VER
    if (sorted_list_entry == NULL)
    {
        return NULL;
    }
#ifdef IDF_VER
    sorted_list = hal_mem_malloc_prefer(sizeof(SORTED_LIST_PTR), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
#else
    sorted_list = malloc(sizeof(SORTED_LIST_PTR));
#endif  // IDF_VER
    if (sorted_list == NULL)
    {
#ifdef IDF_VER
        hal_mem_free(sorted_list_entry);
#else
        free(sorted_list_entry);
#endif  // IDF_VER
        return NULL;
    }
    sorted_list->pdata = sorted_list_entry;
    sorted_list->capacity = (int)count;
    sorted_list->entry_count = 0;
    return sorted_list;
}

/*! \brief Destroy (free) sorted list
    \param sorted_list Sorted list which needs to be destroyed. The sorted list pointed
           by @a sorted_list must have been previously allocated with the create function.
 */
void sorted_list_ptr_destroy(SORTED_LIST_PTR *sorted_list)
{
    SORTED_LIST_PTR_ENTRY *sorted_list_entry = sorted_list->pdata;

    /* Set everything to 0/NULL, if anyone tries to use sorted_list after destroy, we
     * will get exception handler catching this inappropriate use of sorted list.
     */
    sorted_list->pdata = NULL;
    sorted_list->capacity = 0;
    sorted_list->entry_count = 0;
#ifdef IDF_VER
    hal_mem_free(sorted_list);
    hal_mem_free(sorted_list_entry);
#else
    free(sorted_list);
    free(sorted_list_entry);
#endif  // IDF_VER
}

/*! \brief Search for a key in the sorted list
    \param[out] position First position where key was found
    \param sorted_list Sorted list to search
    \param key Key to search for
    \retval false Key was not found in list, position contains where it should have been
    \retval true Key was found in list, position contains first match
 */
int sorted_list_ptr_key_search(int *const position, const SORTED_LIST_PTR *const sorted_list, const SORTED_LIST_PTR_KEY_TYPE key)
{
    ARGUMENT_CHECK(sorted_list != NULL);

    int imin = 0;
    int imid;
    int imax = (int)sorted_list->entry_count;

    while (imin < imax)
    {
        imid = (imin + imax) >> 1;
        if (sorted_list->pdata[imid].key < key)  // leftmost search (<)
        {
            imin = imid + 1;
        }
        else
        {
            imax = imid;
        }
    }

    *position = imin;

    return (imin < sorted_list->entry_count) && (sorted_list->pdata[imin].key == key);
}

/*! \brief Remove an entry at position in list
    \param sorted_list Sorted list to modify
    \param position Position of entry in list to remove
    \retval SORTED_LIST_PTR_OK Entry was removed
 */
SORTED_LIST_PTR_RETURN_VALUE sorted_list_ptr_remove_entry(SORTED_LIST_PTR *const sorted_list, const int position)
{
    ARGUMENT_CHECK(sorted_list != NULL);
    ARGUMENT_CHECK(position < sorted_list->capacity);
    ARGUMENT_CHECK(sorted_list->entry_count);
    ARGUMENT_CHECK(position >= 0);

    const size_t Move_size = sizeof(SORTED_LIST_PTR_ENTRY) * (sorted_list->entry_count - (size_t)position - 1ull);

    memmove(&sorted_list->pdata[position], &sorted_list->pdata[position + 1], Move_size);

    sorted_list->entry_count--;

    return SORTED_LIST_PTR_OK;
}

/*! \brief Deletes all elements in list
    \param sorted_list Sorted list to clear
    \retval SORTED_LIST_PTR_OK List was cleared
 */
SORTED_LIST_PTR_RETURN_VALUE sorted_list_ptr_clear(SORTED_LIST_PTR *const sorted_list)
{
    sorted_list->entry_count = 0;

    return SORTED_LIST_PTR_OK;
}

/*! \brief Get all keys containing value in list
    \param[out] keys Output key array for supplied value
    \param[in,out] key_count Capacity of keys parameter in entries/Number of entries in output
    \param sorted_list Sorted list to search
    \param value Key to search for
    \param remove True: Remove entry from list if value is found
    \retval SORTED_LIST_PTR_OK Key/value pair was found in list, keys assigned
    \retval SORTED_LIST_PTR_FAIL Value was not found in list, keys not assigned
 */
SORTED_LIST_PTR_RETURN_VALUE sorted_list_ptr_get_keys(SORTED_LIST_PTR_KEY_TYPE *const keys, int *key_count, SORTED_LIST_PTR *const sorted_list, const SORTED_LIST_PTR_VALUE_TYPE value, const int remove)
{
    ARGUMENT_CHECK(sorted_list != NULL);
    ARGUMENT_CHECK(sorted_list->pdata != NULL);
    ARGUMENT_CHECK(sorted_list->capacity);
    ARGUMENT_CHECK(keys != NULL);
    ARGUMENT_CHECK(key_count != NULL);

    int count = 0;
    int capacity = *key_count;

    for (int position = 0; position < (int)sorted_list->entry_count; position++)
    {
        if ((count < capacity) && (position < sorted_list->entry_count) && (sorted_list->pdata[position].value == value))
        {
            keys[count++] = sorted_list->pdata[position].key;
            if (remove)
            {
                sorted_list_ptr_remove_entry(sorted_list, position--);
            }
        }
    }

    *key_count = count;
    return count ? SORTED_LIST_PTR_OK : SORTED_LIST_PTR_FAIL;
}

/*! \brief Remove all entries containing value
    \param sorted_list Sorted list to modify
    \param value Value to remove from list
    \retval SORTED_LIST_PTR_OK At least one entry was removed
    \retval SORTED_LIST_PTR_NO_CHANGE No entries contained value
 */
SORTED_LIST_PTR_RETURN_VALUE sorted_list_ptr_remove_value(SORTED_LIST_PTR *const sorted_list, const SORTED_LIST_PTR_VALUE_TYPE value)
{
    int item_was_removed = 0;

    for (int position = (int)sorted_list->entry_count - 1; position >= 0; position--)
    {
        if (sorted_list->pdata[position].value == value)
        {
            sorted_list_ptr_remove_entry(sorted_list, position);
            item_was_removed = 1;
        }
    }

    return item_was_removed ? SORTED_LIST_PTR_OK : SORTED_LIST_PTR_NO_CHANGE;
}

/*! \brief Remove all entries matching key + value
    \param sorted_list Sorted list to modify
    \param value Key of entries to remove from list
    \param value Value of entries to remove from list
    \retval SORTED_LIST_PTR_OK At least one entry was removed
    \retval SORTED_LIST_PTR_NO_CHANGE No entries contained value
 */
SORTED_LIST_PTR_RETURN_VALUE sorted_list_ptr_remove_entries(SORTED_LIST_PTR *const sorted_list, const SORTED_LIST_PTR_KEY_TYPE key, const SORTED_LIST_PTR_VALUE_TYPE value)
{
    int item_was_removed = 0;
    int position;

    if (sorted_list_ptr_entry_search(&position, sorted_list, key, value))
    {
        for (; (position < sorted_list->entry_count) && (sorted_list->pdata[position].key == key) && (sorted_list->pdata[position].value == value); position++)
        {
            sorted_list_ptr_remove_entry(sorted_list, position--);
            item_was_removed = 1;
        }
    }

    return item_was_removed ? SORTED_LIST_PTR_OK : SORTED_LIST_PTR_NO_CHANGE;
}

// UNIQUE

/*! \brief Get value for key in an unique list
    \param[out] value Value for supplied key
    \param sorted_list Sorted list to search
    \param key Key to search for
    \param remove If true, remove entry from list if key is found
    \retval SORTED_LIST_PTR_OK Key/value pair was found in list, value assigned
    \retval SORTED_LIST_PTR_FAIL Key was not found in list, value not assigned
 */
SORTED_LIST_PTR_RETURN_VALUE sorted_list_ptr_unique_get(SORTED_LIST_PTR_VALUE_TYPE *const value, SORTED_LIST_PTR *const sorted_list, const SORTED_LIST_PTR_KEY_TYPE key, const int remove)
{
    ARGUMENT_CHECK(sorted_list != NULL);
    ARGUMENT_CHECK(sorted_list->pdata != NULL);
    ARGUMENT_CHECK(sorted_list->capacity);

    int position;

    if (sorted_list_ptr_key_search(&position, sorted_list, key))
    {
        *value = sorted_list->pdata[position].value;
        if (remove)
        {
            sorted_list_ptr_remove_entry(sorted_list, position--);
        }
        return SORTED_LIST_PTR_OK;
    }
    else
    {
        return SORTED_LIST_PTR_FAIL;
    }
}

/*! \brief Remove an entry from a unique list
    \param sorted_list Sorted list to search
    \param key Key to remove
    \retval SORTED_LIST_PTR_OK Key was found in list and entry was deleted
    \retval SORTED_LIST_PTR_NO_CHANGE Key was not found in list, entry not deleted
 */
SORTED_LIST_PTR_RETURN_VALUE sorted_list_ptr_unique_remove(SORTED_LIST_PTR *const sorted_list, const SORTED_LIST_PTR_KEY_TYPE key)
{
    ARGUMENT_CHECK(sorted_list != NULL);
    ARGUMENT_CHECK(sorted_list->pdata != NULL);
    ARGUMENT_CHECK(sorted_list->capacity);

    int position;

    if (sorted_list_ptr_key_search(&position, sorted_list, key))
    {
        return sorted_list_ptr_remove_entry(sorted_list, position);  // if entry exists, remove it
    }
    else
    {
        return SORTED_LIST_PTR_NO_CHANGE;
    }
}

/*! \brief Add an entry to a unique list
    \param sorted_list Sorted list to add to
    \param key Key of entry to add
    \param value Value of entry to add
    \retval SORTED_LIST_PTR_ENTRY_UPDATED Key was found in list and entry was updated
    \retval SORTED_LIST_PTR_NO_CHANGE Key already existed with supplied value
    \retval SORTED_LIST_PTR_ENTRY_INSERTED Key was not found in list, entry was inserted
    \retval SORTED_LIST_PTR_FAIL Sorted list was full, entry could not be inserted
 */
SORTED_LIST_PTR_RETURN_VALUE sorted_list_ptr_unique_add(SORTED_LIST_PTR *const sorted_list, const SORTED_LIST_PTR_KEY_TYPE key, const SORTED_LIST_PTR_VALUE_TYPE value)
{
    ARGUMENT_CHECK(sorted_list != NULL);
    ARGUMENT_CHECK(sorted_list->pdata != NULL);
    ARGUMENT_CHECK(sorted_list->capacity);

    int position;

    if (sorted_list_ptr_key_search(&position, sorted_list, key))
    {
        return sorted_list_ptr_overwrite_entry(sorted_list, key, value, position);  // if entry already exists, overwrite it
    }
    else
    {
        return sorted_list_ptr_insert_entry(sorted_list, key, value, position);  // otherwise insert it
    }
}

// SINGLE

/*! \brief Get value for key in a single list
    \param[out] values Output value array for supplied key, or NULL if data is not required
    \param[in,out] value_count Capacity of values parameter in entries/Number of entries in output
    \param sorted_list Sorted list to search
    \param key Key to search for
    \param remove True: Remove entry from list if key is found
    \retval SORTED_LIST_PTR_OK Key/value pair was found in list, value assigned
    \retval SORTED_LIST_PTR_FAIL Key was not found in list, values not assigned
 */
SORTED_LIST_PTR_RETURN_VALUE sorted_list_ptr_single_get(SORTED_LIST_PTR_VALUE_TYPE *const values, int *value_count, SORTED_LIST_PTR *const sorted_list, const SORTED_LIST_PTR_KEY_TYPE key, const int remove)
{
    ARGUMENT_CHECK(sorted_list != NULL);
    ARGUMENT_CHECK(sorted_list->pdata != NULL);
    ARGUMENT_CHECK(sorted_list->capacity);
    ARGUMENT_CHECK(value_count != NULL);

    int position;

    if (sorted_list_ptr_key_search(&position, sorted_list, key))
    {
        const int Capacity = *value_count;
        const int No_copy = (values == NULL);

        int count;
        for (count = 0; (No_copy || (count < Capacity)) && (position < sorted_list->entry_count) && (sorted_list->pdata[position].key == key); position++, count++)
        {
            if (!No_copy)
            {
                values[count] = sorted_list->pdata[position].value;
            }

            if (remove)
            {
                sorted_list_ptr_remove_entry(sorted_list, position--);
            }
        }

        *value_count = count;
        return SORTED_LIST_PTR_OK;
    }
    else
    {
        *value_count = 0;
        return SORTED_LIST_PTR_FAIL;
    }
}

/*! \brief Remove entries from a single list
    \param sorted_list Sorted list to search
    \param key Key to remove
    \retval SORTED_LIST_PTR_OK Key was found in list and at least one entry was deleted
    \retval SORTED_LIST_PTR_NO_CHANGE Key was not found in list, nothing was deleted
 */
SORTED_LIST_PTR_RETURN_VALUE sorted_list_ptr_single_remove(SORTED_LIST_PTR *const sorted_list, const SORTED_LIST_PTR_KEY_TYPE key)
{
    ARGUMENT_CHECK(sorted_list != NULL);
    ARGUMENT_CHECK(sorted_list->pdata != NULL);
    ARGUMENT_CHECK(sorted_list->capacity);

    int position;
    SORTED_LIST_PTR_RETURN_VALUE result = SORTED_LIST_PTR_NO_CHANGE;

    if (sorted_list_ptr_key_search(&position, sorted_list, key))
    {
        result = SORTED_LIST_PTR_OK;
        for (; (position < sorted_list->entry_count) && (sorted_list->pdata[position].key == key);)
        {
            sorted_list_ptr_remove_entry(sorted_list, position);
        }
    }

    return result;
}

/*! \brief Add an entry to a single list
    \param sorted_list Sorted list to add to
    \param key Key of entry to add
    \param value Value of entry to add
    \retval SORTED_LIST_PTR_NO_CHANGE Key already existed with supplied value
    \retval SORTED_LIST_PTR_ENTRY_INSERTED Key/value pair was not found in list, entry was inserted
    \retval SORTED_LIST_PTR_FAIL Sorted list was full, entry could not be inserted
 */
SORTED_LIST_PTR_RETURN_VALUE sorted_list_ptr_single_add(SORTED_LIST_PTR *const sorted_list, const SORTED_LIST_PTR_KEY_TYPE key, const SORTED_LIST_PTR_VALUE_TYPE value)
{
    ARGUMENT_CHECK(sorted_list != NULL);
    ARGUMENT_CHECK(sorted_list->pdata != NULL);
    ARGUMENT_CHECK(sorted_list->capacity);

    int position;

    if (sorted_list_ptr_entry_search(&position, sorted_list, key, value))
    {
        return SORTED_LIST_PTR_NO_CHANGE;  // if entry already exists, do nothing
    }
    else
    {
        return sorted_list_ptr_insert_entry(sorted_list, key, value, position);  // otherwise insert it
    }
}

// MULTIPLE

/*! \brief Get value for key in a multiple list
    \param[out] values Output value array for supplied key
    \param[in,out] value_count Capacity of values parameter in entries/Number of entries in output
    \param sorted_list Sorted list to search
    \param key Key to search for
    \param remove True: Remove entry from list if key is found
    \retval SORTED_LIST_OK Key/value pair was found in list, value assigned
    \retval SORTED_LIST_FAIL Key was not found in list, values not assigned
 */
SORTED_LIST_PTR_RETURN_VALUE sorted_list_ptr_multiple_get(SORTED_LIST_PTR_VALUE_TYPE *const values, int *value_count, SORTED_LIST_PTR *const sorted_list, const SORTED_LIST_PTR_KEY_TYPE key, const int remove)
{
    return sorted_list_ptr_single_get(values, value_count, sorted_list, key, remove);
}

/*! \brief Remove entries from a multiple list
    \param sorted_list Sorted list to search
    \param key Key to remove
    \retval SORTED_LIST_PTR_OK Key was found in list and at least one entry was deleted
    \retval SORTED_LIST_PTR_NO_CHANGE Key was not found in list, nothing was deleted
 */
SORTED_LIST_PTR_RETURN_VALUE sorted_list_ptr_multiple_remove(SORTED_LIST_PTR *const sorted_list, const SORTED_LIST_PTR_KEY_TYPE key)
{
    return sorted_list_ptr_single_remove(sorted_list, key);
}

/*! \brief Add an entry to a multiple list
    \param sorted_list Sorted list to add to
    \param key Key of entry to add
    \param value Value of entry to add
    \retval SORTED_LIST_PTR_ENTRY_INSERTED Entry was inserted
    \retval SORTED_LIST_PTR_FAIL Sorted list was full, entry could not be inserted
 */
SORTED_LIST_PTR_RETURN_VALUE sorted_list_ptr_multiple_add(SORTED_LIST_PTR *const sorted_list, const SORTED_LIST_PTR_KEY_TYPE key, const SORTED_LIST_PTR_VALUE_TYPE value)
{
    ARGUMENT_CHECK(sorted_list != NULL);
    ARGUMENT_CHECK(sorted_list->pdata != NULL);
    ARGUMENT_CHECK(sorted_list->capacity);

    int position;

    sorted_list_ptr_entry_search(&position, sorted_list, key, value);

    return sorted_list_ptr_insert_entry(sorted_list, key, value, position);
}

/*! \brief Count how many entries match key/value pair in multiple list
    \param[out] value_count Number of entries that match key/value pair
    \param sorted_list Sorted list to search
    \param key Key to search
    \param value Value to search
    \param remove True: Remove entry from list if key is found
    \retval SORTED_LIST_PTR_OK Key/value pair was found in list, count assigned
    \retval SORTED_LIST_PTR_FAIL Key was not found in list, count not assigned
 */
SORTED_LIST_PTR_RETURN_VALUE sorted_list_ptr_multiple_get_count(int *const value_count, SORTED_LIST_PTR *const sorted_list, const SORTED_LIST_PTR_KEY_TYPE key, const SORTED_LIST_PTR_VALUE_TYPE value, const int remove)
{
    ARGUMENT_CHECK(sorted_list != NULL);
    ARGUMENT_CHECK(sorted_list->pdata != NULL);
    ARGUMENT_CHECK(sorted_list->capacity);
    ARGUMENT_CHECK(value_count != NULL);

    int position, count;

    if (sorted_list_ptr_entry_search(&position, sorted_list, key, value))
    {
        for (count = 0; (position < sorted_list->entry_count) && (sorted_list->pdata[position].key == key) && (sorted_list->pdata[position].value == value); count++, position++)
        {
            if (remove)
            {
                sorted_list_ptr_remove_entry(sorted_list, position--);
            }
        }
        *value_count = count;
        return SORTED_LIST_PTR_OK;
    }
    else
    {
        *value_count = 0;
        return SORTED_LIST_PTR_FAIL;
    }
}
