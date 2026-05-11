//------------------------------------------------------------------------------
// Module:      hal_nvs.c
//------------------------------------------------------------------------------
// Description: To provide abstraction layer for ESP32 NVS (Non Volatile Storage)
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include <stdio.h>
#include "dicm_framework_config.h"
#include "iGeneralDefinitions.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "hal_nvs.h"
#include "esp_log.h"
#ifdef NOHAL 
#else
    nvs_handle_t nvs;
#endif
/*! \brief HAL function for NVS init
	\param None
 */
hal_nvs_error hal_nvs_init(void)
{
    hal_nvs_error nvs_error = HAL_NVS_OK;
    esp_err_t err = nvs_flash_init();

    if ( ( err == ESP_ERR_NVS_NO_FREE_PAGES ) || ( err == ESP_ERR_NVS_NEW_VERSION_FOUND ) )
    {
        err = nvs_flash_erase();
        ZERO_CHECK(err);

        err = nvs_flash_init();
        ZERO_CHECK( err );
    }
    
    if ( ESP_OK != err )
    {
        nvs_error = HAL_NVS_ERROR;
    }

    return nvs_error;
}

/*! \brief HAL function for NVS open
	\param Namespace for storage
 */
hal_nvs_error hal_nvs_open(const char *storage_namespace)
{
    hal_nvs_error nvs_error = HAL_NVS_OK;
    esp_err_t err;

    err = nvs_open(storage_namespace, NVS_READWRITE, &nvs);
    
    ZERO_CHECK( err );

    if ( ESP_OK != err )
    {
        nvs_error = HAL_NVS_ERROR;
    }

    return nvs_error;
}

/*! \brief HAL function for NVS close
	\param none
 */
hal_nvs_error hal_nvs_close(void)
{
    hal_nvs_error nvs_error = HAL_NVS_OK;

    nvs_close(nvs);

    return nvs_error;
}

/*! \brief HAL function for NVS flash erase
	\param none
 */
hal_nvs_error hal_nvs_erase_flash(void)
{
    esp_err_t err;
    hal_nvs_error nvs_error;

    err = nvs_flash_erase();
    
    ZERO_CHECK(err);

    if ( ESP_OK != err )
    {
        /* Set the error code */
        nvs_error = HAL_NVS_ERROR;
    }

    return nvs_error;
}

/*! \brief HAL function for NVS flash init
	\param none
 */
hal_nvs_error hal_nvs_flash_init(void)
{
    esp_err_t err;
    hal_nvs_error nvs_error;

    err = nvs_flash_init();

    ZERO_CHECK(err);

    if ( ESP_OK != err )
    {
        /* Set the error code */
        nvs_error = HAL_NVS_ERROR;
    }

    return nvs_error;
}

/*! \brief HAL function for NVS read
	\param key       Name of the key
    \param datatype  Data Type
    \param out_value Pointer to get the read value
    \param data_len  Data Length
 */
hal_nvs_error hal_nvs_read(const char *key_name, HAL_NVS_DATA_TYPE data_type, void *out_value, uint32_t data_len)
{
    hal_nvs_error nvs_error = HAL_NVS_OK;
    esp_err_t err = ESP_FAIL;

    if ( ( out_value != NULL ) && ( key_name != NULL ) )
    {
        switch (data_type)
        {
            case HAL_NVS_DATA_TYPE_INT8:
                err = nvs_get_i8(nvs, key_name, (int8_t*)out_value);
                break;
            case HAL_NVS_DATA_TYPE_UINT8:
                err = nvs_get_u8(nvs, key_name, (uint8_t*)out_value);
                break;
            case HAL_NVS_DATA_TYPE_INT16:
                err = nvs_get_i16(nvs, key_name, (int16_t*)out_value);
                break;
            case HAL_NVS_DATA_TYPE_UINT16:
                err = nvs_get_u16(nvs, key_name, (uint16_t*)out_value);
                break;
            case HAL_NVS_DATA_TYPE_INT32:
                err = nvs_get_i32(nvs, key_name, (int32_t*)out_value);
                break;
            case HAL_NVS_DATA_TYPE_UINT32:
                err = nvs_get_u32(nvs, key_name, (uint32_t*)out_value);
                break;
            case HAL_NVS_DATA_TYPE_INT64:
                err = nvs_get_i64(nvs, key_name, (int64_t*)out_value);
                break;
            case HAL_NVS_DATA_TYPE_UINT64:
                err = nvs_get_u64(nvs, key_name, (uint64_t*)out_value);
                break;
            case HAL_NVS_DATA_TYPE_ARRAY:
            case HAL_NVS_DATA_TYPE_BLOB:
                err = nvs_get_blob(nvs, key_name, out_value, (size_t*)&data_len);
                break;
            case HAL_NVS_DATA_TYPE_STRING:
                err = nvs_get_str(nvs, key_name, (char*)out_value, (size_t*)&data_len);
                break;
            default:
                /*Do Nothing*/
                break;
        }
    }

    if ( ESP_OK != err )
    {
        /* Set the error code */
        if ( ESP_ERR_NVS_NOT_FOUND == err )
        {
            nvs_error = HAL_NVS_ERROR_NO_DATA_STORED;
            LOG(W, "No DATA stored");
        }
        else
        {
            ZERO_CHECK( err );
            nvs_error = HAL_NVS_ERROR;
        }
    }

    return nvs_error;
}

/*! \brief HAL function for NVS write
	\param key_name   Name of the key
    \param data_type  Data Type
    \param in_data    Pointer to write data
    \param data_len   Data Length
 */
hal_nvs_error hal_nvs_write(const char *key_name, HAL_NVS_DATA_TYPE data_type, const void *in_data, uint32_t data_len)
{
    hal_nvs_error nvs_error = HAL_NVS_OK;
    esp_err_t err = ESP_FAIL;

    if ( ( NULL != key_name ) && ( NULL != in_data ) )
    {
        switch (data_type)
        {
            case HAL_NVS_DATA_TYPE_INT8:
                err = nvs_set_i8(nvs, key_name, (int8_t)(*((int8_t*)(in_data))));
                break;
            case HAL_NVS_DATA_TYPE_UINT8:
                err = nvs_set_u8(nvs, key_name, (uint8_t)(*((uint8_t*)(in_data))));
                break;
            case HAL_NVS_DATA_TYPE_INT16:
                err = nvs_set_i16(nvs, key_name, (int16_t)(*((int16_t*)(in_data))));
                break;
            case HAL_NVS_DATA_TYPE_UINT16:
                err = nvs_set_u16(nvs, key_name, (uint16_t)(*((uint16_t*)(in_data))));
                break;
            case HAL_NVS_DATA_TYPE_INT32:
                err = nvs_set_i32(nvs, key_name, (int32_t)(*((int32_t*)(in_data))));
                break;
            case HAL_NVS_DATA_TYPE_UINT32:
                err = nvs_set_u32(nvs, key_name, (uint32_t)(*((uint32_t*)(in_data))));
                break;
            case HAL_NVS_DATA_TYPE_INT64:
                err = nvs_set_i64(nvs, key_name, (int64_t)(*((int64_t*)(in_data))));
                break;
            case HAL_NVS_DATA_TYPE_UINT64:
                err = nvs_set_u64(nvs, key_name, (uint64_t)(*((uint64_t*)(in_data))));
                break;
            case HAL_NVS_DATA_TYPE_ARRAY:
            case HAL_NVS_DATA_TYPE_BLOB:
                err = nvs_set_blob(nvs, key_name, (const void*)in_data, data_len);
                break;
            case HAL_NVS_DATA_TYPE_STRING:
                err = nvs_set_str(nvs, key_name, (char*)in_data);
                break;       
            default:
                /*Do Nothing*/
                break;
        }

        ZERO_CHECK( err );

        if ( ESP_OK == err )
        {
            err = nvs_commit(nvs);
        }
    }

    if ( ESP_OK != err )
    {
        /* Set the error code */
        nvs_error = HAL_NVS_ERROR;
    }

    return nvs_error;
}
