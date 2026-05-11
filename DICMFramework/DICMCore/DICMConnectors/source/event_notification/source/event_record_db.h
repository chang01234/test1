/**
 * \file        event_record_db.h
 * \date        2024-07-05
 * \author      (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 *
 * \brief       Event Record Database interface
 *
 * \li          2024-07-05  (NR) Initial implementation
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

#ifndef EVENT_RECORD_DB_H_
#define EVENT_RECORD_DB_H_

#include "fifo_fixed.h"
#include "sorted_container.h"
#include "event_record.h"

/**
 * @brief       This is a database containing the event records.
 */
typedef struct event_record_db
{
    SORTED_CONTAINER * sc;
    fifo_fixed_t fifo;
} event_record_db_t;

/**
 * @brief       Initialize event_record database
 *
 * Records are kept in database that uses two data structures:
 * - fifo_fixed: to implement FIFO like behaviour
 * - sorted_container: to store event_record instances and allowing quick search
 *
 * @param       db is a pointer to event record database instance that needs to
 *              be initialized.
 * @param       instance_storage is a pointer to sorted container that will
 *              store event record instances.
 * @param       fifo_storage is a pointer to array of `event_id_t` types that is
 *              used to store event record identifiers.
 *
 * @pre         Parameter @a db must be non-NULL pointer.
 * @pre         Parameter @a instance_storage must be non-NULL pointer.
 * @pre         Parameter @a fifo_storage must be non-NULL pointer.
 * @pre         The number of elements in @a fifo_storage must be equal to the
 *              size of sorted container in @a instance_storage.
 */
void event_record_db_init(
        event_record_db_t * db,
        SORTED_CONTAINER * instance_storage,
        event_id_t * fifo_storage);

/**
 * @brief       Insert a event record at the end of event record FIFO.
 *
 * This function will delete the oldest entry in the database when it is full to
 * make space for newest event record.
 *
 * @param       db is a pointer to event record database instance.
 * @param       event_record is a pointer to event record that will be copied to
 *              event record database.
 *
 * @pre         Parameter @a db must be non-NULL pointer.
 * @pre         Parameter @a event_record must be non-NULL pointer.
 */
void event_record_db_insert(event_record_db_t * db, const event_record_t * event_record);

/**
 * @brief       Purge (delete) all contents of the event record database.
 *
 * @param       db is a pointer to event record database instance.
 *
 * @pre         Parameter @a db must be non-NULL pointer.
 */
void event_record_db_purge(event_record_db_t * db);

/**
 * @brief       Find an entry in event record database by event_id.
 *
 * The function iterates over stored event records and searches for a record
 * that has a matching event_id parameter.
 *
 * @param       db is a pointer to event record database instance.
 * @param       event_id is event identifier to search.
 * @return      Matching event_record entry from the database.
 * @retval      NULL - is returned when event record with given @a event_id does
 *              not exists in the database.
 *
 * @pre         Parameter @a db must be non-NULL pointer.
 */
event_record_t * event_record_db_find_by_id(event_record_db_t * db, uint32_t event_id);

/**
 * @brief       Return the number of occupied slots in event record database.
 *
 * @param       db is a pointer to event record database instance.
 *
 * @pre         Parameter @a db must be non-NULL pointer.
 */
size_t event_record_db_occupied(const event_record_db_t * db);

/**
 * @brief       Iterate over inserted event records.
 *
 * @param       db Pointer to event record database.
 * @param       index Iterator index, valid values are from zero (0) up to
 *              occupied value - 1. The occupied value may be fetched by
 *              \ref event_record_db_occupied function.  Item at index 0
 *              represents an item at tail position, while the item at index
 *              N - 1 represents an item at head position.
 *
 * @pre         Parameter @a db must be a non-NULL pointer.
 * @pre         Parameter @a event_record must be a non-NULL pointer.
 *
 * @note        The function will fail if @a index is equal to or greater than
 *              event record database occupied attribute; in this case
 *              @a event_record is set to NULL.
 */
event_record_t * event_record_db_iterate(const event_record_db_t * db, size_t index);

#endif /* EVENT_RECORD_DB_H_ */