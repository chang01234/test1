#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <regex.h>

#include "configuration.h"
#include "hal_mem.h"
#include "product_conf_manager.h"

#include "cJSON.h"
#include "esp_attr.h"
#include "esp_err.h"
#include "esp_log.h"

#ifndef PRODUCT_CONF_MANAGER_CONF_FILE
static const char conf_spiffs_file_name[] = "/spiffs/prod_conf/conf.json";
#else
static const char conf_spiffs_file_name[] = PRODUCT_CONF_MANAGER_CONF_FILE;
#endif
#if defined(UNITTEST_BUILD)
static const char *conf_spiffs_file_path = conf_spiffs_file_name;
#else
static const char *const conf_spiffs_file_path = conf_spiffs_file_name;
#endif

// Static configuration storage
static EXT_RAM_ATTR product_config_t g_product_config;
static bool g_config_loaded = false;

/**
 * @brief Validate a regex pattern string
 * @param regex_string The regex pattern to validate
 * @return PROD_CONF_ERR_OK if valid, PROD_CONF_ERR_INVALID_PARAMS if invalid
 */
static int validate_regex_pattern(const char *regex_string)
{
    if (!regex_string || (strlen(regex_string) == 0))
    {
        return PROD_CONF_ERR_INVALID_PARAMS;
    }

    regex_t regex;
    int result = regcomp(&regex, regex_string, REG_EXTENDED);

    if (result != 0)
    {
        char error_buffer[256];
        regerror(result, &regex, error_buffer, sizeof(error_buffer));
        LOG(W, "Invalid regex pattern '%s': %s", regex_string, error_buffer);
        return PROD_CONF_ERR_INVALID_PARAMS;
    }

    regfree(&regex);
    LOG(D, "Regex pattern validated: %s", regex_string);
    return PROD_CONF_ERR_OK;
}

/**
 * @brief Initialize the configuration parser
 * @return PROD_CONF_ERR_OK on success, error code on failure
 */
int product_conf_manager_init(void)
{
    LOG(I, "Configuration parser initialized");
    return product_conf_manager_load_file(conf_spiffs_file_path);
}

/**
 * @brief Parse version patterns from JSON object
 * @param version_obj JSON object containing version patterns
 * @param patterns Output structure for version patterns
 * @return PROD_CONF_ERR_OK on success, error code on failure
 */
static int parse_version_patterns(const cJSON *version_obj, version_patterns_t *patterns)
{
    if (!version_obj || !patterns)
    {
        return PROD_CONF_ERR_INVALID_PARAMS;
    }

    memset(patterns, 0, sizeof(version_patterns_t));

    const cJSON *fwver_regex = cJSON_GetObjectItem(version_obj, "fwver_regex");
    if (cJSON_IsString(fwver_regex) && fwver_regex->valuestring)
    {
        // Validate the regex pattern before storing
        if (validate_regex_pattern(fwver_regex->valuestring) == PROD_CONF_ERR_OK)
        {
            strncpy(patterns->fwver_regex, fwver_regex->valuestring, PRODUCT_CONF_MANAGER_MAX_REGEX_LEN - 1);
            patterns->fwver_regex[PRODUCT_CONF_MANAGER_MAX_REGEX_LEN - 1] = '\0';
            patterns->has_fwver_regex = true;
        }
        else
        {
            LOG(W, "Skipping invalid firmware version regex: %s", fwver_regex->valuestring);
        }
    }

    const cJSON *hwver_regex = cJSON_GetObjectItem(version_obj, "hwver_regex");
    if (cJSON_IsString(hwver_regex) && hwver_regex->valuestring)
    {
        // Validate the regex pattern before storing
        if (validate_regex_pattern(hwver_regex->valuestring) == PROD_CONF_ERR_OK)
        {
            strncpy(patterns->hwver_regex, hwver_regex->valuestring, PRODUCT_CONF_MANAGER_MAX_REGEX_LEN - 1);
            patterns->hwver_regex[PRODUCT_CONF_MANAGER_MAX_REGEX_LEN - 1] = '\0';
            patterns->has_hwver_regex = true;
        }
        else
        {
            LOG(W, "Skipping invalid hardware version regex: %s", hwver_regex->valuestring);
        }
    }

    return PROD_CONF_ERR_OK;
}

/**
 * @brief Parse FWID array from JSON
 * @param fwid_array JSON array containing FWID strings
 * @param model Output model structure
 * @return PROD_CONF_ERR_OK on success, error code on failure
 */
static int parse_fwid_array(const cJSON *fwid_array, model_info_t *model)
{
    if (!cJSON_IsArray(fwid_array) || !model)
    {
        return PROD_CONF_ERR_INVALID_PARAMS;
    }

    model->fwid_count = 0;
    const cJSON *fwid_item = NULL;

    cJSON_ArrayForEach(fwid_item, fwid_array)
    {
        if (model->fwid_count >= PRODUCT_CONF_MANAGER_MAX_FWID_COUNT)
        {
            LOG(W, "Maximum FWID count exceeded, truncating");
            break;
        }

        if (cJSON_IsString(fwid_item) && fwid_item->valuestring)
        {
            strncpy(model->fwid[model->fwid_count], fwid_item->valuestring, PRODUCT_CONF_MANAGER_MAX_FWID_LEN - 1);
            model->fwid[model->fwid_count][PRODUCT_CONF_MANAGER_MAX_FWID_LEN - 1] = '\0';
            model->fwid_count++;
        }
        else
        {
            LOG(W, "Invalid fw id string");
        }
    }

    return PROD_CONF_ERR_OK;
}

/**
 * @brief Parse a single model from JSON object
 * @param model_obj JSON object containing model information
 * @param model Output model structure
 * @return PROD_CONF_ERR_OK on success, error code on failure
 */
static int parse_model(const cJSON *model_obj, model_info_t *model)
{
    if (!model_obj || !model)
    {
        return PROD_CONF_ERR_INVALID_PARAMS;
    }

    memset(model, 0, sizeof(model_info_t));

    // Get the first (and should be only) key-value pair from the model object
    const cJSON *model_entry = model_obj->child;
    if (!model_entry)
    {
        return PROD_CONF_ERR_JSON_PARSE;
    }

    // Copy model name
    strncpy(model->model_name, model_entry->string, PRODUCT_CONF_MANAGER_MAX_MODEL_NAME_LEN - 1);
    model->model_name[PRODUCT_CONF_MANAGER_MAX_MODEL_NAME_LEN - 1] = '\0';

    // Parse type field if present, initialize to 0 if not
    model->type = 0;  // Default initialization
    model->has_type = false;
    const cJSON *type_field = cJSON_GetObjectItem(model_entry, "type");
    if (cJSON_IsNumber(type_field))
    {
        model->type = (int)cJSON_GetNumberValue(type_field);
        model->has_type = true;
    }

    // Parse model data
    const cJSON *fwid_array = cJSON_GetObjectItem(model_entry, "fwid");
    if (fwid_array)
    {
        int ret = parse_fwid_array(fwid_array, model);
        if (ret != PROD_CONF_ERR_OK)
        {
            return ret;
        }
    }

    // Parse version patterns if present
    const cJSON *version_patterns_obj = cJSON_GetObjectItem(model_entry, "version_patterns");
    if (version_patterns_obj)
    {
        int ret = parse_version_patterns(version_patterns_obj, &model->version_patterns);
        if (ret == PROD_CONF_ERR_OK)
        {
            model->has_version_patterns = true;
        }
    }

    return PROD_CONF_ERR_OK;
}

/**
 * @brief Parse manufacturer names array
 * @param manuf_array JSON array containing manufacturer names
 * @param config Output manufacturer configuration
 * @return PROD_CONF_ERR_OK on success, error code on failure
 */
static int parse_manufacturer_names(const cJSON *manuf_array, manufacturer_config_t *config)
{
    int ret_val = PROD_CONF_ERR_OK;
    if (!cJSON_IsArray(manuf_array) || !config)
    {
        return PROD_CONF_ERR_INVALID_PARAMS;
    }

    config->manuf_count = 0;
    const cJSON *manuf_item = NULL;

    cJSON_ArrayForEach(manuf_item, manuf_array)
    {
        if (config->manuf_count >= PRODUCT_CONF_MANAGER_MAX_MANUF_COUNT)
        {
            LOG(W, "Maximum manufacturer count exceeded, truncating");
            ret_val = PROD_CONF_ERR_NO_MEMORY;
            break;
        }

        if (cJSON_IsString(manuf_item) && manuf_item->valuestring)
        {
            strncpy(config->manuf_names[config->manuf_count], manuf_item->valuestring, PRODUCT_CONF_MANAGER_MAX_MANUF_NAME_LEN - 1);
            config->manuf_names[config->manuf_count][PRODUCT_CONF_MANAGER_MAX_MANUF_NAME_LEN - 1] = '\0';
            config->manuf_count++;
        }
    }

    return ret_val;
}

/**
 * @brief Parse models array
 * @param model_array JSON array containing model objects
 * @param config Output manufacturer configuration
 * @return PROD_CONF_ERR_OK on success, error code on failure
 */
static int parse_models_array(const cJSON *model_array, manufacturer_config_t *config)
{
    int ret_val = PROD_CONF_ERR_OK;

    if (!cJSON_IsArray(model_array) || !config)
    {
        return PROD_CONF_ERR_INVALID_PARAMS;
    }

    config->model_count = 0;
    const cJSON *model_item = NULL;

    cJSON_ArrayForEach(model_item, model_array)
    {
        if (config->model_count >= PRODUCT_CONF_MANAGER_MAX_MODEL_COUNT)
        {
            ret_val = PROD_CONF_ERR_NO_MEMORY;
            LOG(W, "Maximum model count exceeded, truncating");
            break;
        }

        int ret = parse_model(model_item, &config->models[config->model_count]);
        if (ret == PROD_CONF_ERR_OK)
        {
            config->model_count++;
        }
        else
        {
            ret_val = ret;
            LOG(W, "Failed to parse model, skipping");
            break;
        }
    }

    return ret_val;
}

/**
 * @brief Parse serial numbers array
 * @param sn_array JSON array containing serial numbers
 * @param config Output manufacturer configuration
 * @return PROD_CONF_ERR_OK on success, error code on failure
 */
static int parse_serial_numbers(const cJSON *sn_array, manufacturer_config_t *config)
{
    int ret_val = PROD_CONF_ERR_OK;
    if (!cJSON_IsArray(sn_array) || !config)
    {
        return PROD_CONF_ERR_INVALID_PARAMS;
    }

    config->serial_count = 0;
    const cJSON *sn_item = NULL;

    cJSON_ArrayForEach(sn_item, sn_array)
    {
        if (config->serial_count >= PRODUCT_CONF_MANAGER_MAX_SERIAL_COUNT)
        {
            LOG(W, "Maximum serial number count exceeded, truncating");
            ret_val = PROD_CONF_ERR_NO_MEMORY;
            break;
        }

        if (cJSON_IsString(sn_item) && sn_item->valuestring)
        {
            strncpy(config->serial_numbers[config->serial_count], sn_item->valuestring, PRODUCT_CONF_MANAGER_MAX_SERIAL_NUM_LEN - 1);
            config->serial_numbers[config->serial_count][PRODUCT_CONF_MANAGER_MAX_SERIAL_NUM_LEN - 1] = '\0';
            config->serial_count++;
        }
    }

    return ret_val;
}

/**
 * @brief Parse JSON string into product configuration structure
 * @param json_string JSON string to parse
 * @return PROD_CONF_ERR_OK on success, error code on failure
 */
int product_conf_manager_parse_json_string(const char *json_string)
{
    int ret_val = PROD_CONF_ERR_OK;
    if (!json_string)
    {
        LOG(E, "Invalid arguments");
        return PROD_CONF_ERR_INVALID_PARAMS;
    }

    memset(&g_product_config, 0, sizeof(product_config_t));
    g_config_loaded = false;

    cJSON *json = cJSON_Parse(json_string);
    if (!json)
    {
        LOG(E, "Failed to parse JSON: %s", cJSON_GetErrorPtr());
        return PROD_CONF_ERR_JSON_PARSE;
    }
#if defined(UNITTEST_BUILD)
    char *temp = cJSON_Print(json);
    LOG(D, "%s", temp);
    cJSON_free(temp);
#endif
    if (!cJSON_IsArray(json))
    {
        LOG(E, "Root JSON element is not an array");
        cJSON_Delete(json);
        return PROD_CONF_ERR_INVALID_FORMAT;
    }

    g_product_config.manufacturer_count = 0;
    const cJSON *manuf_obj = NULL;

    cJSON_ArrayForEach(manuf_obj, json)
    {
        if (g_product_config.manufacturer_count >= PRODUCT_CONF_MANAGER_MAX_MANUF_COUNT)
        {
            LOG(W, "Maximum manufacturer count exceeded, truncating");
            ret_val = PROD_CONF_ERR_NO_MEMORY;
            break;
        }

        manufacturer_config_t *current_manuf = &g_product_config.manufacturers[g_product_config.manufacturer_count];
        memset(current_manuf, 0, sizeof(manufacturer_config_t));

        // Parse manufacturer names
        const cJSON *manuf_array = cJSON_GetObjectItem(manuf_obj, "manuf");
        if (manuf_array)
        {
            ret_val = parse_manufacturer_names(manuf_array, current_manuf);
            if (ret_val)
            {
                break;
            }
        }

        // Parse models
        const cJSON *model_array = cJSON_GetObjectItem(manuf_obj, "model");
        if (model_array)
        {
            ret_val = parse_models_array(model_array, current_manuf);
            if (ret_val)
            {
                break;
            }
        }

        // Parse serial numbers
        const cJSON *sn_array = cJSON_GetObjectItem(manuf_obj, "sn");
        if (sn_array)
        {
            ret_val = parse_serial_numbers(sn_array, current_manuf);
            if (ret_val)
            {
                break;
            }
        }

        g_product_config.manufacturer_count++;
    }

    cJSON_Delete(json);
    g_config_loaded = true;
    if (ret_val == PROD_CONF_ERR_OK)
    {
        LOG(I, "Successfully parsed configuration with %d manufacturers", g_product_config.manufacturer_count);
    }
    else
    {
        LOG(W, "Failed to parse configuration: %d", ret_val);
    }
    return ret_val;
}

#if defined(UNITTEST_BUILD)
int product_conf_manager_load_config_file(const char *conf_path)
{
    int ret_value;
    // Set path temporary to given path
    conf_spiffs_file_path = (const char *)conf_path;
    ret_value = product_conf_manager_init();
    conf_spiffs_file_path = (const char *)conf_spiffs_file_name;
    return ret_value;
}

#endif

/**
 * @brief Load and parse configuration from file
 * @param file_path Path to the JSON configuration file
 * @return PROD_CONF_ERR_OK on success, error code on failure
 */
int product_conf_manager_load_file(const char *file_path)
{
    if (g_config_loaded)
    {
        // Already loaded
        LOG(D, "Already initialized");
        return PROD_CONF_ERR_OK;
    }
    if (!file_path)
    {
        LOG(E, "Invalid arguments");
        return PROD_CONF_ERR_INVALID_PARAMS;
    }

    LOG(D, "Open %s ...", file_path);
    FILE *file = fopen(file_path, "r");
    if (!file)
    {
        LOG(E, "Failed to open file: %s", file_path);
        return PROD_CONF_ERR_FILE_NOT_FOUND;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size <= 0)
    {
        LOG(E, "Invalid file size: %ld", file_size);
        fclose(file);
        return PROD_CONF_ERR_INVALID_FILE_SIZE;
    }

    // Allocate buffer for file content
    char *json_string = hal_mem_malloc_prefer(file_size + 1, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    if (!json_string)
    {
        LOG(E, "Failed to allocate memory for file content");
        fclose(file);
        return PROD_CONF_ERR_NO_MEMORY;
    }

    // Read file content
    size_t bytes_read = fread(json_string, 1, file_size, file);
    fclose(file);

    if ((long)bytes_read != file_size)
    {
        LOG(E, "Failed to read entire file");
        hal_mem_free(json_string);
        return PROD_CONF_ERR_JSON_FILE_READ;
    }

    json_string[file_size] = '\0';

    // Parse JSON
    int ret = product_conf_manager_parse_json_string(json_string);
    hal_mem_free(json_string);

    product_conf_manager_print_config();
    return ret;
}

/**
 * @brief Free memory used by configuration structure
 */
void product_conf_manager_free_config(void)
{
    memset(&g_product_config, 0, sizeof(product_config_t));
    g_config_loaded = false;
}

/**
 * @brief Print configuration structure for debugging
 */
void product_conf_manager_print_config(void)
{
    if (!g_config_loaded)
    {
        LOG(W, "No configuration loaded");
        return;
    }

    const product_config_t *config = &g_product_config;

    LOG(D, "=== Product Configuration ===");
    LOG(D, "Manufacturers: %d", config->manufacturer_count);

    for (int i = 0; i < config->manufacturer_count; i++)
    {
        const manufacturer_config_t *manuf = &config->manufacturers[i];
        LOG(D, "\nManufacturer %d:", i + 1);

        LOG(D, "  Names: %d", manuf->manuf_count);
        for (int j = 0; j < manuf->manuf_count; j++)
        {
            LOG(D, "    - %s", manuf->manuf_names[j]);
        }

        LOG(D, "  Models: %d", manuf->model_count);
        for (int j = 0; j < manuf->model_count; j++)
        {
            const model_info_t *model = &manuf->models[j];
            LOG(D, "    Model: %s", model->model_name);
            if (model->has_type)
            {
                LOG(D, "      Type: %d", model->type);
            }
            LOG(D, "      FWIDs: %d", model->fwid_count);
            for (int k = 0; k < model->fwid_count; k++)
            {
                LOG(D, "        - %s", model->fwid[k]);
            }
            if (model->has_version_patterns)
            {
                if (model->version_patterns.has_fwver_regex)
                {
                    LOG(D, "      FW Version Regex: %s", model->version_patterns.fwver_regex);
                }
                if (model->version_patterns.has_hwver_regex)
                {
                    LOG(D, "      HW Version Regex: %s", model->version_patterns.hwver_regex);
                }
            }
        }

        LOG(D, "  Serial Numbers: %d", manuf->serial_count);
        for (int j = 0; j < manuf->serial_count; j++)
        {
            LOG(D, "    - %s", manuf->serial_numbers[j]);
        }
    }
}

/**
 * @brief Find manufacturer configuration by name
 * @param manuf_name Manufacturer name to search for
 * @return Pointer to manufacturer configuration or NULL if not found
 */
manufacturer_config_t *product_conf_manager_find_manufacturer(const char *manuf_name)
{
    if (!manuf_name || !g_config_loaded)
    {
        return NULL;
    }

    for (int i = 0; i < g_product_config.manufacturer_count; i++)
    {
        manufacturer_config_t *manuf = &g_product_config.manufacturers[i];
        for (int j = 0; j < manuf->manuf_count; j++)
        {
            if (strcmp(manuf->manuf_names[j], manuf_name) == 0)
            {
                return manuf;
            }
        }
    }

    return NULL;
}

/**
 * @brief Find model information by name within a manufacturer
 * @param manuf_name Manufacturer name
 * @param model_name Model name to search for
 * @return Pointer to model information or NULL if not found
 */
model_info_t *product_conf_manager_find_model(const char *manuf_name, const char *model_name)
{
    manufacturer_config_t *manuf = product_conf_manager_find_manufacturer(manuf_name);
    if (!manuf || !model_name)
    {
        return NULL;
    }

    for (int i = 0; i < manuf->model_count; i++)
    {
        if (strstr(model_name, manuf->models[i].model_name) != NULL)
        {
            return &manuf->models[i];
        }
    }

    return NULL;
}

/**
 * @brief Check if a model supports a specific FWID
 * @param manuf_name Manufacturer name
 * @param model_name Model name
 * @param fwid FWID to check
 * @return true if supported, false otherwise
 */
bool product_conf_manager_model_supports_fwid(const char *manuf_name, const char *model_name, const char *fwid)
{
    model_info_t *model = product_conf_manager_find_model(manuf_name, model_name);
    if (!model || !fwid)
    {
        return false;
    }

    for (int i = 0; i < model->fwid_count; i++)
    {
        if (strcmp(model->fwid[i], fwid) == 0)
        {
            return true;
        }
    }

    return false;
}

/**
 * @brief Get list of firmware IDs for a specific model
 * @param manuf_name Manufacturer name
 * @param model_name Model name
 * @param fwid_count Output parameter for number of firmware IDs (optional, can be NULL)
 * @return True if call is successful
 * @note The returned array is read-only and valid until product_conf_manager_free_config() is called
 */
bool product_conf_manager_get_model_fwids(const char *manuf_name, const char *model_name, fwid_query_result_t *fwid_result)
{
    model_info_t *model = product_conf_manager_find_model(manuf_name, model_name);
    if (!model)
    {
        if (fwid_result)
        {
            fwid_result->count = 0;
        }
        return false;
    }

    if (fwid_result)
    {
        int num = 0;
        // Copy into result structure
        for (size_t i = 0; i < model->fwid_count; ++i)
        {
            if (i < fwid_result->count)
            {
                strcpy(fwid_result->fwids[num], model->fwid[i]);
                num++;
            }
        }
        fwid_result->count = num;
    }

    return true;
}

/**
 * @brief Get version patterns for a specific model
 * @param manuf_name Manufacturer name
 * @param model_name Model name
 * @return Pointer to version patterns structure, or NULL if not found
 * @note The returned structure is read-only and valid until product_conf_manager_free_config() is called
 */
const version_patterns_t *product_conf_manager_get_model_version_patterns(const char *manuf_name, const char *model_name)
{
    model_info_t *model = product_conf_manager_find_model(manuf_name, model_name);
    if (!model || !model->has_version_patterns)
    {
        return NULL;
    }

    return &model->version_patterns;
}

/**
 * @brief Apply firmware version regex to extract version from string
 * @param manuf_name Manufacturer name
 * @param model_name Model name
 * @param version_string Input version string to parse
 * @param result Output buffer for extracted version
 * @param result_size Size of result buffer
 * @return PROD_CONF_ERR_OK on success, error code on failure
 */
int product_conf_manager_extract_fw_version(const char *manuf_name, const char *model_name, const char *version_string, char *result, size_t result_size)
{
    if (!version_string || !result || (result_size == 0))
    {
        return PROD_CONF_ERR_INVALID_PARAMS;
    }

    const version_patterns_t *patterns = product_conf_manager_get_model_version_patterns(manuf_name, model_name);
    if (!patterns || !patterns->has_fwver_regex)
    {
        LOG(W, "No firmware version regex found for %s/%s", manuf_name ? manuf_name : "NULL", model_name ? model_name : "NULL");
        return PROD_CONF_ERR_INVALID_FORMAT;
    }

    regex_t regex;
    int ret = regcomp(&regex, patterns->fwver_regex, REG_EXTENDED);
    if (ret != 0)
    {
        char error_buffer[256];
        regerror(ret, &regex, error_buffer, sizeof(error_buffer));
        LOG(E, "Failed to compile firmware regex '%s': %s", patterns->fwver_regex, error_buffer);
        return PROD_CONF_ERR_INVALID_FORMAT;
    }

    regmatch_t matches[2];  // We want the first capture group
    ret = regexec(&regex, version_string, 2, matches, 0);

    if (ret == 0 && matches[1].rm_so != -1)
    {
        // Extract the first capture group
        int match_len = matches[1].rm_eo - matches[1].rm_so;
        int copy_len = (match_len < (int)(result_size - 1)) ? match_len : (int)(result_size - 1);

        strncpy(result, version_string + matches[1].rm_so, copy_len);
        result[copy_len] = '\0';

        regfree(&regex);
        LOG(D, "Extracted FW version: '%s' from '%s' using regex '%s'", result, version_string, patterns->fwver_regex);
        return PROD_CONF_ERR_OK;
    }
    else
    {
        regfree(&regex);
        if (ret == REG_NOMATCH)
        {
            LOG(W, "No match found for FW version in '%s' using regex '%s'", version_string, patterns->fwver_regex);
            return PROD_CONF_ERR_INVALID_FORMAT;
        }
        else
        {
            char error_buffer[256];
            regerror(ret, &regex, error_buffer, sizeof(error_buffer));
            LOG(E, "Regex execution error: %s", error_buffer);
            return PROD_CONF_ERR_INVALID_FORMAT;
        }
    }
}

/**
 * @brief Apply hardware version regex to extract version from string
 * @param manuf_name Manufacturer name
 * @param model_name Model name
 * @param version_string Input version string to parse
 * @param result Output buffer for extracted version
 * @param result_size Size of result buffer
 * @return PROD_CONF_ERR_OK on success, error code on failure
 */
int product_conf_manager_extract_hw_version(const char *manuf_name, const char *model_name, const char *version_string, char *result, size_t result_size)
{
    if (!version_string || !result || result_size == 0)
    {
        return PROD_CONF_ERR_INVALID_PARAMS;
    }

    const version_patterns_t *patterns = product_conf_manager_get_model_version_patterns(manuf_name, model_name);
    if (!patterns || !patterns->has_hwver_regex)
    {
        LOG(W, "No hardware version regex found for %s/%s", manuf_name ? manuf_name : "NULL", model_name ? model_name : "NULL");
        return PROD_CONF_ERR_INVALID_FORMAT;
    }

    regex_t regex;
    int ret = regcomp(&regex, patterns->hwver_regex, REG_EXTENDED);
    if (ret != 0)
    {
        char error_buffer[256];
        regerror(ret, &regex, error_buffer, sizeof(error_buffer));
        LOG(E, "Failed to compile hardware regex '%s': %s", patterns->hwver_regex, error_buffer);
        return PROD_CONF_ERR_INVALID_FORMAT;
    }

    regmatch_t matches[2];  // We want the first capture group
    ret = regexec(&regex, version_string, 2, matches, 0);

    if (ret == 0 && matches[1].rm_so != -1)
    {
        // Extract the first capture group
        int match_len = matches[1].rm_eo - matches[1].rm_so;
        int copy_len = (match_len < (int)(result_size - 1)) ? match_len : (int)(result_size - 1);

        strncpy(result, version_string + matches[1].rm_so, copy_len);
        result[copy_len] = '\0';

        regfree(&regex);
        LOG(D, "Extracted HW version: '%s' from '%s' using regex '%s'", result, version_string, patterns->hwver_regex);
        return PROD_CONF_ERR_OK;
    }
    else
    {
        regfree(&regex);
        if (ret == REG_NOMATCH)
        {
            LOG(W, "No match found for HW version in '%s' using regex '%s'", version_string, patterns->hwver_regex);
            return PROD_CONF_ERR_INVALID_FORMAT;
        }
        else
        {
            char error_buffer[256];
            regerror(ret, &regex, error_buffer, sizeof(error_buffer));
            LOG(E, "Regex execution error: %s", error_buffer);
            return PROD_CONF_ERR_INVALID_FORMAT;
        }
    }
}

/**
 * @brief Get the number of manufacturers in the configuration
 * @return Number of manufacturers or -1 if not loaded
 */
int product_conf_manager_get_manufacturer_count(void)
{
    return g_config_loaded ? g_product_config.manufacturer_count : -1;
}

/**
 * @brief Get the number of models for a manufacturer
 * @param manuf_name Manufacturer name
 * @return Number of models or -1 if not found
 */
int product_conf_manager_get_model_count(const char *manuf_name)
{
    manufacturer_config_t *manuf = product_conf_manager_find_manufacturer(manuf_name);
    return manuf ? manuf->model_count : -1;
}

/**
 * @brief Get model type by manufacturer and model name
 * @param manuf_name Manufacturer name
 * @param model_name Model name
 * @return Model type string or NULL if not found or no type specified
 */
int product_conf_manager_get_model_type(const char *manuf_name, const char *model_name, int *type)
{
    if (!type)
    {
        return PROD_CONF_ERR_INVALID_PARAMS;
    }

    model_info_t *model = product_conf_manager_find_model(manuf_name, model_name);
    if (!model)
    {
        return PROD_CONF_ERR_MISSING_ITEM;
    }

    *type = model->type;  // Will be 0 if not set in JSON. This is invalid
    return model->type != 0 ? PROD_CONF_ERR_OK : PROD_CONF_ERR_MISSING_ITEM;
}

/**
 * @brief Check if a manufacturer supports a specific serial number
 * @param manuf_name Manufacturer name
 * @param sn Serial number to check
 * @return true if supported, false otherwise
 */
bool product_conf_manager_manufacturer_supports_sn(const char *manuf_name, const char *sn)
{
    manufacturer_config_t *manuf = product_conf_manager_find_manufacturer(manuf_name);
    if (!manuf || !sn)
    {
        return false;
    }

    for (int i = 0; i < manuf->serial_count; i++)
    {
        if (strcmp(manuf->serial_numbers[i], sn) == 0)
        {
            return true;
        }
    }

    return false;
}

/**
 * @brief Validate a regex pattern (public API)
 * @param regex_pattern The regex pattern to validate
 * @return PROD_CONF_ERR_OK if valid, PROD_CONF_ERR_INVALID_PARAMS if invalid or NULL
 */
int product_conf_manager_validate_regex(const char *regex_pattern)
{
    if (!regex_pattern)
    {
        LOG(E, "Regex pattern is NULL");
        return PROD_CONF_ERR_INVALID_PARAMS;
    }

    return validate_regex_pattern(regex_pattern);
}
