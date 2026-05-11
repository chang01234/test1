/**
 * \file        make_json.h
 * \date        2024-07-09
 * \author      (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 *
 * \brief       Make JSON from event record structure.
 *
 * \li          2024-07-09  (NR) Initial implementation
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

#ifndef MAKE_JSON_H_
#define MAKE_JSON_H_

#include "event_record.h"
#include <stddef.h>

#define MAKE_JSON_NO_ERROR        0
#define MAKE_JSON_ERROR_NO_MEMORY 1
#define MAKE_JSON_ERROR_FAILURE   2

/**
 * @brief       Convert event_record to a JSON string (app side)
 *
 * @param       er is pointer to an event_record that needs to be unparsed to
 *              JSON string.
 * @param       key is a pointer to string buffer containing NULL terminated
 *              string that is used as a key in JSON.
 * @param       device_id is a pointer to string buffer containing NULL terminated
 *              string that is used to reference a device.
 * @param [out] buffer is pointer to allocated buffer where to store generated
 *              string. The buffer size must be of sufficient size to hold the
 *              generated string, otherwise, @ref OPS_ERROR_NO_MEMORY is
 *              returned.
 * @param       buffer_size is the size of @a buffer to avoid buffer overflows.
 * @return      Operation status.
 * @retval      MAKE_JSON_NO_ERROR - is returned when operation completed
 *              successfully.
 * @retval      MAKE_JSON_ERROR_NO_MEMORY - when buffer pointed by @a buffer is
 *              not sufficiently large to hold JSON string.
 * @retval      MAKE_JSON_ERROR_FAILURE - is returned when an internal (stdio)
 *              error happens.
 */
int make_json_event_record(const event_record_t *er, const char *key, const char *device_id, char *buffer, size_t buffer_size);

#endif /* MAKE_JSON_H_ */
