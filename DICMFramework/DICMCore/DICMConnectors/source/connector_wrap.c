/*****************************************************************************
 * \file       connector_wrap.c
 * \brief      Default wrap functions used during unit testing
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
#include <stdint.h>
#include <stdio.h>
#include "connector.h"
extern int __real_connector_send_frame_to_broker(const DDMP2_CONTROL_ENUM control, const uint32_t parameter, const void * value, const uint8_t value_size, uint8_t source_connector, TickType_t timeout);
int __attribute__((weak)) __wrap_connector_send_frame_to_broker(const DDMP2_CONTROL_ENUM control, const uint32_t parameter, const void * value, const uint8_t value_size, uint8_t source_connector, TickType_t timeout)
{
	printf("Weak wrap function\n");
	return __real_connector_send_frame_to_broker(control, parameter, value, value_size, source_connector, timeout);
}


