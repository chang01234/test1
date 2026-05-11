/*
 *  mqtt_db.c
 *
 *  Created on: 27 March 2020
 *      Author: Stefan.Henningsohn
 */

#include "configuration.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "ddm2_parameter_list.h"
#include "hal_mem.h"
#include "mqtt_db.h"
#include <string.h>
#ifdef AWS_IOT_MODEM_WRAPPER
#include "modem/aws_wrapper/include/local_config.h"
#endif

// #define EXTENDED_LOGS

/* For these strange values see file aws_iot_mqtt_client_common_internal.c */
#define MQTT_PL_MAX_SIZE (CONFIG_AWS_IOT_MQTT_TX_BUF_LEN - 2 /* ? */                \
                          - 2                                /* for QOS > 0 */      \
                          - 1                                /* Header */           \
                          - 2                                /* For size < 16382 */ \
)

#define MQTT_MAX_NBR_OF_BLOCKS (MQTT_MAX_SUBSCRIPTIONS_PER_SESSION - 1)
#define MQTT_BLOCK_SIZE        50
#define MQTT_ROOT_BLOCK_SIZE   100
#define MQTT_TMP_BLOCK_SIZE    100

typedef enum
{
    PUBMODE_TIMER,      // Change is published on a timer base
    PUBMODE_ON_CHANGE,  // Change is published on change
    PUBMODE_WINDOW      // Change is published when outside a predefined window
} publish_mode_t;

typedef enum
{
    PARAM_STATE_NONE,     // No change
    PARAM_STATE_UPDATED,  // Value has changed since last time
    PARAM_STATE_DELETED,  // Parameter has been deleted
} parameter_state_t;

typedef struct
{
    uint32_t ddm_parameter;
    union
    {
        int32_t int32;
        uint32_t uint32;
    } cloud_value;  // Value present in Cloud
    union
    {
        int32_t int32;
        uint32_t uint32;
    } last_value;                 // Published value
    parameter_state_t state;      // Indicates the state of the parameter
    int32_t cloud_set;            // Indicates if value can be changed from the Cloud
    int32_t force_update;         // Indicates if a forced update of the Cloud parameter is needed
    publish_mode_t publish_mode;  // Publish mode
    int32_t thing;
    char cloud_name[CLOUD_NAME_MAX_SIZE];
    void *frame;
} mqtt_db_t;

typedef struct
{
    int used_entries;
    int max_entries;
    char name[NETWORK_THING_NAME_MAX_SIZE];
    mqtt_db_t *ptr;
} mqtt_block_t;

typedef struct
{
    uint32_t ddm_parameter;
    uint32_t gw_parameter;
} mqtt_filter_window_t;

typedef struct
{
    int32_t thing_type_id;
    const char *netw_thing_name;
} mqtt_thing_type_t;

typedef struct
{
    int32_t txb;   // payload in bytes
    int32_t txkb;  // payload in kilobytes
} mqtt_stat_t;

DECLARE_SORTED_LIST_EXTRAM_PTR(ddm_table, MQTT_BLOCK_SIZE *MQTT_MAX_NBR_OF_BLOCKS);
DECLARE_SORTED_LIST_EXTRAM_PTR(ddm_par_cloud_pos, MQTT_BLOCK_SIZE *(MQTT_MAX_SUBSCRIPTIONS_PER_SESSION - 1));
EXT_RAM_ATTR mqtt_block_t mqtt_db_tmp;
static EXT_RAM_ATTR mqtt_block_t mqtt_db_root;
static EXT_RAM_ATTR mqtt_block_t mqtt_db_block[MQTT_MAX_NBR_OF_BLOCKS];
static EXT_RAM_ATTR uint32_t mqtt_thing_updated[MQTT_MAX_SUBSCRIPTIONS_PER_SESSION];
// Mutex protection of database
static SemaphoreHandle_t l_mqtt_database_mutex;

static int nbr_of_blocks = 0;  // Number of created blocks of network things

static EXT_RAM_ATTR char json_desired[200];

static mqtt_stat_t mqtt_stat = {0, 0};

// Static function declarations
static int mqtt_alloc_new_root_db(void);
static void update_payload_statistics(uint32_t bytes_txed);
static int mqtt_alloc_new_block_db(size_t size);
static int ddm_str_lookup(const char *devclass, const char *property, int *index);
static int get_network_thing_type_id(int32_t *id);
static int mqtt_get_ddm_parameter_window(uint32_t ddm_par, int32_t *value);
static void mqtt_set_publish_mode(int index, publish_mode_t *mode);
static int add_ddm_parameter(uint32_t ddm_parameter, int32_t value);
static int add_cloud_pos(uint32_t ddm_parameter, int32_t thing);
static void mqtt_block_alloc(size_t size);
static void mqtt_block_check_alloc(void);
static int mqtt_db_add_root_entry(uint32_t parameter, uint32_t *block_index);
static int mqtt_db_add_entry(uint32_t parameter, uint32_t *block_index);
static void mqtt_get_db_entry(uint32_t parameter, mqtt_db_t **entry);
static int mqtt_cloudname_split(const char *str, int32_t *instance, int *index);

static void update_payload_statistics(uint32_t bytes_txed)
{
    int32_t kbytes;

    mqtt_stat.txb += bytes_txed;

    // Larger than 1k?
    if (mqtt_stat.txb >= 1024)
    {
        // divide by 1024
        kbytes = mqtt_stat.txb >> 10;

        // remove kbytes from txb counter
        mqtt_stat.txb -= kbytes * 1024;

        // Add to statistics
        mqtt_stat.txkb += kbytes;

        // Check if counter has wrapped to negative
        if (mqtt_stat.txkb < 0)
        {
            mqtt_stat.txkb = 0;
        }
    }
}

// Only used for root thing
static int mqtt_alloc_new_root_db(void)
{
    mqtt_db_root.ptr = hal_mem_malloc_prefer(sizeof(mqtt_db_t) * MQTT_ROOT_BLOCK_SIZE, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    assert(mqtt_db_root.ptr != NULL);
    memset(mqtt_db_root.ptr, 0, sizeof(mqtt_db_t) * MQTT_ROOT_BLOCK_SIZE);
    mqtt_db_root.used_entries = 0;
    mqtt_db_root.max_entries = MQTT_ROOT_BLOCK_SIZE;
    mqtt_db_root.name[0] = '\0';

    mqtt_db_tmp.ptr = hal_mem_malloc_prefer(sizeof(mqtt_db_t) * MQTT_TMP_BLOCK_SIZE, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    assert(mqtt_db_tmp.ptr != NULL);
    memset(mqtt_db_tmp.ptr, 0, sizeof(mqtt_db_t) * MQTT_TMP_BLOCK_SIZE);
    mqtt_db_tmp.used_entries = 0;
    mqtt_db_tmp.max_entries = MQTT_TMP_BLOCK_SIZE;
    mqtt_db_tmp.name[0] = '\0';

    return 0;
}

// Only used for network things
static int mqtt_alloc_new_block_db(size_t size)
{
    TRUE_CHECK(xSemaphoreTakeRecursive(l_mqtt_database_mutex, portMAX_DELAY));
    mqtt_db_block[nbr_of_blocks].ptr = hal_mem_malloc_prefer(sizeof(mqtt_db_t) * MAX(MQTT_BLOCK_SIZE, size), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    assert(mqtt_db_block[nbr_of_blocks].ptr != NULL);
    memset(mqtt_db_block[nbr_of_blocks].ptr, 0, sizeof(mqtt_db_t) * MAX(MQTT_BLOCK_SIZE, size));
    mqtt_db_block[nbr_of_blocks].used_entries = 0;
    mqtt_db_block[nbr_of_blocks].max_entries = MAX(MQTT_BLOCK_SIZE, size);
    snprintf(mqtt_db_block[nbr_of_blocks].name, sizeof(mqtt_db_block[nbr_of_blocks].name), "/__%d", nbr_of_blocks);
    nbr_of_blocks++;
    TRUE_CHECK(xSemaphoreGiveRecursive(l_mqtt_database_mutex));
#ifdef EXTENDED_LOGS
    LOG(D, "Nbr of blocks=%d", nbr_of_blocks);
#endif
    return 0;
}

static int ddm_str_lookup(const char *devclass, const char *property, int *index)
{
    for (int i = 0; i < DDM2_PARAMETER_COUNT; i++)
    {
        if ((strcmp(Ddm2_parameter_list_data[i].device_class, devclass) == 0) && (strcmp(Ddm2_parameter_list_data[i].property, property) == 0))
        {
            *index = i;
            return 1;
        }
    }
    return 0;
}

static int mqtt_cloudname_split(const char *str, int32_t *instance, int *index)
{
    int instance_len = 0;
    int first_pos = 0;
    char dev_class[32] = {0};
    char property_str[32] = {0};
    char instance_str[32] = {0};
    int result = 0;

    for (size_t i = 0; i < strlen(str); i++)
    {
        if ((str[i] >= 48) && (str[i] <= 57))
        {
            if (!instance_len)
            {
                first_pos = i;
            }
            instance_len++;
        }
        else if (instance_len != 0)
        {
            break;  // Leave loop
        }
    }

    // printf("first_pos=%d, instance_len=%d\n", first_pos, instance_len);

    // In the case of a non expected cloud name with no number as separator
    // check that we have a length.
    if (instance_len)
    {
        strncpy(dev_class, str, first_pos);
        strncpy(instance_str, &str[first_pos], instance_len);
        strcpy(property_str, &str[first_pos + instance_len]);

        result = ddm_str_lookup(dev_class, property_str, index);

        *instance = atoi(instance_str);

        // printf("device class=%s, property=%s instance=%d\n", dev_class, property_str, *instance);
    }

    return result;
}

// Returning 0 is error
static int get_network_thing_type_id(int32_t *id)
{
    *id = cfg0ntwth;

    return 1;
}

/*
 * This function returns the window setting for a ddm parameter.
 */
static int mqtt_get_ddm_parameter_window(uint32_t ddm_par, int32_t *value)
{
    int index;
    int res = 0;
    if ((index = ddm2_parameter_list_lookup(DDM2_PARAMETER_BASE_INSTANCE(ddm_par))) != -1)
    {
        if (Ddm2_parameter_list_data[index].out_type == DDM2_TYPE_NONE)
        {
            switch (Ddm2_parameter_list_data[index].out_unit)
            {
            case DDM2_UNIT_VOLT:
                res = mqtt_db_get_ddm_parameter_last_value(GW0BATWIN, value);
                break;
            case DDM2_UNIT_DEGC:
                res = mqtt_db_get_ddm_parameter_last_value(GW0TEMPWIN, value);
                break;
            case DDM2_UNIT_HOUR:
                res = mqtt_db_get_ddm_parameter_last_value(GW0REMTWIN, value);
                break;
            case DDM2_UNIT_AMPERE:
                *value = 500;  // 0.5 A, needs to go into a new DDM for GW
                res = 1;
                break;
            default:
                break;
            }
        }
    }

    return res;
}

/* This function sets the publish mode for a parameter.
 * Here we assume that Volt, degree C, Hours or Ampere parameters
 * are using a default window for determination of when to publish.
 * The default window is a GW parameter that can be changed remotely.
 */
static void mqtt_set_publish_mode(int index, publish_mode_t *mode)
{
    if ((Ddm2_parameter_list_data[index].in_type == DDM2_TYPE_NONE) && ((Ddm2_parameter_list_data[index].out_unit == DDM2_UNIT_VOLT) || (Ddm2_parameter_list_data[index].out_unit == DDM2_UNIT_DEGC) || (Ddm2_parameter_list_data[index].out_unit == DDM2_UNIT_HOUR) || (Ddm2_parameter_list_data[index].out_unit == DDM2_UNIT_AMPERE)))
    {
        *mode = PUBMODE_WINDOW;
    }
    else
    {
        *mode = PUBMODE_ON_CHANGE;
    }
}

static int add_ddm_parameter(uint32_t ddm_parameter, int32_t value)
{
    return sorted_list_unique_add(&ddm_table, (SORTED_LIST_KEY_TYPE)ddm_parameter, (SORTED_LIST_VALUE_TYPE)value);
}

static int add_cloud_pos(uint32_t ddm_parameter, int32_t thing)
{
    SORTED_LIST_VALUE_TYPE value;

    if (!sorted_list_unique_get(&value, &ddm_par_cloud_pos, (SORTED_LIST_KEY_TYPE)ddm_parameter, 0))
    {
        return sorted_list_unique_add(&ddm_par_cloud_pos, (SORTED_LIST_KEY_TYPE)ddm_parameter, (SORTED_LIST_VALUE_TYPE)thing);
    }
    else
    {
        LOG(E, "Duplicates found for=0x%x. Not expected!", ddm_parameter);
        return 0;
    }
}

static void mqtt_block_alloc(size_t size)
{
#ifdef EXTENDED_LOGS
    LOG(D, "Block alloc with size=%d", size);
#endif
    mqtt_alloc_new_block_db(size);
}

static void mqtt_block_check_alloc(void)
{
    if (!nbr_of_blocks || (mqtt_db_block[nbr_of_blocks - 1].used_entries >= mqtt_db_block[nbr_of_blocks - 1].max_entries))
    {
#ifdef EXTENDED_LOGS
        LOG(D, "Used entries=%d", mqtt_db_block[nbr_of_blocks - 1].used_entries);
#endif
        mqtt_alloc_new_block_db(MQTT_BLOCK_SIZE);
    }
}

#if 0
// This function will return 1 if any parameter already is stored in db
static int mqtt_check_duplicates(void)
{
	SORTED_LIST_VALUE_TYPE value;
	mqtt_db_t *tmp;
	int duplicate = 0;

    tmp = mqtt_db_tmp.ptr;

	for (int i=0; i < mqtt_db_tmp.used_entries; i++, tmp++)
	{
		if (sorted_list_unique_get(&value, &ddm_par_cloud_pos, (SORTED_LIST_KEY_TYPE)tmp->ddm_parameter, 0))
		{
			
			duplicate = 1;
			break; // Leave loop
		}
	}

	return duplicate;
}
#endif

static int mqtt_db_add_root_entry(uint32_t parameter, uint32_t *block_index)
{
    int par_index;
    mqtt_db_t *tmp;
    char instance_str[4] = {0};

    // Point to next entry
    tmp = mqtt_db_root.ptr;
    tmp += mqtt_db_root.used_entries;

    if ((par_index = ddm2_parameter_list_lookup(DDM2_PARAMETER_BASE_INSTANCE(parameter))) != -1)
    {
        strcpy(tmp->cloud_name, Ddm2_parameter_list_data[par_index].device_class);
        snprintf(instance_str, sizeof(instance_str), "%d", DDM2_PARAMETER_INSTANCE_FIELD(parameter));
        strcat(tmp->cloud_name, instance_str);
        strcat(tmp->cloud_name, Ddm2_parameter_list_data[par_index].property);
        tmp->ddm_parameter = parameter;
        if (tmp->ddm_parameter != GW0UPT)
        {
            tmp->publish_mode = PUBMODE_ON_CHANGE;
        }
        else
        {
            tmp->publish_mode = PUBMODE_TIMER;
        }
        tmp->thing = 0;
        tmp->state = PARAM_STATE_UPDATED;
        // Block index is the index in a block (two LSB) and the block index (upper two MSB)
        // to get a fast lookup in the database.
        // Root db uses currently only one block.
        *block_index = mqtt_db_root.used_entries;
        add_ddm_parameter(tmp->ddm_parameter, *block_index);
        mqtt_db_root.used_entries++;

        LOG(D, "Adding entry=%d for=0x%x", mqtt_db_root.used_entries - 1, tmp->ddm_parameter);
        LOG(D, "Cloudname=%s, thing=%d", tmp->cloud_name, tmp->thing);

        return 1;
    }
    return 0;  // Failure
}

static int mqtt_db_add_entry(uint32_t parameter, uint32_t *block_index)
{
    int ret_value = 0;
    int par_index;
    mqtt_db_t *tmp;
    char instance_str[4] = {0};

    // Make sure we have space for new entry
    mqtt_block_check_alloc();

    TRUE_CHECK(xSemaphoreTakeRecursive(l_mqtt_database_mutex, portMAX_DELAY));

    // Point to next entry
    tmp = mqtt_db_block[nbr_of_blocks - 1].ptr;
    tmp += mqtt_db_block[nbr_of_blocks - 1].used_entries;

    if ((par_index = ddm2_parameter_list_lookup(DDM2_PARAMETER_BASE_INSTANCE(parameter))) != -1)
    {
        strcpy(tmp->cloud_name, Ddm2_parameter_list_data[par_index].device_class);
        snprintf(instance_str, sizeof(instance_str), "%d", DDM2_PARAMETER_INSTANCE_FIELD(parameter));
        strcat(tmp->cloud_name, instance_str);
        strcat(tmp->cloud_name, Ddm2_parameter_list_data[par_index].property);
        tmp->ddm_parameter = parameter;
        mqtt_set_publish_mode(par_index, &tmp->publish_mode);
        tmp->thing = nbr_of_blocks - 1;
        tmp->state = PARAM_STATE_UPDATED;
        // Block index is the index in a block (two LSB) and the block index (bit 16-23)
        // to get a fast lookup in the database. Upper MSB indicates if this is the parameter db.
        // Parmeter db means not zero.
        *block_index = (nbr_of_blocks << 24) | ((nbr_of_blocks - 1) << 16) | mqtt_db_block[nbr_of_blocks - 1].used_entries;
        add_ddm_parameter(tmp->ddm_parameter, *block_index);
        add_cloud_pos(tmp->ddm_parameter, tmp->thing);
        mqtt_db_block[nbr_of_blocks - 1].used_entries++;

        LOG(D, "Adding entry=%d for=0x%x", mqtt_db_block[nbr_of_blocks - 1].used_entries - 1, tmp->ddm_parameter);
        LOG(D, "Cloudname=%s, thing=%d", tmp->cloud_name, tmp->thing);

        ret_value = 1;
    }
    TRUE_CHECK(xSemaphoreGiveRecursive(l_mqtt_database_mutex));
    return ret_value;
}

static void mqtt_get_db_entry(uint32_t parameter, mqtt_db_t **entry)
{
    SORTED_LIST_VALUE_TYPE value;
    mqtt_db_t *tmp = NULL;
    int block;
    int block_index;

    TRUE_CHECK(xSemaphoreTakeRecursive(l_mqtt_database_mutex, portMAX_DELAY));
    // Check if parameter already is handled?
    if (!sorted_list_unique_get(&value, &ddm_table, parameter, 0))
    {
        if (DDM2_IS_GATEWAY_CLASS(parameter))
        {
            if (mqtt_db_add_root_entry(parameter, (uint32_t *)&value))
            {
                tmp = mqtt_db_root.ptr;
                tmp += value;
#ifdef EXTENDED_LOGS
                LOG(D, "Updating root index=%d", value);
#endif
            }
        }
        else
        {
            if (mqtt_db_add_entry(parameter, (uint32_t *)&value))
            {
                block = (value >> 16) & 0x00FF;  // bit 23-16
                block_index = value & 0xFFFF;
                tmp = mqtt_db_block[block].ptr;
                tmp += block_index;
#ifdef EXTENDED_LOGS
                LOG(D, "Updating block=%d, index=%d", block, block_index);
#endif
            }
        }
    }
    else
    {
        if (value & 0xFF000000)
        {
            // Parameter db
            block = (value >> 16) & 0x00FF;  // bit 23-16
            block_index = value & 0xFFFF;
            tmp = mqtt_db_block[block].ptr;
            tmp += block_index;
        }
        else
        {
            // Root db
            tmp = mqtt_db_root.ptr;
            tmp += value;
        }
    }

    *entry = tmp;
    TRUE_CHECK(xSemaphoreGiveRecursive(l_mqtt_database_mutex));
}

/*
 * This function returns the ddm parameter for a specific cloud string.
 */
int mqtt_db_get_root_cloud_ddm_parameter(char *cloud_name, uint32_t *ddm_par, int cloud_set, int *index)
{
    int res = 0;
    mqtt_db_t *tmp;

    // Currently we only have one block.
    // Adapt code to more blocks if needed.
    // for (int i=0; (i<1) && (res==0); i++)
    {
        tmp = mqtt_db_root.ptr;
        for (int j = 0; j < mqtt_db_root.used_entries; j++, tmp++)
        {
            if (strcmp(tmp->cloud_name, cloud_name) == 0)
            {
                *index = j;
                if (cloud_set > -1)
                {
                    tmp->cloud_set = cloud_set;
                }
                *ddm_par = tmp->ddm_parameter;
                res = 1;
                break;  // Found, leave for loop
            }
        }
    }

    return res;
}

/*
 * This function returns the ddm parameter for a specific cloud string.
 */
int mqtt_db_get_cloud_ddm_parameter(char *cloud_name, uint32_t *ddm_par, int cloud_set, int *index)
{
    int res = 0;
    mqtt_db_t *tmp;

    for (int i = 0; (i < nbr_of_blocks) && (res == 0); i++)
    {
        tmp = mqtt_db_block[i].ptr;
        for (int j = 0; j < mqtt_db_block[i].used_entries; j++, tmp++)
        {
            if (strcmp(tmp->cloud_name, cloud_name) == 0)
            {
                *index = j;
                if (cloud_set > -1)
                {
                    tmp->cloud_set = cloud_set;
                }
                *ddm_par = tmp->ddm_parameter;
                res = 1;
                break;  // Found, leave for loop
            }
        }
    }

    return res;
}

/*
 * This function returns the last value for a specific DDM parameter.
 */
int mqtt_db_get_ddm_parameter_last_value(uint32_t ddm_par, int32_t *value)
{
    int res = 0, index;
    mqtt_db_t *tmp;

    if (DDM2_IS_GATEWAY_CLASS(ddm_par))
    {
        tmp = mqtt_db_root.ptr;
        for (int j = 0; j < mqtt_db_root.used_entries; j++, tmp++)
        {
            if (tmp->ddm_parameter == ddm_par)
            {
                if ((index = ddm2_parameter_list_lookup(DDM2_PARAMETER_BASE_INSTANCE(ddm_par))) != -1)
                {
                    if (Ddm2_parameter_list_data[index].out_type == DDM2_TYPE_UINT32_T)
                    {
                        *value = tmp->last_value.uint32;
                    }
                    else
                    {
                        *value = tmp->last_value.int32;
                    }
                }
                else
                {
                    *value = tmp->last_value.int32;
                }
                res = 1;
                break;  // Found, leave for loop
            }
        }
    }
    else
    {
        for (int i = 0; (i < nbr_of_blocks) && !res; i++)
        {
            tmp = mqtt_db_block[i].ptr;
            for (int j = 0; j < mqtt_db_block[i].used_entries; j++, tmp++)
            {
                if (tmp->ddm_parameter == ddm_par)
                {
                    if ((index = ddm2_parameter_list_lookup(DDM2_PARAMETER_BASE_INSTANCE(ddm_par))) != -1)
                    {
                        if (Ddm2_parameter_list_data[index].out_type == DDM2_TYPE_UINT32_T)
                        {
                            *value = tmp->last_value.uint32;
                        }
                        else
                        {
                            *value = tmp->last_value.int32;
                        }
                    }
                    else
                    {
                        *value = tmp->last_value.int32;
                    }
                    break;  // Found, leave for loop
                }
            }
        }
    }

    return res;
}

/*
 * This function returns 1 if a set is initiated from the Cloud
 * and if the parameter has been reported as updated
 * OR
 * if "On Change" is set and parameter has been updated.
 */
int mqtt_db_has_root_cloud_set_or_prio(int *block)
{
    int res = 0;
    mqtt_db_t *tmp;

    // Loop trough all data
    // Currently only one block
    // for (int i=0; i<nbr_of_blocks; i++)
    {
        tmp = mqtt_db_root.ptr;
        for (int j = 0; j < mqtt_db_root.used_entries; j++, tmp++)
        {
            if ((tmp->cloud_set && (tmp->state == PARAM_STATE_UPDATED)) ||                          // Any Cloud update
                ((tmp->publish_mode == PUBMODE_ON_CHANGE) && (tmp->state == PARAM_STATE_UPDATED)))  // Any on change update
            {
                res = 1;
                *block = 0;  // Currently only one block
#ifdef EXTENDED_LOGS
                LOG(D, "Root: Set True for 0x%x, %d, %d, %d", tmp->ddm_parameter, tmp->state, tmp->cloud_set, tmp->publish_mode);
#endif
                break;  // No reason to continue, leave for loop
            }
        }
    }

    return res;
}

/*
 * This function returns 1 if a set is initiated from the Cloud
 * and if the parameter has been reported as updated
 * OR
 * if "On Change" is set and parameter has been updated.
 * OR
 * if last parameter value is outside predefined limits.
 */
int mqtt_db_has_cloud_set_or_prio(uint8_t sub_updated[])
{
    int res = 0;
    int index, result;
    mqtt_db_t *tmp;
    int32_t window;
    int32_t last_value, cloud_value;

    // Loop trough all data
    for (int i = 0; i < nbr_of_blocks; i++)
    {
        tmp = mqtt_db_block[i].ptr;
        for (int j = 0; j < mqtt_db_block[i].used_entries; j++, tmp++)
        {
            if ((tmp->cloud_set && (tmp->state >= PARAM_STATE_UPDATED)) ||                          // Any Cloud update
                ((tmp->publish_mode == PUBMODE_ON_CHANGE) && (tmp->state >= PARAM_STATE_UPDATED)))  // Any on change update
            {
                res = 1;
                // This array always start with root thing as zero.
                // As this function only handles non root things we add
                // one to the index.
                sub_updated[tmp->thing + 1] = 1;
#ifdef EXTENDED_LOGS
                LOG(D, "Set True for %d, 0x%x, %d, %d, %d", tmp->thing, tmp->ddm_parameter, tmp->state, tmp->cloud_set, tmp->publish_mode);
#endif
                break;  // Leave inner loop to check next block
            }
            else if ((tmp->publish_mode == PUBMODE_WINDOW) && (tmp->state >= PARAM_STATE_UPDATED))
            {
                result = mqtt_get_ddm_parameter_window(DDM2_PARAMETER_BASE_INSTANCE(tmp->ddm_parameter), &window);
                index = ddm2_parameter_list_lookup(DDM2_PARAMETER_BASE_INSTANCE(tmp->ddm_parameter));

                if (result && (index != -1))
                {
                    uint8_t out_type = Ddm2_parameter_list_data[index].out_type;
                    last_value = out_type == DDM2_TYPE_UINT32_T ? (int32_t)tmp->last_value.uint32 : tmp->last_value.int32;
                    cloud_value = out_type == DDM2_TYPE_UINT32_T ? (int32_t)tmp->cloud_value.uint32 : tmp->cloud_value.int32;

                    // Check if value is outside window
                    if ((last_value >= (cloud_value + window)) || (last_value <= (cloud_value - window)))
                    {
                        // Check also if not too big change, treated as error or non important
                        if (!((last_value >= (cloud_value + (window * 10))) || (last_value <= (cloud_value - (window * 10)))))
                        {
                            res = 1;
                            // This array always start with root thing as zero.
                            // As this function only handles non root things we add
                            // one to the index.
                            sub_updated[tmp->thing + 1] = 1;
#ifdef EXTENDED_LOGS
                            LOG(D, "Set True for 0x%x, %d, %d, %d", tmp->ddm_parameter, last_value, cloud_value, window);
#endif
                            break;  // Leave inner loop to check next block
                        }
                    }
                }
                else
                {
                    LOG(E, "Failed to get window for=0x%x", tmp->ddm_parameter);
                }
            }
        }
    }

    return res;
}

// This function is called when parsing of a "GET" from Cloud is done.
int mqtt_move_entries_to_db(void)
{
    TRUE_CHECK(xSemaphoreTakeRecursive(l_mqtt_database_mutex, portMAX_DELAY));
    // Alloc new database block
    mqtt_block_alloc(mqtt_db_tmp.used_entries);

    // Copy to database
    memcpy(mqtt_db_block[nbr_of_blocks - 1].ptr, mqtt_db_tmp.ptr, sizeof(mqtt_db_t) * mqtt_db_tmp.used_entries);
    mqtt_db_block[nbr_of_blocks - 1].used_entries = mqtt_db_tmp.used_entries;

    // Clean tmp block
    memset(mqtt_db_tmp.ptr, 0, sizeof(mqtt_db_t) * MQTT_TMP_BLOCK_SIZE);
    mqtt_db_tmp.used_entries = 0;
    TRUE_CHECK(xSemaphoreGiveRecursive(l_mqtt_database_mutex));
    return 1;
}

int mqtt_db_add_entry_from_get(const char *cloudname, int thing)
{
    int index;
    int result = 0;
    mqtt_db_t *tmp;
    int32_t instance;
    int block_index;

    TRUE_CHECK(xSemaphoreTakeRecursive(l_mqtt_database_mutex, portMAX_DELAY));

    // Fill up the tmp database block with data
    tmp = mqtt_db_tmp.ptr;
    tmp += mqtt_db_tmp.used_entries;

    result = mqtt_cloudname_split(cloudname, &instance, &index);
    if (result)
    {
        const DDM2_PARAMETER_LIST_DATA *param_data = &Ddm2_parameter_list_data[index];

        strcpy(tmp->cloud_name, cloudname);
        tmp->ddm_parameter = param_data->parameter + (instance << 8);
        mqtt_set_publish_mode(index, &tmp->publish_mode);

        if (!param_data->cloud)
        {
            // This parameter is not allowed to be in the cloud anymore.
            // Mark as deleted.
            LOG(W, "Parameter %s not allowed in cloud, marking as deleted", cloudname);
            tmp->state = PARAM_STATE_DELETED;
        }

        if (thing != nbr_of_blocks)
        {
            LOG(E, "Mismatch of thing and nbr_of_blocks (%d != %d)", thing, nbr_of_blocks);
        }
        tmp->thing = nbr_of_blocks;
        //  Block index is the index in a block (two LSB) and the block index (upper two MSB)
        //  to get a fast lookup in the database.
        block_index = ((nbr_of_blocks + 1) << 24) | (nbr_of_blocks << 16) | mqtt_db_tmp.used_entries;
        add_ddm_parameter(tmp->ddm_parameter, block_index);
        add_cloud_pos(tmp->ddm_parameter, tmp->thing);
        mqtt_db_tmp.used_entries++;
    }
    else
    {
        LOG(W, "Unknown parameter=%s", cloudname);
    }
    TRUE_CHECK(xSemaphoreGiveRecursive(l_mqtt_database_mutex));

    return result;  // Failure
}

int mqtt_db_update_value(DDMP2_FRAME *pframe)
{
    mqtt_db_t *tmp;
    int index;

    TRUE_CHECK(xSemaphoreTakeRecursive(l_mqtt_database_mutex, portMAX_DELAY));
    mqtt_get_db_entry(pframe->frame.publish.parameter, &tmp);

    if ((tmp != NULL) && (tmp->state == PARAM_STATE_DELETED))
    {
        // Parameter has been marked deleted, do not update
        TRUE_CHECK(xSemaphoreGiveRecursive(l_mqtt_database_mutex));
        return 0;
    }

    if ((index = ddm2_parameter_list_lookup(DDM2_PARAMETER_BASE_INSTANCE(tmp->ddm_parameter))) != -1)
    {
        if ((Ddm2_parameter_list_data[index].out_type == DDM2_TYPE_STRING) ||
            (Ddm2_parameter_list_data[index].out_type == DDM2_TYPE_STRUCT) ||
            (Ddm2_parameter_list_data[index].out_type == DDM2_TYPE_OTHER))
        {
            if (tmp->frame != NULL)
            {
                hal_mem_free(tmp->frame);
            }

            // Alloc space for frame
            tmp->frame = hal_mem_malloc_prefer(pframe->frame_size + DDMP2_METADATA_SIZE, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
            assert(tmp->frame != NULL);
            // Copy frame
            memcpy(tmp->frame, pframe, pframe->frame_size + DDMP2_METADATA_SIZE);
            // Mark as updated
            tmp->state = PARAM_STATE_UPDATED;

            // ESP_LOG_BUFFER_HEXDUMP("RAW1", (char*)pframe, pframe->frame_size+DDMP2_METADATA_SIZE, ESP_LOG_INFO);
            // ESP_LOG_BUFFER_HEXDUMP("RAW2", (char*)tmp->frame, pframe->frame_size+DDMP2_METADATA_SIZE, ESP_LOG_INFO);

#ifdef EXTENDED_LOGS
            // Next is only for debugging
            if ((Ddm2_parameter_list_data[index].out_type == DDM2_TYPE_STRING) && ((pframe->frame_size - 5) < (160 - 1)))
            {
                EXT_RAM_ATTR char str[160];
                memset(str, 0, 160);
                memcpy(str, pframe->frame.publish.value.raw, pframe->frame_size - 5);
                LOG(D, "String saved=%s with length=%d", str, pframe->frame_size - 5);
            }
#endif
        }
        else if (Ddm2_parameter_list_data[index].out_type == DDM2_TYPE_INT32_T)
        {
            if (tmp->last_value.int32 != pframe->frame.publish.value.int32)
            {
                tmp->last_value.int32 = pframe->frame.publish.value.int32;
                tmp->state = PARAM_STATE_UPDATED;
            }
        }
        else if (Ddm2_parameter_list_data[index].out_type == DDM2_TYPE_UINT32_T)
        {
            if (tmp->last_value.uint32 != pframe->frame.publish.value.uint32)
            {
                tmp->last_value.uint32 = pframe->frame.publish.value.uint32;
                tmp->state = PARAM_STATE_UPDATED;
            }
        }
    }
    TRUE_CHECK(xSemaphoreGiveRecursive(l_mqtt_database_mutex));
    return 1;
}

int mqtt_db_create_json(char *json, size_t json_size, int root, int next_sub, int min_len, int force_update, int first_publish)
{
    int par_index;
    mqtt_db_t *tmp;
    char *str;
    int idx;
    int nbr_of_entries = 0;
    int cloud_set_active;
    static EXT_RAM_ATTR char tmpstr[100];
    static EXT_RAM_ATTR char sentry[100];

    char num_str[50];
    int32_t things_type_id;
    int used_entries;

    if (!json)
    {
        return 0;
    }

#ifdef EXTENDED_LOGS
    LOG(I, "root=%d, next_sub=%d, force=%d, first=%d", root, next_sub, force_update, first_publish);
#endif
    json[0] = '\0';
    json_desired[0] = '\0';
    cloud_set_active = 0;

    // The thing shadow is a shadow/mirror of device status.
    // First time a publish is done to a shadow we must publish all parameters.
    if ((next_sub < MQTT_MAX_SUBSCRIPTIONS_PER_SESSION) && !mqtt_thing_updated[next_sub])
    {
        if (root)
        {
            // Root is always first subscription
            next_sub = 0;
        }
        else
        {
            // Check if unreasonable value
            if ((next_sub - 1) >= nbr_of_blocks)
            {
                LOG(W, "Wrong input for subscription=%d", next_sub);
                return 0;
            }
        }

        // Subscription is never written to, mark it as written
        mqtt_thing_updated[next_sub] = 1;
        // force update of all parameters
        force_update = 1;
    }

    if (root)
    {
        // build up json file
        // Connection status "acxn":{"connection_status":2},
        strcpy(json, "{\"state\":{\"reported\":{\"acxn\":{\"connection_status\":2,\"fw_ver\":\"");
        const char *tmp_version = gateway_get_firmware_version();
        strcat(json, tmp_version);
        strcat(json, "\"");
    }
    else
    {
        strcpy(json, "{\"state\":{\"reported\":{\"acxn\":{");
    }
#if 0
	else
	{
		// For root we add latlng as first resource
		strcpy(json, "{\"state\":{\"reported\":{\"latlng\":\"59.3817,17.8449\",");
	}
#endif

    // For first time but not for root thing, publish the thing type
    if (first_publish && (!root))
    {
        if (!get_network_thing_type_id(&things_type_id))
        {
            return 0;
        }

        // build up json file with thingtype info in case never published to
        snprintf(num_str, sizeof(num_str), "\"thingType\":%d", things_type_id);
        strcat(json, num_str);
    }
#ifdef EXTENDED_LOGS
    else
    {
        LOG(W, "First=%d, root=%d", first_publish, root);
    }
#endif
    // Add last
    strcat(json, "},");

    // build up desired json placed last in json
    strcpy(json_desired, "},\"desired\":");

    TRUE_CHECK(xSemaphoreTakeRecursive(l_mqtt_database_mutex, portMAX_DELAY));

    if (root || (next_sub == 0))
    {
        tmp = mqtt_db_root.ptr;
        used_entries = mqtt_db_root.used_entries;
    }
    else
    {
        if (next_sub == 0)
        {
            next_sub = 1;
        }
        tmp = mqtt_db_block[next_sub - 1].ptr;
        used_entries = mqtt_db_block[next_sub - 1].used_entries;
    }

    for (int i = 0; i < used_entries; i++, tmp++)
    {
        if (force_update || (tmp->state >= PARAM_STATE_UPDATED))
        {
#ifdef EXTENDED_LOGS
            LOG(W, "force=%d, state=%d, par=0x%x", force_update, tmp->state, tmp->ddm_parameter);
#endif
            if ((par_index = ddm2_parameter_list_lookup(DDM2_PARAMETER_BASE_INSTANCE(tmp->ddm_parameter))) != -1)
            {
                sentry[0] = '\0';
                idx = nbr_of_entries ? 1 : 0;

                // LOG(D, "Found index=%d, type=%d", par_index, Ddm2_parameter_data[par_index].type);

                if (tmp->state == PARAM_STATE_DELETED)
                {
                    if (idx)
                    {
                        sentry[0] = ',';
                    }
                    sprintf(&sentry[idx], "\"%s\":null", tmp->cloud_name);
                    nbr_of_entries++;
                }
                else if (Ddm2_parameter_list_data[par_index].out_type == DDM2_TYPE_STRING)
                {
                    DDMP2_FRAME *pframe = tmp->frame;
                    if (pframe != NULL)
                    {
                        str = hal_mem_malloc_prefer(pframe->frame_size, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
                        if (str)
                        {
                            memset(str, 0, pframe->frame_size);
                            memcpy(str, (void *)&pframe->frame.publish.value.raw, pframe->frame_size - 5);
                            // ESP_LOG_BUFFER_HEXDUMP("RAW", (char*)pframe, pframe->frame_size + DDMP2_METADATA_SIZE, ESP_LOG_INFO);
                            // ESP_LOG_BUFFER_HEXDUMP("RAW", (char*)&pframe->frame.publish.value.raw, pframe->frame_size-5, ESP_LOG_INFO);

                            if (idx)
                            {
                                sentry[0] = ',';
                            }
                            sprintf(&sentry[idx], "\"%s\":\"%s\"", tmp->cloud_name, str);
                            nbr_of_entries++;
                            hal_mem_free(str);
                        }
                        hal_mem_free(pframe);
                        tmp->frame = NULL;
                    }
                }
                else if (Ddm2_parameter_list_data[par_index].out_type == DDM2_TYPE_OTHER ||
                         Ddm2_parameter_list_data[par_index].out_type == DDM2_TYPE_STRUCT)
                {
                    DDMP2_FRAME *pframe = tmp->frame;
                    if (pframe != NULL)
                    {
                        // Alloc string to double size of the raw data (one byte=two characters)
                        str = hal_mem_malloc_prefer(pframe->frame_size * 2, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
                        if (str)
                        {
                            char tstr[3];
                            uint8_t *u8ptr = pframe->frame.publish.value.raw;

                            memset(str, 0, pframe->frame_size * 2);

                            for (int i = 0; i < pframe->frame_size - 5; i++, u8ptr++)
                            {
                                sprintf(&tstr[0], "%02X", *u8ptr);
                                strcat(str, tstr);
                            }
                            // ESP_LOG_BUFFER_HEXDUMP("RAW", (char*)&pframe->frame.publish.value.raw, pframe->frame_size-5, ESP_LOG_INFO);

                            if (idx)
                            {
                                sentry[0] = ',';
                            }
                            sprintf(&sentry[idx], "\"%s\":\"%s\"", tmp->cloud_name, str);
                            nbr_of_entries++;
                            hal_mem_free(str);
                        }
                        hal_mem_free(pframe);
                        tmp->frame = NULL;
                    }
                }
                else if ((Ddm2_parameter_list_data[par_index].out_type == DDM2_TYPE_INT32_T) ||
                         (Ddm2_parameter_list_data[par_index].out_type == DDM2_TYPE_UINT32_T) ||
                         (Ddm2_parameter_list_data[par_index].out_type == DDM2_TYPE_NONE))
                {
                    uint8_t out_type = Ddm2_parameter_list_data[par_index].out_type;
                    int32_t last_value = (out_type == DDM2_TYPE_UINT32_T) ? (int32_t)tmp->last_value.uint32 : tmp->last_value.int32;
                    int32_t cloud_value = (out_type == DDM2_TYPE_UINT32_T) ? (int32_t)tmp->cloud_value.uint32 : tmp->cloud_value.int32;

                    if ((last_value != cloud_value) || force_update || (tmp->state == PARAM_STATE_UPDATED))
                    {
                        if (idx)
                        {
                            sentry[0] = ',';
                        }

                        if (out_type == DDM2_TYPE_UINT32_T)
                        {
                            tmp->cloud_value.uint32 = tmp->last_value.uint32;
                            sprintf(&sentry[idx], "\"%s\":%u", tmp->cloud_name, tmp->last_value.uint32);
                        }
                        else
                        {
                            tmp->cloud_value.int32 = tmp->last_value.int32;
                            sprintf(&sentry[idx], "\"%s\":%d", tmp->cloud_name, tmp->last_value.int32);
                        }
                        nbr_of_entries++;
                    }
                }

                // LOG(D, "sl=%d, size=%d", strlen(sentry), (MQTT_PL_MAX_SIZE-min_len-strlen(&netw_thing_name[thing][0])-strlen(json_end)-strlen(json)));
                if (tmp->cloud_set)
                {
                    // LOG(D, "Cloud SET for 0x%x", tmp->ddm_parameter);
                    if (!cloud_set_active)
                    {
                        cloud_set_active = 1;
                        // First entry, start correct
                        strcat(json_desired, "{");
                    }
                    else
                    {
                        // Next entry, start with comma
                        strcat(json_desired, ",");
                    }

                    sprintf(tmpstr, "\"%s\":null", tmp->cloud_name);
                    strcat(json_desired, tmpstr);

                    // Clear cloud_set flag
                    tmp->cloud_set = 0;
                }

                if (strlen(sentry) < (MQTT_PL_MAX_SIZE - min_len - NETWORK_THING_NAME_MAX_SIZE - strlen(json_desired) - strlen(json) - 6))  // 6=max of characters for ending json_desired below
                {
                    strcat(json, sentry);
                }
            }

            // Clear updated flag
            tmp->state = PARAM_STATE_NONE;
        }
    }
    TRUE_CHECK(xSemaphoreGiveRecursive(l_mqtt_database_mutex));

    if (!cloud_set_active)
    {
        // The change is not orginated from Cloud
        if (next_sub)
        {
            // If next_sub is non zero it is a network thing, then we should always clear every
            // change that is pending in the Cloud.
            // Here we append to the json_desired container.
            strcat(json_desired, "null}}");
        }
        else
        {
            // We should never clear parameters changed in the Cloud for the root thing.
            // To have this approach we can change parameters in Cloud when the GW is off,
            // and they will be received after first publish to the root thing.
            // Here we overwrite the json_desired string container.
            strcpy(json_desired, "}}}");
        }
    }
    else
    {
        // end json in correct way as set was done from Cloud
        strcat(json_desired, "}}}");
    }

    // LOG(D, "json desired=%s", json_desired);

    // end json file
    strcat(json, json_desired);

    if (nbr_of_entries)
    {
        update_payload_statistics(strlen(json));
        return 1;
    }
    else
    {
        // LOG(W, "No entries for root=%d and sub=%d", root, next_sub);
        return 0;
    }
}

void mqtt_db_instance_deleted(const uint32_t class_and_instance)
{
    mqtt_db_t *tmp;

    TRUE_CHECK(xSemaphoreTakeRecursive(l_mqtt_database_mutex, portMAX_DELAY));

    // Search for the db for parameters belonging to this class and instance
    for (int i = 0; i < nbr_of_blocks; i++)
    {
        tmp = mqtt_db_block[i].ptr;
        for (int j = 0; j < mqtt_db_block[i].used_entries; j++, tmp++)
        {
            if (DDM2_PARAMETER_CLASS_INSTANCE(tmp->ddm_parameter) == class_and_instance)
            {
                LOG(D, "Marked for deletion par=0x%x", tmp->ddm_parameter);
                tmp->state = PARAM_STATE_DELETED;
            }
        }
    }

    TRUE_CHECK(xSemaphoreGiveRecursive(l_mqtt_database_mutex));
}

int mqtt_get_nbr_of_created_blocks(void)
{
    return nbr_of_blocks;
}

int mqtt_db_init(void)
{
    l_mqtt_database_mutex = xSemaphoreCreateRecursiveMutex();
    // Explicit allocation in external memory
    INIT_SORTED_LIST_EXTRAM_PTR(ddm_table);
    INIT_SORTED_LIST_EXTRAM_PTR(ddm_par_cloud_pos);
    mqtt_alloc_new_root_db();

    memset(mqtt_thing_updated, 0, sizeof(uint32_t) * MQTT_MAX_SUBSCRIPTIONS_PER_SESSION);

    nbr_of_blocks = 0;

    return 1;
}

int32_t mqtt_db_transmitted_kbytes(void)
{
    return mqtt_stat.txkb;
}

const char *mqtt_get_networkname(int block)
{
    return (const char *)mqtt_db_block[block].name;
}
