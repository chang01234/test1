/*
 * nk_eds_port.h
 *
 *  Created on: Dec 2, 2021
 *      Author: nenad
 */

#ifndef NEON_KIT_GENERIC_SOURCE_NK_EDS_PORT_H_
#define NEON_KIT_GENERIC_SOURCE_NK_EDS_PORT_H_

#include <stdint.h>
#include <stddef.h>

#include "eds_port/eds_port_definition.h"

struct eds_port__sleep;

void
eds_port__sleep_init(struct eds_port__sleep *sleep);
void
eds_port__sleep_wait(struct eds_port__sleep *sleep);
void
eds_port__sleep_signal(struct eds_port__sleep *sleep);

struct eds_port__critical;

void
eds_port__critical_lock(struct eds_port__critical *critical);
void
eds_port__critical_unlock(struct eds_port__critical *critical);

uint_fast8_t
eds_port__ffs(uint32_t value);

size_t
eds_port__align_up(size_t non_aligned_value);

uint32_t
eds_port__tick_duration_ms(void);

uint32_t
eds_port__tick_from_ms(uint32_t ms);

void
eds_port__init(void);

#endif /* NEON_KIT_GENERIC_SOURCE_NK_EDS_PORT_H_ */
