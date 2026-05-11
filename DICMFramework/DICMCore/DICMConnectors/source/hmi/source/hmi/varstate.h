/*! \file varstate.h
 *	\brief Varstate module defines and function headers.
 *
 *	Make sure HMI_VARSTATE_DEBUG_NAMES is defined globally when building fw
 *	and hmi_data for debugging variable names
 */
 
#ifndef VARSTATE_H_
#define VARSTATE_H_

/** Includes ******************************************************************/
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "ddm2_parameter_list.h"
#include "hmi_data_def.h"

/** Defines *******************************************************************/
#define VARSTATE_INVALID_INDEX  ((uint8_t)0xFF)

extern int32_t *p_default_val;
extern int32_t *p_ddmp_set_sub_array;

/** Function prototypes *******************************************************/
void varstate_init(const VARSTATE_CONF *config, void (*varstate_cb)(uint8_t));
const VARSTATE_DYNENTRY_DEF *varstate_get_dyn_table_entry(int index);
const VARSTATE_DYNENTRY_DEF *varstate_find_any_dyn_table_entry_from_var_index(uint8_t var_index);
void varstate_set(uint8_t var_index, int32_t value);
void varstate_set_no_screen_upd(uint8_t var_index, int32_t value);
void varstate_set_changed(uint8_t var_index, bool changed);
void varstate_clear_all_changed(void);
int32_t varstate_get(uint8_t var_index);
bool varstate_get_validated_data(bool *is_struct, uint8_t var_index, int32_t *p_outdata, uint8_t offset, uint8_t len);
bool varstate_get_changed(uint8_t var_index);
const VARSTATE_DEF *varstate_def_get(uint8_t var_index);
void varstate_get_first_and_last_index(uint8_t * first_index, uint8_t * last_index);
uint8_t varstate_get_var_index(uint32_t param1);
uint8_t varstate_get_var_index_both(uint32_t param1);

uint32_t varstate_get_parameter_id(uint8_t var_index);
uint8_t varstate_get_parameter_type(uint8_t var_index, DDM2_TYPE_ENUM *ptype);

void clear_fav_confirm_button(uint8_t fav_confirm_but_arr_index);
void set_hmi_state_array(uint8_t hmi_state_index, uint32_t hmi_state_value);
void load_default_values_to_ddmp_set_array(void);
void ddmp_varstate_set(uint8_t var_index, int32_t value);
void ddmp_uart_varstate_set(uint8_t var_index, int32_t value);

#endif /* VARSTATE_H_ */
