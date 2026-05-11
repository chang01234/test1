/*! \file hal_nvs.h
	\brief To provide abstraction layer for ESP32 NVS (Non Volatile Storage)
*/

#ifndef HAL_NVS_H_
#define HAL_NVS_H_

#include <stddef.h>
#include <stdint.h>

typedef enum _hal_nvs_data_type
{
    HAL_NVS_DATA_TYPE_INT8 = 0,
    HAL_NVS_DATA_TYPE_UINT8,
    HAL_NVS_DATA_TYPE_INT16,
    HAL_NVS_DATA_TYPE_UINT16,
    HAL_NVS_DATA_TYPE_INT32,
    HAL_NVS_DATA_TYPE_UINT32,
    HAL_NVS_DATA_TYPE_INT64,
    HAL_NVS_DATA_TYPE_UINT64,
    HAL_NVS_DATA_TYPE_STRING,    
    HAL_NVS_DATA_TYPE_ARRAY,
    HAL_NVS_DATA_TYPE_BLOB
}HAL_NVS_DATA_TYPE;

typedef struct _HAL_NVS_PARAMETER
{
	const char                     *key;
	const HAL_NVS_DATA_TYPE        type;
	const void                 *storage;
} HAL_NVS_PARAMETER;

#define HAL_NVS_OK                        ((int32_t)    0)
#define HAL_NVS_ERROR                     ((int32_t)    1)
#define HAL_NVS_ERROR_NO_DATA_STORED      ((int32_t)    2)

typedef int32_t hal_nvs_error;
#ifdef NOHAL 
#else
#include "nvs.h"
	extern nvs_handle_t nvs;
#endif
hal_nvs_error hal_nvs_init(void);
hal_nvs_error hal_nvs_open(const char *storage_namespace);
hal_nvs_error hal_nvs_close(void);
hal_nvs_error hal_nvs_erase_flash(void);
hal_nvs_error hal_nvs_flash_init(void);
hal_nvs_error hal_nvs_read(const char *key_name, HAL_NVS_DATA_TYPE data_type, void *out_value, uint32_t data_len);
hal_nvs_error hal_nvs_write(const char *key_name, HAL_NVS_DATA_TYPE data_type, const void *in_data, uint32_t data_len);

#endif 




