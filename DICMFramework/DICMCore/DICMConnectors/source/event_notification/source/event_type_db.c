/**
 * \file        event_type_db.c
 * \date        2024-07-05
 * \author      (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 *
 * \brief       Event Type Database implementation
 *
 * Implementation of Event Type Database.
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

/* Implements */
#include "event_type_db.h"

/* Depends */
#include <string.h>

void event_type_db_init(event_type_db_t * db, SORTED_CONTAINER * sc)
{
    db->sc = sc;
}

int event_type_db_insert(event_type_db_t * db, const event_type_t * event_type)
{
    int retval;

    if (sorted_container__access(db->sc, event_type->type))
    {
        retval = EVENT_TYPE_DB_ERROR_ALREADY_EXISTS;
    }
    else
    {
        event_type_t * type_instance = sorted_container__new(db->sc, event_type->type);
        if (type_instance == NULL)
        {
            retval = EVENT_TYPE_DB_ERROR_NO_MEMORY;
        }
        else
        {
            /* Copy the data to instance */
            memcpy(type_instance, event_type, sizeof(*type_instance));
            retval = EVENT_TYPE_DB_NO_ERROR;
        }
    }
    return retval;
}

size_t event_type_db_occupied(const event_type_db_t * db)
{
    return sorted_container__occupied(db->sc);
}

const event_type_t * event_type_db_iterate(const event_type_db_t * db, uint32_t index)
{
    /* We throw away key since it is already stored into event_type->type member */
    uint32_t key;
    void * event_type;

    sorted_container__iterate(db->sc, index, &event_type, &key);
    return event_type;
}

const event_type_t * event_type_db_find_by_type(const event_type_db_t * db, uint32_t type)
{
    return sorted_container__access(db->sc, type);
}