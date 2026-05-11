/*! \file display_manager.h
 *  \brief Renders the presentation of the model according to the screen definition.
*/

#ifndef DISPLAY_MANAGER_H_
#define DISPLAY_MANAGER_H_

#include <stdint.h>
#include <stddef.h>

void diplay_manager_process_event(uint32_t event_id, const uint8_t * const data, size_t data_size);

#endif //DISPLAY_MANAGER_H_
