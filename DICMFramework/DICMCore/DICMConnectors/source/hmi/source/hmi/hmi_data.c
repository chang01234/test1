/*!
 * \file hmi_data.c
 *
 *  Created on: 20 sep. 2023
 *      Author: Andlun
 */
#include <stdint.h>
#include <string.h>

#include "configuration.h"
#include "esp_idf_version.h"

#include "data_interface.h"
#include "draw.h"
#include "hmi_data_def.h"
#include "screen.h"

#include "esp_partition.h"
#include "nvs.h"
#include "nvs_flash.h"

#define HMI_DATA_INTERFACE_VERSION_NOT_COMP_STRING HMI_DATA_INTERFACE_VERSION_STRING "-nc"

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include "spi_flash_mmap.h"
#define PARTITION_MEMORY_TYPE ESP_PARTITION_MMAP_DATA
#else
#define PARTITION_MEMORY_TYPE SPI_FLASH_MMAP_DATA
#endif
DEF_DATA def_data_header;
IRAM_ATTR const void *hmi_data_out_ptr;

static spi_flash_mmap_handle_t l_out_handle;
static nvs_handle_t l_nvs_handle;
static uint8_t l_partition = 0;
static EXT_RAM_ATTR bool l_is_data_compatible;
static EXT_RAM_ATTR unsigned int l_hmi_data_major;
static EXT_RAM_ATTR unsigned int l_hmi_data_minor;
static EXT_RAM_ATTR unsigned int l_hmi_data_build;

void hmi_data_init(bool is_display_init)
{
    l_is_data_compatible = false;

    nvs_flash_init();
    // Find out which partition is used
    nvs_open("hmi_part", NVS_READWRITE, &l_nvs_handle);
    esp_err_t nvs_err = nvs_get_u8(l_nvs_handle, "part", &l_partition);
    if (ESP_OK != nvs_err)
    {
        // Default to 0
        l_partition = 0u;
        LOG(I, "Defaults to HMI_DATA partition 0");
        nvs_set_u8(l_nvs_handle, "part", l_partition);
        nvs_commit(l_nvs_handle);
    }
    else
    {
        LOG(I, "Using HMI_DATA partition %d", l_partition);
    }
    const esp_partition_t *part = hmi_data_get_partition(l_partition);
    LOG(I, "Found partition: %s at 0x%x, size 0x%x", part->label, part->address, part->size);
    esp_partition_read(part, 0, &def_data_header, sizeof(DEF_DATA));

    esp_err_t err = esp_partition_mmap(part, 0, part->size, PARTITION_MEMORY_TYPE, &hmi_data_out_ptr, &l_out_handle);
    LOG(I, "hmi_data_out_ptr: %p", hmi_data_out_ptr);
    LOG(I, "err: 0x%x : %s", err, esp_err_to_name(err));

    LOG(I, "def_data_header.varstate_conf: %p", ((DEF_DATA *)hmi_data_out_ptr)->varstate_conf);
    LOG(I, "spi_flash_cache2phys(((DEF_DATA*)hmi_data_out_ptr)->varstate_conf): 0x%x", spi_flash_cache2phys(hmi_data_out_ptr));
    LOG(I, "def_data_header.sub_list: %p", ((DEF_DATA *)hmi_data_out_ptr)->sub_list);
    LOG(I, "def_data_header.pub_list: %p", ((DEF_DATA *)hmi_data_out_ptr)->pub_list);
    LOG(I, "def_data_header.menu_boot_state: %p", ((DEF_DATA *)hmi_data_out_ptr)->menu_boot_state);
    LOG(I, "def_data_header.menu_global_state: %p", ((DEF_DATA *)hmi_data_out_ptr)->menu_global_state);
    LOG(I, "def_data_header.events: %p", ((DEF_DATA *)hmi_data_out_ptr)->events);
    LOG(I, "def_data_header.num_events: 0x%x", ((DEF_DATA *)hmi_data_out_ptr)->num_events);
    LOG(I, "def_data_header.draw_direction: %d", ((DEF_DATA *)hmi_data_out_ptr)->draw_direction);
    LOG(I, "HMI Version: %s", ((DEF_DATA *)hmi_data_out_ptr)->version);

    LOG(I, "HMI Engine interface version: %s", HMI_DATA_INTERFACE_VERSION_STRING);

    // Is found version compatible with the HMI Engine interface version?
    int res = strverscmp(HMI_DATA_INTERFACE_VERSION_BUILD_STRING, ((DEF_DATA *)hmi_data_out_ptr)->version);
    if (res == 0)
    {
        LOG(I, "Compatible versions, same");
        l_is_data_compatible = true;
    }
    else if (res < 0)
    {
        LOG(E, "Non-compatible versions. HMI Engine does not support this HMI data version");
        l_is_data_compatible = false;
    }
    else if (res > 0)
    {
        LOG(I, "Compatible versions. HMI Engine supports lower data version");
        l_is_data_compatible = true;
    }

    // Extract version components
    int result = sscanf(def_data_header.version, "%u.%u.%u", &l_hmi_data_major, &l_hmi_data_minor, &l_hmi_data_build);
    if (result != 3)
    {
        LOG(E, "Could not convert string into decimals (%d)", result);
        l_hmi_data_major = UINT32_MAX;
        l_hmi_data_minor = UINT32_MAX;
        l_hmi_data_build = UINT32_MAX;
    }
    if (l_is_data_compatible && is_display_init)
    {
        draw_init(def_data_header.draw_direction);
        LOG(D, "(def_data_header.menu_boot_state)->screen: %p", ((const MENU_STATE *)(HMI_DATA_ADDRESS(def_data_header.menu_boot_state)))->screen);
        LOG(D, "(def_data_header.menu_boot_state)->screen->palette: %p", ((const SCREEN_DEF *)HMI_DATA_ADDRESS(((const MENU_STATE *)(HMI_DATA_ADDRESS(def_data_header.menu_boot_state)))->screen))->palette);
        const SCREEN_DEF *screen = (const SCREEN_DEF *)HMI_DATA_ADDRESS(((const MENU_STATE *)(HMI_DATA_ADDRESS(def_data_header.menu_boot_state)))->screen);
        draw_screen_init((const PALETTE_DEF *)HMI_DATA_ADDRESS(screen->palette),
                         (const BITMAP_DEF *)HMI_DATA_ADDRESS(((const SCREEN_BACKGROUND *)HMI_DATA_ADDRESS(screen->first_background))->bitmap),
                         screen->fb_x_pos, screen->fb_y_pos,
                         screen->fb_x_size, screen->fb_y_size, false);
        draw_screen();
        screen_display_brightness_level_set(8);  // Default of [0-8] levels
        draw_set_suspend(HMI_WAKEUP);
    }
}

const char *hmi_data_get_version(void)
{
    if (l_is_data_compatible)
    {
        return def_data_header.version;
    }
    else
    {
        // Non-compatible. Return the HMI engine interface version to trigger correct HMI data download
        return HMI_DATA_INTERFACE_VERSION_NOT_COMP_STRING;
    }
}

const esp_partition_t *hmi_data_get_partition(uint8_t partition_index)
{
    const esp_partition_t *part;
    if (0 == partition_index)
    {
        part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_UNDEFINED, "hmi_data");
    }
    else
    {
        part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_UNDEFINED, "hmi_data_1");
    }
    return part;
}

uint8_t hmi_data_get_partition_index(void)
{
    return l_partition;
}

uint8_t hmi_data_get_next_partition_index(void)
{
    return (uint8_t)(l_partition ^ 1u);
}

bool hmi_data_save_partition_index(uint8_t partition_index)
{
    bool bReturn = false;
    if (partition_index <= 1)
    {
        // Correct argument
        esp_err_t err = nvs_set_u8(l_nvs_handle, "part", partition_index);
        err |= nvs_commit(l_nvs_handle);
        if (err == ESP_OK)
        {
            l_partition = partition_index;
            bReturn = true;
        }
    }
    return bReturn;
}

bool hmi_data_is_compatible(void)
{
    return l_is_data_compatible;
}

uint32_t hmi_data_get_major_version(void)
{
    return (uint32_t)l_hmi_data_major;
}

uint32_t hmi_data_get_minor_version(void)
{
    return (uint32_t)l_hmi_data_minor;
}

uint32_t hmi_data_get_build_version(void)
{
    return (uint32_t)l_hmi_data_build;
}

bool hmi_data_is_feature_supported(hmi_data_features_t hmi_data_feature)
{
    bool ret_val = false;
    if (hmi_data_feature == HMI_DATA_FEATURE_DYNAMIC_MAPPING)
    {
        if ((uint32_t)l_hmi_data_major >= (uint32_t)HMI_DATA_FEATURE_DYNAMIC_MAPPING_MIN_MAJOR_VERSION)
        {
            ret_val = true;
        }
    }
    return ret_val;
}
