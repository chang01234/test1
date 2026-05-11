/*
 * gateway.h
 *
 *  Created on: 3 feb. 2022
 *      Author: Andlun
 */

#ifndef DICMFRAMEWORK_DICMBROKER_SOURCE_GATEWAY_H_
#define DICMFRAMEWORK_DICMBROKER_SOURCE_GATEWAY_H_

#include <stdint.h>

#include "ddm2.h"

void gateway_handle_set(const DDMP2_FRAME *const pframe);
void gateway_handle_subscribe(const DDMP2_FRAME *pframe);
int gateway_handle_fragment_frame(const DDMP2_FRAME *const pframe);
void gateway_update_and_publish_int32(const uint32_t parameter, const int32_t new_value, int32_t *const value_store);
void gateway_reply_int32(const DDMP2_FRAME *const pframe, const int32_t value);
void gateway_reply(const DDMP2_FRAME *const pframe, const void *const value, const size_t value_size);
void gateway_publish(const uint32_t parameter, const void *const value_store, const size_t value_size);
void gateway_publish_int32(const uint32_t parameter, const int32_t value);
void gateway_accept_and_publish_string(const DDMP2_FRAME *const pframe, uint8_t *const value_store, const char *const key, const size_t store_capacity);
void gateway_parameter_update_task(const DDMP2_FRAME *frame);
void gateway_init(void);

#endif /* DICMFRAMEWORK_DICMBROKER_SOURCE_GATEWAY_H_ */
