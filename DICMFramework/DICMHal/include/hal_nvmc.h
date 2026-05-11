/*! \file hal_nvmc.h
	\brief Flash memory Hardware Abstraction Layer
    
    Used to store data in flash. Data is referred to using 4 byte keys.
*/

#ifndef HAL_NVMC_H
#define HAL_NVMC_H

#include <stddef.h>
#include <stdint.h>

//! \~ General HAL NVMC return values
typedef enum HAL_NVMC_RESULT_ENUM
{
    HAL_NVMC_OK,
    HAL_NVMC_KEY_NOT_FOUND,
    HAL_NVMC_CRC_CHECK_FAILED,
} HAL_NVMC_RESULT_ENUM;

/*! \brief Flash memory Initialization
 *        
 *  Initializer for NVMC HAL. Must be called before calling any other HAL
 *  functions. Should only be called once.
 *  
 *  \return HAL_NVMC_OK if successful. Any other value otherwise
 */
HAL_NVMC_RESULT_ENUM hal_nvmc_init(void);

/*! \brief Write len bytes from buffer to flash memory record accessed by key.
 *  
 *  Any existing record using key is overwritten.
 *
 *  \param key      Key of record to be written.
 *  \param buffer   Pointer to the data which should be written.
 *  \param len      Number of bytes to be written.
 *
 *  \returns HAL_NVMC_OK if successful. Any other value otherwise
 */
HAL_NVMC_RESULT_ENUM hal_nvmc_write(uint32_t key, const void *data, uint16_t len);


/*! \brief Read len bytes from record accessed by key to buffer.
 *
 *  Attempting to read a nonexistent record returns HAL_NVMC_KEY_NOT_FOUND.
 *  Attempting to read a corrupted record returns HAL_NVMC_CRC_CHECK_FAILED.
 *  In both cases data in buffer will be untouched.   
 *
 *  \param key      Key of record to be read.
 *  \param buffer   Pointer to buffer into which data will be read.
 *  \param len      Number of bytes to be read.
 *
 *  \returns HAL_NVMC_OK if successful. Any other value otherwise
 */
HAL_NVMC_RESULT_ENUM hal_nvmc_read(uint32_t key, void *data, uint16_t len);
 

/*! \brief Delete key from flash memory.
 *
 *  Attempting to delete a nonexistent record returns HAL_NVMC_KEY_NOT_FOUND.
 *
 *  \param key      Key of record to be erased.
 *
 *  \returns HAL_NVMC_OK if successful. Any other value otherwise
 */
HAL_NVMC_RESULT_ENUM hal_nvmc_delete(uint32_t key);


#endif //HAL_NVMC_H