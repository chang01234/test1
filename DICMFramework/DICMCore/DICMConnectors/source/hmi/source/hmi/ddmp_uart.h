/*! \file ddmp_uart.h
 *	\brief Handles DDMP2 communication with ESP32.
 */

#ifndef DDMP_UART_H_
#define DDMP_UART_H_

/** Includes ******************************************************************/
#include <stdint.h>
#include <stddef.h>

#include "ddm2_parameter_list.h"

/** Defines *******************************************************************/
//! Struct holding definition data for a DDMP2 publish.
typedef struct DDMP_UART_PARAMETER
{
    const struct DDMP_UART_PARAMETER *next_entry;	//!< Next entry in linked list. NULL for last entry.
    uint32_t parameter_id;							//!< ID of DDM2 parameter.
    DDM2_TYPE_ENUM type;							//!< Type of DDM2 parameter.
    uint8_t var_index;								//!< HMI variable index used to store parameter value.
} DDMP_UART_PARAMETER;

/** Variables *****************************************************************/

/** Function prototypes *******************************************************/
int ddmp_uart_init(const DDMP_UART_PARAMETER *subs, const DDMP_UART_PARAMETER *pubs);
void ddmp_uart_service(uint32_t ticks);

int ddmp_uart_subscribe(uint32_t parameter_id);
int ddmp_uart_set(uint32_t parameter_id, int32_t value, size_t length);
int ddmp_uart_publish(uint32_t parameter_id, int32_t value, size_t length);

void ddmp_uart_varstate_cb(uint8_t state_index);

#endif /* DDMP_UART_H_ */
