/**
 * \file
 * \date        2021-10-08
 * \author      (NR) Nenad Radulovic (nenad.radulovic@gmail.com)
 * \brief       Software simulator
 *
 * Software simulator component allows you to simulate a device connected to a
 * connector. The definition of simulated connector is done exclusively through
 * the use of descriptor structure.
 *
 * In order to create a soft simulator connector the following components needs
 * to be defined in separate compilation unit:
 * 1. Connector
 * 2. DDM `owned` DDM parameters store (optional)
 * 3. DDM `owned` DDM parameters store initial values array (optinal)
 * 4. DDM `other` DDM parameters store (optional)
 * 5. DDM `other` DDM parameters store initial values array
 * 6. Simulator actions array and functions (optional)
 * 7. Simulator generators array and functions (optional)
 * 8. Simulator handle structure
 * 9. Simulator description structure connecting all the part mentioned above.
 *
 * \li          2021-10-26  (NR) Initial implementation
 * \li          2022-07-08  (NR) Refactor of software simulator, add generators and actions
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

#ifndef SOFTWARE_SIMULATOR_H_
#define SOFTWARE_SIMULATOR_H_

#include <stdint.h>
#include <stddef.h>

#include "connector.h"
#include "ddm_store.h"
#include "ddm_entry.h"
/**
 * \brief       Action execution period.
 */
#define SOFTWARE_SIMULATOR__GENERATORS_EXECUTION_PERIOD_MS      100

/**
 * \brief       Operation completed successfully
 */
#define SOFTWARE_SIMULATOR__OPS_SUCCESS         1

/**
 * \brief       Operation was not completed successfully
 *
 */
#define SOFTWARE_SIMULATOR__OPS_FAILURE         0

/*
 * \brief       Maximum number of DDM parameters in action trigger sections
 */
#define SOFTWARE_SIMULATOR__ACTION_MAX_TRIGGERS 8

/*
 * \brief       Set DDM parameter for Broker set
 */
#define SOFTWARE_SIMULATOR__BROKER_SET          0x1

/* Forward declarations */
struct software_simulator;
struct software_simulator__generator_arguments;
struct software_simulator__generator_entry;
struct software_simulator__action_entry;

/**
 * \brief       Describe a soft simulated connector
 *
 * This structure describes a soft simulated connector. The structure should be allocated in ROM
 * space. In order to create a simulated connector the following must be specified:
 *
 * - worker_stack_size: The simulated connector uses a separate thread to process the broker queries.
 *                      Specify here how big the stack should be for the thread.
 * - worker_priority: Specify the worker thread priority that will be used during thread creation.
 * - ddm_owned_store: Mutable values of DDM parameters are stored in DDM store. Along with parameter
 *                    values metadata is stored which describes if a parameter is subscribed to or
 *                    not. This store contains only DDM parameters owned by connector. Set to NULL
 *                    when not used.
 * - ddm_owned_initial_values: Pointer to an array of structures that holds initial values of DDM
 *                             parameters. This member is mandatory since it is also used to create
 *                             entries into DDM store which contains owned DDM parameters. Set to
 *                             NULL when not used.
 * - ddm_owned_initial_values_size: Specifies the size of \a ddm_initial_value array. Usualy
 *                                  \ref ELEMENTS macro is used to calculate array size. Set to 0
 *                                  when not used.
 * - ddm_other_store: Mutable values of DDM parameters are stored in DDM store. Along with parameter
 *                    values metadata is stored which describes if a parameter is subscribed to or
 *                    not. This store contains only DDM parameters not owned by connector but
 *                    subscribed to. Set to NULL when not used.
 * - ddm_other_initial_values: Pointer to an array of structures that holds initial values of DDM
 *                             parameters. This member is mandatory since it is also used to create
 *                             entries into DDM store which contains non-owned DDM parameters. Set
 *                             to NULL when not used.
 * - ddm_other_initial_values_size: Specifies the size of \a ddm_initial_value array. Usualy
 *                                  \ref ELEMENTS macro is used to calculate array size. Set to 0
 *                                  when not used.
 * - generators: Pointer to an array of structures that holds which generators will be executed over
 *               specified DDM parameters. Alongside action type you can also put generator
 *               arguments, if the used generator handler supports arguments. Set to NULL when not
 *               used.
 * - generators_size: Specifies the size of \a generators array. Usualy \ref ELEMENTS macro is used
 *                    to calculate array size. Set to 0 when not used.
 * - actions: Pointer to an array of structures that holds which action will be executed over
 *            specified DDM parameters. Set to NULL when not used.
 * - actions_size: Specifies the size of \a actions array. Usualy \ref ELEMENTS macro is used to
 *                 calculate array size. Set to 0 when not used.
 * - args: Shared arguments or workspace for all actions. Set to NULL when not used.
 * - ddm_class: Specify here the class of DDM parameter. The class is used during the registration
 *              process to make the connector_sim owner of all DDM parameters specified in
 *              \a ddm_initial_values.
 * - name: Name of the configuration.
 *
 * \note        All members of this structure are mandatory.
 */
typedef struct software_simulator__descriptor
{
    size_t worker_stack_size;
    uint32_t worker_priority;
    ddm_store_t * ddm_owned_store;
    const ddm_store_ddm_t * ddm_owned_initial_values;
    size_t ddm_owned_initial_values_size;
    ddm_store_t * ddm_other_store;
    const ddm_store_ddm_t * ddm_other_initial_values;
    size_t ddm_other_initial_values_size;
    const struct software_simulator__generator_entry * generators;
    size_t generators_size;
    const struct software_simulator__action_entry * actions;
    size_t actions_size;
    void * args;
    uint32_t ddm_class;
    const char * name;
    bool (*hook_function)(DDMP2_FRAME *);
} SOFTWARE_SIMULATOR__DESCRIPTOR;

/**
 * \brief       Simulation generator handler
 *
 * Generators are executed periodically. The execution period is fixed and defined by
 * \ref SOFTWARE_SIMULATOR__GENERATORS_EXECUTION_PERIOD_MS. If the generator needs to be executed
 * less often use \ref software_simulator__current_cycle_match function.
 *
 * The simulation generator handler receives the following parameters:
 * - pointer to software simulator instance
 * - pointer to `ddm_entry_t` which is referencing a simulated DDM
 * - specific generator handler arguments provided by simulation generators array
 * - global argument provided to software simulator in description structure, see `args` in
 *   \ref SOFTWARE_SIMULATOR__DESCRIPTOR
 *
 * The generators can get input value(s):
 * 1. from DDM store(s), both `owned` and `other` stores,
 * 2. from generator arguments or
 * 3. from global argument in description structure.
 *
 * To modify the associated DDM parameter value use functions from `ddm_entry` module.
 *
 * To get a value of DDM parameter listed in simulated DDM store use functions from `ddm_store` to
 * get DDM entry and using functions from `ddm_entry` you can get a value.
 *
 * Generator arguments are specified per generator. Each generator may have a different parameter.
 * For details see \ref SOFTWARE_SIMULATOR__GENERATOR_ARGUMENTS.
 *
 * There is one global argument which is set at initialization stage and then passed to each
 * generator. This pointer might point to some custom structure which is shared amongst all
 * generators of a simulated connector. The global argument is set per simulated connector. If there
 * is need that two simulated connectors needs to share data, then set this argument can point to
 * the same structure in both connectors (taking into account that this structure needs to be
 * protected from concurrent access of multiple threads).
 */
typedef void (* SOFTWARE_SIMULATOR__GENERATOR_HANDLER)(
    struct software_simulator * /* connector_wrapper */,
    ddm_entry_t * /* ddm_entry */,
    const struct software_simulator__generator_arguments /* arguments */,
    void * /* global argument */);

/**
 * \brief   Arguments for generator
 *
 * Generator arguments are specified per action. Each generator may have a different arguments.
 * Generator arguments are specified as three groups of types:
 * 1. array type - which is an array containing 4 8-bit unsigned values
 * 2. integral type - which is a single 32-bit unsigned integer
 * 3. void pointer type - which is a pointer that can point to any custom structure needed by the
 *    generator.
 */
typedef struct software_simulator__generator_arguments
{
    union p__parameter_types
    {
        uint8_t array[4];
        uint32_t integral;
        void * any;
    } type;
} SOFTWARE_SIMULATOR__GENERATOR_ARGUMENTS;

/**
 * \brief   Entry for an generator in simulated specification array.
 *
 * In each entry, specify:
 * 1. the DDM parameter name which will be used by generator to modify/access the value.
 * 2. generator handler function.
 * 3. each generator might receive a different parameter. See
 *    \ref SOFTWARE_SIMULATOR__GENERATOR_ARGUMENTS for details.
 */
typedef struct software_simulator__generator_entry
{
	uint32_t ddm_parameter;
    SOFTWARE_SIMULATOR__GENERATOR_HANDLER handler;
    SOFTWARE_SIMULATOR__GENERATOR_ARGUMENTS parameters;
} SOFTWARE_SIMULATOR__GENERATOR_ENTRY;

/**
 * \brief       Simulation action handler
 *
 * Actions are executed only when a DDM which is listed in trigger section is updated.
 *
 * The simulation generator handler receives the following parameters:
 * - pointer to software simulator instance
 * - pointer to `ddm_entry_t` which is referencing a simulated DDM
 * - global argument provided to software simulator in description structure, see `args` in
 *   \ref SOFTWARE_SIMULATOR__DESCRIPTOR
 *
 * The actions can get input value(s):
 * 1. from DDM store(s), both `owned` and `other` stores or
 * 3. from global argument in description structure.
 *
 * To modify the associated DDM parameter value use functions from `ddm_entry` module.
 *
 * To get a value of DDM parameter listed in simulated DDM store use functions from `ddm_store` to
 * get DDM entry and using functions from `ddm_entry` you can get a value.
 *
 * There is one global argument which is set at initialization stage and then passed to each
 * generator. This pointer might point to some custom structure which is shared amongst all
 * generators of a simulated connector. The global argument is set per simulated connector. If there
 * is need that two simulated connectors needs to share data, then set this argument can point to
 * the same structure in both connectors (taking into account that this structure needs to be
 * protected from concurrent access of multiple threads).
 */
typedef bool (* SOFTWARE_SIMULATOR__ACTION_HANDLER)(
    struct software_simulator * /* connector_wrapper */,
    ddm_entry_t * /* ddm_entry */,
    void * /* global arguments */);

/**
 * \brief       Simulation action trigger
 *
 * Trigger specifies when to execute an action. Put in the trigger list DDM parameters which are
 * needed to evaluate an action. Listed DDM parameters must be part of `owned` or `other` stores.
 */
typedef struct software_simulator__action_trigger
{
    uint32_t any[SOFTWARE_SIMULATOR__ACTION_MAX_TRIGGERS];
} SOFTWARE_SIMULATOR__ACTION_TRIGGER;

/**
 * \brief       Entry for an action in simulated specification array.
 *
 * In each entry, specify:
 * 1. the DDM parameter name which will be used by action to modify/access the value.
 * 2. generator handler function.
 * 3. each action is executed only when any of the specified DDM parameter is updated.
 */
typedef struct software_simulator__action_entry
{
	uint32_t ddm_parameter;
    SOFTWARE_SIMULATOR__ACTION_TRIGGER trigger;
    SOFTWARE_SIMULATOR__ACTION_HANDLER handler;
} SOFTWARE_SIMULATOR__ACTION_ENTRY;

/**
 * \brief       Soft simulator connector handle structure
 *
 * This structure contains runtime data necessary to run the simulated connector.
 *
 * \note        All members of this structure are private to connector_sim component.
 */
typedef struct software_simulator
{
    const SOFTWARE_SIMULATOR__DESCRIPTOR * p__description;
    ddm_store_t * p__ddm_owned_store;
    ddm_store_t * p__ddm_other_store;
    CONNECTOR * p__connector;
    uint32_t p__connector_instance;
    struct software_simulator__generator_exec
    {
        uint32_t p__t_prev;
        uint32_t p__current_cycle;
    } p__generator_exec;
    int32_t instance;
    bool (*hook_function)(DDMP2_FRAME *frame);
} SOFTWARE_SIMULATOR;

/**
 * \brief       Initialize and start the soft simulator connector
 *
 * \param       software_simulator Pointer to \ref software_simulator handle structure.
 * \param       description Pointer to \ref software_simulator__descriptor description structure.
 *
 * \return      Status of operation.
 * \retval      0 - in case of an error.
 * \retval      1 - in case when operation was successful.
 *
 * \pre         Parameter \a software_simulator must be a non-NULL pointer.
 * \pre         Parameter \a description must be a non-NULL pointer.
 */
int software_simulator__init(
    SOFTWARE_SIMULATOR * software_simulator,
    CONNECTOR * connector,
    const SOFTWARE_SIMULATOR__DESCRIPTOR * description);

/**
 * \brief       Get `owned` store from software simulator
 *
 * \param       software_simulator Pointer to \ref software_simulator handle structure.
 * \return      ddm_store * pointer to `owned` DDM store.
 * \pre         Parameter \a software_simulator must be a non-NULL pointer.
 */
static inline
ddm_store_t * software_simulator__owned_store(SOFTWARE_SIMULATOR * software_simulator)
{
    return software_simulator->p__ddm_owned_store;
}

/**
 * \brief       Get `other` store from software simulator
 *
 * \param       software_simulator Pointer to \ref software_simulator handle structure.
 * \return      ddm_store * pointer to `other` DDM store.
 * \pre         Parameter \a software_simulator must be a non-NULL pointer.
 */
static inline
ddm_store_t * software_simulator__other_store(SOFTWARE_SIMULATOR * software_simulator)
{
    return software_simulator->p__ddm_other_store;
}

/**
 * \brief       Get the name of software simulator
 *
 * \param       software_simulator Pointer to \ref software_simulator handle structure.
 * \return      Pointer to string containing software simulator name
 * \pre         Parameter \a software_simulator must be a non-NULL pointer.
 */
static inline
const char * software_simulator__name(const SOFTWARE_SIMULATOR * software_simulator)
{
    return software_simulator->p__connector->name;
}

/**
 * \brief       Get the name of software simulator description
 *
 * \param       software_simulator Pointer to \ref software_simulator handle structure.
 * \return      Pointer to string containing software simulator description name
 * \pre         Parameter \a software_simulator must be a non-NULL pointer.
 */
static inline
const char * software_simulator__description_name(const SOFTWARE_SIMULATOR * software_simulator)
{
    return software_simulator->p__description->name;
}

/**
 * \brief       Check if this is a desired execution cycle.
 *
 * Since the generators are called every \ref SOFTWARE_SIMULATOR__GENERATORS_EXECUTION_PERIOD_MS
 * milliseconds, it might be desirable to have an action being executed less often. By using this
 * macro an action might evaluate the time and exit if it still not the time to be executed.
 *
 * In the example below, where we want to execute an action every 1 second, in the action function
 * body, we would have the following check.
 *
 * \param       software_simulator Pointer to \ref software_simulator handle structure.
 * \return      If the current cycle is for running this generator.
 * \retval      true - when the current cycle is valid for running the generator
 * \retval      false - skip this execution cycle
 * \code
 * if (software_simulator__current_cycle_match(software_simulator, 1000))
 * {
 *     // do something in generator
 * }
 * \endcode
 * \pre         Parameter \a software_simulator must be a non-NULL pointer.
 */
static inline
bool software_simulator__current_cycle_match(
    const SOFTWARE_SIMULATOR * software_simulator,
    uint32_t period_ms)
{
    return (software_simulator->p__generator_exec.p__current_cycle %
        (period_ms / SOFTWARE_SIMULATOR__GENERATORS_EXECUTION_PERIOD_MS)) == 0;
}

/**
 * \brief       Set a DDM to other connector over broker
 *
 * \param       software_simulator Pointer to \ref software_simulator handle structure.
 * \param       entry Pointer to a `ddm_entry_t` containing DDM parameter and DDM data.
 * \return      int Operation status
 * \retval      SOFTWARE_SIMULATOR__OPS_SUCCESS - Operation completed successfully
 * \retval      SOFTWARE_SIMULATOR__OPS_FAILURE - Operation did not completed successfully.
 * \pre         Parameter \a software_simulator must be a non-NULL pointer.
 * \pre         Parameter \a entry must be a non-NULL pointer.
 * \note        If the DDM parameter is not containing any value (or type) this function will be a
 *              NOP (no operation) function.
 */
int software_simulator__broker_set(
    const SOFTWARE_SIMULATOR * software_simulator,
    const ddm_entry_t * entry);

/**
 * \brief       Publish a DDM to other connector over broker
 *
 * \param       software_simulator Pointer to \ref software_simulator handle structure.
 * \param       entry Pointer to a `ddm_entry_t` containing DDM parameter and DDM data.
 * \return      int Operation status
 * \retval      SOFTWARE_SIMULATOR__OPS_SUCCESS - Operation completed successfully
 * \retval      SOFTWARE_SIMULATOR__OPS_FAILURE - Operation did not completed successfully.
 * \pre         Parameter \a software_simulator must be a non-NULL pointer.
 * \pre         Parameter \a entry must be a non-NULL pointer.
 */
int software_simulator__broker_publish(
    const SOFTWARE_SIMULATOR * software_simulator,
    const ddm_entry_t * entry);

/**
 * \brief       Subscribe to a DDM to other connector over broker
 *
 * \param       software_simulator Pointer to \ref software_simulator handle structure.
 * \param       entry Pointer to a `ddm_entry_t` containing DDM parameter and DDM data.
 * \return      int Operation status
 * \retval      SOFTWARE_SIMULATOR__OPS_SUCCESS - Operation completed successfully
 * \retval      SOFTWARE_SIMULATOR__OPS_FAILURE - Operation did not completed successfully.
 * \pre         Parameter \a software_simulator must be a non-NULL pointer.
 * \pre         Parameter \a entry must be a non-NULL pointer.
 */
int software_simulator__broker_subscribe(
    const SOFTWARE_SIMULATOR  * software_simulator,
    const ddm_entry_t * entry);

#endif /* SOFTWARE_SIMULATOR_H_ */
