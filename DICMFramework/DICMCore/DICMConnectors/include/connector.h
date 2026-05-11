/*****************************************************************************
 * \file       connector.h
 * \brief      List of connector used by the application
 * \copyright  Dometic Group
 *             This source file and the information contained in it are
 *             confidential and proprietary to Dometic Group
 *             The reproduction or disclosure, in whole or in part,
 *             to anyone outside of Dometic Group without the written
 *             approval of a Dometic Group officer under a Non-Disclosure
 *             Agreement is expressly prohibited.
 *
 *             All rights reserved
 *****************************************************************************/
#ifndef CONNECTOR_H_
#define CONNECTOR_H_
#include "iGeneralDefinitions.h"
#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include "ddm2.h"
#include "hal_mem.h"
#include "esp_heap_caps.h"

#ifndef TO_BROKER_RINGBUFFER_SIZE
#define TO_BROKER_RINGBUFFER_SIZE			(1024 * 4)
#endif
#ifndef TO_CONNECTOR_RINGBUFFER_SIZE
#define TO_CONNECTOR_RINGBUFFER_SIZE		(10 * 1024)
#endif
#define CONNECTOR_SUB_DEPTH					512

#define CONNECTOR_INIT_SUCCESS				1
#define CONNECTOR_INIT_FAILURE				0

/*****************************************************************************
 * Public types
 *****************************************************************************/
typedef int (*CONNECTOR_INIT)(void);
typedef void(*CONNECTOR_TASK_INIT)(void);
typedef void(*CONNECTOR_PROCESS_EVENT)(const DDMP2_FRAME * frame);

typedef struct CONNECTOR
{
	RingbufHandle_t to_broker;						//!< \~ Incoming ringbuffer to broker,					4B	provided by broker
	RingbufHandle_t to_connector;					//!< \~ Outgoing ringbuffer to connector,				4B	provided by broker (broker_ringbuffer_handle)
	char *name;										//!< \~ Human-readable name of connector,				4B	provided by connector
	CONNECTOR_INIT initialize;						//!< \~ Pointer to connector initialization function,	4B	provided by connector
	CONNECTOR_TASK_INIT task_init;					//!< \~ Pointer to connector init function (optional),	4B	provided by connector
	CONNECTOR_PROCESS_EVENT process_event;			//!< \~ Pointer to connector run function (optional),	4B	provided by connector
	uint8_t data_lines;								//!< \~ Extra connector IDs for data lines				1B	provided by connector, changed to sub connector ID by broker
	uint8_t connector_id;							//!< \~ Connector ID,									1B	provided by broker
	uint8_t disabled;                               //!< \~ Disabled means it should not be initialized     1B  provided by connector
} CONNECTOR;

typedef struct CONNECTOR_SLOT
{
	StaticRingbuffer_t ringbuffer;					//!< \~ Ringbuffer data structure,						236B provided by broker
	RingbufHandle_t to_broker;						//!< \~ Incoming ringbuffer to broker,					4B	provided by broker
	RingbufHandle_t to_connector;					//!< \~ Outgoing ringbuffer to connector,				4B	provided by broker (broker_ringbuffer_handle)
	char *name;										//!< \~ Human-readable name of connector,				4B	provided by connector
	CONNECTOR_INIT initialize;						//!< \~ Pointer to connector initialization function,	4B	provided by connector
	CONNECTOR_TASK_INIT task_init;					//!< \~ Pointer to connector init function (optional),	4B	provided by connector
	CONNECTOR_PROCESS_EVENT process_event;			//!< \~ Pointer to connector run function (optional),	4B	provided by connector
	int connector_id;								//!< \~ Connector ID,									4B	provided by broker
	uint32_t task_timer;							//!< \~ Time of last call of process_event				4B	provided by broker
	int sub_connector_id;							//!< \~ Sub connector ID,								4B	provided by broker
} CONNECTOR_SLOT;

/*****************************************************************************
 * Public variables
 *****************************************************************************/
extern int connector_count;
extern CONNECTOR_SLOT ** connectors;

int connector_send_frame_to_broker(const DDMP2_CONTROL_ENUM control, const uint32_t parameter, const void * value, const uint8_t value_size, const uint8_t source_connector, const TickType_t timeout);
int connector_send_frame_to_connector(const DDMP2_CONTROL_ENUM control, const uint32_t parameter, const void * const value, const uint8_t value_size, const uint8_t connector, const TickType_t timeout);
int connector_forward_frame_to_connector(const DDMP2_FRAME * pframe, const int destination_connector);
int connector_forward_frame_to_broker(const DDMP2_FRAME * const pframe);
int connectors_enabled(void);
void install_connectors(const RingbufHandle_t broker_queue_handle, CONNECTOR_PROCESS_EVENT extra_task);

static inline void *connector_wait_for_frame(const CONNECTOR * const connector, size_t * msg_size)
{
	void *ddmp_msg = xRingbufferReceive(connector->to_connector, msg_size, portMAX_DELAY);
	TRUE_CHECK(ddmp_msg != NULL);
	return ddmp_msg;
}

void disable_connectors(void);

/*****************************************************************************/
#endif // CONNECTOR_H_
