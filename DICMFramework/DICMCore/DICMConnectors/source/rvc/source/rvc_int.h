/*
 * rvc_int.h
 *
 *  Created on: 17 feb. 2023
 *      Author: Andlun
 */

#ifndef RVC_INT_H_
#define RVC_INT_H_
#include <stdint.h>

uint8_t get_dimm_instance_app(void);
void set_dimm_instance_app(uint8_t instance);
uint8_t get_cur_oper(void);
void set_cur_oper(uint8_t cur_op);

#endif /* RVC_INT_H_ */
