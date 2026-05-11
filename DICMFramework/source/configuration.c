/*! \file configuration.c
    \brief Configuration source

    Configuration parameters and data storage.
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "configuration.h"

#include "broker.h"
#include "crc32.h"
#include "ddm2.h"
#include "esp_err.h"
#include "esp_idf_version.h"
#include "esp_mac.h"
#include "esp_partition.h"
#include "esp_spiffs.h"
#include "esp_system.h"
#include "hal_cpu.h"
#include "hal_mem.h"
#include "network_discovery.h"
#include "nvs_flash.h"
#include "sorted_list.h"
#include "utils.h"
#ifdef CONNECTOR_BLE
#include "ble_peripheral.h"
#endif
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include "esp_app_desc.h"
#else
#include "esp_ota_ops.h"
#endif

#define NVS_FACTORY_PARTITION "nvs_factory"
#define LOG_PARTITION         "log"

const int32_t Minone = -1;
const int32_t Zero = 0;
const int32_t One = 1;

EventGroupHandle_t network_events;  //!< \~ Network event bits
EventGroupHandle_t system_events;   //!< \~ System event bits

nvs_handle_t nvs;                 //!< \~ Handle to general flash storage
static nvs_handle_t nvs_factory;  //!< \~ Handle to factory parameter flash storage
static bool found_nvs_factory;

static const esp_vfs_spiffs_conf_t fs_conf = {
    .base_path = "/spiffs",
    .partition_label = "storage",
    .max_files = 10,
    .format_if_mount_failed = true,
};
static const esp_vfs_spiffs_conf_t fs_conf2 = {
    .base_path = "/spiffs",
    .partition_label = "storage2",
    .max_files = 10,
    .format_if_mount_failed = true,
};
static char spiffs_partition_version[32] = "0.0.0-na";

static esp_vfs_spiffs_conf_t const *my_conf = &fs_conf;
static size_t MAYBE_UNUSED fs_total_size = 0;
static size_t MAYBE_UNUSED fs_used_size = 0;

// parameter storage variables of large sizes (DDMP2_JUMBO_FRAME_MAX_SIZE) to be allocated on heap (external memory)
int32_t gw0tst;
EXT_RAM_ATTR uint8_t *gw0awssc;
EXT_RAM_ATTR uint8_t *gw0awscc;
EXT_RAM_ATTR uint8_t *gw0awsccpk;
EXT_RAM_ATTR uint8_t *gw0awsca;
EXT_RAM_ATTR uint8_t *gw0otasc;
EXT_RAM_ATTR uint8_t *gw0otacc;
EXT_RAM_ATTR uint8_t *gw0otaccpk;
EXT_RAM_ATTR uint8_t *gw0otaca;
EXT_RAM_ATTR uint8_t gw0dsn[ONBOARDING_STRING_SIZE];
EXT_RAM_ATTR uint8_t gw0sku[MAX_SKU_LENGTH + 1];  // string + NULL
EXT_RAM_ATTR uint8_t gw0pnc[ONBOARDING_STRING_SIZE];
EXT_RAM_ATTR uint8_t gw0thing[ONBOARDING_STRING_SIZE];
EXT_RAM_ATTR char cfg0loglvl[LOG_LEVEL_STRING_SIZE];
EXT_RAM_ATTR char cfg0opath[OTA_URL_STRING_SIZE];
EXT_RAM_ATTR uint8_t gw0connurl[CONN_URL_STRING_SIZE];
int32_t gw0cupdt = 60 * 60 * 1000;  // 60 min
int32_t gw0batwin = 0.2 * 1000;     // 0.2 V
int32_t gw0tempwin = 1 * 1000;      // 1 ^C
int32_t gw0remtwin = 1 * 1000;      // 1 h
int32_t cfg0itempth = 5000;         // 5 ^C
int32_t cfg0wwtrth = 75;            // 75 %
int32_t cfg0fwtrth = 25;            // 25 %
int32_t cfg0batth = 5;              // 5 %
int32_t gw0dev1st;
int32_t gw0ict;
int32_t cfg0ostat;
int32_t gw0ocnt;  // OTA counter, starts from 0
int32_t gw0rcnt;  // Restart counter
int32_t eolpass;

#ifdef THING_TYPE_ID
int32_t gw0thtyid = THING_TYPE_ID;
#else   // THING_TYPE_ID
int32_t gw0thtyid;
#endif  // THING_TYPE_ID

#ifdef NTW_THING_TYPE_ID
int32_t cfg0ntwth = NTW_THING_TYPE_ID;
#else   // NTW_THING_TYPE_ID
int32_t cfg0ntwth;
#endif  // NTW_THING_TYPE_ID

int32_t cfg0dport;  // NOTE: Change name?
static int32_t spiffsid = 0;

static const char *default_ota_url = "https://d2qgzse404v740.cloudfront.net/";
static const FLASH_PARAMETER_STRING Flash_parameter_string_table[] = {
    {"gw0awssc", DDM2_TYPE_STRING, (void **)&gw0awssc, DDMP2_JUMBO_FRAME_MAX_SIZE},
    {"gw0awscc", DDM2_TYPE_STRING, (void **)&gw0awscc, DDMP2_JUMBO_FRAME_MAX_SIZE},
    {"gw0awsccpk", DDM2_TYPE_STRING, (void **)&gw0awsccpk, DDMP2_JUMBO_FRAME_MAX_SIZE},
    {"gw0awsca", DDM2_TYPE_STRING, (void **)&gw0awsca, DDMP2_JUMBO_FRAME_MAX_SIZE},
#if 0  // Optimize allocated
		{"gw0otasc",    DDM2_TYPE_STRING,  (void **)&gw0otasc, DDMP2_JUMBO_FRAME_MAX_SIZE},
		{"gw0otacc",    DDM2_TYPE_STRING,  (void **)&gw0otacc, DDMP2_JUMBO_FRAME_MAX_SIZE},
		{"gw0otaccpk",  DDM2_TYPE_STRING,  (void **)&gw0otaccpk, DDMP2_JUMBO_FRAME_MAX_SIZE},
		{"gw0otaca",    DDM2_TYPE_STRING,  (void **)&gw0otaca, DDMP2_JUMBO_FRAME_MAX_SIZE},
#endif
};

static const FLASH_PARAMETER Nvs_parameter_table[] = {
    //    Key           Type               Storage       Size (max)
    {"gw0tst", DDM2_TYPE_INT32_T, &gw0tst, sizeof(gw0tst)},
    {"gw0thing", DDM2_TYPE_STRING, &gw0thing, sizeof(gw0thing)},
    {"gw0thtyid", DDM2_TYPE_INT32_T, &gw0thtyid, sizeof(gw0thtyid)},
    {"gw0cupdt", DDM2_TYPE_INT32_T, &gw0cupdt, sizeof(gw0cupdt)},
    {"gw0batwin", DDM2_TYPE_INT32_T, &gw0batwin, sizeof(gw0batwin)},
    {"gw0tempwin", DDM2_TYPE_INT32_T, &gw0tempwin, sizeof(gw0tempwin)},
    {"gw0remtwin", DDM2_TYPE_INT32_T, &gw0remtwin, sizeof(gw0remtwin)},
    {"cfg0itempth", DDM2_TYPE_INT32_T, &cfg0itempth, sizeof(cfg0itempth)},
    {"cfg0wwtrth", DDM2_TYPE_INT32_T, &cfg0wwtrth, sizeof(cfg0wwtrth)},
    {"cfg0fwtrth", DDM2_TYPE_INT32_T, &cfg0fwtrth, sizeof(cfg0fwtrth)},
    {"cfg0batth", DDM2_TYPE_INT32_T, &cfg0batth, sizeof(cfg0batth)},
    {"cfg0opath", DDM2_TYPE_STRING, &cfg0opath, sizeof(cfg0opath)},
    {"cfg0ntwth", DDM2_TYPE_INT32_T, &cfg0ntwth, sizeof(cfg0ntwth)},
    {"cfg0dport", DDM2_TYPE_INT32_T, &cfg0dport, sizeof(cfg0dport)},
    {"gw0connurl", DDM2_TYPE_STRING, &gw0connurl, sizeof(gw0connurl)},
};

static const FLASH_PARAMETER Nvs_factory_parameter_table[] = {
    //    Key           Type               Storage       Size (max)
    {"gw0dsn", DDM2_TYPE_STRING, &gw0dsn, sizeof(gw0dsn)},
    {"gw0sku", DDM2_TYPE_STRING, &gw0sku, sizeof(gw0sku)},
    {"gw0pnc", DDM2_TYPE_STRING, &gw0pnc, sizeof(gw0pnc)},
    {"gw0ocnt", DDM2_TYPE_INT32_T, &gw0ocnt, sizeof(gw0ocnt)},
    {"gw0rcnt", DDM2_TYPE_INT32_T, &gw0rcnt, sizeof(gw0rcnt)},
    {"eolpass", DDM2_TYPE_INT32_T, &eolpass, sizeof(eolpass)},
    {"spiffsid", DDM2_TYPE_INT32_T, &spiffsid, sizeof(spiffsid)},
};

//! \~ Operational parameters about this specific gateway
DEVICE_INFORMATION device_information = {
    .bluetooth_enabled = BLUETOOTH_ON,
    .default_name = DEFAULT_DEVICE_NAME_PREFIX,
    .id = "",
    .id_string = "",
};

static bool check_nvs_key(const char *const key);
static nvs_handle_t config_get_nvs(const char *key);
static nvs_handle_t config_get_factory_nvs(void);

static void check_nvs_result(const esp_err_t result, const char *const key);
static esp_err_t load_nvs_table_entry(const FLASH_PARAMETER *const entry);

static void erase_log_partition(void);

/**
 * @brief Returns the nvs handle used for a specific key
 *
 * Will search the Nvs_factory_parameter_table for the key in try to use (return)
 * corresponding nvs handle (if exists)
 *
 * @param[in] key
 * @return nvs handle depending on key
 */
static nvs_handle_t config_get_nvs(const char *key)
{
    nvs_handle_t handle_nvs = nvs;
    for (int i = 0; i < (int)ELEMENTS(Nvs_factory_parameter_table); i++)
    {
        if (strcmp(Nvs_factory_parameter_table[i].key, key) == 0)
        {
            // Use factory nvs partition
            handle_nvs = config_get_factory_nvs();
            break;
        }
    }
    return handle_nvs;
}

/**
 * @brief Returns the nvs partition handle for factory storage
 *
 * @return factory nvs handle if existing, otherwise default nvs handle
 */
nvs_handle_t config_get_factory_nvs(void)
{
    return found_nvs_factory ? nvs_factory : nvs;
}

// \~ Check and print nvs parameter status
static void check_nvs_result(const esp_err_t result, const char *const key)
{
    if (result != ESP_OK)
    {
        if (result == ESP_ERR_NVS_NOT_FOUND)
        {
            BROKER_LOG(I, "No data stored for %s", key);
        }
        else
        {
            LOG(E, "Could not restore %s from flash! (0x%x)", key, result);
        }
    }
    else
    {
        LOG(I, "Restored value of %s", key);
    }
}

// \~ Process an entry in an NVS parameter table
static esp_err_t load_nvs_table_entry(const FLASH_PARAMETER *const entry)
{
    esp_err_t result = ESP_FAIL;
    nvs_handle_t handle_nvs = config_get_nvs(entry->key);  // Default nvs
    switch (entry->type)
    {
    case DDM2_TYPE_INT32_T:
        result = nvs_get_i32(handle_nvs, entry->key, entry->storage);
        break;
    case DDM2_TYPE_STRING:
        memset(entry->storage, 0, entry->size);
        size_t length = entry->size;
        result = nvs_get_str(handle_nvs, entry->key, entry->storage, &length);
        break;
    default:
        break;
    }

    check_nvs_result(result, entry->key);

    return result;
}

HAL_GPIO_RESULT_ENUM LED_R(int level) { return HAL_GPIO_ERROR_PIN_UNAVAILABLE; }
HAL_GPIO_RESULT_ENUM LED_G(int level) { return HAL_GPIO_ERROR_PIN_UNAVAILABLE; }
HAL_GPIO_RESULT_ENUM LED_B(int level) { return HAL_GPIO_ERROR_PIN_UNAVAILABLE; }
HAL_GPIO_RESULT_ENUM READ_LED_R(int *level) { return HAL_GPIO_ERROR_PIN_UNAVAILABLE; }
HAL_GPIO_RESULT_ENUM READ_LED_G(int *level) { return HAL_GPIO_ERROR_PIN_UNAVAILABLE; }
HAL_GPIO_RESULT_ENUM READ_LED_B(int *level) { return HAL_GPIO_ERROR_PIN_UNAVAILABLE; }

uint8_t gateway_advertisement_flag_byte(void)
{
    return (uint8_t)((MULTIBROKER << 1) |
#ifdef CONNECTOR_BLE
                     (uint8_t)(BT0PAIR_OUT_PAIRING_MODE_ACTIVE == ble_peripheral_get_bond_mode()));
#else
                     0);
#endif
}

void gateway_factory_reset(void)
{
    erase_log_partition();

    nvs_close(nvs);
    ZERO_CHECK(nvs_flash_erase());
    if (found_nvs_factory)
    {
        nvs_close(nvs_factory);
    }
    hal_cpu_reset(HALCPU_RESET_FLAG_NONE);
}

// \~ Loads data from flash
void gateway_load_configuration(void)
{
    esp_err_t result;
    size_t length;

    // Allocate in external memory
#if CONFIG_IDF_TARGET_ESP32C2
    gw0awssc = NULL;
    gw0awscc = NULL;
    gw0awsccpk = NULL;
    gw0awsca = NULL;
#else
    TRUE_CHECK((gw0awssc = hal_mem_malloc_prefer(DDMP2_JUMBO_FRAME_MAX_SIZE, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM)) != NULL);
    TRUE_CHECK((gw0awscc = hal_mem_malloc_prefer(DDMP2_JUMBO_FRAME_MAX_SIZE, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM)) != NULL);
    TRUE_CHECK((gw0awsccpk = hal_mem_malloc_prefer(DDMP2_JUMBO_FRAME_MAX_SIZE, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM)) != NULL);
    TRUE_CHECK((gw0awsca = hal_mem_malloc_prefer(DDMP2_JUMBO_FRAME_MAX_SIZE, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM)) != NULL);
#endif
    // For optimization reasons, we disregard all GW0OTAXX parameters
#if 0
	TRUE_CHECK(gw0otasc = hal_mem_malloc_prefer(DDMP2_JUMBO_FRAME_MAX_SIZE, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM));
	TRUE_CHECK(gw0otacc = hal_mem_malloc_prefer(DDMP2_JUMBO_FRAME_MAX_SIZE, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM));
	TRUE_CHECK(gw0otaccpk = hal_mem_malloc_prefer(DDMP2_JUMBO_FRAME_MAX_SIZE, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM));
	TRUE_CHECK(gw0otaca = hal_mem_malloc_prefer(DDMP2_JUMBO_FRAME_MAX_SIZE, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM));
#else
    gw0otasc = NULL;
    gw0otacc = NULL;
    gw0otaccpk = NULL;
    gw0otaca = NULL;
#endif

    LOG(I, "Initializing NVS");
    result = nvs_flash_init();
    if (result == ESP_ERR_NVS_NO_FREE_PAGES)
    {
        ZERO_CHECK(nvs_flash_erase());
        ZERO_CHECK(nvs_flash_init());
    }
    result = nvs_flash_init_partition(NVS_FACTORY_PARTITION);
    if (result == ESP_ERR_NVS_NO_FREE_PAGES)
    {
        ZERO_CHECK(nvs_flash_erase_partition(NVS_FACTORY_PARTITION));
        ZERO_CHECK(nvs_flash_init_partition(NVS_FACTORY_PARTITION));
    }

    nvs_stats_t nvs_stats;
    ZERO_CHECK(nvs_get_stats(NULL, &nvs_stats));
    LOG(I, "Count: UsedEntries = (%d), FreeEntries = (%d), AllEntries = (%d)", nvs_stats.used_entries, nvs_stats.free_entries, nvs_stats.total_entries);

    ZERO_CHECK(esp_efuse_mac_get_default((uint8_t *)&device_information.id));  // get base MAC address
    snprintf(device_information.default_name, sizeof(device_information.default_name), DEFAULT_DEVICE_NAME_PREFIX MAC3FORMAT, MAC3VALUE(device_information.id));
    LOG(C, "Name: %s", device_information.default_name);
    snprintf(device_information.id_string, sizeof(device_information.id_string), MACFORMAT, MACVALUE(device_information.id));
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    const esp_app_desc_t *my_app_info = esp_app_get_description();
#else
    const esp_app_desc_t *my_app_info = esp_ota_get_app_description();
#endif
    strcpy(device_information.firmware_version, my_app_info->version);

    ZERO_CHECK(nvs_open("rwdata", NVS_READWRITE, &nvs));  // open the general parameter NVS namespace readwrite
    // Open the general parameter NVS_FACTORY_PARTITION namespace readwrite
    result = nvs_open_from_partition(NVS_FACTORY_PARTITION, "rwdata", NVS_READWRITE, &nvs_factory);
    ZERO_CHECK(result);
    if (result == ESP_ERR_NVS_PART_NOT_FOUND)
    {
        // Error. Partition cannot be used. We could be using an older partition table.
        found_nvs_factory = false;
        LOG(W, "No " NVS_FACTORY_PARTITION " partition found. Using default instead.");
    }
    else
    {
        found_nvs_factory = true;
    }
    // Scan parameter table large strings
    for (int i = 0; i < (int)ELEMENTS(Flash_parameter_string_table); i++)
    {
        // Check type
        switch (Flash_parameter_string_table[i].type)
        {
        case DDM2_TYPE_STRING:
            if ((*((uint8_t **)Flash_parameter_string_table[i].storage)) != NULL)
            {
                memset((void *)(*((uint8_t **)Flash_parameter_string_table[i].storage)), 0, Flash_parameter_string_table[i].size);
                length = Flash_parameter_string_table[i].size;
                result = nvs_get_str(nvs, Flash_parameter_string_table[i].key, (void *)(*((uint8_t **)Flash_parameter_string_table[i].storage)), &length);
            }
            else
            {
                result = ESP_FAIL;
            }
            break;
        default:
            result = ESP_FAIL;
            break;
        }
        // Check result
        check_nvs_result(result, Flash_parameter_string_table[i].key);
    }

    // Scan parameter table others
    for (int i = 0; i < (int)ELEMENTS(Nvs_parameter_table); i++)
    {
        // Load and check result
        result = load_nvs_table_entry(&Nvs_parameter_table[i]);
    }
    for (int i = 0; i < (int)ELEMENTS(Nvs_factory_parameter_table); i++)  // scan and load gereral NVS parameter table
    {
        load_nvs_table_entry(&Nvs_factory_parameter_table[i]);
    }

    // Make sure we have an OTA path available
    if (strlen(cfg0opath) == 0)
    {
        strcpy(cfg0opath, default_ota_url);
    }
    // Make sure we get the init value correct
    if (strlen((char *)gw0sku) == 0)
    {
        strncpy((char *)gw0sku, STOCK_KEEPING_UNIT, sizeof(gw0sku));
        ZERO_CHECK(nvs_set_str(config_get_factory_nvs(), "gw0sku", (char *)gw0sku));
    }
    if (strlen((char *)gw0pnc) == 0)
    {
        strncpy((char *)gw0pnc, PRODUCT_NUMBER_CODE, sizeof(gw0pnc));
        ZERO_CHECK(nvs_set_str(config_get_factory_nvs(), "gw0pnc", (char *)gw0pnc));
    }

    // Increment restart counter
    gw0rcnt++;
    ZERO_CHECK(nvs_set_i32(config_get_factory_nvs(), "gw0rcnt", gw0rcnt));
    ZERO_CHECK(nvs_commit(config_get_factory_nvs()));

    snprintf(cfg0loglvl, sizeof(cfg0loglvl), "*:%d", esp_log_level_get("*"));
}

const char *gateway_get_firmware_version(void)
{
    return (const char *)device_information.firmware_version;
}

// \~ Returns file system info
void get_file_system_info(size_t *total_size, size_t *used_size)
{
#ifndef CONFIG_IDF_TARGET_LINUX
    ZERO_CHECK(esp_spiffs_info(my_conf->partition_label, &fs_total_size, &fs_used_size));

    *total_size = fs_total_size;
    *used_size = fs_used_size;
#endif
}

const char *gateway_get_file_system_partition_version(void)
{
    return (const char *)spiffs_partition_version;
}

bool gateway_file_system_save_next_partition(void)
{
    int32_t new_spiffsid = spiffsid ^ 1;
    esp_err_t result;
    nvs_handle_t spiffsid_nvs = config_get_factory_nvs();

    LOG(I, "Saving next SPIFFS partition id: %d", new_spiffsid);

    if ((result = nvs_set_i32(spiffsid_nvs, "spiffsid", new_spiffsid)) != ESP_OK)
    {
        LOG(E, "Failed setting SPIFFS id to nvs_factory (0x%x)", result);
        return false;
    }
    if ((result = nvs_commit(spiffsid_nvs)) != ESP_OK)
    {
        LOG(E, "Failed committing SPIFFS id to nvs_factory (0x%x)", result);
        return false;
    }

    return true;
}

const esp_partition_t *gateway_file_system_get_unused_partition(void)
{
    esp_vfs_spiffs_conf_t const *unsued_conf = &fs_conf;
    if (spiffsid == 0)
    {
        unsued_conf = &fs_conf2;
    }
    else
    {
        unsued_conf = &fs_conf;
    }

    return esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, unsued_conf->partition_label);
}

// \~ Installs file system
void install_file_system(void)
{
    esp_err_t result;
    nvs_handle_t spiffsid_nvs = config_get_factory_nvs();

    if ((result = nvs_get_i32(spiffsid_nvs, "spiffsid", &spiffsid)) != ESP_OK)
    {
        spiffsid = 0;
        LOG(W, "SPIFFS id is not found (0x%x). Using default (%d)", result, spiffsid);

        if (found_nvs_factory)
        {
            // Migrate existing value from nvs to nvs_factory if it exists
            if ((result = nvs_get_i32(nvs, "spiffsid", &spiffsid)) != ESP_OK)
            {
                LOG(W, "SPIFFS id not in nvs (0x%x). Using default (%d).", result, spiffsid);
            }
            else
            {
                LOG(I, "Migrating SPIFFS id (%d) from nvs to nvs_factory.", spiffsid);
                // Remove spiffsid from nvs
                if ((result = nvs_erase_key(nvs, "spiffsid")) != ESP_OK)
                {
                    LOG(E, "Failed erasing SPIFFS id from nvs (0x%x)", result);
                }
                else
                {
                    if ((result = nvs_commit(nvs)) != ESP_OK)
                    {
                        LOG(E, "Failed committing erase of SPIFFS id from nvs (0x%x)", result);
                    }
                }
            }

            // Store in nvs_factory
            if ((result = nvs_set_i32(spiffsid_nvs, "spiffsid", spiffsid)) != ESP_OK)
            {
                LOG(E, "Failed setting SPIFFS id to nvs_factory (0x%x)", result);
            }
            if ((result = nvs_commit(spiffsid_nvs)) != ESP_OK)
            {
                LOG(E, "Failed committing SPIFFS id to nvs_factory (0x%x)", result);
            }
        }
    }

    // Check if we have two partitions
    if (spiffsid == 0)
    {
        // Use default
        my_conf = &fs_conf;
    }
    if (NULL == esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, fs_conf2.partition_label))
    {
        // There is no fs_conf2 partition. Use default fs_conf regardless
        // We simulate that we are running fs_conf2 for allowing OTA.
        spiffsid = 1;
        my_conf = &fs_conf;
    }
    else if (spiffsid != 0)
    {
        my_conf = &fs_conf2;
    }

    LOG(I, "Initializing SPIFFS at path=%s for maximum %d files", my_conf->base_path, my_conf->max_files);

#ifndef CONFIG_IDF_TARGET_LINUX
    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(my_conf);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            LOG(E, "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            LOG(E, "Failed to find SPIFFS partition");
        }
        else
        {
            LOG(E, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    LOG(I, "Mounted (%s) partition as SPIFFS, spiffsid (%d)", my_conf->partition_label, spiffsid);
    // Read version file
    struct stat st;
    if (stat("/spiffs/version", &st) != 0)
    {
        LOG(W, "Could not find SPIFFS partition version file: %s", (char *)"/spiffs/version");
    }
    else
    {
        FILE *f = fopen((char *)"/spiffs/version", "r");
        if (f == NULL)
        {
            LOG(W, "Could not open version file: %s", (char *)"/spiffs/version");
        }
        else
        {
            LOG(D, "Reading %d bytes from %s", (int)st.st_size, (char *)"/spiffs/version");
            ASSERT((unsigned int)st.st_size < sizeof(spiffs_partition_version));
            fread(spiffs_partition_version, 1, st.st_size, f);
            fclose(f);
            size_t loc = strcspn(spiffs_partition_version, "\n\r");
            spiffs_partition_version[loc] = '\0';
            LOG(I, "FS version: %s", spiffs_partition_version);
        }
    }
    ret = esp_spiffs_info(my_conf->partition_label, &fs_total_size, &fs_used_size);
    if (ret != ESP_OK)
    {
        LOG(E, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    }
    else
    {
        LOG(I, "Partition size: total: %d, used: %d", fs_total_size, fs_used_size);
    }
#endif
}

static bool check_nvs_key(const char *const key)
{
    if (strlen(key) >= NVS_KEY_NAME_MAX_SIZE)  // NVS_KEY_NAME_MAX_SIZE includes the null character
    {
        LOG(E, "NVS key (%s) is too long", key);
        return false;
    }
    return true;
}

// \~ Non-volatile storage pass through
int config_erase(const char *const key)
{
    nvs_handle_t handle_nvs = config_get_nvs(key);

    if (check_nvs_key(key))
    {
        return nvs_erase_key(handle_nvs, key);
    }
    return ESP_FAIL;
}

int config_get_i32(const char *const key, int32_t *const value)
{
    nvs_handle_t handle_nvs = config_get_nvs(key);

    if (check_nvs_key(key))
    {
        return nvs_get_i32(handle_nvs, key, value);
    }
    return ESP_FAIL;
}

int32_t config_get_i32_def(const char *const key, const int32_t value_def)
{
    int32_t value;
    nvs_handle_t handle_nvs = config_get_nvs(key);

    esp_err_t err = ESP_OK;
    if (!check_nvs_key(key))
    {
        err = ESP_FAIL;
        LOG(E, "NVS key check failed: %s", key);
    }
    else
    {
        err = nvs_get_i32(handle_nvs, key, &value);
        if (err != ESP_OK)
        {
            LOG(E, "NVS get i32 failed for key %s: 0x%x", key, err);
        }
    }

    if (err == ESP_OK)
    {
        LOG(D, "NVS load %s = %08x", key, value);
    }
    else
    {
        value = value_def;
        LOG(W, "NVS load %s failed, using default %08x", key, value);
    }
    return value;
}

int config_get_blob(const char *const key, void *const data, size_t *const size)
{
    nvs_handle_t handle_nvs = config_get_nvs(key);
    esp_err_t err = ESP_OK;

    if (check_nvs_key(key))
    {
        err = nvs_get_blob(handle_nvs, key, data, size);
    }
    else
    {
        err = ESP_FAIL;
    }

    if (err == ESP_OK)
    {
        LOG(D, "NVS load %s: %d bytes", key, *size);
    }
    return err;
}

int config_get_str(const char *const key, char *const str, size_t *const length)
{
    nvs_handle_t handle_nvs = config_get_nvs(key);

    if (check_nvs_key(key))
    {
        return nvs_get_str(handle_nvs, key, str, length);
    }
    else
    {
        return ESP_FAIL;
    }
}

int config_set_i32(const char *const key, const int32_t value)
{
    esp_err_t result = ESP_OK;
    nvs_handle_t handle_nvs = config_get_nvs(key);
    if (check_nvs_key(key))
    {
        LOG(D, "NVS save %s = %08x", key, value);
        result = nvs_set_i32(handle_nvs, key, value);
        ZERO_CHECK(result);
        ZERO_CHECK(nvs_commit(handle_nvs));
    }
    else
    {
        result = ESP_FAIL;
    }
    return result;
}

int config_set_blob(const char *const key, const void *const data, const size_t size)
{
    esp_err_t result = ESP_OK;
    nvs_handle_t handle_nvs = config_get_nvs(key);

    if (check_nvs_key(key))
    {
        LOG(D, "NVS save %s (%d bytes)", key, size);
        result = nvs_set_blob(handle_nvs, key, data, size);
        ZERO_CHECK(result);
        ZERO_CHECK(nvs_commit(handle_nvs));
    }
    else
    {
        result = ESP_FAIL;
    }
    return result;
}

int config_set_str(const char *const key, const char *const str)
{
    esp_err_t result = ESP_OK;
    nvs_handle_t handle_nvs = config_get_nvs(key);
    if (check_nvs_key(key))
    {
        LOG(D, "NVS save %s: %s", key, str);
        result = nvs_set_str(handle_nvs, key, str);
        ZERO_CHECK(result);
        ZERO_CHECK(nvs_commit(handle_nvs));
    }
    else
    {
        result = ESP_FAIL;
    }
    return result;
}

// general instance nvs helpers

// \~ Get a value from nvs key in the form "classXproperty"
esp_err_t config_get_instance(const nvs_handle_t handle_nvs, const int parameter_index, const int instance, void *const storage, size_t storage_size)
{
    char key_buffer[32];
    const DDM2_PARAMETER_LIST_DATA *const data = &Ddm2_parameter_list_data[parameter_index];
    esp_err_t result;

    snprintf(key_buffer, sizeof(key_buffer), "%s%d%s", data->device_class, instance, data->property);

    switch (data->out_type)
    {
    case DDM2_TYPE_INT32_T:
        result = nvs_get_i32(handle_nvs, key_buffer, storage);
        break;
    case DDM2_TYPE_STRING:
        memset(storage, 0, storage_size);
        result = nvs_get_str(handle_nvs, key_buffer, (char *)storage, &storage_size);
        break;
    case DDM2_TYPE_OTHER:
        memset(storage, 0, storage_size);
        result = nvs_get_blob(handle_nvs, key_buffer, storage, &storage_size);
        break;
    default:
        result = ESP_FAIL;
        break;
    }

    return result;
}

// \~ Set a value from nvs key in the form "classXproperty"
esp_err_t config_set_instance(const nvs_handle_t handle_nvs, const int parameter_index, const int instance, const void *const storage, size_t storage_size)
{
    char key_buffer[32];
    const DDM2_PARAMETER_LIST_DATA *const data = &Ddm2_parameter_list_data[parameter_index];
    esp_err_t result;

    snprintf(key_buffer, sizeof(key_buffer), "%s%d%s", data->device_class, instance, data->property);

    switch (data->out_type)
    {
    case DDM2_TYPE_INT32_T:
        result = nvs_set_i32(handle_nvs, key_buffer, *((int32_t *)storage));
        break;
    case DDM2_TYPE_STRING:
        result = nvs_set_str(handle_nvs, key_buffer, (char *)storage);
        break;
    case DDM2_TYPE_OTHER:
        result = nvs_set_blob(handle_nvs, key_buffer, storage, storage_size);
        break;
    default:
        result = ESP_FAIL;
        break;
    }
    if (ESP_OK == result)
    {
        ZERO_CHECK(nvs_commit(handle_nvs));
    }
    return result;
}

/*! \brief Get a value from nvs key in the form "tag|property|id"
    \param handle_nvs Handle to opened NVS context
    \param parameter_index Index into DDM2 parameter list
    \param tag Initial string of key
    \param id Data to attach as hex to the end of the key
    \param id_size Size of ID data
    \param storage Pointer to data store
    \param storage_size Size of data store
    \return ESP_OK if successfully loaded
 */
esp_err_t config_get_id(const nvs_handle_t handle_nvs, const int parameter_index, const char *const tag, const void *const id, size_t const id_size, void *const storage, size_t *const storage_size)
{
    char prefix_buffer[32];
    char key_buffer[32];
    const DDM2_PARAMETER_LIST_DATA *const data = &Ddm2_parameter_list_data[parameter_index];
    uint32_t property = DDM2_PARAMETER_PROPERTY_FIELD(data->parameter);
    esp_err_t result;

    snprintf(prefix_buffer, sizeof(prefix_buffer), "%s%02x", tag, property);  // tag+property

    if (!hexdump_string(key_buffer, sizeof(key_buffer), prefix_buffer, '\0', id, id_size))  // id
    {
        return 0;
    }

    switch (data->out_type)  // use nvs function appropriate to DDM2 parameter
    {
    case DDM2_TYPE_INT32_T:
        memset(storage, 0, *storage_size);
        result = nvs_get_i32(handle_nvs, key_buffer, storage);
        *storage_size = sizeof(int32_t);
        break;
    case DDM2_TYPE_STRING:
        memset(storage, 0, *storage_size);  // default to zero
        result = nvs_get_str(handle_nvs, key_buffer, (char *)storage, storage_size);
        if (result)
        {
            *storage_size = 0;
        }
        else
        {
            *storage_size = MAX(0, (*storage_size) - 1);  // do not count terminating NULL
        }
        break;
    case DDM2_TYPE_OTHER:
        memset(storage, 0, *storage_size);  // default to zero
        result = nvs_get_blob(handle_nvs, key_buffer, storage, storage_size);
        if (result)
        {
            *storage_size = 0;
        }
        break;
    default:
        result = ESP_FAIL;
        break;
    }

    return result;
}

/*! \brief Set a value from nvs key in the form "tag|property|id"
    \param handle_nvs Handle to opened NVS context
    \param parameter_index Index into DDM2 parameter list
    \param tag Initial string of key
    \param id Data to attach as hex to the end of the key
    \param id_size Size of ID data
    \param storage Pointer to data store
    \param storage_size Size of data store
    \return ESP_OK if successfully saved
 */
esp_err_t config_set_id(const nvs_handle_t handle_nvs, const int parameter_index, const char *const tag, const void *const id, size_t id_size, void *const storage, const size_t storage_size)
{
    char prefix_buffer[32];
    char key_buffer[32];
    const DDM2_PARAMETER_LIST_DATA *const data = &Ddm2_parameter_list_data[parameter_index];
    uint32_t property = DDM2_PARAMETER_PROPERTY_FIELD(data->parameter);
    esp_err_t result;

    snprintf(prefix_buffer, sizeof(prefix_buffer), "%s%02x", tag, property);  // tag+property

    if (!hexdump_string(key_buffer, sizeof(key_buffer), prefix_buffer, '\0', id, id_size))  // id
    {
        return 0;
    }

    switch (data->out_type)  // use nvs function appropriate to DDM2 parameter
    {
    case DDM2_TYPE_INT32_T:
        result = nvs_set_i32(handle_nvs, key_buffer, *((int32_t *)storage));
        break;
    case DDM2_TYPE_STRING:
        result = nvs_set_str(handle_nvs, key_buffer, (char *)storage);
        break;
    case DDM2_TYPE_OTHER:
        result = nvs_set_blob(handle_nvs, key_buffer, storage, storage_size);
        break;
    default:
        result = ESP_FAIL;
        break;
    }
    if (ESP_OK == result)
    {
        ZERO_CHECK(nvs_commit(handle_nvs));
    }

    return result;
}

void set_eolpass(int32_t value)
{
    config_set_i32("eolpass", value);
}

int32_t get_eolpass(void)
{
    int32_t value;
    config_get_i32("eolpass", &value);
    return value;
}

const esp_partition_t *get_log_partition(void)
{
    return esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_UNDEFINED, LOG_PARTITION);
}

const char *get_log_partition_label(void)
{
    return LOG_PARTITION;
}

static void erase_log_partition(void)
{
    const esp_partition_t *log_partition = get_log_partition();
    if (log_partition == NULL)
    {
        LOG(E, "'%s' partition does not exist in the active partition table!", LOG_PARTITION);
        return;
    }
    ZERO_CHECK(esp_partition_erase_range(log_partition, 0, log_partition->size));
}
