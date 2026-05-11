#ifndef DDM_LOG_H_
#define DDM_LOG_H_

#include <stdint.h>
#include <stddef.h>
#include <time.h>

#include "connector.h"
#include "ddm2.h"
#include "ddm_log_spec_parser.h"

/* Forward declarations */
struct ddm_store;
struct ddm_store_ddm;

#define DDM_LOG__SPEC_MAX                16

/**
 * \brief       Describe a ddm log connector
 *
 * This structure describes a ddm log connector. The structure should be
 * allocated in ROM space. In order to create a ddm log connector the
 * following must be specified:
 *
 * - worker_stack_size: connector uses a separate thread to process the
 *                      broker queries. Specify here how big the stack
 *                      should be for the thread.
 * - worker_priority: Specify the worker thread priority that will be used
 *                    during thread creation.
 * - ddm_store_owned: Mutable values of DDM parameters owned by connector_ddm_log
 *                    are stored in DDM store. Along with parameter values metadata
 *                    is stored which describes if a parameter is subscribed to ot not.
 * - ddm_initial_owned_values: Pointer to an array of structures that holds initial
 *                             values of DDM parameters. This member is mandatory
 *                             since it is also used to create entries into DDM store.
 * - ddm_initial_owned_values_size: Specifies the size of \a ddm_initial_owned_values
 *                            array. Usualy \ref ELEMENTS macro is used to calculate
 *                            array size.
 * - ddm_store_subscribed: Mutable values of DDM parameters that connector_ddm_log is
 *                         subscribed to.
 * - ddm_class: Specify here the class of DDM parameter. The class is used
 *              during the registration process to make the connector_ddm_log
 *              owner of all DDM parameters specified in \a ddm_initial_owned_values.
 * - connector: Pointer to \a CONNECTOR structure instance.
 *
 * \note        All members of this structure are mandatory.
 */
struct ddm_log__descriptor
{
    size_t worker_stack_size;
    uint32_t worker_priority;
    struct ddm_store * ddm_store_owned;
    const struct ddm_store_ddm * ddm_initial_owned_values;
    size_t ddm_initial_owned_values_size;
    struct ddm_store * ddm_store_subscribed;
    uint32_t ddm_class;
    const CONNECTOR * connector;
};

/**
 * \brief       DDM Log connector handle structure
 *
 * This structure contains runtime data necessary to run ddm log
 * connector.
 *
 * \note        All members of this structure are private to connector_ddm_log
 *              component.
 */
struct ddm_log
{
    const struct ddm_log__descriptor * p__description;
    struct ddm_store * p__ddm_store_owned;
    struct ddm_store * p__ddm_store_subscribed;
};

/**
 * \brief       Log specification information used in runtime
 *
 * Keeps additional items than the one stored in flash,
 * which will be used only in runtime
 *
 * - spec_is_valid: If the instance is valid, then specification is valid.
 * - spec_prereq_items: Only when pre required items are set,
 *                      \a ddm_log_spec_parser__spec_data items are stored in flash
 * - time_interval: Keeps the time when new entry is allowed to be inserted.
 * - retention: \a time_unit_s: time unit calculated is seconds
 *              \a ret_period: counted number of time periods in selected time unit
 *              \a n_entries: counted number of entries in selected time unit
 * - spec_entry: Pointer to \a ddm_entry that keeps info about the type
 *               of the parameter that we subscribed to and its last published
 *               value. \a ddm_entry flag is used to store the index that is
 *               used to access the corresponding log specification
 * - data: Pointer to \a ddm_log_spec_parser__spec_data struct that keeps all the
 *         configuration data stored in memory
 *
 */
struct ddm_log__spec
{
    int spec_is_valid;
    int spec_prereq_items;
    time_t time_interval;
    struct ddm_log__spec_ret
    {
        time_t time_unit_s;
        uint32_t ret_period;
        uint32_t n_entries;
    } retention;
    struct ddm_entry * spec_entry;
    struct ddm_log_spec_parser__spec_data * data;
};

typedef struct ddm_log_read_specification_t
{
    uint8_t instance;
    char name[32];
    char *config;
} ddm_log_read_specification_t;

typedef struct ddm_log_specification_t
{
    uint8_t instance;
    char name[32];
    char *config;
    int num_read_configs;
    ddm_log_read_specification_t *p_read_configs;
} ddm_log_specification_t;

/**
 * @brief Get the localtime object
 *
 * @param time_tm
 */
bool ddm_log__get_localtime(struct tm * const tm_time);

/**
 * \brief       Initialize and start the ddm log connector
 *
 * \param       ddm_log Pointer to \ref ddm_log handle structure.
 * \param       description Pointer to \ref ddm_log__descriptor description
 *              structure.
 *
 * \return      Status of operation.
 * \retval      0 - in case of an error.
 * \retval      1 - in case when operation was successful.
 *
 * \pre         Parameter \a ddm_log must be a non-NULL pointer.
 * \pre         Parameter \a description must be a non-NULL pointer.
 */
int ddm_log__init(struct ddm_log * ddm_log, const struct ddm_log__descriptor * description);

#endif /* DDM_LOG_H_ */
