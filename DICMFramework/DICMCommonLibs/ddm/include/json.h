/*! \file json.c
	\brief JSON header.

	JSON helper functions.
*/

#ifndef JSON_H_
#define JSON_H_

#include <stdint.h>

#include "ddm.h"

#define DDMP_WIFI_PROTOCOL_VERSION	0

char *generate_announcement_json(uint8_t *serial_number, uint8_t *device_name, uint8_t pid);
char *generate_parameter_json(DDMP_FRAME_BUFFER *buffer, size_t size);
int parse_announcement(const char *announcement, char *destination);
int parse_json(const char *json, DDMP_FRAME *frame);
int find_separator(uint8_t *buffer,int pos);

#endif /* JSON_H_ */
