/*!
 * \file hmi_data_def.h
 * \brief Internal API for managing the HMI data
 *
 *  Created on: 22 sep. 2023
 *      Author: Andreas Lundeen
 */

#ifndef HMI_DATA_DEF_H_
#define HMI_DATA_DEF_H_
#include <stdint.h>

#include "data_interface.h"
#include "hmi_data.h"

extern DEF_DATA def_data_header;
extern const void *hmi_data_out_ptr;

#define HMI_DATA_ADDRESS(x) (((uint32_t)hmi_data_out_ptr + ((uint32_t)(x) - (uint32_t)0x3F700000)))

#define HMI_DATA_DYNAMIC_PARAM_MASK 0x80000000

typedef enum
{
    HMI_DATA_FEATURE_DYNAMIC_MAPPING = 0,
    HMI_DATA_FEATURE_MAX
} hmi_data_features_t;

#define HMI_DATA_FEATURE_DYNAMIC_MAPPING_MIN_MAJOR_VERSION  2

/**
 * @brief Check is HMI engine and HMI data versions are compatible
 * @return true of false
 */
bool hmi_data_is_compatible(void);

/**
 * @brief Get major version of HMI data in decimal
 * @return version or UINT32_MAX if failed to parse version string
 */
uint32_t hmi_data_get_major_version(void);

/**
 * @brief Get minor version of HMI data in decimal
 * @return version or UINT32_MAX if failed to parse version string
 */
uint32_t hmi_data_get_minor_version(void);

/**
 * @brief Get build version of HMI data in decimal
 * @return version or UINT32_MAX if failed to parse version string
 */
uint32_t hmi_data_get_build_version(void);

bool hmi_data_is_feature_supported(hmi_data_features_t hmi_data_feature);

#endif /* HMI_DATA_DEF_H_ */
