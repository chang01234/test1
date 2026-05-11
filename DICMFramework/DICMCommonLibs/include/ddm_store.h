/**
 * \file        ddm_store.h
 * \date        2021-10-08
 * \author      (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 * \author      (BB) Borjan Bozhinovski (ext-borjan.bozhinovski@dometic.com)
 *
 * \brief       DDM store
 *
 * DDM store allows you to store and access the values of DDM parameters
 * in a standardised way.
 *
 * \li          2021-10-08  (NR) Initial implementation
 * \li          2022-07-07  (NR) Add ddm_store__capacity
 * \li          2022-07-08  (NR) Add ddm_store__init_with_empty_entries
 * \li          2022-05-16  (BB) Comment handling for NULL string entries
 * \li          2024-06-26  (NR) Added instance handling
 * \li          2024-06-26  (NR) Added first few examples how to use ddm_store
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

/**
 * \name        DDM store examples
 *
 * \code
 * // Example of usage when the connector is the owner of a single DDM class
 *
 * // Create initial values of owned DDM parameters
 * static const ddm_store_ddm_t initial_values[] =
 * {
 *     ...
 * };
 *
 * // Allocate DDM store of sufficient storage for owned DDM parameters
 * DDM_STORE__DECLARE_EXTRAM(my_owned_store, ELEMENTS(initial_values));
 *
 * // In code register own class with Broker. The Broker will return instance
 * // number. Use the instance number to initialize DDM parameters:
 * ddm_store__load_entries(&my_owned_store, initial_values, ELEMENTS(initial_values), instance);
 * \endcode
 *
 * \code
 * // Example of usage when the connector wants to store DDM parameters that are owned by other
 * // connectors. If the instance of DDM parameters that needs to be subscribed is known in advance
 * // that can also be stated.
 *
 * // Create array of parameter that we are going to subscribe to
 * static const ddm_store_ddm_t subscribed_values[] =
 * {
 *      {
 *          .ddm_parameter = AC0TEMP
 *      },
 *      {
 *          .ddm_parameter = HTR0TEMP | DDM2_PARAMETER_INSTANCE(2),
 *      },
 *      {
 *          .ddm_parameter = MBAT0SOC | DDM2_PARAMETER_INSTANCE(5),
 *      },
 * };
 *
 * // Allocate DDM store of sufficient storage for non-owned DDM parameters
 * DDM_STORE__DECLARE_EXTRAM(non_owned_store, ELEMENTS(subscribed_values));
 *
 * // Create ddm_entry entries with DDM instances stated in subscribed_values
 * ddm_store__init_with_empty_entries(&non_owned_store, initial_values, ELEMENTS(initial_values), 0);
 * \endcode
 *
 * \code
 * // Example of usage when the connector contains owned and other DDM parameters.
 *
 * // Create array of parameters that we are owning
 * static const ddm_store_ddm_t initial_values =
 * {
 *      ...
 * };
 *
 * // Allocate DDM store of sufficient storage for owned DDM parameters
 * DDM_STORE__DECLARE_EXTRAM(my_owned_store, ELEMENTS(initial_values));
 *
 * // Create array of parameter that we are going to subscribe to
 * static const ddm_store_ddm_t subscribed_values[] =
 * {
 *      ...
 * };
 *
 * // Allocate DDM store of sufficient storage for non-owned DDM parameters
 * DDM_STORE__DECLARE_EXTRAM(non_owned_store, ELEMENTS(subscribed_values));
 *
 * // In code register own class with Broker. The Broker will return instance
 * // number. Use the instance number to initialize DDM parameters:
 * ddm_store__load_entries(&my_owned_store, initial_values, ELEMENTS(initial_values), instance);
 *
 * // Create ddm_entry entries with DDM instances stated in subscribed_values
 * ddm_store__init_with_empty_entries(&non_owned_store, initial_values, ELEMENTS(initial_values), 0);
 * \endcode
 *
 * \code
 * // Example of usage when connector wants to add owned DDM entries dynamically.
 *
 * // Allocate DDM store of sufficient storage for owned DDM parameters
 * DDM_STORE__DECLARE_EXTRAM(my_owned_store, 100);
 *
 * // In code add an DDM parameter, HTRxITEMP
 * ddm_entry_t * new_ddm = ddm_store__new_entry(&my_owned_store, HTR0ITEMP | DDM2_PARAMETER_INSTANCE(x));
 * if (new_ddm)
 * {
 *      // Initialize DDM with a value
 *      ddm_entry__set__value_u32(new_ddm, 42);
 * }
 * \endcode
 */

#ifndef DDM_STORE_H_
#define DDM_STORE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ddm_entry.h"
#include "sorted_container.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * \brief       Declare and define a DDM store (private to file scope).
 *
 * This macro will statically allocate the DDM store structure and DDM store
 * data.
 *
 * \param       name Name of allocated DDM store structure.
 * \param       size Maximum number of entries in the DDM store.
 */
#define DDM_STORE__DECLARE(name, size)           \
    SORTED_CONTAINER__DECLARE(name##_container,  \
                              size,              \
                              struct ddm_entry); \
    static struct ddm_store name =               \
        {                                        \
            .containers = &name##_container}

/**
 * \brief       Declare and define a DDM store (DDM store structure can be public).
 *
 * This macro will statically allocate the DDM store structure and DDM store
 * data, but the \a name structure will have `extern` linkage.
 *
 * \param       name Name of allocated DDM store structure.
 * \param       size Maximum number of entries in the DDM store.
 */
#define DDM_STORE__DECLARE_PUBLIC(name, size)    \
    SORTED_CONTAINER__DECLARE(name##_container,  \
                              size,              \
                              struct ddm_entry); \
    struct ddm_store name =                      \
        {                                        \
            .containers = &name##_container}

/**
 * \brief       Declare and define a DDM store (private file scope) in EXTRAM.
 *
 * This macro will statically allocate the DDM store structure in internal RAM
 * and DDM store data in EXTRAM.
 *
 * \param       name Name of allocated DDM store structure.
 * \param       size Maximum number of entries in the DDM store.
 */
#define DDM_STORE__DECLARE_EXTRAM(name, size)           \
    SORTED_CONTAINER__DECLARE_EXTRAM(name##_container,  \
                                     size,              \
                                     struct ddm_entry); \
    static struct ddm_store name =                      \
        {                                               \
            .containers = &name##_container}

/**
 * \brief       Declare and define a DDM store (DDM store structure is public).
 *
 * This macro will statically allocate the DDM store structure in internal RAM
 * and DDM store data in EXTRAM. The \a name structure will have `extern`
 * linkage.
 *
 * \param       name Name of allocated DDM store structure.
 * \param       size Maximum number of entries in the DDM store.
 */
#define DDM_STORE__DECLARE_EXTRAM_PUBLIC(name, size)    \
    SORTED_CONTAINER__DECLARE_EXTRAM(name##_container,  \
                                     size,              \
                                     struct ddm_entry); \
    struct ddm_store name =                             \
        {                                               \
            .containers = &name##_container}

/**
 * \brief       DDM store entry
 *
 * This structure is used when you want to setup values of multiple parameters
 * in one function call. Create an array of these structures and pass it to
 * \ref ddm_store__load_entries function.
 */
typedef struct ddm_store_ddm
{
    uint32_t ddm_parameter;  /*!< Parameter ID */
    ddm_entry_value_t value; /*!< Value of the parameter */
} ddm_store_ddm_t;

/**
 * \brief       DDM store handle structure
 *
 * This structure contains necessary data needed to manage DDM store. No members
 * of the structure are ment to be accessed directly. This structure is to be
 * used only when user wants to embed the structure into an another structure,
 * otherwise, the DECLARE macros will do the necessary instantiation of this
 * structure.
 *
 * \note        All members of this structure are private to ddm_store
 *              component.
 */
typedef struct ddm_store
{
    struct sorted_container *containers;
} ddm_store_t;

typedef void(ddm_store__map_fn_t)(ddm_entry_t * ddm_entry, void *map_context);

typedef bool(ddm_store__condition_fn_t)(const ddm_entry_t *const ddm_entry, const void *filter_context);

/**
 * \brief       Manually initialize DDM store
 *
 * This initialization is only needed if you manually allocate \ref ddm_store_t
 * structure. This initialization is otherwise done by the DECLARE macros such
 * as @ref DDM_STORE__DECLARE_EXTRAM_PUBLIC, @ref DDM_STORE__DECLARE_EXTRAM,
 * @ref DDM_STORE__DECLARE_PUBLIC or @ref DDM_STORE__DECLARE.
 *
 * After the initialization the database is empty, no parameter value is defined.
 *
 * \param       ddm_store Points to a \ref ddm_store structure which is to be
 *              initialized.
 * \param       entries Points to \ref sorted_container structure.
 *
 * \pre         Parameter \a ddm_store must be a non-NULL pointer.
 * \pre         Parameter \a entries must be a non-NULL pointer and pointing to
 *              already initialized \ref sorted_container structure.
 */
void ddm_store__init(ddm_store_t *ddm_store, SORTED_CONTAINER *entries);

/**
 * \brief       Terminate previosly initialized DDM store.
 *
 *              This function will terminate all currently allocated instances and then
 *              make the ddm_store invalid so it can not be used after termination.
 *
 * \param       ddm_store Points to a \ref ddm_store structure which is to be
 *              terminated.
 *
 * \pre         Parameter \a ddm_store must be a non-NULL pointer.
 */
void ddm_store__terminate(ddm_store_t *ddm_store);

/**
 * \brief       Create dynamically allocated DDM store.
 *
 * \param       nbr_of_entries is the capacity of DDM store.
 * \pre         Parameter \a nbr_of_entries must be at least one.
 *
 * \return      Pointer to a new memory space containing sorted container initialized
 *              instance.
 * \retval      NULL no available memory is available in the heap memory.
 */
ddm_store_t *ddm_store__create(size_t nbr_of_entries);

/**
 * \brief       Destroy dynamically allocated DDM store instance.
 *
 *              This function will terminate (destroy) all store DDM entries and then
 *              free the memory used by DDM store.
 *
 * \param       ddm_store Pointer to DDM store instance that will be destroyed.
 *
 * \pre         Parameter \a ddm_store must be a non-NULL pointer to the previosly
 *              initialized DDM store instance.
 */
void ddm_store__destroy(ddm_store_t *ddm_store);

/**
 * \brief       Create multiple parameter values
 *
 * The function will create multiple parameters as specified in \a entries array. All created
 * entries will have `has_value` flag set to false.
 *
 * \param       ddm_store DDM store.
 * \param       entries Pointer to an array of \ref ddm_store_ddm structures. Each entry in this
 *              array specifies a parameter and its value. The value is ignored by the function
 *              and entries are created with `has_value` set to false.
 * \param       entries_size Specifies how many entries are in \a entries array.
 * \param       instance Is an instance number returned from Broker. This
 *              instance number will be used when creating all ddm_entry objects
 *              by this function. The instance number provided here is
 *              internally OR-ed with instance number of DDM parameter ID
 *              defined in the @a entries array. By providing 0 (zero) here the
 *              instance number defined in @a entries array is used.
 *
 * \pre         Parameter \a ddm_store must be a non-NULL pointer.
 * \pre         Parameter \a entries must be a non-NULL pointer.
 *
 * \note        The function will fail if the number of stored parameters exceeds maximum number of
 *              parameters of the DDM store. The maximum number is specified at the initialization
 *              stage (either via a function call or by using DECLARE macros).
 * \note        The function will fail if RAM allocation for the string storage is not successful.
 * \note        In the case of string type entries, encountering a null pointer triggers automatic
 *              memory allocation and initialization with an empty string("").
 * \return      Status of operation.
 * \retval      0 - is returned when the operation was successful.
 * \retval      -1 - is returned when the destination store cannot accept new
 *              DDM entries (since it is full).
 */
int ddm_store__create_entries(
    struct ddm_store *ddm_store,
    const struct ddm_store_ddm *entries,
    size_t entries_size,
    uint32_t instance);

/**
 * \brief       Create and initialize multiple parameter values
 *
 * The function will create multiple parameters and initialize values as specified in \a entries
 * array.
 *
 * \param       ddm_store DDM store.
 * \param       entries Pointer to an array of \ref ddm_store_ddm structures. Each entry in this
 *              array specifies a parameter and its value. The function will create entries and
 *              initialize them with specified value.
 * \param       entries_size Specifies how many entries are in \a entries array.
 * \param       instance Is an instance number returned from Broker. This
 *              instance number will be used when creating all ddm_entry objects
 *              by this function. The instance number provided here is
 *              internally OR-ed with instance number of DDM parameter ID
 *              defined in the @a entries array. By providing 0 (zero) here the
 *              instance number defined in @a entries array is used.
 *
 * \pre         Parameter \a ddm_store must be a non-NULL pointer.
 * \pre         Parameter \a entries must be a non-NULL pointer.
 *
 * \note        The function will fail if the number of stored parameters exceeds maximum number of
 *              parameters of the DDM store. The maximum number is specified at the initialization
 *              stage (either via a function call or by using DECLARE macros).
 * \note        The function will fail if RAM allocation for the string storage is not successful.
 * \note        In the case of string type entries, encountering a null pointer triggers automatic
 *              memory allocation and initialization with an empty string("").
 * \return      Status of operation.
 * \retval      0 - is returned when the operation was successful.
 * \retval      -1 - is returned when the destination store cannot accept new
 *              DDM entries (since it is full or the type is not supported).
 */
int ddm_store__load_entries(
    ddm_store_t *ddm_store,
    const ddm_store_ddm_t *entries,
    size_t entries_size,
    uint32_t instance);

/**
 * \brief       Copy entries from source DDM store to destination DDM store
 *
 * \param       ddm_store is the destination DDM store.
 * \param       from_ddm_store is the source DDM store.
 * \return      Status of copy operation.
 * \retval      0 - is returned when the operation was successful.
 * \retval      -1 - is returned when the destination store cannot accept new
 *              DDM entries (since it is full).
 *
 * \pre         Parameter \a ddm_store must be a non-NULL pointer.
 * \pre         Parameter \a entries must be a non-NULL pointer.
 * \pre         Both DDM stores must be previosly initialized either via a function call or by using
 *              DECLARE macros.
 */
int ddm_store__copy_entries(ddm_store_t *ddm_store, const ddm_store_t *from_ddm_store);

/**
 * \brief       Create new data record for a DDM parameter.
 *
 * This function will fail if the paramater is not previosly created with a
 * call to set functions documented above. Once the parameter is created with
 * a set function it can be accessed using this function.
 *
 * \param       ddm_store DDM store.
 * \param       ddm_parameter DDM parameter ID.
 *
 * \return      A pointer to paramater entry structure.
 * \retval      NULL pointer is returned in case no DDM parameter with given
 *              identifier specified in \a ddm_parameter is found.
 *
 * \pre         Parameter \a ddm_store must be a non-NULL pointer.
 */
ddm_entry_t *ddm_store__new_entry(ddm_store_t *ddm_store, uint32_t ddm_parameter);

/**
 * \brief       Delete a data record for the DDM parameter.
 *
 * \param       ddm_store DDM store.
 * \param       ddm_parameter DDM parameter ID.
 *
 * \pre         Parameter \a ddm_store must be a non-NULL pointer.
 */
void ddm_store__delete_entry(ddm_store_t *ddm_store, uint32_t ddm_parameter);

/**
 * \brief       Delete all DDM entries currently stored.
 *
 * \param       ddm_store DDM store.
 *
 * \pre         Parameter \a ddm_store must be a non-NULL pointer.
 */
void ddm_store__delete_all(ddm_store_t *ddm_store);

/**
 * \brief       Delete DDM entries that match the given DDM parameter class
 *
 * \param       ddm_store DDM store.
 * \param       ddm_parameter_class DDM parameter classes that needs to be deleted.
 */
void ddm_store__delete_by_class(ddm_store_t *ddm_store, uint32_t ddm_parameter_class);

/**
 * \brief       Delete DDM entries that match the given DDM parameter class instance
 * 
 * \param       ddm_store DDM store.
 * \param       ddm_parameter_class DDM parameter class instance that needs to be deleted.
 */
void ddm_store__delete_by_class_instance(ddm_store_t *ddm_store, uint32_t ddm_parameter_class_instance);

/**
 * \brief       Access whole data record of a DDM parameter.
 *
 * This function will fail if the paramater is not previosly created with a
 * call to set functions documented above. Once the parameter is created with
 * a set function it can be accessed using this function.
 *
 * \param       ddm_store DDM store.
 * \param       ddm_parameter DDM parameter ID.
 *
 * \return      A pointer to paramater entry structure.
 * \retval      NULL pointer is returned in case no DDM parameter with given
 *              identifier specified in \a ddm_parameter is found.
 *
 * \pre         Parameter \a ddm_store must be a non-NULL pointer.
 */
ddm_entry_t *ddm_store__access(ddm_store_t *ddm_store, uint32_t ddm_parameter);

/**
 * \brief       Get the number of occupied slots in DDM store
 *
 * This function is usually used in setting up the for loops to iterate over
 * DDM store entries.
 *
 * \param       ddm_store DDM store.
 *
 * \return      Number of occupied slots.
 *
 * \pre         Parameter \a ddm_store must be a non-NULL pointer.
 */
size_t ddm_store__occupied(const ddm_store_t *ddm_store);

/**
 * \brief       Iterate over entries in DDM store
 *
 * This function is usually used to iterate over DDM store entries in for loops.
 * By using the \ref ddm_store__occupied() function you iterate index from 0 to
 * N. Pass the DDM store and index and the function will return associated
 * parameter ID and parameter data.
 *
 * \param       ddm_store DDM store.
 * \param       index Specifying an index in range 0 .. \ref ddm_store__occupied
 * \param[out]  entry Parameter entry data.
 *
 * \pre         Parameter \a index must be in range 0 .. up to value returned
 *              by \ref ddm_store__occupied function.
 * \pre         Parameter \a ddm_store must be a non-NULL pointer.
 * \pre         Parameter \a entry must be a non-NULL pointer.
 * \pre         Parameter \a ddm_parameter must be a non-NULL pointer.
 */
void ddm_store__iterate(ddm_store_t *ddm_store, uint32_t index, ddm_entry_t **entry);

/**
 * \brief       Iterate over entries in DDM store (const type)
 *
 * This function is usually used to iterate over DDM store entries in for loops.
 * By using the \ref ddm_store__occupied() function you iterate index from 0 to
 * N. Pass the DDM store and index and the function will return associated
 * parameter ID and parameter data.
 *
 * \param       ddm_store DDM store.
 * \param       index Specifying an index in range 0 .. \ref ddm_store__occupied
 * \param[out]  entry Parameter entry data.
 *
 * \pre         Parameter \a index must be in range 0 .. up to value returned
 *              by \ref ddm_store__occupied function.
 * \pre         Parameter \a ddm_store must be a non-NULL pointer.
 * \pre         Parameter \a entry must be a non-NULL pointer.
 * \pre         Parameter \a ddm_parameter must be a non-NULL pointer.
 */
void ddm_store__iterate_const(const ddm_store_t * ddm_store, uint32_t index, ddm_entry_t ** entry);

/**
 * \brief       Returns capacity of the DDM store in number of elements
 *
 * \param       ddm_store DDM store.
 *
 * \return      Number of elements in the DDM store.
 *
 * \pre			Parameter \a ddm_store must be a non-NULL pointer.
 */
static inline size_t ddm_store__capacity(const ddm_store_t *ddm_store)
{
    return sorted_container__capacity(ddm_store->containers);
}

/**
 * \brief       Map a function to each entry in DDM store
 *
 * The function executes a specified function for each DDM entry in DDM store. The DDM entry
 * and \a map_context is sent to function as parameters.
 *
 * \param       ddm_store DDM store.
 * \param       map_fn Pointer to map function. See \ref ddm_store__map_fn_t for details.
 * \param       map_context Is a void pointer which is just forwarded to map function when it
 *              is executed.
 */
void ddm_store__map(ddm_store_t *ddm_store, ddm_store__map_fn_t *map_fn, void *map_context);

/**
 * \brief       Map a function to only filtered entries in DDM store.
 *
 * The function executes the condition function with \a condition_context as argument. If the
 * condition function retuns `true` the entry is then forwarded to map function. The map function
 * receives the \a map_context argument.
 *
 * \param       ddm_store DDM store.
 * \param       condition_fn Pointer to condition function. See \ref ddm_store__condition_fn_t
 *              for details.
 * \param       condition_context is a void pointer which is just forwarded to condition function when
 *              it is executed.
 * \param       map_fn Pointer to map function. See \ref ddm_store__map_fn_t for details.
 * \param       map_context Is a void pointer which is just forwarded to map function when it
 *              is executed.
 */
void ddm_store__filter_then_map(ddm_store_t *ddm_store, ddm_store__condition_fn_t *condition_fn, const void *condition_context, ddm_store__map_fn_t *map_fn, void *map_context);

/**
 * \brief       Condition function for filtering of a given parameter of any instance
 * 
 * Use this function in conjuction with \ref ddm_store__filter_then_map. The context
 * for this condition function is a pointer to uint32_t integer.
 * 
 * \code
 * uint32_t parameter_id = ...;
 * ddm_store__filter_then_map(&a_ddm_store, ddm_store__condition_by_parameter_any_instance, &parameter_id, ..., ...);
 * \endcode 
 */
bool ddm_store__condition_by_parameter_any_instance(const ddm_entry_t *entry, const void *condition_context);

#ifdef __cplusplus
}
#endif

#endif /* DDM_STORE_H_ */
