/**
 * \file        event_record_db.c
 * \date        2024-07-05
 * \author      (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 *
 * \brief       Event Record Database implementation
 *
 * Implementation of Event Record Database.
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
#include "event_record_db.h"

/* Depends */

void event_record_db_init(
        event_record_db_t * db,
        SORTED_CONTAINER * instance_storage,
        event_id_t * fifo_storage)
{
    db->sc = instance_storage;
    fifo_fixed_init(
        &db->fifo,
        fifo_storage,
        sizeof(event_id_t),
        sorted_container__capacity(instance_storage));
}

void event_record_db_insert(event_record_db_t * db, const event_record_t * event_record)
{
    event_record_t * new_event_record;

    if (fifo_fixed_is_full(&db->fifo))
    {
        event_id_t oldest_event_record_id;
        event_record_t * oldest_event_record;

        /* When event record database is full, we first delete the oldest entry
         */
        /* First we need to get the oldest event_record id */
        fifo_fixed_dequeue(&db->fifo, &oldest_event_record_id);
        /* After getting the ID we need to terminate the event record */
        oldest_event_record = sorted_container__access(db->sc, oldest_event_record_id);
        event_record_terminate(oldest_event_record);
        /* With this id we delete from storage */
        sorted_container__delete(db->sc, oldest_event_record_id);
        /* Now we have room for one event_record, so we can add it below */
    }
    /* Store the event record ID in the FIFO */
    fifo_fixed_enqueue_back(&db->fifo, &event_record->id);
    /* Store the event record data in the sorted container */
    new_event_record = sorted_container__new(db->sc, event_record->id);
    TRUE_CHECK_RETURN(new_event_record != NULL);
    /* Copy contents of event_record into a newly allocated one. */
    event_record_copy(event_record, new_event_record);
}

void event_record_db_purge(event_record_db_t * db)
{
    /* Terminate all event records */
    for (size_t i = 0u; i < sorted_container__occupied(db->sc); i++)
    {
        event_record_t * event_record;
        uint32_t dummy_key;

        sorted_container__iterate(db->sc, i, (void **)&event_record, &dummy_key);
        event_record_terminate(event_record);
    }
    /* First we delete all event record data */
    sorted_container__delete_all(db->sc);
    /* Now delete all event record IDs */
    fifo_fixed_delete_all(&db->fifo);
}

event_record_t * event_record_db_find_by_id(event_record_db_t * db, uint32_t event_id)
{
    return sorted_container__access(db->sc, event_id);
}

size_t event_record_db_occupied(const event_record_db_t * db)
{
    return fifo_fixed_get_occupied_items(&db->fifo);
}

event_record_t * event_record_db_iterate(const event_record_db_t * db, size_t index)
{
    const event_id_t * event_id; // Key is not used since the event_id is already stored.

    /* Start by looking from tail */
    event_id = fifo_fixed_peek(&db->fifo, index);
    return sorted_container__access(db->sc, *event_id);
}
