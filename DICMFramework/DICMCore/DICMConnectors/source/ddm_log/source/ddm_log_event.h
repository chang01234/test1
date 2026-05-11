#ifndef DDM_LOG_EVENT_H_
#define DDM_LOG_EVENT_H_

#include <stdint.h>
#include "ddm_log_spec_parser.h"
#include "ddm2.h"

 /*  Forward declaration */
struct ddm_log_query_parser__query_data;
struct ddm_log_event__descriptor;

#define MAGIC_NUM                           0x55555555
#define ENTRY_HEADER                        0
#define ENTRY_VALUE                         1
#define ENTRY_HEADER_SIZE                   ( sizeof(ddm_log_event__data_hdr_t) )
#define ENTRY_VALUE_SIZE(entry)             ( (entry)->val.header.value_size )
#define ENTRY_SIZE(entry)                   ( ENTRY_HEADER_SIZE + ENTRY_VALUE_SIZE(entry) )

typedef struct ddm_log_event__data_hdr
{
    uint32_t value_size;  // len of value;
    uint32_t ddm_id;      // parameter id
    uint16_t offset;      // offset from specification
    uint16_t repeat;      // offset from specification
    uint32_t timestamp;   // in seconds
    uint32_t magic_num;   // validity of header
} PACKED ddm_log_event__data_hdr_t;

typedef struct ddm_log_event__data_val_t
{
    ddm_log_event__data_hdr_t header;
    uint8_t value[];
} ddm_log_event__data_val_t;


typedef struct ddm_log_event__data
{
    ddm_log_event__data_val_t val;
} ddm_log_event__data_t;

typedef struct ddm_log_event__functions
{
    int (*ddm_log__write)(struct ddm_log_event__descriptor * descriptor, void * value, uint32_t size);
    int (*ddm_log__read)(struct ddm_log_event__descriptor * descriptor, void * value, uint32_t size, uint8_t type);
    void (*ddm_log__set_read_position)(struct ddm_log_event__descriptor * descriptor);
    int (*ddm_log__all_entries_read)(struct ddm_log_event__descriptor * descriptor);
} ddm_log_event__functions_t;

typedef struct ddm_log_event__descriptor
{
    uint32_t read_pos;   // location were we read next entry
    uint32_t write_pos;  // location of next address to be populated/overriden
    ddm_log_event__functions_t funcs;
} ddm_log_event__descriptor_t;

/**
 * @brief Init event descriptor
 *
 * Creates event descriptor that keeps information about the storages and event logs
 *
 * \return descriptor that stores information about the the storages and event logs
 */
uint32_t ddm_log_event__init(uint32_t memory, uint32_t size);

/**
 * @brief Close event descriptor
 *
 * Closes event descriptor that keeps inforamtion about the storages and event logs
 *
 * @param      descriptor   \a ddm_log_event__descriptor_t
 */
void ddm_log_event__close_descriptor(ddm_log_event__descriptor_t *descriptor);

/**
 * @brief Write entry in memory
 *
 * Log entry should be stored in memory specified in \a ddm_log_specification_data_t struct
 *
 * @param      memory In which memory to write the event
 * @param      data Event data to be written
 *
 * \pre        Parameter \a memory must be a non-NULL pointer
 * \pre        Parameter \a data must be a non-NULL pointer
 */
int ddm_log_event__write(uint32_t memory, ddm_log_event__data_t *data);

/**
 * @brief Read entries from memory
 *
 * \a events is pointer to a array of ddm_log_event__data structures of \a nEvents items
 *
 * \param       memory  from which memory to read
 * \param       descriptor descriptor of the event
 * \param       qdata refers to the output filtering rules. If NULL, no filtering rules are applied
 * \param[out]  events_ptr Pointer to array of \a ddm_log_event__data pointers.
 * \param       numbOfEntries Maximum number of log events to be read
 *
 * \pre         Parameter \a events_ptr must be a non_NULL pointer.
 * \pre         Parameter \a descriptor must be a non-NULL pointer.
 * \pre         Parameter \a qdata might be a NULL pointer.
 * \pre         Parameter \a nEvents must be a non-NULL pointer.
 *
 * \return      Number of entries read
 */
int ddm_log_event__read(const uint32_t memory, struct ddm_log_query_parser__query_data * qdata, ddm_log_event__data_t ** events_ptr, uint32_t numbOfEntries);

/**
 * @brief Checks whether max size requested by user is exceed
 *
 * @param qdata refers to the output filtering rules
 *
 * \pre         Parameter \a qdata must be a non-NULL pointer.
 *
 * @return if true
 */
int ddm_log_event__size_exceeded(const struct ddm_log_query_parser__query_data * qdata);

/**
 * @brief Allocates event's memory
 *
 * It will allocate memory for the whole \a ddm_log_event__data struct,
 * taking into account the size needed for the parameter's value
 *
 * @param value_size  Size of the parameter value
 *
 * @return ddm_log_event__data_t*
 */
ddm_log_event__data_t * ddm_log_event__create(size_t value_size);

/**
 * @brief Deallocates event's memory
 *
 * @param event_log    \a event_log that needs to be deallocated
 */
void ddm_log_event__free(ddm_log_event__data_t * event_log);

#endif // DDM_LOG_EVENT_COMP_H_
