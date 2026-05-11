//#pragma warning(disable:4996) //_CRT_SECURE_NO_WARNINGS
/*! \file json.c
	\brief JSON source.

	JSON helper functions.
*/
#include "json.h"

#include <stdint.h>
#include <stdio.h>

#include "ddm.h"
#include "cJSON.h"

//! \~ Generates an announcement json object
char *generate_announcement_json(uint8_t *serial_number, uint8_t *device_name, uint8_t pid)
{
	char *string;
	cJSON *root = cJSON_CreateObject();

	cJSON_AddNumberToObject(root, "version", DDMP_WIFI_PROTOCOL_VERSION);
	cJSON_AddNumberToObject(root, "pid", pid);
	cJSON_AddStringToObject(root, "id", (char*)serial_number);
	cJSON_AddStringToObject(root, "name", (char*)device_name);

	string = cJSON_Print(root);
	cJSON_Minify(string);
	cJSON_Delete(root);

	return string;
}

//! \~ Generates a JSON object containing a DDMP frame
char *generate_parameter_json(DDMP_FRAME_BUFFER *buffer, size_t size)
{
	char *string;
	cJSON* array;
	cJSON *root = cJSON_CreateObject();

	array = cJSON_AddArrayToObject(root, "ddmp");

	for (size_t index = 0; index < size; index++)
	{
		cJSON *number = cJSON_CreateNumber(((uint8_t*)buffer)[index]);

		cJSON_AddItemToArray(array, number);
	}

	string = cJSON_Print(root);
	cJSON_Minify(string);
	cJSON_Delete(root);

	return string;
}

int parse_announcement(const char *announcement, char *destination)
{
	cJSON *root, *name, *id, *pid;

	root = cJSON_Parse(announcement);
	if (root == NULL)	//Something went wrong in parsing
	{
		return 0;
	}

	name = cJSON_GetObjectItemCaseSensitive(root, "name");
	id = cJSON_GetObjectItemCaseSensitive(root, "id");
	pid = cJSON_GetObjectItemCaseSensitive(root, "pid");

	sprintf(destination,"%s (pid:%02x, id:%s)",name->valuestring,pid->valueint,id->valuestring);

	cJSON_Delete(root);
	return 1;
}

//const char *error_ptr = cJSON_GetErrorPtr();
int parse_json(const char *json, DDMP_FRAME *frame)
{
	cJSON *root;
	const cJSON *ddmp;
	const cJSON *value;
	DDMP_FRAME_BUFFER buffer;
	uint8_t *frame_ptr = (uint8_t*)buffer;
	uint8_t frame_pos;

	root = cJSON_Parse(json);
	if (root == NULL)	//Something went wrong in parsing
	{		
		return 0;
	}
	ddmp = cJSON_GetObjectItemCaseSensitive(root, "ddmp");	//Search for ddmp object

	if (!cJSON_IsArray(ddmp))
	{
		cJSON_Delete(root);
		return 0;
	}

	frame_pos = 0;

	cJSON_ArrayForEach(value, ddmp)				//Loop through all values of characteristic
	{
		if (!cJSON_IsNumber(value))
		{
			cJSON_Delete(root);
			return 0;
		}

		frame_ptr[frame_pos++] = value->valueint;				//Convert to binary representation
	}

	ddmp_unpack(frame, buffer, frame_pos);

	cJSON_Delete(root);
	return 1;
}

//! \~ Finds a JSON object separator (CR) terminating a transmission
int find_separator(uint8_t *buffer,int pos)
{
	for (int i=0;i<pos;i++)
	{
		if (buffer[i]=='\r')
			return i;
	}
	return -1;
}
