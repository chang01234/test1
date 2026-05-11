/**
 * \file        event_type_db.h
 * \date        2024-07-05
 * \author      (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 *
 * \brief       Event Type Database interface
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

#ifndef EVENT_TYPE_DB_H_
#define EVENT_TYPE_DB_H_

#include <stdint.h>
#include <stddef.h>
#include "sorted_container.h"
#include "event_type.h"

#define EVENT_TYPE_DB_NO_ERROR              0
#define EVENT_TYPE_DB_ERROR_ALREADY_EXISTS  1
#define EVENT_TYPE_DB_ERROR_NO_MEMORY       2

/**
 * @brief       This is a database containing the event types.
 *
 * After parsing NVS stored JSON file the information is stored in event_type
 * structures which are stored in this database.
 */
typedef struct event_type_db
{
    SORTED_CONTAINER * sc;
} event_type_db_t;

/**
 * @brief       Initialize event type database
 *
 * @param       db is a pointer to event type database instance that needs to be
 *              initialized.
 * @param       sc is a pointer to SORTED_CONTAINER structure that is used by
 *              event type database to store event types.
 *
 * @pre         Parameter @a db must be non-NULL pointer.
 * @pre         Parameter @a sc must be non-NULL pointer.
 */
void event_type_db_init(event_type_db_t * db, SORTED_CONTAINER * sc);

/**
 * @brief       This function inserts an event type into a database.
 *
 * It returns 0 if the insertion is successful, -1 if the type already exists in
 * the database, and -2 if there is not enough memory to allocate for the new
 * event type instance.
 *
 * @param       db The event_type_db instance where the insertion will take
 *              place.
 * @param       event_type A pointer to the event_type to be inserted into the
 *              database.
 * @return      int The function returns @ref EVENT_TYPE_DB_NO_ERROR if the
 *              insertion is successful, @ref EVENT_TYPE_DB_ERROR_ALREADY_EXISTS
 *              if the type already exists, and
 *              @ref EVENT_TYPE_DB_ERROR_NO_MEMORY if there is not enough memory.
 *
 * @pre         Parameter @a db must be non-NULL pointer.
 * @pre         Parameter @a event_type must be non-NULL pointer.
 */
int event_type_db_insert(event_type_db_t * db, const event_type_t * event_type);

/**
 * @brief       Returns how many event types are stored in the database.
 *
 * @param       db is a pointer to event type database.
 *
 * @return      Number of event types stored in database.
 *
 * @pre         Parameter @a db must be non-NULL pointer.
 */
size_t event_type_db_occupied(const event_type_db_t * db);

/**
 * @brief       Iterate over event type instances which are stored in the
 *              database.
 *
 * @param       db is a pointer to event type database.
 * @param       index is an index of stored type. This index must be smaller
 *              than the number returned by @ref event_type_db_occupied.
 *
 * @return      Returns event type entry indexed by @a index.
 *
 * @pre         Parameter @a db must be non-NULL pointer.
 * @pre         Value of index must be smaller than the value returned by
 *              @ref event_type_db_occupied function.
 */
const event_type_t * event_type_db_iterate(const event_type_db_t * db, uint32_t index);

/**
 * @brief       Find an event type instance by @a type.
 *
 * @param       db is the poiner to event type database to query.
 * @param       type is the type of event to query for.
 * @return      Returns a pointer to the queried event type, or NULL if the type
 *              is not found.
 *
 * This function takes an event type database and a type as input, and returns
 * a pointer to the corresponding event type in the database. If the event type
 * is not found in the database, the function returns NULL. The database is
 * implemented as a sorted container, allowing for efficient access of event
 * types.
 */
const event_type_t * event_type_db_find_by_type(const event_type_db_t * db, uint32_t type);

#endif /* EVENT_TYPE_H_ */