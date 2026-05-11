/*!
 * \file hmi_data.h
 * \brief API for managing and initializing which HMI data partition to use
 *
 *  Created on: 20 sep. 2023
 *      Author: Andreas Lundeen
 */

#ifndef HMI_DATA_H_
#define HMI_DATA_H_
#include <stdint.h>
#include <stdbool.h>
#include "iGeneralDefinitions.h"
#include "esp_partition.h"

/** Defines the interface version supported by the HMI engine */
#define HMI_DATA_INTERFACE_VERSION_VAL(major, minor, patch) (((major) << 16) | ((minor) << 8) | (patch))

/**
 * Current HMI data interface version, as an integer
 *
 * To be used in comparisons, such as HMI_DATA_INTERFACE_VERSION >= HMI_DATA_INTERFACE_VERSION_VAL(2, 0, 0)
 */
#define HMI_DATA_INTERFACE_VERSION  HMI_DATA_INTERFACE_VERSION_VAL(HMI_DATA_INTERFACE_VERSION_MAJOR, \
                                                                   HMI_DATA_INTERFACE_VERSION_MINOR, \
                                                                   HMI_DATA_INTERFACE_VERSION_BUILD)

#define HMI_DATA_INTERFACE_VERSION_BUILD_STRING   STR(HMI_DATA_INTERFACE_VERSION_MAJOR) "." STR(HMI_DATA_INTERFACE_VERSION_MINOR) "." STR(HMI_DATA_INTERFACE_VERSION_BUILD)
#define HMI_DATA_INTERFACE_VERSION_STRING   STR(HMI_DATA_INTERFACE_VERSION_MAJOR) "." STR(HMI_DATA_INTERFACE_VERSION_MINOR) ".0"
/**
 * @brief Initializes the management of HMI data partition
 *
 * @param[in] is_display_init if set to true also the display will be initialized with defined boot screen
 */
void hmi_data_init(bool is_display_init);

/**
 * @brief Get a pointer to HMI data version string
 * @return Pointer to version string
 */
const char *hmi_data_get_version(void);

/**
 * @brief Get a pointer to requested partition
 * @param[in] partition_index 0/1 - partition to get
 * @return Pointer to esp_partition requested
 */
const esp_partition_t *hmi_data_get_partition(uint8_t partition_index);

/**
 * @brief Get the currently stored partition index
 * @return 0 or 1
 */
uint8_t hmi_data_get_partition_index(void);

/**
 * @brief Get the next partition index that is unused
 * @return 0 or 1
 */
uint8_t hmi_data_get_next_partition_index(void);

/**
 * @brief Stores the given partition index in non-volatile memory
 * @param partition_index index to be stored (0/1)
 * @return true if successful
 */
bool hmi_data_save_partition_index(uint8_t partition_index);

#endif /* HMI_DATA_H_ */
