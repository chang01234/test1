/**
 * \file        load_configurations_event_notification.h
 * \date        2025-06-05
 * \author      (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 *
 * \brief       Event manager load configurations.
 *
 * \li          2024-06-05  (NR) Initial implementation
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

#ifndef LOAD_CONFIGURATIONS_EVENT_NOTIFICATION_H_
#define LOAD_CONFIGURATIONS_EVENT_NOTIFICATION_H_

#include "load_configurations.h"

#define LOAD_CONFIGURATIONS__DESCRIPTOR_EVENT_NOTIFICATION
#define LOAD_CONFIGURATIONS__DESCRIPTOR_EVENT_NOTIFICATION_NAME event_notification__descriptor

extern const struct load_configurations__descriptor event_notification__descriptor;

int connector_event_notification_load_configurations(const struct load_configurations__configuration *config);

#endif  // LOAD_CONFIGURATIONS_EVENT_NOTIFICATION_H_
