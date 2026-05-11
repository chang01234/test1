#ifndef MQTT_DB_H
#define MQTT_DB_H

#include "iot_client.h"
#include <stdint.h>
#include "ddm2.h"

#define INST1 (256)
#define INST2 (512)

#define NETWORK_THING_MAX_NUMBER	(49)	// Maximal numbers of network things connected to one thing
#define NETWORK_THING_NAME_MAX_SIZE	(5)		// "__48" = NETWORK_THING_MAX_NUMBER
//#define NETWORK_THING_NAME_MAX_SIZE	(14)		// During test of old code
#define THING_NAME_MAX_SIZE 		CONFIG_AWS_IOT_SHADOW_MAX_SIZE_OF_THING_NAME
#define CLOUD_NAME_MAX_SIZE     	(28)
#define MQTT_MAX_SUBSCRIPTIONS_PER_SESSION 50		// AWS MQTT broker limitation

#define MAX_NBR_OF_SUBSCRIPTIONS (CONFIG_AWS_IOT_MQTT_NUM_SUBSCRIBE_HANDLERS)

int mqtt_db_init(void);
int mqtt_db_get_root_cloud_ddm_parameter(char *cloud_name, uint32_t *ddm_par, int cloud_set, int *index);
int mqtt_db_get_cloud_ddm_parameter(char *cloud_name, uint32_t *ddm_par, int cloud_set, int *index);
int mqtt_db_update_value(DDMP2_FRAME *pframe);
int mqtt_db_create_json(char *json, size_t json_size, int root, int next_sub, int min_len, int force_update, int first_publish);
int mqtt_db_has_root_cloud_set_or_prio(int *block);
int mqtt_db_has_cloud_set_or_prio(uint8_t sub_updated[]);
int mqtt_db_get_ddm_parameter_last_value(uint32_t ddm_par, int32_t *value);
int mqtt_db_add_entry_from_get(const char *cloudname, int thing);
int mqtt_move_entries_to_db(void);
int mqtt_get_nbr_of_created_blocks(void);
int32_t mqtt_db_transmitted_kbytes(void);
const char *mqtt_get_networkname(int block);
void mqtt_db_instance_deleted(const uint32_t class_and_instance);
#endif
