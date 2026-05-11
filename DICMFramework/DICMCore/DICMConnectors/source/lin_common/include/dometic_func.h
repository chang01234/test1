/*
 * dometic.h
 *
 *  Created on: 17 Feb 2020
 *      Author: Stefan.Henningsohn
 */

#ifndef DOMETIC_FUNC_H_
#define DOMETIC_FUNC_H_

#include "pstore.h"
#include <stdint.h>
#include "ddm2_parameter_list.h"

extern int32_t dometic_ac_mode_set(pstore_table_t eIndex, int32_t i32Value);
extern int32_t dometic_ac_mode_get(void);
extern int32_t dometic_ac_target_temp_set(pstore_table_t eIndex, int32_t i32Value);
extern int32_t dometic_ac_target_temp_convert(int32_t i32Value);
extern int32_t dometic_ac_fan_speed_set(pstore_table_t eIndex, int32_t i32Value);
extern int32_t dometic_ac_fan_speed_get(pstore_table_t eIndex, DDM2_TYPE_ENUM type);

#endif //DOMETIC_FUNC_H_
