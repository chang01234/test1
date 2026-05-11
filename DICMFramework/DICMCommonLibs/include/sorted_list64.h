/*! \file sorted_list64.h
\brief Sorted list header

    GENERATED FILE, DO NOT EDIT!
    Exported from "sorted_list.h.template"

    to export:

    Make sure the template files are in LF format
    bash sorted_list.h.template
*/

/*! \defgroup Manual Sorted List manual
@{
Description for sorted_list64.h
========================

This is an implementation of a sorted associative array. It was created for the need to make fast lookups of parameters in the DDM2 broker.

- Entries in the list consist of one key and one value.
- The list is doubly sorted. It is sorted by key first and value second.
- In this implementation, the key and value are both of type uint64_t.
- A list is formally a struct with metadata, often paired with a data storage array.
- There are macros to help to declare variables for a list and its storage.
- Three different usage patterns supported:
    - In a unique list all keys are unique; there can be only one value for a key.
    - In a single list all key/value pairs are unique; a key can have multiple values.
    - In a multiple list there can be duplicate key/value pairs in the list.

Example:

    #include "sorted_list64.h"

    DECLARE_SORTED_LIST64(unique, 16);
    DECLARE_SORTED_LIST64(single, 16);
    DECLARE_SORTED_LIST64(multiple, 16);

    int main(void)
    {
        sorted_list64_unique_add(&unique, 1, 10);
        sorted_list64_unique_add(&unique, 1, 10);
        sorted_list64_unique_add(&unique, 1, 30);
        sorted_list64_unique_add(&unique, 2, 10);
        sorted_list64_unique_add(&unique, 2, 30);

        sorted_list64_single_add(&single, 1, 10);
        sorted_list64_single_add(&single, 1, 10);
        sorted_list64_single_add(&single, 1, 30);
        sorted_list64_single_add(&single, 2, 10);
        sorted_list64_single_add(&single, 2, 30);

        sorted_list64_multiple_add(&multiple, 1, 10);
        sorted_list64_multiple_add(&multiple, 1, 10);
        sorted_list64_multiple_add(&multiple, 1, 30);
        sorted_list64_multiple_add(&multiple, 2, 10);
        sorted_list64_multiple_add(&multiple, 2, 30);

        return 0;
    }

Resulting lists:

    |  unique |
    |key|value|
    | 1 |  30 |
    | 2 |  30 |

    |  single |
    |key|value|
    | 1 |  10 |
    | 1 |  30 |
    | 2 |  10 |
    | 2 |  30 |

    | multiple|
    |key|value|
    | 1 |  10 |
    | 1 |  10 |
    | 1 |  30 |
    | 2 |  10 |
    | 2 |  30 |

Version history:

    2021-06-18: Reset output array count to 0 when failing to find key
    2021-11-26: Merged the two sorted lists into one template
    2021-11-30: Added description
    2021-12-21: Exposed sorted_list_remove_entry()
    2022-05-13: Added SORTED_LIST64_NO_CHANGE to sorted_list64_remove_value()
    2024-05-01: Improved argument checks, adjusted return value of remove functions
    2024-08-23: Added possibility to NULL output value array
    2024-09-06: Added dynamic creation and destruction of sorted list
    2024-10-08: Added sorted_list64_remove_entries()
    2025-05-06: Added uintptr_t variant
    2025-10-23: Reformatting

@} */

#ifndef SORTED_LIST64_H
#define SORTED_LIST64_H

// The STM8 master header defines its own stdint types that clash with stdint.h
#ifdef __IAR_SYSTEMS_ICC__
#include "stm8s.h"
#else
#include "hal_mem.h"
#include <stdint.h>
#endif

//! \~ Declare a private SORTED_LIST64 with private data storage
#define DECLARE_SORTED_LIST64(name, size) \
    static SORTED_LIST64_ENTRY name##_data[size]; \
    static SORTED_LIST64 name = { .pdata = name##_data, .capacity = size, .entry_count = 0, }

//! \~ Declare a public SORTED_LIST64 with public data storage
#define DECLARE_SORTED_LIST64_PUBLIC(name, size) \
    SORTED_LIST64_ENTRY name##_data[size]; \
    SORTED_LIST64 name = { .pdata = name##_data, .capacity = size, .entry_count = 0, }

//! \~ Declare a private SORTED_LIST64 with private data storage in EXTRAM
#define DECLARE_SORTED_LIST64_EXTRAM(name, size) \
    static EXT_RAM_ATTR SORTED_LIST64_ENTRY name##_data[size]; \
    static SORTED_LIST64 name = { .pdata = name##_data, .capacity = size, .entry_count = 0, }

//! @brief Declare a private SORTED_LIST64 with private data storage in EXTRAM.
//!
//! @note need to allocate buffer on heap using INIT_SORTED_LIST_EXTRAM_PTR
#define DECLARE_SORTED_LIST64_EXTRAM_PTR(name, size) \
    static EXT_RAM_ATTR SORTED_LIST64_ENTRY *name##_data; \
    static SORTED_LIST64 name = { .pdata = NULL, .capacity = size, .entry_count = 0, }

#define INIT_SORTED_LIST64_EXTRAM_PTR(name) \
    name##_data = hal_mem_malloc_prefer(sizeof(SORTED_LIST64_ENTRY) * name.capacity, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM); \
    assert(name##_data != NULL); \
    name.pdata = name##_data

//! \~ Declare a public SORTED_LIST64 with public data storage in EXTRAM
#define DECLARE_SORTED_LIST64_EXTRAM_PUBLIC(name, size)                             \
    EXT_RAM_ATTR SORTED_LIST64_ENTRY name##_data[size]; \
    SORTED_LIST64 name = { .pdata = name##_data, .capacity = size, .entry_count = 0, }

typedef uint64_t SORTED_LIST64_KEY_TYPE;    //!< \~	Key data type (uint64_t)
typedef uint64_t SORTED_LIST64_VALUE_TYPE;  //!< \~	Value data type (uint64_t)

//! \~ Sorted list entry; key+value
typedef struct SORTED_LIST64_ENTRY
{
    SORTED_LIST64_KEY_TYPE key;      //!< \~ Subscribed parameter number
    SORTED_LIST64_VALUE_TYPE value;  //!< \~ Subscribed connector or device on connector
} SORTED_LIST64_ENTRY;

//! \~ Sorted list metadata
typedef struct SORTED_LIST64
{
    SORTED_LIST64_ENTRY *pdata;  //!< \~ Pointer to sorted list data
    int capacity;                      //!< \~ Capacity of sorted list in entries
    int entry_count;                   //!< \~ Number of entries currently in list
} SORTED_LIST64;

//! \~ Function return values
typedef enum SORTED_LIST64_RETURN_VALUE
{
    SORTED_LIST64_FAIL = 0,                        //!< \~
    SORTED_LIST64_ENTRY_UPDATED = 1,               //!< \~
    SORTED_LIST64_ENTRY_INSERTED = 2,              //!< \~
    SORTED_LIST64_OK = 3,                          //!< \~
    SORTED_LIST64_COMPARE_RESULT_GREATER = 4,      //!< \~ Entry comparison result >
    SORTED_LIST64_COMPARE_RESULT_EQUAL = 5,        //!< \~ Entry comparison result <=
    SORTED_LIST64_COMPARE_RESULT_LESS = 6,         //!< \~ Entry comparison result <
    SORTED_LIST64_NO_CHANGE = 7,                   //!< \~ List was not updated
    SORTED_LIST64_INVALID_ARGUMENT = 8,            //!< \~ An argument was invalid
} SORTED_LIST64_RETURN_VALUE;

SORTED_LIST64 *sorted_list64_create(const size_t count);
void sorted_list64_destroy(SORTED_LIST64 *sorted_list);

int sorted_list64_key_search(int *const position, const SORTED_LIST64 *const sorted_list, const SORTED_LIST64_KEY_TYPE key);

SORTED_LIST64_RETURN_VALUE sorted_list64_remove_entry(SORTED_LIST64 *const sorted_list, const int position);
SORTED_LIST64_RETURN_VALUE sorted_list64_clear(SORTED_LIST64 *const sortlist);
SORTED_LIST64_RETURN_VALUE sorted_list64_get_keys(SORTED_LIST64_KEY_TYPE *const keys, int *key_count, SORTED_LIST64 *const sorted_list, const SORTED_LIST64_VALUE_TYPE value, const int remove);
SORTED_LIST64_RETURN_VALUE sorted_list64_remove_value(SORTED_LIST64 *const sorted_list, const SORTED_LIST64_VALUE_TYPE value);
SORTED_LIST64_RETURN_VALUE sorted_list64_remove_entries(SORTED_LIST64 *const sorted_list, const SORTED_LIST64_KEY_TYPE key, const SORTED_LIST64_VALUE_TYPE value);

SORTED_LIST64_RETURN_VALUE sorted_list64_unique_get(SORTED_LIST64_VALUE_TYPE *const value, SORTED_LIST64 *const sorted_list, const SORTED_LIST64_KEY_TYPE key, const int remove);
SORTED_LIST64_RETURN_VALUE sorted_list64_unique_remove(SORTED_LIST64 *const sorted_list, const SORTED_LIST64_KEY_TYPE key);
SORTED_LIST64_RETURN_VALUE sorted_list64_unique_add(SORTED_LIST64 *const sorted_list, const SORTED_LIST64_KEY_TYPE key, const SORTED_LIST64_VALUE_TYPE value);

SORTED_LIST64_RETURN_VALUE sorted_list64_single_get(SORTED_LIST64_VALUE_TYPE *const values, int *value_count, SORTED_LIST64 *const sorted_list, const SORTED_LIST64_KEY_TYPE key, const int remove);
SORTED_LIST64_RETURN_VALUE sorted_list64_single_remove(SORTED_LIST64 *const sorted_list, const SORTED_LIST64_KEY_TYPE key);
SORTED_LIST64_RETURN_VALUE sorted_list64_single_add(SORTED_LIST64 *const sorted_list, const SORTED_LIST64_KEY_TYPE key, const SORTED_LIST64_VALUE_TYPE value);

SORTED_LIST64_RETURN_VALUE sorted_list64_multiple_get(SORTED_LIST64_VALUE_TYPE *const values, int *value_count, SORTED_LIST64 *const sorted_list, const SORTED_LIST64_KEY_TYPE key, const int remove);
SORTED_LIST64_RETURN_VALUE sorted_list64_multiple_remove(SORTED_LIST64 *const sorted_list, const SORTED_LIST64_KEY_TYPE key);
SORTED_LIST64_RETURN_VALUE sorted_list64_multiple_add(SORTED_LIST64 *const sorted_list, const SORTED_LIST64_KEY_TYPE key, const SORTED_LIST64_VALUE_TYPE value);
SORTED_LIST64_RETURN_VALUE sorted_list64_multiple_get_count(int *const value_count, SORTED_LIST64 *const sorted_list, const SORTED_LIST64_KEY_TYPE key, const SORTED_LIST64_VALUE_TYPE value, const int remove);

#endif /* SORTED_LIST64_H */
