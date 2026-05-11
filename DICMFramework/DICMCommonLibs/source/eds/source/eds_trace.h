/*
 * eds_trace.h
 *
 *  Created on: Dec 22, 2021
 *      Author: nenad
 */

#ifndef NEON_KIT_EDS_SOURCE_EDS_TRACE_H_
#define NEON_KIT_EDS_SOURCE_EDS_TRACE_H_

#include <stdint.h>

#define EDS_TRACE__SOURCE_AGENT_CREATE      0x01
#define EDS_TRACE__SOURCE_NETWORK_ADD_AGENT 0x02
#define EDS_TRACE__SOURCE_ETIMER_CREATE     0x03
#define EDS_TRACE__SOURCE_AGENT_SEND        0x04
#define EDS_TRACE__SOURCE_EVENT_CREATE      0x05
#define EDS_TRACE__SOURCE_EVENT_CANCEL      0x06
#define EDS_TRACE__SOURCE_EVENT_KEEP        0x07
#define EDS_TRACE__SOURCE_EVENT_TOSS        0x08
#define EDS_TRACE__SOURCE_ETIMER_SEND_AFTER 0x09
#define EDS_TRACE__SOURCE_ETIMER_SEND_EVERY 0x0a
#define EDS_TRACE__SOURCE_ETIMER_CANCEL     0x0b
#define EDS_TRACE__SOURCE_EDS_TMR           0xf0
#define EDS_TRACE__SOURCE_EDS_SMP           0xf1

#if defined(EDS_TRACE__ENABLE)
#define EDS_TRACE__EXIT(source, error, format, ...)     eds_trace__error(source, error, format, __VA_ARGS__)
#else
#define EDS_TRACE__EXIT(source, error, format, ...)
#endif

#if defined(EDS_TRACE__ENABLE)
#define EDS_TRACE__INFO(source, format, ...)            eds_trace__info(source, format, __VA_ARGS__)
#else
#define EDS_TRACE__INFO(source, format, ...)
#endif

extern const char * eds_trace__g__sources[];

extern void
eds_trace__error(uint32_t, uint32_t, const char * , ...);

extern void
eds_trace__info(uint32_t, const char *, ...);

#endif /* NEON_KIT_EDS_SOURCE_EDS_TRACE_H_ */
