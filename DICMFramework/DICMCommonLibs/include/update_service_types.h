/*****************************************************************************
* \file       update_service_types.h
* \brief      TDB
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
#ifndef UPDATE_SERVICE_TYPES_H
#define UPDATE_SERVICE_TYPES_H

#include <stdint.h>
#include "ddm2.h"

/*****************************************************************************
 * Public types
 ****************************************************************************/

#ifdef _MSC_VER
#pragma pack(push)
#pragma pack(1)
#endif //_MSC_VER

typedef enum
{
	US_ACK_RESULT_OK,
	US_ACK_RESULT_NOTOK,
	US_ACK_RESULT_RESEND,
	US_ACK_RESULT_ABORT,
	US_ACK_RESULT_OK_RESTART
} us_ack_result_t;

typedef struct
{
	char filedes[1];		//!< \~ File descriptor (JSON string)
} PACKED us_des_descriptor_t;

typedef struct
{
	char filename[1];		//!< \~ Filename to download
} PACKED us_dl_download_t;

typedef struct
{
	uint32_t size;
	uint32_t crc;
	uint32_t service;	// 0x1=Service 0 should be updated, 0x3= service 0 and 1 should be updated, 0x8 1000 0xFFFFFFFF
	char fwid[1];		//!< \~ Downloaded firmware FWID
} PACKED us_dd_download_done_t;

typedef struct
{
	uint8_t version;		//!< \~ Update version supported
	uint16_t frame_size;	//!< \~ Frame size service can handle
	uint16_t block_size;	//!< \~ Max block size data service can handle
	char transfer_info[1];	//!< \~ FWID, SSID and password (of AP) separated by null
} PACKED us_rrq_read_request_t;

typedef struct
{
	uint8_t data[1];		//!< \~ Data
} PACKED us_data_frame_t;

typedef struct
{
	uint8_t result;			//!< \~ Result (Ack)
} PACKED usm_data_read_ack_t;

typedef struct
{
	uint8_t result;			//!< \~ Result (Ack)
	uint32_t crc;			//!< \~ Block transfer crc
} PACKED us_data_read_ack_t;

typedef struct
{
	uint16_t block_size;	//!< \~ Max block size manager wants from App
	char transfer_info[1];	//!< \~ FWID, SSID and password (of AP) separated by null
} PACKED usm_rrq_read_request_t;

#ifdef _MSC_VER
#pragma pack(pop)
#endif //_MSC_VER

// FSM event definitions
// FSM event definitions
#define USMDD_SET_EVENT	(2u)
#define USMDD_PUB_EVENT	(3u)
#define USMDATA_SET_EVENT	(4u)
#define USMDATA_PUB_EVENT	(5u)
#define USRRQ_SET_EVENT	(6u)
#define USRRQ_PUB_EVENT	(7u)
#define USDATA_SET_EVENT	(8u)
#define USDATA_PUB_EVENT	(9u)
#define USBTR_SET_EVENT	(10u)
#define USBTR_PUB_EVENT	(11u)
#define USSTAT_SET_EVENT	(12u)
#define USSTAT_PUB_EVENT	(13u)
#define USMTBS_SET_EVENT	(14u)
#define USMTBS_PUB_EVENT	(15u)
#define USMSTAT_SET_EVENT	(16u)
#define USMSTAT_PUB_EVENT	(17u)
#define USMVER_SET_EVENT	(18u)
#define USMMODE_SET_EVENT	(19u)
#define USMSTATE_SET_EVENT	(20u)
#define USPROG_PUB_EVENT	(21u)

/*****************************************************************************
 * Public functions
 ****************************************************************************/

#endif // UPDATE_SERVICE_TYPES_H
