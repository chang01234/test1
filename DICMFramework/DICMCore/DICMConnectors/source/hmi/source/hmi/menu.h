/*! \file menu.h
 *  \brief Menu module public defines and function headers.
 *
 *  Make sure HMI_MENU_DEBUG_NAMES is defined globally when building both hmi_data
 *  and fw when debugging state names
 */
 
#ifndef MENU_H_
#define MENU_H_

/** Includes ******************************************************************/
#include <stdint.h>

#include "hmi_data_def.h"

#include "screen.h"

/** Defines *******************************************************************/
typedef void(*menu_ddmp_set_cb)(uint32_t, uint8_t);
/** Function prototypes *******************************************************/
void menu_init(const MENU_STATE *boot_state, menu_ddmp_set_cb ddmp_set_cb);
void menu_update(uint8_t menu_event);

#endif /* MENU_H_ */
