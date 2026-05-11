/*****************************************************************************
 * \file       connector_system.h
 * \brief      Connector for system services (hardware/others)
 * \copyright  Dometic Group
 *             This source file and the information contained in it are
 *             confidential and proprietary to Dometic Group
 *             The reproduction or disclosure, in whole or in part,
 *             to anyone outside of Dometic Group without the written
 *             approval of a Dometic Group officer under a Non-Disclosure
 *             Agreement is expressly prohibited.
 *
 *             All rights reserved
 *****************************************************************************/
#ifndef CONNECTOR_SYSTEM_H_
#define CONNECTOR_SYSTEM_H_

#include "configuration.h"

#include <time.h>
#include <stddef.h>
#include "connector.h"

#define WDAY_SUNDAY     0
#define WDAY_MONDAY     1
#define WDAY_TUESDAY    2
#define WDAY_WEDNESDAY  3
#define WDAY_THURSDAY   4
#define WDAY_FRIDAY     5
#define WDAY_SATURDAY   6

extern CONNECTOR connector_system;

int connector_system_set_system_time_posix(const time_t posix_time);
bool connector_system_get_local_time(struct tm * const date_time);
size_t connector_system_get_local_time_string(char * const time_string, const size_t time_string_size, struct tm * const local_time);

//---------------------------------------------------------
#endif // CONNECTOR_SYSTEM_H_
