#ifndef PRODUCT_CONF_MANAGER_H
#define PRODUCT_CONF_MANAGER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "configuration.h"

// Maximum string lengths
#ifndef PRODUCT_CONF_MANAGER_MAX_MANUF_NAME_LEN
#define PRODUCT_CONF_MANAGER_MAX_MANUF_NAME_LEN 32
#endif
#ifndef PRODUCT_CONF_MANAGER_MAX_MODEL_NAME_LEN
#define PRODUCT_CONF_MANAGER_MAX_MODEL_NAME_LEN 32
#endif
#ifndef PRODUCT_CONF_MANAGER_MAX_FWID_LEN
#define PRODUCT_CONF_MANAGER_MAX_FWID_LEN 16
#endif
#ifndef PRODUCT_CONF_MANAGER_MAX_REGEX_LEN
#define PRODUCT_CONF_MANAGER_MAX_REGEX_LEN 128
#endif
#ifndef PRODUCT_CONF_MANAGER_MAX_SERIAL_NUM_LEN
#define PRODUCT_CONF_MANAGER_MAX_SERIAL_NUM_LEN 32
#endif
#ifndef PRODUCT_CONF_MANAGER_MAX_MANUF_COUNT
#define PRODUCT_CONF_MANAGER_MAX_MANUF_COUNT 4
#endif
#ifndef PRODUCT_CONF_MANAGER_MAX_MODEL_COUNT
#define PRODUCT_CONF_MANAGER_MAX_MODEL_COUNT 16
#endif
#ifndef PRODUCT_CONF_MANAGER_MAX_FWID_COUNT
#define PRODUCT_CONF_MANAGER_MAX_FWID_COUNT 4
#endif
#ifndef PRODUCT_CONF_MANAGER_MAX_SERIAL_COUNT
#define PRODUCT_CONF_MANAGER_MAX_SERIAL_COUNT 4
#endif

#define PROD_CONF_ERR_OK                0
#define PROD_CONF_ERR_NO_MEMORY         -1
#define PROD_CONF_ERR_ARRAY_EMPTY       -2
#define PROD_CONF_ERR_INVALID_FORMAT    -3
#define PROD_CONF_ERR_FILE_NOT_FOUND    -4
#define PROD_CONF_ERR_FILE_NOT_OPENED   -5
#define PROD_CONF_ERR_MISSING_ITEM      -6
#define PROD_CONF_ERR_JSON_PARSE        -7
#define PROD_CONF_ERR_INVALID_PARAMS    -8
#define PROD_CONF_ERR_JSON_FILE_READ    -9
#define PROD_CONF_ERR_INVALID_FILE_SIZE -10

/**
 * @brief Structure to hold firmware ID query results
 */
typedef struct
{
    char **fwids;  // Array of firmware ID strings
    size_t count;  // Number of firmware IDs
} fwid_query_result_t;

// Structure for version patterns
typedef struct
{
    char fwver_regex[PRODUCT_CONF_MANAGER_MAX_REGEX_LEN];
    char hwver_regex[PRODUCT_CONF_MANAGER_MAX_REGEX_LEN];
    bool has_fwver_regex;
    bool has_hwver_regex;
} version_patterns_t;

// Structure for a single model
typedef struct
{
    char model_name[PRODUCT_CONF_MANAGER_MAX_MODEL_NAME_LEN];
    int type;
    bool has_type;
    char fwid[PRODUCT_CONF_MANAGER_MAX_FWID_COUNT][PRODUCT_CONF_MANAGER_MAX_FWID_LEN];
    uint8_t fwid_count;
    version_patterns_t version_patterns;
    bool has_version_patterns;
} model_info_t;

// Structure for manufacturer configuration
typedef struct
{
    char manuf_names[PRODUCT_CONF_MANAGER_MAX_MANUF_COUNT][PRODUCT_CONF_MANAGER_MAX_MANUF_NAME_LEN];
    uint8_t manuf_count;
    model_info_t models[PRODUCT_CONF_MANAGER_MAX_MODEL_COUNT];
    uint8_t model_count;
    char serial_numbers[PRODUCT_CONF_MANAGER_MAX_SERIAL_COUNT][PRODUCT_CONF_MANAGER_MAX_SERIAL_NUM_LEN];
    uint8_t serial_count;
} manufacturer_config_t;

// Main configuration structure
typedef struct
{
    manufacturer_config_t manufacturers[PRODUCT_CONF_MANAGER_MAX_MANUF_COUNT];
    uint8_t manufacturer_count;
} product_config_t;

// Function prototypes
int product_conf_manager_init(void);
int product_conf_manager_load_file(const char *file_path);
int product_conf_manager_parse_json_string(const char *json_string);
void product_conf_manager_free_config(void);
void product_conf_manager_print_config(void);

// Helper functions for finding configurations
manufacturer_config_t *product_conf_manager_find_manufacturer(const char *manuf_name);
model_info_t *product_conf_manager_find_model(const char *manuf_name, const char *model_name);
bool product_conf_manager_model_supports_fwid(const char *manuf_name, const char *model_name, const char *fwid);
bool product_conf_manager_get_model_fwids(const char *manuf_name, const char *model_name, fwid_query_result_t *fwid_result);
const version_patterns_t *product_conf_manager_get_model_version_patterns(const char *manuf_name, const char *model_name);
int product_conf_manager_extract_fw_version(const char *manuf_name, const char *model_name, const char *version_string, char *result, size_t result_size);
int product_conf_manager_extract_hw_version(const char *manuf_name, const char *model_name, const char *version_string, char *result, size_t result_size);

// Additional query functions
int product_conf_manager_get_manufacturer_count(void);
int product_conf_manager_get_model_count(const char *manuf_name);
int product_conf_manager_get_model_type(const char *manuf_name, const char *model_name, int *type);
bool product_conf_manager_manufacturer_supports_sn(const char *manuf_name, const char *sn);

// Utility functions
int product_conf_manager_validate_regex(const char *regex_pattern);
#if defined(UNITTEST_BUILD)
int product_conf_manager_load_config_file(const char *conf_path);
#endif

#ifdef __cplusplus
}
#endif

#endif  // PRODUCT_CONF_MANAGER_H
