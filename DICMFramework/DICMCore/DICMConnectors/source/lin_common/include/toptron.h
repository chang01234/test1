/*
 * toptron.h
 *
 *  Created on: 18 Jun 2019
 *      Author: Stefan.Henningsohn
 */

#ifndef MAIN_TOPTRON_H_
#define MAIN_TOPTRON_H_

#include <stdint.h>
#include "payloads.h"

typedef struct
{
	uint8_t state1;
	uint8_t state2;
	uint8_t reserved[6];
} el624v2_distribution_ctrl_t;

typedef struct
{
	uint8_t ambient_temp;
	uint8_t inside_temp;
	uint8_t mo_bat_voltage;
	uint8_t co_bat_voltage;
	uint8_t water_levels;
	uint8_t misc1;
	uint8_t misc2;
	uint8_t reserved[1];
} el624v2_distribution_info_t;

typedef struct
{
	uint8_t state;
	uint8_t keys1;
	uint8_t wlan1;
	uint8_t wlan2;
	uint8_t reserved[4];
} el624v2_led_dimmer2_ctrl_t;

typedef struct
{
	uint8_t state1;
	uint8_t state2;
	uint8_t reserved[6];
} el624v2_led_dimmer2_info_t;

typedef struct
{
	uint8_t charging_voltage;
	uint16_t battery_voltage;
	uint8_t silent_mode;
} toptron_el603_charger_ctrl_t;

typedef struct
{
	uint8_t charging_current;
	uint8_t silent_mode;
	uint8_t reduced_power;
	uint8_t error_active;
	uint8_t charging_acitve;
} toptron_el603_charger_info_t;

typedef struct
{
	uint8_t temp_out;
	uint8_t temp_in;
	uint8_t bat_voltage;
	uint8_t fresh_wtr_lvl;
	uint8_t light_bed_left;
	uint8_t light_bed_right;
	uint8_t light_ceiling;
	uint8_t light_wall;
	uint8_t ms_light_status;
	uint8_t ms_all_status;
	uint8_t add_1_status;
	uint8_t add_2_status;
	uint8_t add_3_status;
	uint8_t light_awning_status;
	uint8_t light_amb_1_status;
	uint8_t light_amb_2_status;
	uint8_t light_amb_3_status;
	uint8_t heater_status;
	uint8_t floor_heater_status;
	uint8_t light_kitchen_1_status;
	uint8_t light_kitchen_2_status;
	uint8_t light_wc_status;
	uint8_t light_washroom_status;
	uint8_t fair_mode_status;
	uint8_t water_meas_rdy;
	uint8_t engine_running;
	uint8_t towing_mode;
	uint8_t mains_connected;
} toptron_lightbox_info_t;

typedef struct
{
	uint8_t light_bed_left;
	uint8_t light_bed_right;
	uint8_t light_ceiling;
	uint8_t light_wall;
} toptron_lightbox_ctrl_t;

#define TOPTRON_LIGHTBOX_CTRL_ID (0x01)
#define TOPTRON_LIGHTBOX_INFO_ID (0x02)
#define TOPTRON_CHARGER0_CTRL_ID (0x15)
#define TOPTRON_CHARGER0_INFO_ID (0x16)
#define TOPTRON_CHARGER1_CTRL_ID (0x18)
#define TOPTRON_CHARGER1_INFO_ID (0x19)

#define TOPTRON_CHARGER_CTRL_SIZE 4
#define TOPTRON_CHARGER_INFO_SIZE 2
extern void toptron_charger_C_Extract(toptron_el603_charger_ctrl_t* pCtrl, uint8_t*  pu8Data);
extern void toptron_charger_I_Extract(toptron_el603_charger_info_t* pInfo, uint8_t*  pu8Data);
extern void toptron_charger_C_Stuff(uint8_t* pu8Data, toptron_el603_charger_ctrl_t* pCtrl);

#define TOPTRON_LIGHTBOX_CTRL_SIZE 8
extern void toptron_lightbox_C_Extract(toptron_lightbox_ctrl_t* pCtrl, uint8_t* pu8Data);
extern void toptron_lightbox_I_Extract(toptron_lightbox_info_t* pInfo,uint8_t*  pu8Data);
extern void toptron_lightbox_C_Stuff(uint8_t* pu8Data, toptron_lightbox_ctrl_t* pCtrl);

extern int32_t toptron_charger_current_convert(int32_t i32Value);

#endif /* MAIN_TOPTRON_H_ */
