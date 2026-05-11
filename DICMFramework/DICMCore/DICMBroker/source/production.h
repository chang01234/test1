/*
 * production.h
 *
 *  Created on: 15 feb. 2024
 *      Author: Andlun
 */

#ifndef PRODUCTION_H_
#define PRODUCTION_H_

#include <stdint.h>
#include "ddm2.h"

void production_init(void);
void production_handle_subscribe(const DDMP2_FRAME * const pframe);
void production_handle_set(const DDMP2_FRAME * const pframe);

#endif /* PRODUCTION_H_ */
