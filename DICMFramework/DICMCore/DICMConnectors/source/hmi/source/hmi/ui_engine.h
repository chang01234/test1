/*! \file ui_engine.h
	\brief Responsible for managing the data of the model.
    Manipulates the data according the controller output or internal(delayed) events.
*/

#ifndef UI_ENGINE_H_
#define UI_ENGINE_H_

#include <stdint.h>
#include <stddef.h>

#include "event.h"
#include "menu.h"

// Public functions
void ui_engine_init(const EVENT_DEF * data_event_list, uint8_t data_num_events, const MENU_STATE * menu_boot_state, void(ddmp_set_cb)(uint32_t, uint8_t));
void ui_engine_get_event_definition_data(uint32_t event_id, void * const data, size_t data_size);
void ui_engine_process_event(uint32_t event_id, const void * const data, size_t data_size);

#endif //UI_ENGINE_H_