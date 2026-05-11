/**
 * \file        ddm_entry.h
 * \date        2021-10-08
 * \author      (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 * \brief       DDM entry
 *
 * DDM entry allows you to store, fetch and access the values of DDM parameters
 * in a standardised way.
 *
 * \li          2021-10-08  (NR) Initial implementation
 * \li          2022-07-07  (NR) Added ddm_entry__value, ddm_entry__set__value
 * \li          2024-05-22  (NR) Fix: Incorrect handling of argument type sign
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
#ifndef DDM_ENTRY_H_
#define DDM_ENTRY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ddm2_parameter_list.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief       Internal bit representing is_subscribed DDM entry member.
 *
 * @note        This macro is for internal use only.
 */
#define DDM_ENTRY__INTERNAL_FLAG_IS_SUBSCRIBED_MASK (0x1u << 0u)

/**
 * @brief       Internal bit representing has_changed DDM entry member.
 *
 * @note        This macro is for internal use only.
 */
#define DDM_ENTRY__INTERNAL_FLAG_HAS_CHANGED_MASK (0x1u << 1u)

/**
 * @brief       Internal bit representing has_value DDM entry member.
 *
 * @note        This macro is for internal use only.
 */
#define DDM_ENTRY__INTERNAL_FLAG_HAS_VALUE_MASK (0x1u << 2u)

/**
 * \brief       DDM value structure
 *
 * The structure contains actual data payload and type enumerator which
 * specifies what data type is contained within.
 */
typedef struct ddm_entry__value
{
    union ddm_entry__value_store
    {
        int32_t i32;     /*!< Storage for int32_t type. */
        uint32_t u32;    /*!< Storage for uint32_t type */
        char *str;       /*!< Storage for NULL terminated string */
        void *other;     /*!< Storage for Other type */
        void *structure; /*!< Storage for Struct type */
    } storage;           /*!< Storage of data */
    size_t size;         /*!< Size of data contained in \a storage member in bytes */
    DDM2_TYPE_ENUM type; /*!< Type of data contained in \a storage member */
} ddm_entry_value_t;

/**
 * \brief       DDM entry structure
 *
 * One DDM entry represents a DDM with a value and metadata. The metadata
 * information contains the following data fields:
 * - Member \a is_susbscribed can be accessed by
 *   @ref ddm_entry__set__is_subscribed, and @ref ddm_entry__is_subscribed.
 * - Member \a has_changed can be accessed by @ref ddm_entry__set__has_changed,
 *   @ref ddm_entry__set__has_changed_conditionally and
 *   @ref ddm_entry__has_changed.
 * - 32-bit integer containing user defined flags which can be accessed by
 *   @ref ddm_entry__set__flags and @ref ddm_entry__flags.
 *
 * \note        All members of this structure are private to ddm_entry
 *              component.
 */
typedef struct ddm_entry
{
    uint32_t p__index;          /*!< Index of the parameter in DDM2 arrays */
    uint32_t p__flags;          /*!< User defined flags */
    ddm_entry_value_t p__value; /*!< Value of DDM parameter */
    uint8_t p__instance;        /*!< Instance of the parameter*/
    uint8_t p__internal_flags;  /*!< is_subscribed (bit 0), has_changed (bit 1), has_value (bit 2) */
} ddm_entry_t;

/**
 * \brief       Initialize a DDM entry structure
 *
 * This function will initialize all structure fields to default values and do
 * the lookup to DDM parameter list to fetch additional DDM information, like
 * DDM in/out type etc. The default DDM entry values are the following:
 * - index: extracted from \a parameted_id parameter using DDM2 lookup table,
 * - instance: extracted from \a parameter_id parameter,
 * - is_subscribed: set to false,
 * - has_changed: set to false,
 * - flags: are set to zero,
 * - value: is zero (integral types) or null (string types).
 *
 * Use of this function is mandatory and it must be used before invoking any
 * other function from this component. After parameter initialization its `has_value`
 * flag is set to false.
 *
 * \param       entry Pointer to \ref ddm_entry structure which needs to be initialized.
 * \param       ddm_parameter Associate \a entry with the specified parameter.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 *
 * \note        Do not call this function with already initialized entry, as that might lead to
 *              memory leaks (in case the entry was previously a string type DDM).
 * \return      Status of operation
 * \retval      0 - Operation was successful
 * \retval      Non-zero number - It was not possible to find DDM with given parameter ID in
 *              DDM2 parameter lookup table.
 */
int ddm_entry__init(ddm_entry_t *entry, uint32_t ddm_parameter);

/*
 * \brief       Initialize a copy DDM entry from another DDM entry.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously initialized.
 * \param       from Pointer to \ref ddm_entry from which to copy value.
 * \return      Status of operation
 * \retval      0 - Operation was successful
 * \retval      Non-zero number - It was not possible to copy DDM to destination.
 */
int ddm_entry__init_copy(ddm_entry_t *entry, const ddm_entry_t *from);

/**
 * \brief       When there is no more need for a DDM entry to exist call this function to terminate
 *              it.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previosly initialized.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 */
void ddm_entry__terminate(ddm_entry_t *entry);

/**
 * \brief       Return the parameter identification number which is pointed by
 *              the entry.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously
 *              initialized.
 *
 * \return      Parameter identification number (ID).
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 * \pre         Entry must be previously initialized with a call to
 *              \ref ddm_entry__init function.
 */
uint32_t ddm_entry__parameter_id(const ddm_entry_t *const entry);

/**
 * \brief       Return the parameter output type.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously
 *              initialized.
 *
 * \return      Parameter type enumerator.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 * \pre         Entry must be previously initialized with a call to
 *              \ref ddm_entry__init function.
 */
DDM2_TYPE_ENUM ddm_entry__out_type(const ddm_entry_t *const entry);

/**
 * \brief       Return the parameter output unit.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously
 *              initialized.
 *
 * \return      Parameter unit enumerator.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 * \pre         Entry must be previously initialized with a call to
 *              \ref ddm_entry__init function.
 */
DDM2_UNIT_ENUM ddm_entry__out_unit(const ddm_entry_t *const entry);

/**
 * \brief       Return the parameter input type.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously
 *              initialized.
 *
 * \return      Parameter type enumerator.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 * \pre         Entry must be previously initialized with a call to
 *              \ref ddm_entry__init function.
 */
DDM2_TYPE_ENUM ddm_entry__in_type(const ddm_entry_t *const entry);

/**
 * \brief       Return the parameter input unit.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously
 *              initialized.
 *
 * \return      Parameter unit enumerator.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 * \pre         Entry must be previously initialized with a call to
 *              \ref ddm_entry__init function.
 */
DDM2_UNIT_ENUM ddm_entry__in_unit(const ddm_entry_t *const entry);

/**
 * \brief       Return if the parameter is accessible on cloud.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously
 *              initialized.
 *
 * \return      Boolean value which states if parameter is on cloud.
 * \return      true - parameter is accessble on cloud
 * \return      false - parameter is not accessible on cloud
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 * \pre         Entry must be previously initialized with a call to
 *              \ref ddm_entry__init function.
 */
bool ddm_entry__is_on_cloud(const ddm_entry_t *const entry);

/**
 * \brief       Return the parameter device class string.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously
 *              initialized.
 *
 * \return      Constant string describing the device class.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 * \pre         Entry must be previously initialized with a call to
 *              \ref ddm_entry__init function.
 */
const char *ddm_entry__device_class(const ddm_entry_t *const entry);

/**
 * \brief       Return the parameter property string.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously
 *              initialized.
 *
 * \return      Constant string describing the property.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 * \pre         Entry must be previously initialized with a call to
 *              \ref ddm_entry__init function.
 */
const char *ddm_entry__property(const ddm_entry_t *const entry);

/**
 * \brief       Return the type of DDM value.
 *
 * Some DDM parameters may have different input and output types. During the
 * DDM processing their value type will change from input to output type and
 * vice versa.
 *
 * Thus function will return the type of value stored in the DDM.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously
 *              initialized.
 *
 * \return      Type enumerator
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 * \pre         Entry must be previously initialized with a call to
 *              \ref ddm_entry__init function.
 */
static inline DDM2_TYPE_ENUM ddm_entry__value_type(const ddm_entry_t *const entry)
{
    return entry->p__value.type;
}

/**
 * \brief       Return the size of DDM value in bytes.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously
 *              initialized.
 *
 * \return      Size of stored data in bytes. If the DDM entry type is
 *              DDM2_TYPE_NONE the size is 0 bytes.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 * \pre         Entry must be previously initialized with a call to
 *              \ref ddm_entry__init function.
 */
static inline size_t ddm_entry__value_size(const ddm_entry_t *const entry)
{
    return entry->p__value.size;
}

/**
 * \brief       Return if the type of DDM value is int32_t.
 *
 * Some DDM parameters may have different input and output types. During the
 * DDM processing their value type will change from input to output type and
 * vice versa.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously
 *              initialized.
 *
 * \return      If the type of DDM value is int32_t.
 * \retval      true - Type of DDM value is int32_t.
 * \retval      false - Type of DDM value is not int32_t.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 * \pre         Entry must be previously initialized with a call to
 *              \ref ddm_entry__init function.
 */
static inline bool ddm_entry__is_value_int32(const ddm_entry_t *const entry)
{
    return entry->p__value.type == DDM2_TYPE_INT32_T ? true : false;
}

/**
 * \brief       Return if the type of DDM value is uint32_t.
 *
 * Some DDM parameters may have different input and output types. During the
 * DDM processing their value type will change from input to output type and
 * vice versa.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously
 *              initialized.
 *
 * \return      If the type of DDM value is uint32_t.
 * \retval      true - Type of DDM value is uint32_t.
 * \retval      false - Type of DDM value is not uint32_t.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 * \pre         Entry must be previously initialized with a call to
 *              \ref ddm_entry__init function.
 */
static inline bool ddm_entry__is_value_uint32(const ddm_entry_t *const entry)
{
    return entry->p__value.type == DDM2_TYPE_UINT32_T ? true : false;
}

/**
 * \brief       Return if the type of DDM value is C string.
 *
 * Some DDM parameters may have different input and output types. During the
 * DDM processing their value type will change from input to output type and
 * vice versa.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously
 *              initialized.
 *
 * \return      If the type of DDM value is C string.
 * \retval      true - Type of DDM value is C string.
 * \retval      false - Type of DDM value is not C string.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 * \pre         Entry must be previously initialized with a call to
 *              \ref ddm_entry__init function.
 */
static inline bool ddm_entry__is_value_str(const ddm_entry_t *const entry)
{
    return entry->p__value.type == DDM2_TYPE_STRING ? true : false;
}

/**
 * \brief       Return if the type of DDM value is OTHER.
 *
 * Some DDM parameters may have different input and output types. During the
 * DDM processing their value type will change from input to output type and
 * vice versa.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously
 *              initialized.
 *
 * \return      If the type of DDM value is OTHER.
 * \retval      true - Type of DDM value is OTHER.
 * \retval      false - Type of DDM value is not OTHER.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 * \pre         Entry must be previously initialized with a call to
 *              \ref ddm_entry__init function.
 */
static inline bool ddm_entry__is_value_other(const ddm_entry_t *const entry)
{
    return entry->p__value.type == DDM2_TYPE_OTHER ? true : false;
}

/**
 * \brief       Return if the type of DDM value is STRUCT.
 *
 * Some DDM parameters may have different input and output types. During the
 * DDM processing their value type will change from input to output type and
 * vice versa.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously
 *              initialized.
 *
 * \return      If the type of DDM value is STRUCT.
 * \retval      true - Type of DDM value is STRUCT.
 * \retval      false - Type of DDM value is not STRUCT.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 * \pre         Entry must be previously initialized with a call to
 *              \ref ddm_entry__init function.
 */
static inline bool ddm_entry__is_value_struct(const ddm_entry_t *const entry)
{
    return entry->p__value.type == DDM2_TYPE_STRUCT ? true : false;
}

/**
 * \brief       Return if the type of DDM value is none.
 *
 * Some DDM parameters may have different input and output types. During the
 * DDM processing their value type will change from input to output type and
 * vice versa.
 *
 * DDM can be of type NONE when it holds no value, which means it was just
 * created or it was explicitely deleted.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously
 *              initialized.
 *
 * \return      If the type of DDM value is OTHER.
 * \retval      true - Type of DDM value is OTHER.
 * \retval      false - Type of DDM value is not OTHER.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 * \pre         Entry must be previously initialized with a call to
 *              \ref ddm_entry__init function.
 */
static inline bool ddm_entry__is_value_none(const ddm_entry_t *const entry)
{
    return entry->p__value.type == DDM2_TYPE_NONE ? true : false;
}

/**
 * \brief       Return if the type of DDM value is a jumbo type.
 *
 * Some DDM parameters may have different input and output types. During the
 * DDM processing their value type will change from input to output type and
 * vice versa.
 *
 * DDM can be of type NONE when it holds no value, which means it was just
 * created or it was explicitely deleted.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously
 *              initialized.
 *
 * \return      If the type of DDM value is jumbo.
 * \retval      true - Type of DDM value is jumbo.
 * \retval      false - Type of DDM value is not jumbo.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 * \pre         Entry must be previously initialized with a call to
 *              \ref ddm_entry__init function.
 */
static inline bool ddm_entry__is_value_jumbo(const ddm_entry_t *const entry)
{
    return entry->p__value.type == DDM2_TYPE_JUMBO ? true : false;
}

/**
 * \brief       Return if the type of DDM value is a void type.
 *
 * Some DDM parameters may have different input and output types. During the
 * DDM processing their value type will change from input to output type and
 * vice versa.
 *
 * DDM can be of type NONE when it holds no value, which means it was just
 * created or it was explicitely deleted.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously
 *              initialized.
 *
 * \return      If the type of DDM value is void.
 * \retval      true - Type of DDM value is void.
 * \retval      false - Type of DDM value is not void.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 * \pre         Entry must be previously initialized with a call to
 *              \ref ddm_entry__init function.
 */
static inline bool ddm_entry__is_value_void(const ddm_entry_t *const entry)
{
    return entry->p__value.type == DDM2_TYPE_VOID ? true : false;
}

/**
 * \brief       Return if the type of DDM value is a count type.
 *
 * Some DDM parameters may have different input and output types. During the
 * DDM processing their value type will change from input to output type and
 * vice versa.
 *
 * DDM can be of type NONE when it holds no value, which means it was just
 * created or it was explicitely deleted.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously
 *              initialized.
 *
 * \return      If the type of DDM value is count.
 * \retval      true - Type of DDM value is count.
 * \retval      false - Type of DDM value is not count.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 * \pre         Entry must be previously initialized with a call to
 *              \ref ddm_entry__init function.
 */
static inline bool ddm_entry__is_value_count(const ddm_entry_t *const entry)
{
    return entry->p__value.type == DDM2_TYPE_COUNT ? true : false;
}

/**
 * \brief       Delete a value contained in DDM and set type to NONE.
 *
 * Some DDM parameters may have different input and output types. During the
 * DDM processing their value type will change from input to output type and
 * vice versa.
 *
 * When switching over from one value type to another value type, it is needed
 * to delete old value before assigning a new value with other type. In case of
 * assining a new value with the same type, then call to this function is not
 * necessary, since set/write functions will set the correct type.
 *
 * DDM entry parameter ID and user flags are not affected by this function, only
 * the value and internal flags are modified.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously
 *              initialized.
 *
 * \return      Returns if the value has been changed.
 * \retval      true - previous value was different then the new one.
 * \retval      false - previous valus is the same as new one.
 * \pre         Parameter \a entry must be a non-NULL pointer.
 * \pre         Entry must be previously initialized with a call to
 *              \ref ddm_entry__init function.
 */
bool ddm_entry__delete__value(ddm_entry_t *entry);

/**
 * \brief       Set the value to a DDM (input type).
 *
 * The type of value is determined by DDM parameter list data. The function will based on that data
 * fetch the data using the void pointer argument and put the DDM into correct in_type as specified
 * by the DDM parameter list. It will also check the \a data_size argument for integral types.
 *
 * The function will first delete any previously hold data and data type and then put new data and
 * associated type.
 *
 * By writing to the parameter `has_value` is set to true. If the DDM entry is of unsupported type
 * (NONE, VOID, JUMBO, COUNT) the flag will not be set.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously initialized.
 * \param       data Pointer to data that needs to be put into DDM.
 * \param       data_size Size of data (in bytes) pointed by \a data argument.
 * \return      Returns if the value has been changed.
 * \retval      true - previous value was different then the new one.
 * \retval      false - previous valus is the same as new one.
 * \pre         Parameter \a entry must be a non-NULL pointer.
 * \pre         Parameter \a data must be a non-NULL pointer.
 * \pre         Entry must be previously initialized with a call to \ref ddm_entry__init function.
 * \note        This function is usually used when setting a DDM in own connector.
 */
bool ddm_entry__write__value(ddm_entry_t *entry, const void *data, size_t data_size);

/**
 * \brief       Get the value from a DDM  (output type).
 *
 * Get the data of any type from DDM. Based on the DDM parameter list description the function knows
 * which `OUTPUT` type of data it should access.
 *
 * The function has two output arguments. The \a data argument will put the pointer to data, and
 * \a data_size will put the value of size of data in bytes.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously initialized.
 * \param       data Pointer to data pointer that will point to some data.
 * \param       data_size Pointer to size of data (in bytes) variable.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 * \pre         Parameter \a data must be a non-NULL pointer.
 * \pre         Parameter \a data_size must be a non-NULL pointer.
 * \pre         Entry must be previously initialized with a call to \ref ddm_entry__init function.
 *
 * The following code snippet is of not typical use, but it might prove to be
 * usefull:
 *
 * \code
 * const int32_t *data_i32;
 * size_t data_size;
 *
 * ddm_entry__read_value(ddm_entry, &data_i32, &data_size);
 * ASSERT(data_size == sizeof(int32_t));
 * ...
 * \endcode
 *
 * The usual use-case is to just pass the data to broker frame function:
 *
 * \code
 * const void * data;
 * size_t data_size;
 *
 * ddm_entry__read_value(ddm_entry, &data_i32, &data_size);
 * connector_send_frame_to_broker(..., data, data_size, ...);
 * \endcode
 */
void ddm_entry__read__value(const ddm_entry_t *entry, const void **data, size_t *data_size);

/**
 * \brief       Copy value of one DDM entry to another DDM entry if both are in same data type
 *
 * This function copies the value from \a from DDM entry to \a entry DDM entry. Internal flags,
 * user flags and parameter IDs are not affected.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously initialized.
 * \param       from Pointer to \ref ddm_entry from which to copy value.
 * \return      Returns if the value has been changed.
 * \retval      true - previous value was different then the new one.
 * \retval      false - previous valus is the same as new one.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer to previously initialized DDM
 *              parameter with a call to \ref ddm_entry__init or \ref ddm_entry__init_copy
 *              functions.
 * \pre         Parameter \a from must be a non-NULL pointer to previously initialized DDM
 *              parameter with a call to \ref ddm_entry__init or \ref ddm_entry__init_copy
 *              functions.
 */
bool ddm_entry__copy__value(ddm_entry_t *entry, const ddm_entry_t *from);

/**
 * \brief       Copy entire DDM entry into another one.
 *
 * During the copying the destination DDM entry will be overwritten. During the
 * copy, DDM parameter ID will also be overwritten, changing the destination DDM entry
 * parameter ID. Any previosly stored value is deleted before copy. Also, the data type
 * contained in original \a entry DDM entry might be changed.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously initialized.
 * \param       from Pointer to \ref ddm_entry from which to copy value.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer to previously initialized DDM
 *              parameter with a call to \ref ddm_entry__init or \ref ddm_entry__init_copy
 *              functions.
 * \pre         Parameter \a from must be a non-NULL pointer to previously initialized DDM
 *              parameter with a call to \ref ddm_entry__init or \ref ddm_entry__init_copy
 *              functions.
 */
void ddm_entry__copy(ddm_entry_t *entry, const ddm_entry_t *from);

/**
 * \brief       Set the value to a DDM (output type).
 *
 * The type of value is determined by DDM parameter list data. The function will based on that data
 * fetch the data using the void pointer argument and put the DDM into correct `out_type` as specified
 * by the DDM parameter list. It will also check the \a data_size argument for integral types.
 *
 * The function will first delete any previously hold data and data type and then put new data and
 * associated type.
 *
 * By setting the parameter, the internal flag `has_value` is set to true. If the DDM entry is of unsupported type
 * (NONE, VOID, JUMBO, COUNT) the flag will not be set.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously initialized.
 * \param       data Pointer to data that needs to be put into DDM.
 * \param       data_size Size of data (in bytes) pointed by \a data argument.
 * \return      Returns if the value has been changed.
 * \retval      true - previous value was different then the new one.
 * \retval      false - previous valus is the same as new one.
 * \pre         Parameter \a entry must be a non-NULL pointer.
 * \pre         Parameter \a data must be a non-NULL pointer.
 * \pre         Entry must be previously initialized with a call to \ref ddm_entry__init function.
 * \note        This function is usually used when setting DDM to _other_ connector.
 */
bool ddm_entry__set__value(ddm_entry_t *entry, const void *data, size_t data_size);

/**
 * \brief       Set a int32_t type of value to a DDM.
 *
 * By setting the parameter, the internal flag `has_value` is set to true.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously initialized.
 * \param       i32 Signed integer 32-bit value.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 *
 * \note        The function will assert if given DDM is allowed to contain
 *              signed integer 32-bit value.
 *
 * \return      Returns if the value has been changed.
 * \retval      true - previous value was different then the new one.
 * \retval      false - previous valus is the same as new one.
 */
bool ddm_entry__set__value_i32(ddm_entry_t *entry, int32_t i32);

/**
 * \brief       Set a uint32_t type of value to a DDM.
 *
 * By setting the parameter, the internal flag `has_value` is set to true.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously initialized.
 * \param       u32 Unsigned integer 32-bit value.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 *
 * \note        The function will assert if given DDM is allowed to contain
 *              unsigned integer 32-bit value.
 *
 * \return      Returns if the value has been changed.
 * \retval      true - previous value was different then the new one.
 * \retval      false - previous valus is the same as new one.
 */
bool ddm_entry__set__value_u32(ddm_entry_t *entry, uint32_t u32);

/**
 * \brief       Set a string type of value to a DDM.
 *
 * This function will set a new string value in the DDM. For string allocation
 * it uses SPI RAM memory. The function uses lazy free algorithm. This means if
 * a new string can fit into existing string storage, then it will just use the
 * already allocated storage. If the new string does not fit the existing string
 * storage it will free the previous string storage and allocate a new storage.
 *
 * By setting the parameter, the internal flag `has_value` is set to true.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was
 *              previously initialized.
 * \param       new_str Pointer to string
 * \param       new_str_len Pointed string length, like returned by \a strlen.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 * \pre         Parameter \a new_str must be a non-NULL pointer.
 *
 * \note        The function will assert if given DDM is allowed to contain a
 *              string value.
 * \note        The function will fail if the string storage RAM allocation is
 *              not successful.
 * \return      Returns if the value has been changed.
 * \retval      true - previous value was different then the new one.
 * \retval      false - previous valus is the same as new one.
 */
bool ddm_entry__set__value_str(
    ddm_entry_t *entry,
    const char *new_str,
    size_t new_str_len);

/**
 * \brief       Set a n other type of value to a DDM.
 *
 * This function will set a new other value in the DDM. For other structure
 * allocation it uses SPI RAM memory. The function uses lazy free algorithm.
 * This means if a new other can fit into existing other value storage, then it
 * will just use the already allocated memory. If the new other does not fit the
 * existing other storage it will free the previous other storage moemory and
 * allocate a new storage memory.
 *
 * By setting the parameter, the internal flag `has_value` is set to true.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was
 *              previously initialized.
 * \param       new_other Pointer to other structure.
 * \param       new_other_size Pointed other size in bytes.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 * \pre         Parameter \a new_other must be a non-NULL pointer.
 *
 * \note        The function will assert if given DDM is allowed to contain a
 *              other value type.
 * \note        The function will fail if the other storage RAM allocation is
 *              not successful.
 * \return      Returns if the value has been changed.
 * \retval      true - previous value was different then the new one.
 * \retval      false - previous valus is the same as new one.
 */
bool ddm_entry__set__value_other(
    ddm_entry_t *entry,
    const void *new_other,
    size_t new_other_size);

/**
 * \brief       Set a n struct type of value to a DDM.
 *
 * This function will set a new struct value in the DDM. For other structure
 * allocation it uses SPI RAM memory. The function uses lazy free algorithm.
 * This means if a new struct can fit into existing struct value storage, then it
 * will just use the already allocated memory. If the new struct does not fit the
 * existing struct storage it will free the previous struct storage moemory and
 * allocate a new storage memory.
 *
 * By setting the parameter, the internal flag `has_value` is set to true.
 *
 * \param       ddm_entry Pointer to \ref ddm_entry structure which was
 *              previously initialized.
 * \param       new_struct Pointer to struct structure.
 * \param       new_struct_size Pointed struct size in bytes.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 * \pre         Parameter \a new_struct must be a non-NULL pointer.
 *
 * \note        The function will assert if given DDM is allowed to contain a
 *              struct value type.
 * \note        The function will fail if the struct storage RAM allocation is
 *              not successful.
 * \return      Returns if the value has been changed.
 * \retval      true - previous value was different then the new one.
 * \retval      false - previous valus is the same as new one.
 */
bool ddm_entry__set__value_struct(
    ddm_entry_t *entry,
    const void *new_struct,
    size_t new_struct_size);

/**
 * \brief       Set a DDM is_subscribed metadata value.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously
 *              initialized.
 * \param       is_subscribed New value of \a is_subscribed flag.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 */
static inline void ddm_entry__set__is_subscribed(ddm_entry_t *entry, bool is_subscribed)
{
    if (is_subscribed)
    {
        entry->p__internal_flags |= DDM_ENTRY__INTERNAL_FLAG_IS_SUBSCRIBED_MASK;
    }
    else
    {
        entry->p__internal_flags &= ~DDM_ENTRY__INTERNAL_FLAG_IS_SUBSCRIBED_MASK;
    }
}

/**
 * \brief       Set a DDM has_changed metadata value.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously initialized.
 * \param       has_changed New value of \a has_changed flag.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 */
static inline void ddm_entry__set__has_changed(ddm_entry_t *entry, bool has_changed)
{
    if (has_changed)
    {
        entry->p__internal_flags |= DDM_ENTRY__INTERNAL_FLAG_HAS_CHANGED_MASK;
    }
    else
    {
        entry->p__internal_flags &= ~DDM_ENTRY__INTERNAL_FLAG_HAS_CHANGED_MASK;
    }
}

/**
 * \brief       Set a DDM has_changed metadata value to true only if it was false.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously initialized.
 * \param       has_changed New value of \a has_changed flag.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 */
static inline void ddm_entry__set__has_changed_conditionally(ddm_entry_t *entry, bool has_changed)
{
    if (has_changed)
    {
        entry->p__internal_flags |= DDM_ENTRY__INTERNAL_FLAG_HAS_CHANGED_MASK;
    }
}

/**
 * \brief       Set a DDM has_value metadata value.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously
 *              initialized.
 * \param       has_value New value of \a has_value flag.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 */
static inline void ddm_entry__set__has_value(ddm_entry_t *entry, bool has_value)
{
    if (has_value)
    {
        entry->p__internal_flags |= DDM_ENTRY__INTERNAL_FLAG_HAS_VALUE_MASK;
    }
    else
    {
        entry->p__internal_flags &= ~DDM_ENTRY__INTERNAL_FLAG_HAS_VALUE_MASK;
    }
}

/**
 * \brief       Set a DDM flags metadata value.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously
 *              initialized.
 * \param       flags New value of \a flags member.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 */
static inline void ddm_entry__set__flags(ddm_entry_t *entry, uint32_t flags)
{
    entry->p__flags = flags;
}

/**
 * \brief       Get a value from DDM (input type)
 *
 * Get the data of any type from DDM. Based on the DDM parameter list description the function knows
 * which `INPUT` type of data it should access.
 *
 * The function has two output arguments. The \a data argument will put the pointer to data, and
 * \a data_size will put the value of size of data in bytes.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously initialized.
 * \param       data Pointer to data pointer that will point to some data.
 * \param       data_size Pointer to size of data (in bytes) variable.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 * \pre         Parameter \a data must be a non-NULL pointer.
 * \pre         Parameter \a data_size must be a non-NULL pointer.
 * \pre         Entry must be previously initialized with a call to \ref ddm_entry__init function.
 */
void ddm_entry__value(const ddm_entry_t *entry, const void **data, size_t *data_size);

/**
 * \brief       Get int32_t value from DDM.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously initialized.
 *
 * \return      Integer value contained in DDM.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 *
 * \note        Before invoking this function the DDM value type can be asserted
 *              with \ref ddm_entry__is_value_int32() function.
 */
int32_t ddm_entry__value_i32(const ddm_entry_t *entry);

/**
 * \brief       Get uint32_t value from DDM.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously initialized.
 *
 * \return      Unsigned integer value contained in DDM.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 *
 * \note        Before invoking this function the DDM value type can be asserted
 *              with \ref ddm_entry__is_value_uint32() function.
 */
uint32_t ddm_entry__value_u32(const ddm_entry_t *entry);

/**
 * \brief       Get string value from a DDM
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously
 *              initialized.
 *
 * \return      String value contained in DDM.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 *
 * \note        Before invoking this function the DDM value type can be asserted
 *              with \ref ddm_entry__is_value_str() function.
 */
const char *ddm_entry__value_str(const ddm_entry_t *entry);

/**
 * \brief       Get the other type value from a DDM
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously
 *              initialized.
 * \return      Other type value contained in DDM.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 *
 * \note        Before invoking this function the DDM value type can be asserted
 *              with \ref ddm_entry__is_value_str() function.
 */
const void *ddm_entry__value_other(const ddm_entry_t *entry);

/**
 * \brief       Get the struct type value from a DDM
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously
 *              initialized.
 * \return      Struct type value contained in DDM.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 *
 * \note        Before invoking this function the DDM value type can be asserted
 *              with \ref ddm_entry__is_value_str() function.
 */
const void *ddm_entry__value_struct(const ddm_entry_t *entry);

/**
 * \brief       Get \a is_subscribed flag value from a DDM.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously
 *              initialized.
 *
 * \return      Flag value contained in the DDM.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 */
static inline bool ddm_entry__is_subscribed(const ddm_entry_t *entry)
{
    return entry->p__internal_flags & DDM_ENTRY__INTERNAL_FLAG_IS_SUBSCRIBED_MASK;
}

/**
 * \brief       Get \a has_changed flag value from a DDM.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously
 *              initialized.
 *
 * \return      Flag value contained in the DDM.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 */
static inline bool ddm_entry__has_changed(const ddm_entry_t *entry)
{
    return entry->p__internal_flags & DDM_ENTRY__INTERNAL_FLAG_HAS_CHANGED_MASK;
}

/**
 * \brief       Get \a has_value flag value from a DDM.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously
 *              initialized.
 *
 * \return      Flag value contained in the DDM.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 */
static inline bool ddm_entry__has_value(const ddm_entry_t *entry)
{
    return entry->p__internal_flags & DDM_ENTRY__INTERNAL_FLAG_HAS_VALUE_MASK;
}

/**
 * \brief       Get user flag value from a DDM.
 *
 * \param       entry Pointer to \ref ddm_entry structure which was previously
 *              initialized.
 *
 * \return      Flag value contained in the DDM.
 *
 * \pre         Parameter \a entry must be a non-NULL pointer.
 */
static inline uint32_t ddm_entry__flags(const ddm_entry_t *entry)
{
    return entry->p__flags;
}

#ifdef __cplusplus
}
#endif

#endif /* DDM_ENTRY_H_ */
