#ifndef DDM_LOG_SPEC_COMP_H_
#define DDM_LOG_SPEC_COMP_H_

#include <stdint.h>

#include "connector.h"
#include "ddm2.h"

/* Forward declaration */
struct ddm_log_spec_parser__spec_data;

/**
 * @brief Compose DDM ID name
 *  Takes ddm paratemer id and composes a string with its name
 *  Output format: 'ac0on' or 'ac0ttemp' etc..
 * 
 * @param spec_data       Points to a \a ddm_log_spec_parser__spec_data structure.
 * @param value           Will be assigned to memory location where the string is stored
 * @param len             The lenght of the string will be stored in \a len
 * 
 * \pre       Parameter \a spec_data_frame must be a non-NULL pointer
 * \pre       Parameter \a value might be a non-NULL pointer
 * \pre       Parameter \a len must be a non-NULL pointer
 * 
 * @return unsuccessful copmposition: -1
 */
int ddm_log_spec_composer__comp_ddm_id(const struct ddm_log_spec_parser__spec_data * spec_data, char ** value, size_t * len);

/**
 * @brief Compose DDM storage location
 *  Takes \a ddm_log_spec_parser__spec_storage and composes a string suitable memory name
 *  Output format: 'RAM' or 'FLASH'
 * 
 * @param spec_data       Points to a \a ddm_log_spec_parser__spec_data structure.
 * @param value           Will be assigned to memory location where the string is stored
 * @param len             The lenght of the string will be stored in \a len
 * 
 * \pre       Parameter \a spec_data_frame must be a non-NULL pointer
 * \pre       Parameter \a value might be a non-NULL pointer
 * \pre       Parameter \a len must be a non-NULL pointer
 * 
 * @return unsuccessful copmposition: -1
 */
int ddm_log_spec_composer__comp_ddm_storage(const struct ddm_log_spec_parser__spec_data * spec_data, char ** value, size_t * len);

/**
 * @brief Compose DDM retention
 *  Takes retention period and composes a string in predefined format
 *  Output format: '20Y#3' or '5d#1' etc...
 *  
 * @param spec_data       Points to a \a ddm_log_spec_parser__spec_data structure.
 * @param value           Will be assigned to memory location where the string is stored
 * @param len             The lenght of the string will be stored in \a len
 * 
 * \pre       Parameter \a spec_data_frame must be a non-NULL pointer
 * \pre       Parameter \a value might be a non-NULL pointer
 * \pre       Parameter \a len must be a non-NULL pointer
 * 
 * @return unsuccessful copmposition: -1
 */
int ddm_log_spec_composer__comp_ddm_retention(const struct ddm_log_spec_parser__spec_data * spec_data, char ** value, size_t * len);

/**
 * @brief Compose DDM interval
 *  Takes interval and composes a string in predefined format
 *  Output format: '20Y 30d 5h' or '1h 15min' or '1d 5h 2min' etc...
 * 
 * @param spec_data       Points to a \a ddm_log_spec_parser__spec_data structure.
 * @param value           Will be assigned to memory location where the string is stored
 * @param len             The lenght of the string will be stored in \a len
 * 
 * \pre       Parameter \a spec_data_frame must be a non-NULL pointer
 * \pre       Parameter \a value might be a non-NULL pointer
 * \pre       Parameter \a len must be a non-NULL pointer
 * 
 * @return unsuccessful copmposition: -1
 */
int ddm_log_spec_composer__comp_ddm_interval(const struct ddm_log_spec_parser__spec_data * spec_data, char ** value, size_t * len);

/**
 * @brief Compose DDM filter
 *  Takes filter and composes a string in predefined format
 *  Output format: 
 * 
 * @param spec_data       Points to a \a ddm_log_spec_parser__spec_data structure.
 * @param value           Will be assigned to memory location where the string is stored
 * @param len             The lenght of the string will be stored in \a len
 * 
 * \pre       Parameter \a spec_data_frame must be a non-NULL pointer
 * \pre       Parameter \a value might be a non-NULL pointer
 * \pre       Parameter \a len must be a non-NULL pointer
 * 
 * @return unsuccessful copmposition: -1
 */
int ddm_log_spec_composer__comp_ddm_filter(const struct ddm_log_spec_parser__spec_data * spec_data, char ** value, size_t * len);

#endif // DDM_LOG_SPEC_COMP_H_