/*
 * connector_prem_rvc.c
 *
 *  Created on: 4 Sep 2019
 *      Author: Stefan.Henningsohn
 */

#include "configuration.h"

#include <string.h>
#include "driver/gpio.h"
#include "connector.h"
#include "broker.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "ddm2.h"
#include "freertos/event_groups.h"
#include "ddm2_parameter_list.h"
#include "esp_log.h"
#include "hal_cpu.h"
#include "HALCAN.h"
#include "NMEA2K.h"
#include "RVCDGN.h"
#include "MsgCAN.h"
#include "PImage.h"
#include "rvc_to_ddm.h"
#include "esp_pm.h"
#include "esp_sleep.h"
#include "hal_can.h"
#include "freertos/timers.h"
#include "product_database.h"

#include "hmi_data.h"

#define HMI_SRC_ADDRESS 88 /* Main Thermostat: from Table 7.2 RV-C Full Layer-05-14-22_0 specification */

#define MY_CAN_DEVICE (HAL_CAN_DEVICE)BSP_CAN_RVC
//#define HMI_EXTENDED_LOG
//#define HMI_IGNORE_CRITICAL_ERROR
//#define CAN_EXTENDED_LOG
//#define TWAI_EXTENDED_LOG

#define HTR0ATEMP_10_DEGC                             (10  * Ddm2_unit_factor_list[DDM2_UNIT_DEGC])
#define HTR0ATEMP_35_DEGC                             (35  * Ddm2_unit_factor_list[DDM2_UNIT_DEGC])
#define HTR0ATEMP_MINUS_50_DEGC                       (-50 * Ddm2_unit_factor_list[DDM2_UNIT_DEGC])
#define HTR0ATEMP_IS_NOT_IN_RANGE(temp)          \
	((((temp) > HTR0ATEMP_MINUS_50_DEGC && (temp) < HTR0ATEMP_10_DEGC)) || ((temp) > HTR0ATEMP_35_DEGC))

#define VAR_TYPE_UINT8  1
#define VAR_TYPE_UINT16 2
#define VAR_TYPE_INT8   4
#define VAR_TYPE_INT16  5
#define VAR_TYPE_INT32  6

#define RESPONSE_TICK          (pdMS_TO_TICKS(3000)) /* 4 seconds, it should be greater than the timeout given to set the Target temperature to get proper status back */
#define WTR_FROZEN_WARN        425          /* Water frozen warning */
#define TIMER_COUNT_10_MS      10
#define TIMER_COUNT_2_S        2000
#define TIMER_PERIODIC_10_MS   (10 * 1000)  /* Convert ms to us */
#define TIMER_PERIODIC_1000_MS (1000 * 1000)/* Convert ms to us */

#define DGN_DATE_TIME_COMMAND 131070

static uint8_t t_flag = 1;

typedef void (*parameter_changed_t)(uint8_t table_index, int32_t i32Value);

typedef struct 
{
	PIMAGE_eTABLE rvc_status_var; /* RVC changed parameter index */
	PIMAGE_eTABLE rvc_cmd_var;    /* RVC parameter to reset */
	uint32_t cmd_dgn;
	int32_t ignore_data;          /* Data to write to reset parameter */
	uint32_t ddm_parameter;       /* Gateway DDM parameter number to publish */
	uint8_t type;
	uint8_t size;                 /* Size of published parameter */
	uint8_t pub;
	uint8_t sub;
	parameter_changed_t func;
} rvc_cross_ref_t;

typedef struct 
{
	uint8_t txflag;
	TickType_t txtick;
} rvc_cross_ref_data_t;

typedef enum
{
	DATE_DAY   = 0,
	DATE_MONTH = 1,
	DATE_YEAR  = 2,
	DATE_SIZE  = 3,
}rvc_date_type_t;

static uint8_t local_date [DATE_SIZE];
static bool local_date_is_updated = false;

static int initialize_can(void);
static int initialize_can_driver(void);
static void initialize_can_gpio(void);
static void can_error_cb(int32 error_code);
static void rvc_process_task(void* Parameter);
static void rvc_txrx_task(void* Parameter);
static void Main_ProcessTimers(void);
static void rvc_parameter_update_cb(PIMAGE_eTABLE parameter, int32 iValue);
static void handleSetDateParameter(uint8_t table_index, int32_t i32Value);
static void handleSetCommandParameter(uint8_t index, int32_t i32value);
static void handleSetWHONParameter(uint8_t index, int32_t i32value);
static void handleSetTargetTempParameter(uint8_t table_index, int32_t i32Value);
static void handlePublishStatus(uint8_t index, int32_t i32value);
static void handlePublishRoomTemp(uint8_t table_index, int32_t i32Value);
static void handleSubscribeVersion(uint8_t table_index, int32_t i32Value);
static void rx_isr_handler(void *args);

CONNECTOR connector_prem_rvc =
{
	.name = "Prem RV/C connector",
	.initialize = initialize_can,
};

static const rvc_cross_ref_t rvc_table[] = 
{
	{VAR_DGN_130559_ENERGY_SOURCE, VAR_DGN_130558_ENERGY_SOURCE,       130558, RVCDGN_UINT4_NO_DATA,  HTR0ESEL,      VAR_TYPE_UINT8,  4, 1, 0, handleSetCommandParameter},
	{VAR_DGN_130559_AIR_HEATER_CMD, VAR_DGN_130558_AIR_HEATER_CMD,     130558, RVCDGN_UINT2_NO_DATA,  HTR0AON,       VAR_TYPE_UINT8,  4, 1, 0, handleSetCommandParameter},
	{VAR_DGN_130559_WTR_HEATER_CMD, VAR_DGN_130558_WTR_HEATER_CMD,     130558, RVCDGN_UINT2_NO_DATA,  HTR0WTRON,     VAR_TYPE_UINT8,  4, 1, 0, handleSetWHONParameter},
	{VAR_DGN_130559_AIR_HEATER_MODE, VAR_DGN_130558_AIR_HEATER_MODE,   130558, RVCDGN_UINT4_NO_DATA,  HTR0AMD,       VAR_TYPE_UINT8,  4, 1, 0, handleSetCommandParameter},
	{VAR_DGN_130559_TARGET_ROOM_TEMP, VAR_DGN_130558_TARGET_ROOM_TEMP, 130558, RVCDGN_INT16_NO_DATA,  HTR0ATEMP,     VAR_TYPE_INT16,  4, 1, 0, handleSetTargetTempParameter},
	{VAR_DGN_130559_SILENT_FAN_MAX, VAR_DGN_130558_SILENT_FAN_MAX,     130558, RVCDGN_UINT4_NO_DATA,  HTR0SMAXFAN,   VAR_TYPE_UINT8,  4, 1, 0, handleSetCommandParameter},
	{VAR_DGN_130559_VENT_FAN_MIN, VAR_DGN_130558_VENT_FAN_MIN,         130558, RVCDGN_UINT4_NO_DATA,  HTR0VMINFAN,   VAR_TYPE_INT8,   4, 1, 0, handleSetCommandParameter},
	{VAR_DGN_130559_WTR_HEATER_MODE, VAR_DGN_130558_WTR_HEATER_MODE,   130558, RVCDGN_UINT4_NO_DATA,  HTR0WTRTEMP,   VAR_TYPE_UINT8,  4, 1, 0, handleSetCommandParameter},
	{VAR_DGN_130559_UNDERVOLT_THRES, VAR_DGN_130558_UNDERVOLT_THRES,   130558, RVCDGN_UINT8_NO_DATA,  HTR0UVTH,      VAR_TYPE_UINT8,  4, 1, 0, handleSetCommandParameter},
	{VAR_DGN_130559_SYSTEM_UNITS, VAR_DGN_130558_SYSTEM_UNITS,         130558, RVCDGN_UINT2_NO_DATA,  HTR0SYSU,      VAR_TYPE_UINT8,  4, 1, 0, handleSetCommandParameter},
	{VAR_DGN_130556_ROOM_TEMP, 0,                                      130556, RVCDGN_INT16_NO_DATA,  TH0ITEMP,      VAR_TYPE_INT16,  4, 0, 1, handlePublishRoomTemp},
	{VAR_DGN_130556_BUTTON_FAV, 0,                                     130556, RVCDGN_UINT2_NO_DATA,  TH0BUT0,       VAR_TYPE_UINT8,  4, 0, 1, handlePublishStatus},
	{VAR_DGN_130556_BUTTON_MENU, 0,                                    130556, RVCDGN_UINT2_NO_DATA,  TH0BUT1,       VAR_TYPE_UINT8,  4, 0, 1, handlePublishStatus},
	{VAR_DGN_130556_BUTTON_HOME, 0,                                    130556, RVCDGN_UINT2_NO_DATA,  TH0BUT2,       VAR_TYPE_UINT8,  4, 0, 1, handlePublishStatus},
	{VAR_DGN_130552_COMFORT_SW_VER_MA, 0,                              130552, RVCDGN_UINT16_NO_DATA, HTR0CVER,      VAR_TYPE_UINT16, 4, 1, 0, handleSubscribeVersion},
	{VAR_DGN_130552_COMFORT_SW_VER_MI, 0,                              130552, RVCDGN_UINT16_NO_DATA, HTR0CVER,      VAR_TYPE_UINT16, 4, 1, 0, handleSubscribeVersion},
	{VAR_DGN_130552_BURNER_SW_VER_MA, 0,                               130552, RVCDGN_UINT16_NO_DATA, HTR0BVER,      VAR_TYPE_UINT16, 4, 1, 0, NULL},
	{VAR_DGN_130552_BURNER_SW_VER_MI, 0,                               130552, RVCDGN_UINT16_NO_DATA, HTR0BVER,      VAR_TYPE_UINT16, 4, 1, 0, NULL},
	{VAR_DGN_130552_PCBA_VER, 0,                                       130552, RVCDGN_UINT8_NO_DATA,  HTR0PCBA,      VAR_TYPE_UINT8,  4, 1, 0, NULL},
	{VAR_DGN_130552_PROTOCOL_VER_MA, 0,                                130552, RVCDGN_UINT16_NO_DATA, HTR0PROT,      VAR_TYPE_UINT16, 4, 1, 0, NULL},
	{VAR_DGN_130552_PROTOCOL_VER_MI, 0,                                130552, RVCDGN_UINT16_NO_DATA, HTR0PROT,      VAR_TYPE_UINT16, 4, 1, 0, NULL},
	{VAR_DGN_130553_WARNING_FAULT_ACTIVE, 0,                           130553, RVCDGN_UINT2_NO_DATA,  HTR0ERRST,     VAR_TYPE_UINT8,  4, 1, 0, NULL},
	{VAR_DGN_130553_CRITICAL_FAULT_ACTIVE, 0,                          130553, RVCDGN_UINT2_NO_DATA,  HTR0ERRST,     VAR_TYPE_UINT8,  4, 1, 0, NULL},
	{VAR_DGN_130553_ACTIVE_FAULT_CODE_1, 0,                            130553, RVCDGN_UINT16_NO_DATA, HTR0ERRCD1,    VAR_TYPE_UINT16, 4, 1, 0, NULL},
	{VAR_DGN_130553_ACTIVE_FAULT_CODE_2, 0,                            130553, RVCDGN_UINT16_NO_DATA, HTR0ERRCD2,    VAR_TYPE_UINT16, 4, 1, 0, NULL},
	{VAR_DGN_130553_ACTIVE_FAULT_CODE_3, 0,                            130553, RVCDGN_UINT16_NO_DATA, HTR0ERRCD3,    VAR_TYPE_UINT16, 4, 1, 0, NULL},
	{VAR_DGN_130553_ACTIVE_FAULT_CODE_4, 0,                            130553, RVCDGN_UINT16_NO_DATA, HTR0ERRCD4,    VAR_TYPE_UINT16, 4, 1, 0, NULL},
	{VAR_DGN_130557_AC_PRESENT, 0,                                     130557, RVCDGN_UINT2_NO_DATA,  HTR0ACST,      VAR_TYPE_UINT8,  4, 1, 0, NULL},
	{VAR_DGN_130557_GAS_HEATER_WTR, 0,                                 130557, RVCDGN_UINT4_NO_DATA,  HTR0GASWTRHST, VAR_TYPE_UINT8,  4, 1, 0, NULL},
	{VAR_DGN_130557_AC_HEATER_WTR, 0,                                  130557, RVCDGN_UINT4_NO_DATA,  HTR0ACWTRHST,  VAR_TYPE_UINT8,  4, 1, 0, NULL},
	{VAR_DGN_130557_ROOM_TEMP, 0,                                      130557, RVCDGN_INT16_NO_DATA,  HTR0RTS,       VAR_TYPE_INT16,  4, 1, 0, NULL},
	{VAR_DGN_130557_WATER_TEMP, 0,                                     130557, RVCDGN_UINT3_NO_DATA,  HTR0WTRTS,     VAR_TYPE_UINT8,  4, 1, 0, NULL},
	{VAR_DGN_131071_YEAR, VAR_DGN_131070_YEAR,                         131070, RVCDGN_UINT8_NO_DATA,  HTR0DATEY,     VAR_TYPE_UINT8,  4, 1, 0, handleSetDateParameter},
	{VAR_DGN_131071_MONTH, VAR_DGN_131070_MONTH,                       131070, RVCDGN_UINT8_NO_DATA,  HTR0DATEM,     VAR_TYPE_UINT8,  4, 1, 0, handleSetDateParameter},
	{VAR_DGN_131071_DAY, VAR_DGN_131070_DAY,                           131070, RVCDGN_UINT8_NO_DATA,  HTR0DATED,     VAR_TYPE_UINT8,  4, 1, 0, handleSetDateParameter},
	{VAR_DGN_131071_HOUR, VAR_DGN_131070_HOUR,                         131070, RVCDGN_UINT8_NO_DATA,  HTR0TIMEH,     VAR_TYPE_UINT8,  4, 1, 0, handleSetCommandParameter},
	{VAR_DGN_131071_MINUTE, VAR_DGN_131070_MINUTE,                     131070, RVCDGN_UINT8_NO_DATA,  HTR0TIMEM,     VAR_TYPE_UINT8,  4, 1, 0, handleSetCommandParameter},
	{VAR_DGN_131071_SECOND, VAR_DGN_131070_SECOND,                     131070, RVCDGN_UINT8_NO_DATA,  HTR0TIMES,     VAR_TYPE_UINT8,  4, 1, 0, handleSetCommandParameter},
	{VAR_DGN_131071_TIMEZONE, VAR_DGN_131070_TIMEZONE,                 131070, RVCDGN_UINT8_NO_DATA,  HTR0TTZ,       VAR_TYPE_UINT8,  4, 1, 0, handleSetCommandParameter},
	{VAR_DGN_130555_AIR_HTR_OFF_STAT, VAR_DGN_130554_AIR_HTR_OFF_STAT, 130554, RVCDGN_UINT8_NO_DATA,  HTR0AHTOFFST,  VAR_TYPE_UINT8,  4, 1, 0, handleSetCommandParameter},
	{VAR_DGN_130555_AIR_HTR_OFF_HOUR, VAR_DGN_130554_AIR_HTR_OFF_HOUR, 130554, RVCDGN_UINT8_NO_DATA,  HTR0AHTOFFH,   VAR_TYPE_UINT8,  4, 1, 0, handleSetCommandParameter},
	{VAR_DGN_130555_AIR_HTR_OFF_MIN, VAR_DGN_130554_AIR_HTR_OFF_MIN,   130554, RVCDGN_UINT8_NO_DATA,  HTR0AHTOFFM,   VAR_TYPE_UINT8,  4, 1, 0, handleSetCommandParameter},
	{VAR_DGN_130555_AIR_HTR_ON_STAT, VAR_DGN_130554_AIR_HTR_ON_STAT,   130554, RVCDGN_UINT8_NO_DATA,  HTR0AHTONST,   VAR_TYPE_UINT8,  4, 1, 0, handleSetCommandParameter},
	{VAR_DGN_130555_AIR_HTR_ON_HOUR, VAR_DGN_130554_AIR_HTR_ON_HOUR,   130554, RVCDGN_UINT8_NO_DATA,  HTR0AHTONH,    VAR_TYPE_UINT8,  4, 1, 0, handleSetCommandParameter},
	{VAR_DGN_130555_AIR_HTR_ON_MIN, VAR_DGN_130554_AIR_HTR_ON_MIN,     130554, RVCDGN_UINT8_NO_DATA,  HTR0AHTONM,    VAR_TYPE_UINT8,  4, 1, 0, handleSetCommandParameter},
	{VAR_DGN_130555_WTR_HTR_ON_STAT, VAR_DGN_130554_WTR_HTR_ON_STAT,   130554, RVCDGN_UINT8_NO_DATA,  HTR0WTRTST,    VAR_TYPE_UINT8,  4, 1, 0, handleSetCommandParameter},
	{VAR_DGN_130555_WTR_HTR_ON_HOUR, VAR_DGN_130554_WTR_HTR_ON_HOUR,   130554, RVCDGN_UINT8_NO_DATA,  HTR0WTRTONH,   VAR_TYPE_UINT8,  4, 1, 0, handleSetCommandParameter},
	{VAR_DGN_130555_WTR_HTR_ON_MIN, VAR_DGN_130554_WTR_HTR_ON_MIN,     130554, RVCDGN_UINT8_NO_DATA,  HTR0WTRTONM,   VAR_TYPE_UINT8,  4, 1, 0, handleSetCommandParameter},
	{VAR_DGN_130555_WTR_HTR_KEEP_ON_TIME, VAR_DGN_130554_WTR_HTR_KEEP_ON_TIME, 130554, RVCDGN_UINT8_NO_DATA, HTR0WTRTKET, VAR_TYPE_UINT8, 4, 1, 0, handleSetCommandParameter},
	{VAR_DGN_65259_MODEL, 0,                                            65259, RVCDGN_UINT16_NO_DATA,  HTR0MDL,      VAR_TYPE_UINT16, 4, 1, 0, NULL},
};
static EXT_RAM_ATTR rvc_cross_ref_data_t rvc_data_table[ELEMENTS(rvc_table)];

typedef struct 
{
	PIMAGE_eTABLE rvc_var_id;
	uint16_t cross_ref_id;
	char * string; /* RVC parameter to reset */
} rvc_string_t;

static const rvc_string_t rvc_string_table[] = 
{
	{VAR_DGN_65259_MODEL, HTR0MDL_SHARC_CH6000,  "CH6000"},
	{VAR_DGN_65259_MODEL, HTR0MDL_SHARC_CH6000E, "CH6000E"},
	{VAR_DGN_65259_MODEL, HTR0MDL_SHARC_CH4000,  "CH4000"},
	{VAR_DGN_65259_MODEL, HTR0MDL_SHARC_CH4000E, "CH4000E"},
};

static volatile uint16_t main_u16TicksInit  = 0;
static volatile uint16_t main_u16TicksSys   = 0;
static volatile uint16_t main_u16Elapsed_ms = 0;
static volatile uint16_t main_u16Ticks10ms  = 0;
static volatile uint16_t main_u16Ticks1000ms = 0;
static int sleep_indication = 0;

static const TickType_t inactive_to_unknown_timeout = pdMS_TO_TICKS(10); //!< Time with no packets before going to unknown state
static const TickType_t activity_detect_timeout = pdMS_TO_TICKS(2000);   //!< Time to listen for bus activity in unknown state

/* Time to sleep in unknown state before listening for bus activity, incremental backoff */
static const TickType_t retry_backoff_ticks[] = {
	pdMS_TO_TICKS(30000),
	pdMS_TO_TICKS(60000),
	pdMS_TO_TICKS(120000),
	pdMS_TO_TICKS(240000),
	pdMS_TO_TICKS(480000),
};

static TimerHandle_t timeout_timer;       //!< \~ Reset timer
static TimerHandle_t set_date_timer;      //!< \~ Set Date timer

TaskHandle_t can_txrx_task_handle = NULL; //!< TX RX task handle for task notifications from ISR

static void feedback_handler(TimerHandle_t xTimer)
{
	(void)xTimer;
	xEventGroupSetBits(callback_accept_handle, CB_TARGET_TEMP_NOT_BLOCKED);
	LOG(I, "Callback accepted");
}

static void set_date_handler(TimerHandle_t xTimer)
{
	NMEA2K_SetTxRequest(BSP_CAN_RVC, DGN_DATE_TIME_COMMAND, 0);
}

static void rvc_string_to_id(const PIMAGE_eTABLE_SRTING string_type)
{
	/* Temporary storage for device information string value */
	char string[RVCDGN_DGN_65259_FIELD_SIZE];
	char *save;
	prod_database_t prod_data = {.mdl = {0}, .name = {0}, .sn = {0}, .uid = {0}};
	int32_t ddm_instance = -1;
	const int32_t clist = HTR0;

	PIMAGE_GetString(string_type, string);
	int i = 0;
	char *token = strtok_r(string, "*", &save);
	while (token != NULL && i < 3)
	{
		/* Process DGN device information fields “Make*Model*Serial-Number*Unit-number” */
		switch (i)
		{
		case 0 : /* Make */
			/* Ignore */
			break;
		case 1 : /* Model */
			strcpy(prod_data.mdl, token);
			break;
		case 2 : /* Serial-Number */
			strcpy(prod_data.sn, token);
			break;
		case 3 : /* Unit-number */
			/* Ignore */
			break;
		default:
			break;
		}
		token = strtok_r(NULL, "*", &save);
		i++;
	}
	/* Get model string and convert it to ID according to table */
	for(int i = 0; i < (int)ELEMENTS(rvc_string_table); i++)
	{
		if (strcmp(prod_data.mdl, rvc_string_table[i].string) == 0)
		{
			if (PIMAGE_u16GetValue(rvc_string_table[i].rvc_var_id) != rvc_string_table[i].cross_ref_id)
			{
				/* Create product and store value if its a valid model and device was not previously registered */
				strcpy(prod_data.name, "My Combo heater");
				ddm_instance = ProdDBProdClassNodeCreate(&prod_data, sizeof(prod_database_t), connector_prem_rvc.connector_id);
				if (ddm_instance != -1)
				{
					ProdDBUpdateCache((const void*)&clist, sizeof(int32_t), FIELD_CLIST, ddm_instance);
				}
				else
				{
					LOG(E,"ProdDBProdClassNodeCreate returned error %d", ddm_instance);
				}
				/* Save PImage value of device ID */
				PIMAGE_SetValue(rvc_string_table[i].rvc_var_id, rvc_string_table[i].cross_ref_id);
				/* If it was changed on RVC then publish updated value and store current value */
				connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
											rvc_table[i].ddm_parameter,
											&rvc_string_table[i].cross_ref_id,
											sizeof(rvc_string_table[i].cross_ref_id),
											connector_prem_rvc.connector_id,
											portMAX_DELAY);
			}
		}
	}
}

/* Responsible for checking of the command vs status byte.
	A correct communication is that a command is responded with a status update.
	If no status update then the command was not accepted or is lost.
	This function re-publish the current status into the system. */
static void response_checker(void)
{
	TickType_t cur_tick;
	int32_t stored_val;

	for (uint32_t i = 0; i < ELEMENTS(rvc_table); i++)
	{
		if (rvc_data_table[i].txflag)
		{
			cur_tick = xTaskGetTickCount();
			
			if ((rvc_table[i].rvc_cmd_var == VAR_DGN_130558_TARGET_ROOM_TEMP) || (rvc_table[i].rvc_status_var == VAR_DGN_130559_TARGET_ROOM_TEMP))
			{
				if (t_flag == 0)
				{
					/* Unblock task if we received updated value from heater over RVC */
					if (PIMAGE_u8GetValue(rvc_table[i].rvc_status_var) == PIMAGE_u8GetValue(rvc_table[i].rvc_cmd_var))
					{
						xEventGroupSetBits(callback_accept_handle, CB_TARGET_TEMP_NOT_BLOCKED);
					}
					if ((cur_tick - rvc_data_table[i].txtick) > RESPONSE_TICK)
					{
						/* Check if the status and command variable are equal.
							If they differ then the bus has not responded within resonable time.
							Publish status variable to the system to be sure everyone knows the correct one. */
						
						if (rvc_table[i].rvc_status_var != rvc_table[i].rvc_cmd_var)
						{
							if (rvc_table[i].type == VAR_TYPE_UINT8)
							{
								/* Read the status frame value */
								stored_val = (int32_t)PIMAGE_u8GetValue(rvc_table[i].rvc_status_var);
							}
							else if (rvc_table[i].type == VAR_TYPE_INT8)
							{
								/* Read the status frame value */
								stored_val = (int32_t)PIMAGE_s8GetValue(rvc_table[i].rvc_status_var);
							}
							else /* if (rvc_table[i].type == VAR_TYPE_INT16) */
							{
								/* Read the status frame value */
								stored_val = PIMAGE_s16GetValue(rvc_table[i].rvc_status_var);
							}

							/* If the status variable is not "ignore" then publish */
							if (stored_val != rvc_table[i].ignore_data)
							{
								LOG(I, "Mismatch! inside if Publish of 0x%x, value=0x%x", rvc_table[i].ddm_parameter, (uint32_t)stored_val);
								if (convert_rvc_to_ddm_system_value(rvc_table[i].rvc_status_var, &stored_val))
								{
									connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_table[i].ddm_parameter, &stored_val, sizeof(stored_val), connector_prem_rvc.connector_id, portMAX_DELAY);
								}
							}
						}
					}
				}
			}
			else
			{
				if ((cur_tick - rvc_data_table[i].txtick) > RESPONSE_TICK)
				{
					/* Check if the status and command variable are equal.
						If they differ then the bus has not responded within resonable time.
						Publish status variable to the system to be sure everyone knows the correct one. */
					if (rvc_table[i].rvc_status_var != rvc_table[i].rvc_cmd_var)
					{
						if (rvc_table[i].type == VAR_TYPE_UINT8)
						{
							/* Read the status frame value */
							stored_val = (int32_t)PIMAGE_u8GetValue(rvc_table[i].rvc_status_var);
						}
						else if (rvc_table[i].type == VAR_TYPE_INT8)
						{
							/* Read the status frame value */
							stored_val = (int32_t)PIMAGE_s8GetValue(rvc_table[i].rvc_status_var);
						}
						else /* if (rvc_table[i].type == VAR_TYPE_INT16) */
						{
							/* Read the status frame value */
							stored_val = PIMAGE_s16GetValue(rvc_table[i].rvc_status_var);
						}

						/* If the status variable is not "ignore" then publish */
						if (stored_val != rvc_table[i].ignore_data)
						{
							LOG(I, "Mismatch! Publish of 0x%x, value=0x%x", rvc_table[i].ddm_parameter, stored_val);
							if (convert_rvc_to_ddm_system_value(rvc_table[i].rvc_status_var, &stored_val))
							{
								connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_table[i].ddm_parameter, &stored_val, sizeof(stored_val), connector_prem_rvc.connector_id, portMAX_DELAY);
							}
						}
					}
				}
			}
			/* Clear TX flag */
			rvc_data_table[i].txflag = 0;
		}
		/* Check for model ID parameter */
		if (rvc_table[i].rvc_status_var == VAR_DGN_65259_MODEL)
		{
			rvc_string_to_id(STRINGS_DGN_65259_MODEL);
		}
	}
}

static void install_parameters(void)
{
	uint32_t htr_class = HTR0;
	/* Register as the owner of HTR0 class */
	NONNEG_CHECK(broker_register_instance(&htr_class, connector_prem_rvc.connector_id));
	/* Subscribe to HT0AVL */
	TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, TH0AVL, NULL, 0, connector_prem_rvc.connector_id, portMAX_DELAY));
}

static void start_subscribe(void)
{
	/* Start subscription for some parameters */
	for (uint32_t i = 0; i < ELEMENTS(rvc_table); i++)
	{
		/* Start subscription */
		if (rvc_table[i].sub)
		{
			TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, rvc_table[i].ddm_parameter, NULL, 0, connector_prem_rvc.connector_id, portMAX_DELAY));
		}
	}	
}

/**
 * @brief Initialize CAN
 *
 * @return true if initialization is successlul
 */
static int initialize_can_driver(void)
{
	int ret = 0;
	int err;

	/* Install CAN driver */
	if (HALCAN_Start(BSP_CAN_RVC))
	{
		LOG(I, "CAN started");
		err = ProdDBInit();
		if (err != 0)
		{
			LOG(E, "ProdDBInit returned error %d", err);
		}
		ret = 1;
	}
	else
	{
		LOG(E, "Failed to install CAN driver\n");
	}
	return ret;
}

static void initialize_can_gpio(void)
{
	/* Pin is connected to CAN EN/STB pin depending on CAN tranceiver used. Enable CAN / disable sleep. */
	/* Config done at startup; GPIO config */
	CAN_SLEEP(!CAN_SLEEP_LEVEL);
}

/* This function is called when CAN TX is failing.
 * When no communication is available towards the controller we here
 * assume that no error codes should be saved and we use the first error code
 * parameter for HMI display. When the error disapear we also assume we can
 * clear the critical error fault.
 */
static void can_error_cb(int32 error_code)
{
	uint8_t warn;

	PIMAGE_SetValue(VAR_DGN_130553_ACTIVE_FAULT_CODE_1, error_code);

	TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, HTR0ERRCD1, &error_code, sizeof(error_code), connector_prem_rvc.connector_id, portMAX_DELAY));

	/* Normalize */
	error_code = !!error_code;

	LOG(W, "Critical error=%d", (int)error_code);

	/* Read warning state */
	warn = PIMAGE_u8GetValue(VAR_DGN_130553_WARNING_FAULT_ACTIVE);

	error_code <<= 1;
	error_code |= warn;

	/* Publish read warning and critical status */
	TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, HTR0ERRST, &error_code, sizeof(error_code), connector_prem_rvc.connector_id, portMAX_DELAY));
}

static void RVC_Library_Initialize(void)
{
	/* Initialize PI */
	PIMAGE_Initialize();

	/* Initialize CAN */
	HALCAN_Initialize(can_error_cb);

	/* Update SWId on CAN */
	const char *fw_version = gateway_get_firmware_version();
	const char *hmi_version = hmi_data_get_version(); 
	MSGCAN_UpdateSWId(0, (void*)fw_version,  strlen(fw_version));  /* DICM FW version */
	MSGCAN_UpdateSWId(1, (void*)hmi_version, strlen(hmi_version)); /* HMI FW version */
	MSGCAN_UpdateSWId(2, " ", 1); /* Not used, use one space */
	MSGCAN_UpdateSWId(3, " ", 1); /* Not used, use one space */

	/* Initialize CAN mail boxes */
	/* TODO: Define parameters below */
	MSGCAN_Initialize(HMI_SRC_ADDRESS, 12345678, 0x01, rvc_parameter_update_cb);
}

static void IRAM_ATTR rx_isr_handler(void *args)
{
	(void)args;
	BaseType_t higher_priority_task_woken = pdFALSE;

	gpio_intr_disable(DEVICE_TWAI_RX);
#if defined (CAN_RS_PIN_NUM)
	CAN_RS(CAN_RS_HIGH_SPEED);
#endif
	/* Wakeup can_txrx_task. */
	if (can_txrx_task_handle != NULL)
	{
		vTaskNotifyGiveFromISR(can_txrx_task_handle, &higher_priority_task_woken);
	}

	if (higher_priority_task_woken == pdTRUE)
	{
		portYIELD_FROM_ISR();
	}
}

static int initialize_can(void)
{
	int result = 0;

	RVC_Library_Initialize();

	if (initialize_can_driver())
	{
		initialize_can_gpio();
		hal_can_pause(MY_CAN_DEVICE, true);

		callback_accept_handle = xEventGroupCreate();

		TRUE_CHECK(timeout_timer=xTimerCreate(NULL, pdMS_TO_TICKS(3000), pdFALSE, (void*)0, feedback_handler));
		TRUE_CHECK(set_date_timer=xTimerCreate(NULL, pdMS_TO_TICKS(10), pdFALSE, (void*)0, set_date_handler));
		TRUE_CHECK(xTaskCreate(rvc_process_task, "rvc_ddmp", 4096, NULL, xTASK_PRIORITY_NORMAL, NULL));
		TRUE_CHECK(xTaskCreate(rvc_txrx_task, "rvc_txrx", 3500, NULL, xTASK_PRIORITY_NORMAL, &can_txrx_task_handle));

		/* Set default value so that callbacks are accepted = 1 */
		xEventGroupSetBits(callback_accept_handle, CB_TARGET_TEMP_NOT_BLOCKED);

		/* Register RX ISR */
		/* DEVICE_TWAI_RX is setup by HAL, here it is reconfigured and added interrupt type and interrupt handler */
#if defined (CAN_RS_PIN_NUM)
		CAN_RS(CAN_RS_LOW_POWER);
#endif
		TRUE_CHECK(gpio_isr_handler_add(DEVICE_TWAI_RX, rx_isr_handler, NULL) == ESP_OK);
		TRUE_CHECK(gpio_set_intr_type(DEVICE_TWAI_RX, GPIO_INTR_NEGEDGE) == ESP_OK);
		TRUE_CHECK(gpio_intr_enable(DEVICE_TWAI_RX) == ESP_OK);

		/* Init local date variable */
		local_date[DATE_DAY]   = NMEA2K_UINT8_NO_DATA;
		local_date[DATE_MONTH] = NMEA2K_UINT8_NO_DATA;
		local_date[DATE_YEAR]  = NMEA2K_UINT8_NO_DATA;

		result = 1;
	}

	return result;
}

/* This function is only called when parameter value has been changed */
static void rvc_parameter_update_cb(PIMAGE_eTABLE parameter, int32 iValue)
{
#ifdef HMI_EXTENDED_LOG
	LOG(I, "Rxed PImage no=%d", parameter);
#endif

	for (uint32_t i = 0; i < (sizeof(rvc_table) / sizeof(rvc_cross_ref_t)); i++)
	{
		if (rvc_table[i].rvc_status_var == parameter)
		{
			/* Clear the TX flag */
			rvc_data_table[i].txflag = 0;

			/* Convert to system value */
			if (convert_rvc_to_ddm_system_value(rvc_table[i].rvc_status_var, (int32_t *)&iValue))
			{
#ifdef HMI_EXTENDED_LOG
				LOG(I, "Pimage with value=%d", (int32_t)iValue);
#endif
				
#ifdef HMI_IGNORE_CRITICAL_ERROR
				if (rvc_table[i].ddm_parameter != HTR0ERRST)
				{
#endif
				/* Create publish and send to broker */
				uint8_t result = connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_table[i].ddm_parameter, &iValue, sizeof(iValue), connector_prem_rvc.connector_id, pdMS_TO_TICKS(1000));
				if (result == pdFALSE)
				{
					LOG(E, "Broker queue full! Message lost QueueCnt=%d", uxQueueMessagesWaiting(connector_prem_rvc.to_broker));
				}
#ifdef HMI_IGNORE_CRITICAL_ERROR
				}
#endif

				/* Special handling for HTR0AON. 0 means sleep.
				 *
				 * The system will never recover from sleep. The way to override
				 * this is to reboot (or toggle power).
				 * 
				 * In sleep the Controller is sleeping and CAN traffic is none.
				 * To wake up controller the middle button on HMI is pressed.
				 * This generates a WAKEUP on CAN and a toggling of power to HMI
				 * so it will reboot.
				 */
				if ((rvc_table[i].rvc_status_var == HTR0AON) && (iValue == 0))
				{
					int32_t available = 0;
					sleep_indication = 1;

					/* Make heater unavailable */
					connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, HTR0AVL, &available, sizeof(available), connector_prem_rvc.connector_id, portMAX_DELAY);
				}
			}

			/* Reset command frame to Ignore,, i.e a status frame change is reflected to the command frame as Ignore.
			 * This "set" should do no harm even if we are not responsible for the original change.
			 * A forced Tx is not required here. Let the scheduler put the data out according to scheduling table.
			 * If no cmd parameter then ignore setting value.
			 */
			if (rvc_table[i].rvc_cmd_var != 0)
			{
				/* Do not reset seconds value until it is sent over RVC, 
				 * because Heater updates seconds value more often than HMI does,
				 * Heater will almost always overwrite HMI seconds value 
				 */
				if (rvc_table[i].ddm_parameter != HTR0TIMES)
				{
					PIMAGE_SetValue(rvc_table[i].rvc_cmd_var, rvc_table[i].ignore_data);
				}
			}

			break; /* Leave for loop */
		}
	}
}

static bool is_leap_year(uint8_t year)
{
	bool response;
	/* Convert RVC year to actual year value offset is 2000 according to RVC standart */
	uint32_t converted_year = (uint32_t)year + (uint32_t)2000;

	/* Check if provided year is a leap year. */
	if (!(converted_year % 4))
	{
		if (!(converted_year % 100))
		{
			response = false;
			if (!(converted_year % 400))
			{
				response = true;
			}
		}
		else
		{
			response = true;
		}
	}
	else
	{
		response = false;
	}
	return response;
}

static int32_t get_index_by_parameter(uint32_t param)
{
	uint8_t return_index = -1;
	for(uint32_t table_index = 0; table_index < ELEMENTS(rvc_table); table_index++)
	{
		if (rvc_table[table_index].ddm_parameter == param)
		{
			return_index = table_index;
			break;
		}
	}
	return return_index;
}

static void log_rvc_data_change_event(int32_t ddm_parameter, int32_t i32Value, uint32_t dgn)
{
	uint8_t name_buffer[32];
	size_t name_length = sizeof(name_buffer);

	LOG(I, "RV-C set value changed for %s to %d in DGN %d",                               \
		/* DDM parameter */ddm2_parameter_name(ddm_parameter, name_buffer, &name_length), \
		/* DDM value     */i32Value,                                                      \
		/* DDM ID        */dgn);
}

static void handleSetDateParameter(uint8_t table_index, int32_t i32Value)
{
	uint8 stored_u8;
	uint8 month_local;
	int changed = 0;

	if (convert_ddm_system_value_to_rvc_value(rvc_table[table_index].ddm_parameter, &i32Value))
	{
		if (rvc_table[table_index].type == VAR_TYPE_UINT8)
		{
			/*  Read the status frame value */
			switch (rvc_table[table_index].ddm_parameter)
			{
			case HTR0DATED:
				/* Check if day value is in range */
				if ((i32Value >= 1) && (i32Value <= 31))
				{
					month_local = local_date[DATE_MONTH];
					/* Limit February month days depending on current year */
					if (month_local == 2)
					{
						if (is_leap_year(local_date[DATE_YEAR]))
						{
							/* If it is a leap year then limit day to 29 February */
							if (i32Value > 29)
							{
								 i32Value = local_date[DATE_DAY];
							}
						}
						else
						{
							/* If it is NOT a leap year then limit day to 28 February */
							if (i32Value > 28)
							{
								i32Value = local_date[DATE_DAY];
							}
						}
					}
					/* Limit month days depending on current month */
					if ((month_local == 4) || /* If April */
						(month_local == 6) || /* If June */
						(month_local == 9) || /* If September */
						(month_local == 11))  /* If November */
					{
						/* For month above max. is 30 days in a month */
						if (i32Value > 30)
						{
							i32Value = local_date[DATE_DAY];
						}
					}
				}
				else
				{
					/* If DAY value is out of range take prevoious valid value */
					i32Value = local_date[DATE_DAY];
				}
				break;
			case HTR0DATEM:
				/* Check if month value is in range */
				if ((i32Value >= 1) && (i32Value <= 12))
				{
					if (i32Value == 2)
					{
						if (is_leap_year(local_date[DATE_YEAR]))
						{
							/* If it is a leap year then limit day to 29 February */
							if (local_date[DATE_DAY] > 29)
							{
								PIMAGE_SetValue(rvc_table[get_index_by_parameter(HTR0DATED)].rvc_cmd_var, 29);
								local_date[DATE_DAY] = 29;
							}
						}
						else
						{
							/* If it is NOT a leap year then limit day to 28 February */
							if (local_date[DATE_DAY] > 28)
							{
								PIMAGE_SetValue(rvc_table[get_index_by_parameter(HTR0DATED)].rvc_cmd_var, 28);
								local_date[DATE_DAY] = 28;
							}
						}
					}
					/* Limit month days depending on current month */
					if ((i32Value == 4) ||
						(i32Value == 6) ||
						(i32Value == 9) ||
						(i32Value == 11))
					{
						/* For month above max. is 30 days in a month */
						if (local_date[DATE_DAY] > 30)
						{
							PIMAGE_SetValue(rvc_table[get_index_by_parameter(HTR0DATED)].rvc_cmd_var, 30);
							local_date[DATE_DAY] = 30;
						}
					}
				}
				else
				{
					/* If MONTH value is out of range take prevoious valid value */
					i32Value = local_date[DATE_MONTH];
				}
				break;
			case HTR0DATEY:
				if (((i32Value >= 20) && (i32Value <= 99)))
				{
					/* If current month is February check for leap year */
					if (local_date[DATE_MONTH] == 2)
					{
						if (is_leap_year(i32Value))
						{
							if (local_date[DATE_DAY] > 29)
							{
								/* If it is a leap year then limit day to 29 February */
								PIMAGE_SetValue(rvc_table[get_index_by_parameter(HTR0DATED)].rvc_cmd_var, 29);
								local_date[DATE_DAY] = 29;
							}
						}
						else
						{
							if (local_date[DATE_DAY] > 28)
							{
								/* If it is NOT a leap year then limit day to 28 February */
								PIMAGE_SetValue(rvc_table[get_index_by_parameter(HTR0DATED)].rvc_cmd_var, 28);
								local_date[DATE_DAY] = 28;
							}
						}
					}
				}
				else
				{
					i32Value = local_date[DATE_YEAR];
				}
				break;
			}
		}

		stored_u8 = PIMAGE_u8GetValue(rvc_table[table_index].rvc_status_var);
		changed = ((uint8)i32Value != stored_u8) ? 1 : 0;
		if (changed)
		{
			/* Store current date into local variable */
			switch (rvc_table[table_index].ddm_parameter)
			{
			case HTR0DATED:
				local_date[DATE_DAY] = i32Value;
				break;
			case HTR0DATEM:
				local_date[DATE_MONTH] = i32Value;
				break;
			case HTR0DATEY:
				local_date[DATE_YEAR] = i32Value;
				break;
			default:
				break;
			}
			log_rvc_data_change_event(rvc_table[table_index].ddm_parameter, i32Value, rvc_table[table_index].cmd_dgn);
			/* Set the command frame */
			PIMAGE_SetValue(rvc_table[table_index].rvc_cmd_var, i32Value);

			/* Set one shot timer to see if other date parameters has changed before setting tx request */
			if (xTimerStart(set_date_timer, portMAX_DELAY) != pdPASS)
			{
				LOG(W,"Date transmit handler: set_date_timer failed to start");
			}
		}
	}
}

static void handleSetCommandParameter(uint8_t table_index, int32_t i32Value)
{
	uint8 stored_u8;
	int8 stored_i8;
	int16 stored_i16;
	int changed = 0;
	
	/* TODO: Works only for two types, which we currently have */

#ifdef HMI_EXTENDED_LOG
	LOG(I, "Handle set 0x%x", i32Value);
#endif
	if (convert_ddm_system_value_to_rvc_value(rvc_table[table_index].ddm_parameter, &i32Value))
	{
#ifdef HMI_EXTENDED_LOG
		LOG(I, "Converted to = 0x%x", i32Value);
#endif
		
		if (rvc_table[table_index].type == VAR_TYPE_UINT8)
		{
			/* Read the status frame value */
			stored_u8 = PIMAGE_u8GetValue(rvc_table[table_index].rvc_status_var);

			/* TODO: Is this really needed. Checking that the value differs. Check with Pierre. */
			changed = ((uint8)i32Value != stored_u8) ? 1 : 0;
		}
		else if (rvc_table[table_index].type == VAR_TYPE_INT8)
		{
			/* Read the status frame value */
			stored_i8 = PIMAGE_s8GetValue(rvc_table[table_index].rvc_status_var);

			/* TODO: Is this really needed. Checking that the value differs. Check with Pierre. */
			changed = ((uint8)i32Value != stored_i8) ? 1 : 0;
		}
		else if (rvc_table[table_index].type == VAR_TYPE_INT16)
		{
			/* Read the status frame value */
			stored_i16 = PIMAGE_s16GetValue(rvc_table[table_index].rvc_status_var);

			/* TODO: Is this really needed. Checking that the value differs. Check with Pierre. */
			changed = ((int16)i32Value != stored_i16) ? 1 : 0;
		}

		if (changed)
		{
			log_rvc_data_change_event(rvc_table[table_index].ddm_parameter, i32Value, rvc_table[table_index].cmd_dgn);
			/* Set the command frame */
			PIMAGE_SetValue(rvc_table[table_index].rvc_cmd_var, i32Value);
			NMEA2K_SetTxRequest(BSP_CAN_RVC, rvc_table[table_index].cmd_dgn, 0);
		}
	}
}

/*! \brief Handle invalid target temperature values
*
*	Target temperature values consider as valid values are:
*		- target_temperature == HTR0ATEMP_MINUS_50_DEGC (used for turning OFF the heater and HMI)
*		- target_temperature >= HTR0ATEMP_10_DEGC && target_temperature <= HTR0ATEMP_35_DEGC
*
*	This function will PUBLISH back the currently active valid temperature value if invalid
*	temperature value is provided by the system. Also will prevent propagating the invalid
*	temperature value to the heater, which will prevent triggering the heater for the
*	same value.
*
*	i.e. if \ref target_temperature == 5 and current rvc_status_var == 20, rvc_status_var
*	value will be PUBLISHED back in the system, and the heater will not be triggered with
*	the invalid target_temperature value.
*/
static bool hanlde_target_temperature_boundaries(uint8_t table_index, int32_t target_temperature)
{
	bool is_target_temp_in_range = true;

	/* If out of boundaries, override with current temperature value */
	if (HTR0ATEMP_IS_NOT_IN_RANGE(target_temperature))
	{
		/* Inform system that the provided invalid value was overriden with the current active value */
		int32_t current_temp = PIMAGE_s16GetValue(rvc_table[table_index].rvc_status_var);
		if (convert_rvc_to_ddm_system_value(rvc_table[table_index].rvc_status_var, &current_temp))
		{
			LOG(I, "Temperature[%d] out of range: publish value = %d", target_temperature, current_temp);
			connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
				rvc_table[table_index].ddm_parameter,
				&current_temp,
				sizeof(current_temp),
				connector_prem_rvc.connector_id,
				(TickType_t)portMAX_DELAY);
		}

		/* Prevent triggering the heater if invalid temperature values are received */
		is_target_temp_in_range = false;
	}

	return is_target_temp_in_range;
}

static void handleSetTargetTempParameter(uint8_t table_index, int32_t i32Value)
{
	int16_t stored_i16;
	int changed;
	int32_t temp_value = i32Value;
	portMUX_TYPE myMutex = portMUX_INITIALIZER_UNLOCKED;

	if (HTR0ATEMP_MINUS_50_DEGC > i32Value)
	{
		/* If target temperature is lower than HTR0ATEMP_MINUS_50_DEGC, limit it to HTR0ATEMP_MINUS_50_DEGC */
		/* This will prevent override of target temperature with default value "0" */
		i32Value = HTR0ATEMP_MINUS_50_DEGC;
		/* NOTE: Temperatures above 150C will be displayed as "0" because this is defalut RVC value*/
	}
	
	if (convert_ddm_system_value_to_rvc_value(rvc_table[table_index].ddm_parameter, &i32Value))
	{
#ifdef HMI_EXTENDED_LOG
		LOG(I, "Converted temp to = 0x%x", i32Value);
#endif
		stored_i16 = PIMAGE_s16GetValue(rvc_table[table_index].rvc_status_var);
		
		/* TODO: Is this really needed. Checking that the value differs. Check with Pierre. */
		changed = ((int16)i32Value != stored_i16) ? 1 : 0;

		/* If provided value by connectors is not in range, do not trigger the */
		/* Heater and publish back currently valid temperature value to the system */
		bool is_target_temp_in_range = hanlde_target_temperature_boundaries(table_index, temp_value);

		if (changed && is_target_temp_in_range)
		{
			t_flag = 1;
			log_rvc_data_change_event(rvc_table[table_index].ddm_parameter, i32Value, rvc_table[table_index].cmd_dgn);
			/* Set the command frame */
			PIMAGE_SetValue(rvc_table[table_index].rvc_cmd_var, i32Value);
			NMEA2K_SetTxRequest(BSP_CAN_RVC, rvc_table[table_index].cmd_dgn, 0);

			portENTER_CRITICAL(&myMutex);
			{
				/* Set the TX flag */
				rvc_data_table[table_index].txflag = 1;
				rvc_data_table[table_index].txtick = xTaskGetTickCount();
				portEXIT_CRITICAL(&myMutex);
			}

			/* Restart timer */
			TRUE_CHECK(xTimerReset(timeout_timer, portMAX_DELAY));
			if(xTimerReset(timeout_timer, portMAX_DELAY))
			{
				t_flag = 0;
			}
			/* Indicate that callback for target temp is halted */
			xEventGroupClearBits(callback_accept_handle, CB_TARGET_TEMP_NOT_BLOCKED);
			LOG(I, "Callback blocked");
		}
	}
}

static void handleSetWHONParameter(uint8_t table_index, int32_t i32Value)
{
	uint8 stored_u8;
	int16 fault1=0, fault2 = 0, fault3 = 0, fault4 = 0;
	int changed;
	
	/* TODO: Works only for two types, which we currently have */

#ifdef HMI_EXTENDED_LOG
	LOG(I, "Handle set 0x%x", i32Value);
#endif
	if (convert_ddm_system_value_to_rvc_value(rvc_table[table_index].ddm_parameter, &i32Value))
	{
#ifdef HMI_EXTENDED_LOG
		LOG(I, "Converted to = 0x%x", i32Value);
#endif
		
		if (rvc_table[table_index].type == VAR_TYPE_UINT8)
		{
			/* Read the status frame value */
			stored_u8 = PIMAGE_u8GetValue(rvc_table[table_index].rvc_status_var);

			/* TODO: Is this really needed. Checking that the value differs. Check with Pierre. */
			changed = ((uint8)i32Value != stored_u8) ? 1 : 0;
		}
		else
		{
			changed = 0;
		}
		
		fault1 = PIMAGE_s16GetValue(VAR_DGN_130553_ACTIVE_FAULT_CODE_1); /* Reading the Value of error code 1 */
		fault2 = PIMAGE_s16GetValue(VAR_DGN_130553_ACTIVE_FAULT_CODE_2);
		fault3 = PIMAGE_s16GetValue(VAR_DGN_130553_ACTIVE_FAULT_CODE_3);
		fault4 = PIMAGE_s16GetValue(VAR_DGN_130553_ACTIVE_FAULT_CODE_4);

		if(fault1 == WTR_FROZEN_WARN)
		{
				log_rvc_data_change_event(rvc_table[table_index].ddm_parameter, i32Value, rvc_table[table_index].cmd_dgn);
				/* Set the command frame */
				PIMAGE_SetValue(rvc_table[table_index].rvc_cmd_var, i32Value);
				NMEA2K_SetTxRequest(BSP_CAN_RVC, rvc_table[table_index].cmd_dgn, 0); /* Not checking the stored value when warning 425 is received */
		}
		else if(fault2 == WTR_FROZEN_WARN)
		{
				log_rvc_data_change_event(rvc_table[table_index].ddm_parameter, i32Value, rvc_table[table_index].cmd_dgn);
				/* Set the command frame */
				PIMAGE_SetValue(rvc_table[table_index].rvc_cmd_var, i32Value);
				NMEA2K_SetTxRequest(BSP_CAN_RVC, rvc_table[table_index].cmd_dgn, 0); /* Not checking the stored value when warning 425 is received */
		}
		else if(fault3 == WTR_FROZEN_WARN)
		{
				log_rvc_data_change_event(rvc_table[table_index].ddm_parameter, i32Value, rvc_table[table_index].cmd_dgn);
				/* Set the command frame */
				PIMAGE_SetValue(rvc_table[table_index].rvc_cmd_var, i32Value);
				NMEA2K_SetTxRequest(BSP_CAN_RVC, rvc_table[table_index].cmd_dgn, 0); /* Not checking the stored value when warning 425 is received */
		}
		else if(fault4 == WTR_FROZEN_WARN)
		{
				log_rvc_data_change_event(rvc_table[table_index].ddm_parameter, i32Value, rvc_table[table_index].cmd_dgn);
				/* Set the command frame */
				PIMAGE_SetValue(rvc_table[table_index].rvc_cmd_var, i32Value);
				NMEA2K_SetTxRequest(BSP_CAN_RVC, rvc_table[table_index].cmd_dgn, 0); /* Not checking the stored value when warning 425 is received */
		}
		else
		{
			if (changed)
			{
				log_rvc_data_change_event(rvc_table[table_index].ddm_parameter, i32Value, rvc_table[table_index].cmd_dgn);
				/* Set the command frame */
				PIMAGE_SetValue(rvc_table[table_index].rvc_cmd_var, i32Value);
				NMEA2K_SetTxRequest(BSP_CAN_RVC, rvc_table[table_index].cmd_dgn, 0); /* When there is no warning code checking the store value and if feedback is different than command then send command */
			}
		}
		LOG(I, "Callback of wh");
	}
}

static void handlePublishStatus(uint8_t table_index, int32_t i32Value)
{
	if (convert_ddm_system_value_to_rvc_value(rvc_table[table_index].ddm_parameter, &i32Value))
	{
#ifdef HMI_EXTENDED_LOG
		LOG(I, "Pub value converted to rvc value=%d", i32Value);
#endif
		PIMAGE_SetValue(rvc_table[table_index].rvc_status_var, i32Value);
	}
}

static void handlePublishRoomTemp(uint8_t table_index, int32_t i32Value)
{
	if (convert_ddm_system_value_to_rvc_value(rvc_table[table_index].ddm_parameter, &i32Value))
	{
#ifdef HMI_EXTENDED_LOG
		LOG(I, "Pub value converted to rvc value=%d", i32Value);
#endif
		/* Set value to ignore if an unreasonable value */
		if (i32Value < -5000)
		{
			i32Value = NMEA2K_INT16_NO_DATA;

#ifdef HMI_EXTENDED_LOG
			LOG(I, "Pub value set to ignore=%d", i32Value);
#endif
		}

		PIMAGE_SetValue(rvc_table[table_index].rvc_status_var, i32Value);
	}
}

static void handleSubscribeVersion(uint8_t table_index, int32_t i32Value)
{
	if (convert_rvc_to_ddm_system_value(rvc_table[table_index].rvc_status_var, (int32_t *)&i32Value))
	{
		TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_table[table_index].ddm_parameter, &i32Value, sizeof(i32Value), connector_prem_rvc.connector_id, portMAX_DELAY));
	}
}

static void update_local_date(void)
{
	uint8_t rvc_status_date;

	/* Check if DAY value is updated from RVC and update local DAY */
	rvc_status_date = PIMAGE_u8GetValue(rvc_table[get_index_by_parameter(HTR0DATED)].rvc_status_var);
	if ((local_date[DATE_DAY] == NMEA2K_UINT8_NO_DATA) && (rvc_status_date != NMEA2K_UINT8_NO_DATA))
	{
		local_date[DATE_DAY] = rvc_status_date;
#ifdef HMI_EXTENDED_LOG
		LOG(I,"RVC updated local day value %d", local_date[DATE_DAY]);
#endif
		/* Check if MONTH value is updated from RVC and update local MONTH */
		rvc_status_date = PIMAGE_u8GetValue(rvc_table[get_index_by_parameter(HTR0DATEM)].rvc_status_var);
		if ((local_date[DATE_MONTH] == NMEA2K_UINT8_NO_DATA) && (rvc_status_date != NMEA2K_UINT8_NO_DATA))
		{
			local_date[DATE_MONTH] = rvc_status_date;
#ifdef HMI_EXTENDED_LOG
			LOG(I,"RVC updated local month value %d",local_date[DATE_MONTH]);
#endif
			/* Check if YEAR value is updated from RVC and update local YEAR */
			rvc_status_date = PIMAGE_u8GetValue(rvc_table[get_index_by_parameter(HTR0DATEY)].rvc_status_var);
			if ((local_date[DATE_YEAR] == NMEA2K_UINT8_NO_DATA) && (rvc_status_date != NMEA2K_UINT8_NO_DATA))
			{
				local_date[DATE_YEAR] = rvc_status_date;
#ifdef HMI_EXTENDED_LOG
				LOG(I,"RVC updated local year value %d",local_date[DATE_YEAR]);
#endif
				/* Set flag that local date is updated */
				local_date_is_updated = true;
			}
		}
	}
}

static void rvc_process_task(void* Parameter)
{
	DDMP2_FRAME * ddmp_msg;
	size_t msg_size;
	uint32_t error_code;

	install_parameters();
	start_subscribe();

	/* Get Heater FW Version IDs */
	error_code = NMEA2K_TxISORequest(0, 0xFF, 130552);
	if (error_code == 0)
	{
		LOG(E, "NMEA2K_TxISORequest failed for 130552");
	}
	/* Get Heater device information */
	error_code = NMEA2K_TxISORequest(0, 0xFF, 65259);
	if (error_code == 0)
	{
		LOG(E, "NMEA2K_TxISORequest failed for 65259");
	}
	
	for (;;)
	{
		TRUE_CHECK(ddmp_msg = xRingbufferReceive(connector_prem_rvc.to_connector, &msg_size, portMAX_DELAY));
		/* Update once at beginning local date values */
		ProdDBFrameHandler(ddmp_msg);
		if (!local_date_is_updated)
		{
			update_local_date();
		}
#ifdef HMI_EXTENDED_LOG
		ESP_LOG_BUFFER_HEXDUMP("DDMP->RVC  ", (uint8_t *)ddmp_msg, ddmp_msg->frame_size + DDMP2_METADATA_SIZE, ESP_LOG_INFO);

		LOG(I, "RV-C action 0x%x", ddmp_msg->frame.control);
#endif

		if (ddmp_msg->frame.control == DDMP2_CONTROL_SET)
		{
#ifdef HMI_EXTENDED_LOG
			LOG(I, "RV-C SET\n");
#endif
			for (size_t i = 0; i < ELEMENTS(rvc_table); i++)
			{
				if (rvc_table[i].ddm_parameter == ddmp_msg->frame.set.parameter)
				{
					if (rvc_table[i].func)
					{
						rvc_table[i].func(i, ddmp_msg->frame.set.value.int32);
					}
					break;
				}
			}
		}
		else if (ddmp_msg->frame.control == DDMP2_CONTROL_PUBLISH)
		{
			for (size_t i = 0; i < ELEMENTS(rvc_table); i++)
			{
				if (rvc_table[i].ddm_parameter == ddmp_msg->frame.publish.parameter)
				{
					if (rvc_table[i].func)
					{
						rvc_table[i].func(i, ddmp_msg->frame.publish.value.int32);
					}
					break;
				}
			}
			if ((ddmp_msg->frame.publish.parameter == TH0AVL) && (ddmp_msg->frame.publish.value.int32 != 0))
			{
				start_subscribe();
			}
		}
		else if (ddmp_msg->frame.control == DDMP2_CONTROL_SUBSCRIBE)
		{
			if (DDM2_IS_AVAIL_PROPERTY(ddmp_msg->frame.subscribe.parameter))
			{
				int32_t available = sleep_indication ? 0 : 1;

#ifdef HMI_EXTENDED_LOG
				LOG(I, "RVC: Subscribe AVL");
#endif					
				/* Create publish and send to broker */
				TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, HTR0AVL, &available, sizeof(available), connector_prem_rvc.connector_id, portMAX_DELAY));
			}
			else
			{
				for (size_t i = 0; i < ELEMENTS(rvc_table); i++)
				{
					if (rvc_table[i].ddm_parameter == ddmp_msg->frame.subscribe.parameter)
					{
						int32_t val = 0;

						if (rvc_table[i].type == VAR_TYPE_UINT8)
						{
							val = (int32_t)PIMAGE_u8GetValue(rvc_table[i].rvc_status_var);
						}
						else if (rvc_table[i].type == VAR_TYPE_INT8)
						{
							val = (int32_t)PIMAGE_s8GetValue(rvc_table[i].rvc_status_var);
						}
						else if (rvc_table[i].type == VAR_TYPE_UINT16)
						{
							val = (int32_t)PIMAGE_u16GetValue(rvc_table[i].rvc_status_var);
						}
						else if (rvc_table[i].type == VAR_TYPE_INT16)
						{
							val = (int32_t)PIMAGE_s16GetValue(rvc_table[i].rvc_status_var);
						}

#ifdef HMI_EXTENDED_LOG
						LOG(I, "RVC: Subscribe, val=%d", val);
#endif					
						if (convert_rvc_to_ddm_system_value(rvc_table[i].rvc_status_var, &val))
						{
#ifdef HMI_IGNORE_CRITICAL_ERROR
							if (rvc_table[i].ddm_parameter == HTR0ERRST)
							{
								val = 0;
							}
#endif
							/* Create publish and send to broker */
							TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, rvc_table[i].ddm_parameter, &val, sizeof(val), connector_prem_rvc.connector_id, portMAX_DELAY));
						}

						break;
					}
				}
			}			
		}

		vRingbufferReturnItem(connector_prem_rvc.to_connector, ddmp_msg);
	}
}

static bool can_transfers_pending(void)
{
	bool pending = false;

	if (((HALCAN_TxSize(HAL_CAN_DEVICE_TWAI) - HALCAN_TxFree(HAL_CAN_DEVICE_TWAI)) > 0) ||
		((HALCAN_RxSize(HAL_CAN_DEVICE_TWAI) - HALCAN_RxFree(HAL_CAN_DEVICE_TWAI)) > 0))
	{
		pending = true;
	}

	return pending;
}

#ifdef TWAI_EXTENDED_LOG
void twai_logs(void)
{
    twai_status_info_t status_info;
	TRUE_CHECK(twai_get_status_info(&status_info) == ESP_OK);
		LOG(I,"LOG TWAI status %d: txerr = %d, rxerr = %d, txfail = %d, rxfail = %d, rxover = %d, arblos = %d, buserr = %d, txq = %d, rxq = %d", 
			status_info.state,
			status_info.tx_error_counter,
			status_info.rx_error_counter,
			status_info.tx_failed_count,
			status_info.rx_missed_count,
			status_info.rx_overrun_count,
			status_info.arb_lost_count,
			status_info.bus_error_count,
			status_info.msgs_to_tx,
			status_info.msgs_to_rx);
	get_print_twai_bus_error();
}
#endif /* CAN_EXTENDED_LOG */

static void rvc_txrx_task(void* Parameter)
{
	typedef enum 
	{
		CAN_STATUS_UNKNOWN = 0,
		CAN_STATUS_ACTIVE,
		CAN_STATUS_INACTIVE,
	} CAN_STATUS;

	uint8_t backoff_idx = 0;
	uint16_t notify_counter = 0;
	CAN_STATUS can_status = CAN_STATUS_UNKNOWN;
	
	for (;;)
	{
		switch (can_status)
		{
		case CAN_STATUS_UNKNOWN:
				/* Waiting for bus activity */
#ifdef CAN_EXTENDED_LOG
			LOG(D, "Looking for traffic.");
#endif
			if (ulTaskNotifyTake(pdTRUE, activity_detect_timeout) > 0)
			{
				/* Activity detected */
#ifdef CAN_EXTENDED_LOG
				LOG(D, "Unknown -> Active");
#endif
				can_status = CAN_STATUS_ACTIVE;
				backoff_idx = 0;

				/* Disable interrupts and enable driver */
				hal_can_resume(MY_CAN_DEVICE, true);
			}
			else
			{
				/* No activity, disable transceiver and wait */
#ifdef CAN_EXTENDED_LOG
				LOG(D, "No activity. Sleeping.");
#endif
				CAN_SLEEP(CAN_SLEEP_LEVEL);
				vTaskDelay(retry_backoff_ticks[backoff_idx]);

				if (backoff_idx < (ELEMENTS(retry_backoff_ticks) - 1))
				{
					backoff_idx++;
				}
				CAN_SLEEP(!CAN_SLEEP_LEVEL);
			}
			break;

		case CAN_STATUS_ACTIVE:
			/* Process timers */
			Main_ProcessTimers();
			/* Inputs */
			NMEA2K_ProcessRx();

			/* Task 10 ms */
			if (main_u16Ticks10ms >= 10)
			{
				NMEA2K_UpdateTimers(main_u16Ticks10ms);
				MSGCAN_Process(main_u16Ticks10ms);
				main_u16Ticks10ms = 0;
			}

			/* Task 1000 ms */
			if (main_u16Ticks1000ms >= 1000)
			{
				response_checker();
				main_u16Ticks1000ms -= 1000;
			}
			/* Outputs */
			NMEA2K_ProcessTx();
#ifdef TWAI_EXTENDED_LOG
			twai_logs();
#endif
			/* Switch state when no transfers are pending */
			if (can_transfers_pending())
			{
				/* Can't sleep communication ongoing */
			}
			else
			{
				/* Enable interrupts and pause driver */
#ifdef CAN_EXTENDED_LOG
				LOG(D, "Active -> Inactive");
#endif
				can_status = CAN_STATUS_INACTIVE;
#if defined (CAN_RS_PIN_NUM)
				CAN_RS(CAN_RS_LOW_POWER);
#endif
				TRUE_CHECK(gpio_intr_enable(DEVICE_TWAI_RX) == ESP_OK);
				hal_can_pause(MY_CAN_DEVICE, false);
			}
			break;

		case CAN_STATUS_INACTIVE:
			/* Let task sleep until next RX interrupt */
			if (ulTaskNotifyTake(pdTRUE, inactive_to_unknown_timeout) > 0)
			{
				/* Traffic detected, unpause driver */
#ifdef CAN_EXTENDED_LOG
				LOG(D, "Inactive -> Active");
#endif
				main_u16Elapsed_ms = notify_counter = 0;
				can_status = CAN_STATUS_ACTIVE;
				hal_can_resume(MY_CAN_DEVICE, false);
			}
			else
			{
				Main_ProcessTimers();
				/* Task 10 ms */
				if (main_u16Ticks10ms >= 10)
				{
					NMEA2K_UpdateTimers(main_u16Ticks10ms);
					MSGCAN_Process(main_u16Ticks10ms);
					main_u16Ticks10ms = 0;
				}
				/* Go to RVC unknown state after 1 second of inactivity */
				if (notify_counter >= TIMER_COUNT_2_S)
				{
					/* No traffic detected after a timeout */
#ifdef CAN_EXTENDED_LOG
					LOG(D, "Inactive -> Unknown");
#endif
					notify_counter = 0;
					main_u16Elapsed_ms = 0;
					can_status = CAN_STATUS_UNKNOWN;
					/* Enable interrupts and disable driver */
#if defined (CAN_RS_PIN_NUM)
					CAN_RS(CAN_RS_LOW_POWER);
#endif
					TRUE_CHECK(gpio_intr_enable(DEVICE_TWAI_RX) == ESP_OK);
					hal_can_stop(MY_CAN_DEVICE);
				}
				notify_counter = main_u16Elapsed_ms;
			}
			break;

		default:
			can_status = CAN_STATUS_UNKNOWN;
			break;
		}
	}
}

/**
 * @brief Update main timers
 *
 * @return none
 */
static void Main_ProcessTimers(void)
{
	/* Extract elapsed time since the last call */
	uint16_t u16Elapsed;
	uint16_t u16Now = hal_cpu_get_millis();

	if (main_u16TicksInit == 0)
	{
		u16Elapsed = 0;
		main_u16TicksInit = 1;
	}
	else
	{
		/* Handle wrap around of timer */
		if (main_u16TicksSys <= u16Now)
		{
			u16Elapsed = u16Now - main_u16TicksSys;
		}
		else
		{
			u16Elapsed = 0xFFFF - main_u16TicksSys + u16Now + 1;
		}
	}

	main_u16TicksSys = u16Now;

	/* Update timers */
	main_u16Elapsed_ms += u16Elapsed;
	main_u16Ticks10ms  += u16Elapsed;
	main_u16Ticks1000ms += u16Elapsed;
}
