/**
 * \brief Feature database library which stores and caches GROUP<X> parameters
 *        that are used as features.
 */

#ifndef FEATURE_DATABASE_H_
#define FEATURE_DATABASE_H_

#include "configuration.h"

#define FEAT_DB_ERR_MUTEX_NOT_CREATED      -1
#define FEAT_DB_ERR_MEM_ALLOC_FAILED       -2
#define FEAT_DB_ERR_INVALID_DATA           -3
#define FEAT_DB_ERR_CANNOT_WRITE_TO_NVS    -4
#define FEAT_DB_ERR_FEATURE_ALREADY_EXISTS -5
#define FEAT_DB_ERR_UID_NOT_GENERATED      -6
#define FEAT_DB_ERR_INVALID_CONNECTOR_ID   -7
#define FEAT_DB_ERR_INVALID_CLASS_INSTANCE -8

typedef enum
{
    FEAT_DB_FIELD_TYPE = 0,
    FEAT_DB_FIELD_ACTIVE,
    FEAT_DB_FIELD_ENABLE,
    FEAT_DB_FIELD_UID,
    FEAT_DB_FIELD_ID,
    FEAT_DB_FIELD_NAME,
    FEAT_DB_FIELD_INTERFACE_CLASS_INST,
    FEAT_DB_FIELD_RULES,
    FEAT_DB_FIELD_INVALID
} feature_database_field_t;

int32_t feat_db_init(void);
void feat_db_load_cache(GROUP0TYPE_ENUM group_type, uint8_t connector_id);
int32_t feat_db_cache_entry_create(GROUP0TYPE_ENUM group_type, uint8_t connector_id, int32_t id, int32_t enable, int32_t active, int32_t *group_class_instance);
void feat_db_cache_entry_delete(int32_t class_instance);
void feat_db_update_cache(const void *feat_db_field, size_t feat_db_field_size, feature_database_field_t field, int32_t class_instance);
void feat_db_read_cache(feature_database_field_t field, int32_t class_instance, void *data, size_t *data_size);
int32_t feat_db_find_first_by_uid_interface(const char *uid);
int32_t feat_db_find_first_by_class_instance_interface(uint32_t class_instance);
int32_t feat_db_find_first_by_id_and_type(int32_t id, GROUP0TYPE_ENUM group_type);
int32_t feat_db_frame_handler(const DDMP2_FRAME *const p_frame);
void feat_db_update_interface_all(const UPDLINKEDCLASS_T *data, size_t data_size, GROUP0TYPE_ENUM group_type);
void feat_db_get_all_active_ids_of_type(int32_t *data, size_t *size, GROUP0TYPE_ENUM group_type);
void feat_db_get_all_active_instances_of_type(int32_t *data, size_t *size, GROUP0TYPE_ENUM group_type);
void feat_db_get_all_enabled_ids_of_type(int32_t *data, size_t *size, GROUP0TYPE_ENUM group_type);
void feat_db_get_all_enabled_instances_of_type(int32_t *data, size_t *size, GROUP0TYPE_ENUM group_type);
void feat_db_get_all_by_class_instance_interfaces_of_type(uint32_t *data, int32_t *outdata, size_t *size, GROUP0TYPE_ENUM group_type);
#endif /* FEATURE_DATABASE_H_ */
