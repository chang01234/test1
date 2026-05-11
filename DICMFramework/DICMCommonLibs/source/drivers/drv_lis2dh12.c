/*! \file drv_lis2dh12.c
 *	\brief LIS2DH12 driver.
 */
#include "iGeneralDefinitions.h"

#ifdef DEVICE_LIS2DH12

#include "drv_lis2dh12.h"
#include "lis2dh12_reg.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "hal_gpio.h"
#include "hal_spi_master.h"
#include "hal_i2c_master.h"

/* By pass lib related define  */
#define I2C_PORT 0
uint8_t status_reg_g = 0x00U;
acc_event_t event;


// Global variables
uint8_t i2c_port = I2C_PORT;
uint8_t device_add = (LIS2DH12_I2C_ADD_H >> 1);

stmdev_ctx_t lis2dh12_ctxt;   // Global lis2dh12 device context
uint8_t intr1_flag = false;   //Interrupt1 flag for LIS2DH12 sensor
uint8_t intr2_flag = false;   //Interrupt2 flag for LIS2DH12 sensor


static LIS2DH12_SEQ_CB eventdet_callback;
lis2dh12_config_values lis2dh12_user_cnf_val;

static LIS2DH12_INTF_RET_TYPE lis2dh12_write_i2c(void *intf_ptr, uint8_t reg_addr, const uint8_t *reg_data, uint16_t len);
static LIS2DH12_INTF_RET_TYPE lis2dh12_read_i2c(void *intf_ptr, uint8_t reg_addr, uint8_t *reg_data, uint16_t len);

float lis2dh12_accel_rawdata_to_mg(int16_t accel_axis_raw);

/*! \brief  This function enable freefall event configuration.
 *         
 * 		Refer seq\lis2dh12_reg.h for more information
 * 
 */
void enable_freefall_event()
{
	disable_all_event();
	lis2dh12_user_cnf_val.ctrl_reg1        = 0x57;
	lis2dh12_user_cnf_val.ctrl_reg2        = 0x00;
	//lis2dh12_user_cnf_val.ctrl_reg3        = 0x40;
	lis2dh12_user_cnf_val.ctrl_reg6        = 0x40;
	lis2dh12_user_cnf_val.ctrl_reg4        = 0x00;
	//lis2dh12_user_cnf_val.ctrl_reg5        = 0x08;
	lis2dh12_user_cnf_val.ctrl_reg5        = 0x02;

	lis2dh12_user_cnf_val.int1_ths         = 0x0f;
	lis2dh12_user_cnf_val.int1_duration    = 0x03;
	lis2dh12_user_cnf_val.int1_cfg         = 0x95;
	
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG1, &lis2dh12_user_cnf_val.ctrl_reg1);
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG2, &lis2dh12_user_cnf_val.ctrl_reg2);
	//LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG3, &lis2dh12_user_cnf_val.ctrl_reg3);
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG6, &lis2dh12_user_cnf_val.ctrl_reg6);
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG4, &lis2dh12_user_cnf_val.ctrl_reg4);
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG5, &lis2dh12_user_cnf_val.ctrl_reg5);
	
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_INT2_THS,  		&lis2dh12_user_cnf_val.int1_ths);
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_INT2_DURATION,  &lis2dh12_user_cnf_val.int1_duration);
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_INT2_CFG,  		&lis2dh12_user_cnf_val.int1_cfg);

	// lis2dh12_user_cnf_val.ctrl_reg1        = 0x57;
	// lis2dh12_user_cnf_val.ctrl_reg2        = 0x00;
	// lis2dh12_user_cnf_val.ctrl_reg3        = 0x20;
	// lis2dh12_user_cnf_val.ctrl_reg4        = 0x00;
	// lis2dh12_user_cnf_val.ctrl_reg5        = 0x02;
	// lis2dh12_user_cnf_val.ctrl_reg6        = 0x40;

	// lis2dh12_user_cnf_val.int1_ths         = 0x0f;
	// lis2dh12_user_cnf_val.int1_duration    = 0x03;
	// lis2dh12_user_cnf_val.int1_cfg         = 0x95;
	
	// LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG1, &lis2dh12_user_cnf_val.ctrl_reg1);
	// LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG2, &lis2dh12_user_cnf_val.ctrl_reg2);
	// LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG3, &lis2dh12_user_cnf_val.ctrl_reg3);
	// LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG4, &lis2dh12_user_cnf_val.ctrl_reg4);
	// LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG5, &lis2dh12_user_cnf_val.ctrl_reg5);
	// LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG6, &lis2dh12_user_cnf_val.ctrl_reg6);
	
	// LIS2DH12_SeqConfig_Set(LIS2DH12_SET_INT1_THS,  		&lis2dh12_user_cnf_val.int1_ths);
	// LIS2DH12_SeqConfig_Set(LIS2DH12_SET_INT1_DURATION,  &lis2dh12_user_cnf_val.int1_duration);
	// LIS2DH12_SeqConfig_Set(LIS2DH12_SET_INT1_CFG,  		&lis2dh12_user_cnf_val.int1_cfg);

}
/*! \brief  This function enable wakeup event.
 *         
 *		Refer seq\lis2dh12_reg.h for more information
 *
 */
void enable_wakeup_event()
{
	disable_all_event();
	lis2dh12_user_cnf_val.ctrl_reg1        = 0x57;
	lis2dh12_user_cnf_val.ctrl_reg2        = 0x00;
	//lis2dh12_user_cnf_val.ctrl_reg3        = 0x40;
	lis2dh12_user_cnf_val.ctrl_reg6        = 0x40;
	lis2dh12_user_cnf_val.ctrl_reg4        = 0x00;
	//lis2dh12_user_cnf_val.ctrl_reg5        = 0x08;
	lis2dh12_user_cnf_val.ctrl_reg5        = 0x02;

	lis2dh12_user_cnf_val.int1_ths         = 0x10;
	lis2dh12_user_cnf_val.int1_duration    = 0x01;
	lis2dh12_user_cnf_val.int1_cfg         = 0x0A;
	
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG1, &lis2dh12_user_cnf_val.ctrl_reg1);
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG2, &lis2dh12_user_cnf_val.ctrl_reg2);
	//LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG3, &lis2dh12_user_cnf_val.ctrl_reg3);
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG6, &lis2dh12_user_cnf_val.ctrl_reg6);
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG4, &lis2dh12_user_cnf_val.ctrl_reg4);
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG5, &lis2dh12_user_cnf_val.ctrl_reg5);
	
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_INT2_THS,  		&lis2dh12_user_cnf_val.int1_ths);
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_INT2_DURATION,  &lis2dh12_user_cnf_val.int1_duration);
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_INT2_CFG,  		&lis2dh12_user_cnf_val.int1_cfg);

	// lis2dh12_user_cnf_val.ctrl_reg1        = 0x57;
	// lis2dh12_user_cnf_val.ctrl_reg2        = 0x00;
	// lis2dh12_user_cnf_val.ctrl_reg3        = 0x20;
	// lis2dh12_user_cnf_val.ctrl_reg4        = 0x00;
	// lis2dh12_user_cnf_val.ctrl_reg5        = 0x02;
	// lis2dh12_user_cnf_val.ctrl_reg6        = 0x40;

	// lis2dh12_user_cnf_val.int1_ths         = 0x10;
	// lis2dh12_user_cnf_val.int1_duration    = 0x01;
	// lis2dh12_user_cnf_val.int1_cfg         = 0x0A;
	
	// LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG1, &lis2dh12_user_cnf_val.ctrl_reg1);
	// LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG2, &lis2dh12_user_cnf_val.ctrl_reg2);
	// LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG3, &lis2dh12_user_cnf_val.ctrl_reg3);
	// LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG4, &lis2dh12_user_cnf_val.ctrl_reg4);
	// LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG5, &lis2dh12_user_cnf_val.ctrl_reg5);
	// LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG6, &lis2dh12_user_cnf_val.ctrl_reg6);
	
	// LIS2DH12_SeqConfig_Set(LIS2DH12_SET_INT1_THS,  		&lis2dh12_user_cnf_val.int1_ths);
	// LIS2DH12_SeqConfig_Set(LIS2DH12_SET_INT1_DURATION,  &lis2dh12_user_cnf_val.int1_duration);
	// LIS2DH12_SeqConfig_Set(LIS2DH12_SET_INT1_CFG,  		&lis2dh12_user_cnf_val.int1_cfg);
}
/*! \brief  This function enable tilt event and configuration.
 *		Refer seq\lis2dh12_reg.h for more information
 *         
 */
void enable_tilt_event()
{
	disable_all_event();
	uint8_t ref_buf;
	lis2dh12_user_cnf_val.ctrl_reg1        = 0x57;
	lis2dh12_user_cnf_val.ctrl_reg2        = 0x09;
	//lis2dh12_user_cnf_val.ctrl_reg3        = 0x40;
	lis2dh12_user_cnf_val.ctrl_reg6        = 0x40;
	lis2dh12_user_cnf_val.ctrl_reg4        = 0x00;
	//lis2dh12_user_cnf_val.ctrl_reg5        = 0x08;
	lis2dh12_user_cnf_val.ctrl_reg5        = 0x02;

	lis2dh12_user_cnf_val.int1_ths         = 0x10;
	lis2dh12_user_cnf_val.int1_duration    = 0x00;
	lis2dh12_user_cnf_val.int1_cfg         = 0x2A;
	
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG1, &lis2dh12_user_cnf_val.ctrl_reg1);
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG2, &lis2dh12_user_cnf_val.ctrl_reg2);
	//LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG3, &lis2dh12_user_cnf_val.ctrl_reg3);
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG6, &lis2dh12_user_cnf_val.ctrl_reg6);
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG4, &lis2dh12_user_cnf_val.ctrl_reg4);
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG5, &lis2dh12_user_cnf_val.ctrl_reg5);
	
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_INT2_THS,  		&lis2dh12_user_cnf_val.int1_ths);
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_INT2_DURATION,  &lis2dh12_user_cnf_val.int1_duration);
	lis2dh12_filter_reference_get(&lis2dh12_ctxt,&ref_buf);
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_INT2_CFG,  		&lis2dh12_user_cnf_val.int1_cfg);

	// uint8_t ref_buf;
	// lis2dh12_user_cnf_val.ctrl_reg1        = 0x57;
	// lis2dh12_user_cnf_val.ctrl_reg2        = 0x0A;
	// lis2dh12_user_cnf_val.ctrl_reg3        = 0x20;
	// lis2dh12_user_cnf_val.ctrl_reg4        = 0x00;
	// lis2dh12_user_cnf_val.ctrl_reg5        = 0x02;
	// lis2dh12_user_cnf_val.ctrl_reg6        = 0x40;

	// lis2dh12_user_cnf_val.int1_ths         = 0x10;
	// lis2dh12_user_cnf_val.int1_duration    = 0x00;
	// lis2dh12_user_cnf_val.int1_cfg         = 0x2A;
	
	// LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG1, &lis2dh12_user_cnf_val.ctrl_reg1);
	// LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG2, &lis2dh12_user_cnf_val.ctrl_reg2);
	// LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG3, &lis2dh12_user_cnf_val.ctrl_reg3);
	// LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG4, &lis2dh12_user_cnf_val.ctrl_reg4);
	// LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG5, &lis2dh12_user_cnf_val.ctrl_reg5);
	// LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG6, &lis2dh12_user_cnf_val.ctrl_reg6);
	
	// LIS2DH12_SeqConfig_Set(LIS2DH12_SET_INT1_THS,  		&lis2dh12_user_cnf_val.int1_ths);
	// LIS2DH12_SeqConfig_Set(LIS2DH12_SET_INT1_DURATION,  &lis2dh12_user_cnf_val.int1_duration);
	// lis2dh12_filter_reference_get(&lis2dh12_ctxt,&ref_buf);
	// LIS2DH12_SeqConfig_Set(LIS2DH12_SET_INT1_CFG,  		&lis2dh12_user_cnf_val.int1_cfg);
}
void enable_motion_and_tap_event()
{
	disable_all_event();
	lis2dh12_user_cnf_val.ctrl_reg1        = 0x57;
	lis2dh12_user_cnf_val.ctrl_reg2        = 0x04;
	//lis2dh12_user_cnf_val.ctrl_reg3        = 0xC0;
	lis2dh12_user_cnf_val.ctrl_reg6        = 0xC0;
	lis2dh12_user_cnf_val.ctrl_reg4        = 0x40;
	lis2dh12_user_cnf_val.ctrl_reg5        = 0x02;

	lis2dh12_user_cnf_val.int1_ths         = 0x10;
	lis2dh12_user_cnf_val.int1_duration    = 0x01;
	lis2dh12_user_cnf_val.int1_cfg         = 0x0A;

	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG1, &lis2dh12_user_cnf_val.ctrl_reg1);
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG2, &lis2dh12_user_cnf_val.ctrl_reg2);
	//LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG3, &lis2dh12_user_cnf_val.ctrl_reg3);
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG6, &lis2dh12_user_cnf_val.ctrl_reg6);
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG4, &lis2dh12_user_cnf_val.ctrl_reg4);
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG5, &lis2dh12_user_cnf_val.ctrl_reg5);

	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_INT2_THS,  		&lis2dh12_user_cnf_val.int1_ths);
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_INT2_DURATION,  &lis2dh12_user_cnf_val.int1_duration);
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_INT2_CFG,  		&lis2dh12_user_cnf_val.int1_cfg);

    lis2dh12_user_cnf_val.data_rate        = LIS2DH12_ODR_100Hz;   //setting the data rate as 100HZ
    lis2dh12_user_cnf_val.full_scale       = LIS2DH12_2g;   		//setting the full scale value as +/-2g
	lis2dh12_user_cnf_val.tap_ths_set	  	= 0x06;					//setting the Tap threshold as 562mg

	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_DATA_RATE,  &lis2dh12_user_cnf_val.data_rate);
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_FULL_SCALE, &lis2dh12_user_cnf_val.full_scale);
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_TAP_THRHLD, &lis2dh12_user_cnf_val.tap_ths_set);

	enableTapDetection();
	lis2dh12_shock_dur_set(&lis2dh12_ctxt,0x81);
	lis2dh12_quiet_dur_set(&lis2dh12_ctxt,0x15);
}

/*! \brief  This function enable tap event functionality.
 *         
 *	   Refer seq\lis2dh12_reg.h for more information
 *
 */
void enable_tap_event()
{
	 disable_all_event();
	 lis2dh12_user_cnf_val.ctrl_reg1		= 0x07;
	 lis2dh12_user_cnf_val.ctrl_reg2		= 0x04;
	 //lis2dh12_user_cnf_val.ctrl_reg3		= 0x80;
	 lis2dh12_user_cnf_val.ctrl_reg6		= 0x80;
	 lis2dh12_user_cnf_val.ctrl_reg4		= 0x40;
	 
	 LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG1, &lis2dh12_user_cnf_val.ctrl_reg1);
	 LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG2, &lis2dh12_user_cnf_val.ctrl_reg2);
	 //LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG3, &lis2dh12_user_cnf_val.ctrl_reg3);
	 LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG6, &lis2dh12_user_cnf_val.ctrl_reg6);
	 LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG4, &lis2dh12_user_cnf_val.ctrl_reg4); 

     lis2dh12_user_cnf_val.data_rate        = LIS2DH12_ODR_100Hz;   //setting the data rate as 100HZ
     lis2dh12_user_cnf_val.full_scale       = LIS2DH12_2g;   		//setting the full scale value as +/-2g
	 lis2dh12_user_cnf_val.tap_ths_set	  	= 0x06;					//setting the Tap threshold as 562mg

	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_DATA_RATE,  &lis2dh12_user_cnf_val.data_rate);
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_FULL_SCALE, &lis2dh12_user_cnf_val.full_scale);
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_TAP_THRHLD, &lis2dh12_user_cnf_val.tap_ths_set);

	enableTapDetection();
	lis2dh12_shock_dur_set(&lis2dh12_ctxt,0x81);
	lis2dh12_quiet_dur_set(&lis2dh12_ctxt,0x15);

	//  lis2dh12_user_cnf_val.ctrl_reg1		= 0x07;
	//  lis2dh12_user_cnf_val.ctrl_reg2		= 0x04;
	//  lis2dh12_user_cnf_val.ctrl_reg3		= 0x80;
	//  lis2dh12_user_cnf_val.ctrl_reg4		= 0x40;
	//  lis2dh12_user_cnf_val.ctrl_reg6		= 0x40;
	 
	//  LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG1, &lis2dh12_user_cnf_val.ctrl_reg1);
	//  LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG2, &lis2dh12_user_cnf_val.ctrl_reg2);
	//  LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG3, &lis2dh12_user_cnf_val.ctrl_reg3);
	//  LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG4, &lis2dh12_user_cnf_val.ctrl_reg4); 
	//  LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG6, &lis2dh12_user_cnf_val.ctrl_reg6); 

    //  lis2dh12_user_cnf_val.data_rate        = LIS2DH12_ODR_400Hz;   //setting the data rate as 100HZ
    //  lis2dh12_user_cnf_val.full_scale       = LIS2DH12_2g;   		//setting the full scale value as +/-2g
	//  lis2dh12_user_cnf_val.tap_ths_set	  	= 0x06;					//setting the Tap threshold as 562mg

	// LIS2DH12_SeqConfig_Set(LIS2DH12_SET_DATA_RATE,  &lis2dh12_user_cnf_val.data_rate);
	// LIS2DH12_SeqConfig_Set(LIS2DH12_SET_FULL_SCALE, &lis2dh12_user_cnf_val.full_scale);
	// LIS2DH12_SeqConfig_Set(LIS2DH12_SET_TAP_THRHLD, &lis2dh12_user_cnf_val.tap_ths_set);

	// enableTapDetection();
	// lis2dh12_shock_dur_set(&lis2dh12_ctxt,0x81);
	// lis2dh12_quiet_dur_set(&lis2dh12_ctxt,0x15);
}

void enableInt1click(void)
{
  //lis2dh12_ctrl_reg3_t val;
  lis2dh12_ctrl_reg6_t val;
  //lis2dh12_pin_int1_config_get(&lis2dh12_ctxt, &val);
  lis2dh12_pin_int2_config_get(&lis2dh12_ctxt, &val);

  //val.i1_click = 1; //Enable Tap on INT1
  val.i2_click = 1; //Enable Tap on INT2

  //lis2dh12_pin_int1_config_set(&lis2dh12_ctxt, &val);
   lis2dh12_pin_int2_config_set(&lis2dh12_ctxt, &val);
}

//Enable single tap detection
void enableTapDetection()
{  
  lis2dh12_click_cfg_t newBits;
  if (lis2dh12_tap_conf_get(&lis2dh12_ctxt, &newBits) == 0)
  {
    newBits.xs = true;
    newBits.ys = true;
    newBits.zs = true;
    lis2dh12_tap_conf_set(&lis2dh12_ctxt, &newBits);
  }
}

void disable_all_event()
{
	lis2dh12_user_cnf_val.ctrl_reg1        = 0x07;
	lis2dh12_user_cnf_val.ctrl_reg2        = 0x00;
	lis2dh12_user_cnf_val.ctrl_reg3        = 0x00;
	lis2dh12_user_cnf_val.ctrl_reg4        = 0x00;
	lis2dh12_user_cnf_val.ctrl_reg5        = 0x00;
	lis2dh12_user_cnf_val.ctrl_reg6        = 0x00;

	lis2dh12_user_cnf_val.int1_ths         = 0x00;
	lis2dh12_user_cnf_val.int1_duration    = 0x00;
	lis2dh12_user_cnf_val.int1_cfg         = 0x00;

	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG1, &lis2dh12_user_cnf_val.ctrl_reg1);
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG2, &lis2dh12_user_cnf_val.ctrl_reg2);
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG3, &lis2dh12_user_cnf_val.ctrl_reg3);
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG4, &lis2dh12_user_cnf_val.ctrl_reg4);
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG5, &lis2dh12_user_cnf_val.ctrl_reg5);
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_CTRL_REG6, &lis2dh12_user_cnf_val.ctrl_reg6);

	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_INT2_THS,  		&lis2dh12_user_cnf_val.int1_ths);
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_INT2_DURATION,  &lis2dh12_user_cnf_val.int1_duration);
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_INT2_CFG,  		&lis2dh12_user_cnf_val.int1_cfg);

	lis2dh12_user_cnf_val.tap_ths_set	   = 0x00;
	LIS2DH12_SeqConfig_Set(LIS2DH12_SET_TAP_THRHLD, &lis2dh12_user_cnf_val.tap_ths_set);

	uint8_t newBits = 0x00;
	lis2dh12_tap_conf_set(&lis2dh12_ctxt, (lis2dh12_click_cfg_t*)&newBits);
	lis2dh12_shock_dur_set(&lis2dh12_ctxt,0x00);
	lis2dh12_quiet_dur_set(&lis2dh12_ctxt,0x00);

	LIS2DH12_Default_SeqConfig();
}

bool isTapped(void)
{
//   lis2dh12_click_src_t interruptSource;
//   lis2dh12_tap_source_get(&lis2dh12_ctxt, &interruptSource);
//   if (interruptSource.x || interruptSource.y || interruptSource.z) //Check if ZYX bits are set
//   {
//     return (true);
//   }
//   return (false);

  lis2dh12_click_src_t interruptSource;
  lis2dh12_tap_source_get(&lis2dh12_ctxt, &interruptSource);
  if (interruptSource.x || interruptSource.y || interruptSource.z) //Check if ZYX bits are set
  {
    return (true);
  }
  return (false);
}

bool getInt1(void)
{
	// lis2dh12_int1_src_t val;
	// lis2dh12_int1_gen_source_get(&lis2dh12_ctxt, &val);
	// if (val.ia)
	// 	return (true);
	// return (false);

	lis2dh12_int2_src_t val;
	lis2dh12_int2_gen_source_get(&lis2dh12_ctxt, &val);
	if (val.ia)
		return (true);
	return (false);
}

/*Reading the status register value*/
void reading_status_reg()
{
	lis2dh12_status_reg_t status_reg = {0};
	lis2dh12_status_get(&lis2dh12_ctxt,&status_reg);
	//printf("status_reg = %u", *(uint8_t*)&status_reg);
	status_reg_g = *(uint8_t*)&status_reg;
}
/*! \brief LIS2DH12 sensor polling_for_acc_data function.
 *
 *  This function will be called to get acceleration data
 *   
 *
 *  \param raw_data     New raw data pointer.
 
 *  \return 0 if successful. Any other value otherwise.
 */
int32_t polling_for_acc_data(int16_t *raw_data)
{	
	uint8_t dev_id = LIS2DH12_OUT_X_L | 0x80; //Register address

	reading_status_reg();

	if (!status_reg_g)
		return -1;

	hal_i2c_master_writeread(I2C_PORT, device_add, &dev_id, sizeof(dev_id), (uint8_t*)raw_data, 6);

	return 0;
}

/*! \brief LIS2DH12 sensor sequencer lis2dh12_Tampering_Tap_Detection function.
 *
 *  This function will be used for checking the getting event Tapper or Tampering 
 *  based on the Threshold value
 *
 *  \return 0 if successful. Any other value otherwise.
 */
bool lis2dh12_Tampering_Tap_Detection(void)
{
    bool status = false; // Single Tap = false, Tampering - true
    uint8_t thrhold;

    lis2dh12_tap_threshold_get(&lis2dh12_ctxt, &thrhold); //reading Threshold value
    if(thrhold >=20)
      status = true;

    return status;
}

/*! \brief This internal API is for LIS2DH12 sensor interrupt callback function handler.
 *
 * \param   all_int_src read the interrupt source register  
 *
 *  \return No return value.
 */
void lis2dh12_int1_cb_handler(int device, int port, int pin)
{
   // LOG(I,"IO_EX_ISR %d %d %d",device,port,pin);

	intr2_flag = true;
	
    lis2dh12_int2_src_t all_int2_src;
	lis2dh12_click_src_t tap_int_src;

    lis2dh12_int2_gen_source_get(&lis2dh12_ctxt, &all_int2_src);
	lis2dh12_tap_source_get(&lis2dh12_ctxt, &tap_int_src);
	// LOG(I,"%x",all_int2_src.ia)
	// LOG(I,"%x",tap_int_src.ia)
	// LOG(I,"%x",tap_int_src.dclick)
    
	if ((all_int2_src.ia && 0x01) == 0x01)
    {
		//LOG(I,"all_int2_src")
		//LOG(I,"event=%d",event)
        if (event == Moving || event == motion_and_tap)
			eventdet_callback(LIS2DH12_SEQ_ID, LIS2DH12_PARAM_EDM_MOTION);	      /* Interrupt source is Motion */
		if (event == Tilting)
			eventdet_callback(LIS2DH12_SEQ_ID, LIS2DH12_PARAM_EDM_TILT);          /* Interrupt source is Tilt */
		if (event == Fall_Detection)
			eventdet_callback(LIS2DH12_SEQ_ID, LIS2DH12_PARAM_EDM_FREEFALL);      /* Interrupt source is Free Fall */
    }
    else if ((tap_int_src.ia && 0x01) == 0x01)
    {
		//LOG(I,"tap_int_src")
		if ((tap_int_src.sclick && 0x01) == 0x01)
    	{
        	// Single Tap or Tampering detection
        	if (!lis2dh12_Tampering_Tap_Detection())
        	{
           		eventdet_callback(LIS2DH12_SEQ_ID, LIS2DH12_PARAM_EDM_SINGLE_TAP); /* Interrupt source is Single Tap */
        	}
        	else
        	{
            	eventdet_callback(LIS2DH12_SEQ_ID, LIS2DH12_PARAM_EDM_TAMPERING); /* Interrupt source is Tampering */
        	}
    	}
    	else  if ((tap_int_src.dclick && 0x01) == 0x01)
    	{
			//LOG(I,"tap_int_src.dclick")
        	eventdet_callback(LIS2DH12_SEQ_ID, LIS2DH12_PARAM_EDM_DOUBLE_TAP);    /* Interrupt source is Double Tap */
    	}else if((tap_int_src.sclick && 0x01) == 0x01)
		{
			LOG(I,"tap_int_src.sclick");
			eventdet_callback(LIS2DH12_SEQ_ID,LIS2DH12_PARAM_EDM_SINGLE_TAP);
		}
    }
}

/*!
 * @brief This internal API is used to validate the device structure pointer for
 * null conditions. 
 *
 *  \return 0 if successful. Any other value otherwise.
 */
static int8_t lis2dh12_null_ptr_check(const stmdev_ctx_t *dev)
{
  int8_t rslt;

    if ((dev == NULL) || (dev->write_reg == NULL) || (dev->read_reg == NULL))
    {
      /* Device structure pointer is not valid */
        rslt = LIS2DH12_E_NULL_PTR;
    } 
    else
    {
      /* Device structure is fine */
        rslt = LIS2DH12_OK;
    }

  return rslt;
}

/*!
 *  @brief This API is the entry point and initializes the sensor and verifies it's communication.
 *  It reads the device-id(Who Am I) from the sensor and make sure communication is fine.
 *
 *  \return 0 if successful. Any other value otherwise.
 */
int8_t lis2dh12_init(stmdev_ctx_t *dev)
{
  int8_t rslt;

  /* device id read try count */
  uint8_t try_count = 5;
  uint8_t device_id = 0;

  /* Check for null pointer in the device structure*/
  rslt = lis2dh12_null_ptr_check(dev);

  /* Proceed if null check is fine */
    if (rslt == LIS2DH12_OK)
    {
        while (try_count)
        {
            /* Read the chip-id of LIS2DH12 sensor */
            rslt = lis2dh12_device_id_get(dev, &device_id);

            /* Check for chip id validity */
            if ((rslt == LIS2DH12_OK) && (device_id == LIS2DH12_ID))
            {
                printf("Communication with LIS2DH12 is successful\n");
                break;
            }

            /* Wait for 10 ms */
            vTaskDelay(10);
            --try_count;
        }

        /* Device id check failed */
        if (!try_count)
        {
            rslt = LIS2DH12_E_DEV_NOT_FOUND;
        }
    }
  return rslt;
}

#if 1
/*! \brief  This function changes the sensor and/or sequencer configuration based on the
 *            provided Default configuration data set.
 *         
 *            Refer seq\lis2dh12_reg.h for more information
 *
 *
 *  \return 0 if successful. Any other value otherwise.
 */
uint8_t LIS2DH12_Default_SeqConfig(void)
{
    uint8_t status = false;

    /* set the all the sensor config parameter */
     lis2dh12_user_cnf_val.data_rate        = LIS2DH12_ODR_100Hz;   //setting the data rate as 100HZ
     lis2dh12_user_cnf_val.operating_mode   = LIS2DH12_HR_12bit;    //setting the Operating Mode as HR_12bits
     lis2dh12_user_cnf_val.full_scale       = LIS2DH12_2g;   		//setting the full scale value as +/-2g
     lis2dh12_user_cnf_val.block_data_update= LIS2DH12_conti_update;

     status = LIS2DH12_SeqConfig_Set(LIS2DH12_SET_DATA_RATE, &lis2dh12_user_cnf_val.data_rate);

     status = LIS2DH12_SeqConfig_Set(LIS2DH12_SET_OPERATING_MODE, &lis2dh12_user_cnf_val.operating_mode);

     status = LIS2DH12_SeqConfig_Set(LIS2DH12_SET_FULL_SCALE, &lis2dh12_user_cnf_val.full_scale);

	 status = LIS2DH12_SeqConfig_Set(LIS2DH12_SET_DATA_BLOCK_MODE, &lis2dh12_user_cnf_val.block_data_update);

	 lis2dh12_temperature_meas_set(&lis2dh12_ctxt, (lis2dh12_temp_en_t)0x03);

    return status;
}
#endif

/*! \brief LIS2DH12 sensor sequencer init function.
 *
 *  This function will be called by the scheduler once on boot to initialize the sequencer
 *   
 *
 *  \param bus_conf     Platform-specific bus and GPIO configuration data
 *  \param new_data     New event data pointer.
 
 *  \return 0 if successful. Any other value otherwise.
 */
int lis2dh12_seq_init(const lis2dh12_bus_conf *bus_conf, LIS2DH12_SEQ_CB cb)
{
    bool error = false;
	
    /* Register a callback */
    eventdet_callback = (void *) cb;

    switch (LIS2DH12_BUS_CONF_TYPE_I2C)//bus_conf->type) 
    {
        case LIS2DH12_BUS_CONF_TYPE_I2C:
			lis2dh12_ctxt.handle = &i2c_port;
            //*(uint8_t *)lis2dh12_ctxt.handle = &port;//bus_conf->i2c.port;
            lis2dh12_ctxt.write_reg = (void *) lis2dh12_write_i2c;
            lis2dh12_ctxt.read_reg = lis2dh12_read_i2c;
            //error = hal_i2c_master_init(*(uint8_t *)lis2dh12_ctxt.handle, bus_conf->i2c.sda, bus_conf->i2c.scl, bus_conf->i2c.bitrate);
            break;
        default:
            error = true; //invalid valid bus type specified.
            break;
    }
    if (!error)
    {
        error = lis2dh12_init(&lis2dh12_ctxt);
        if (error)
        {
            printf("Failed to communicate with LIS2DH12 sensor\n");
            error = true; // Error communicating with LIS sensor
        }
        error = LIS2DH12_Default_SeqConfig();	
		enable_tap_event();		
		// enable_freefall_event();
		// enable_wakeup_event();
		// enable_tilt_event();
		// enable_motion_and_tap_event();
        /*else 
        {
            error = hal_gpio_init();
            error = hal_gpio_pinsetup(bus_conf->gpio.device, bus_conf->gpio.port, bus_conf->gpio.pin, HAL_GPIO_PINMODE_READ, HAL_GPIO_INTRMODE_POSEDGE, lis2dh12_int_cb_handler);
        }*/
    }
    
    return error;
}

/*! \brief LIS2DH12 sensor internal function for acceleration Raw data to mg conversion.
 *
 *  This function will be called to convert raw acceleration data of any axis to mg value
 *   No chnages in the raw data to mg conversion even if sensor is in LP mode as per the datasheet
 *
 *  \param parameter    ID of configuration parameter to be changed.
 *  \param Out          Pointer to Output data write address.
 *
 *  \return 0 if successful. Any other value otherwise.
 */
float lis2dh12_accel_rawdata_to_mg(int16_t accel_axis_raw) 
{
    bool error = false;
    uint8_t fs_val;
	uint8_t  power_mode;
    float accel_mg_value = 0;

    error = lis2dh12_full_scale_get(&lis2dh12_ctxt, (lis2dh12_fs_t*)&fs_val); // Get the FS value wich has been set previously
    error = lis2dh12_operating_mode_get(&lis2dh12_ctxt, (lis2dh12_op_md_t *)&power_mode);
    if(!error)
    {
        if(fs_val == LIS2DH12_4g)       // if the FS value set was +-4g
        {
			switch (power_mode)
		    {
				case LIS2DH12_HR_12bit: //High resolution
			    	accel_mg_value = lis2dh12_from_fs4_hr_to_mg(accel_axis_raw);
			      	break;
			    case LIS2DH12_NM_10bit: //Normal mode
			    	accel_mg_value = lis2dh12_from_fs4_nm_to_mg(accel_axis_raw);
 				    break;
				case LIS2DH12_LP_8bit: //Low power mode
				    accel_mg_value = lis2dh12_from_fs4_lp_to_mg(accel_axis_raw);
				    break;
			}
        }
        else if(fs_val == LIS2DH12_8g)  // if the FS value set was +-8g
        {
        	switch (power_mode)
		    {
				case LIS2DH12_HR_12bit: //High resolution
			    	accel_mg_value = lis2dh12_from_fs8_hr_to_mg(accel_axis_raw);
			      	break;
			    case LIS2DH12_NM_10bit: //Normal mode
			    	accel_mg_value = lis2dh12_from_fs8_nm_to_mg(accel_axis_raw);
 				    break;
				case LIS2DH12_LP_8bit: //Low power mode
				    accel_mg_value = lis2dh12_from_fs8_lp_to_mg(accel_axis_raw);
				    break;
			}
        }
        else if(fs_val == LIS2DH12_16g) // if the FS value set was +-16g
        {
        switch (power_mode)
		    {
				case LIS2DH12_HR_12bit: //High resolution
			    	accel_mg_value = lis2dh12_from_fs16_hr_to_mg(accel_axis_raw);
			      	break;
			    case LIS2DH12_NM_10bit: //Normal mode
			    	accel_mg_value = lis2dh12_from_fs16_nm_to_mg(accel_axis_raw);
 				    break;
				case LIS2DH12_LP_8bit: //Low power mode
				    accel_mg_value = lis2dh12_from_fs16_lp_to_mg(accel_axis_raw);
				    break;
			}
        }
        else                            // if the FS value set was +-2g
        {
        switch (power_mode)
		    {
				case LIS2DH12_HR_12bit: //High resolution
			    	accel_mg_value = lis2dh12_from_fs2_hr_to_mg(accel_axis_raw);
			      	break;
			    case LIS2DH12_NM_10bit: //Normal mode
			    	accel_mg_value = lis2dh12_from_fs2_nm_to_mg(accel_axis_raw);
 				    break;
				case LIS2DH12_LP_8bit: //Low power mode
				    accel_mg_value = lis2dh12_from_fs2_lp_to_mg(accel_axis_raw);
				    break;
			}
        }
    }
    else    // Error in reading FS value from sensor
    {
        error = true;
    }
    
    return accel_mg_value;  // Converted mg value
}

float lis2dh12_accel_rawtemp_to_celsius(int16_t accel_raw_temp)
{
	bool error = false;
	uint8_t  operating_mode;
    float accel_temp_celsius_value = 0;

	error = lis2dh12_operating_mode_get(&lis2dh12_ctxt, (lis2dh12_op_md_t *)&operating_mode);

	if(!error)
    {
    	switch (operating_mode)
		{
			case LIS2DH12_HR_12bit: //High resolution
				accel_temp_celsius_value = lis2dh12_from_lsb_hr_to_celsius(accel_raw_temp);
				break;
			case LIS2DH12_NM_10bit: //Normal mode
				accel_temp_celsius_value = lis2dh12_from_lsb_nm_to_celsius(accel_raw_temp);
				break;
			case LIS2DH12_LP_8bit: //Low power mode
				accel_temp_celsius_value = lis2dh12_from_lsb_lp_to_celsius(accel_raw_temp);
				break;
		}
	}
	else    // Error in reading operating mode value from sensor
    {
        error = true;
    }
	return accel_temp_celsius_value;
}


/*! \brief LIS2DH12 sensor sequencer Read function.
 *
 *  This function will be called by the scheduler to Read the sensor Data ()
 *   
 *
 *  \param parameter    ID of configuration parameter to be changed.
 *  \param Out          Pointer to Output data write address.
 *
 *  \return 0 if successful. Any other value otherwise.
 */
int lis2dh12_seq_read(lis2dh12_read_params parameter, void *out)
{
    bool error = false;

    if (parameter == LIS2DH12_MEAS_TYPE_WHOAMI)     // Parameter to get Device ID of Sensor
    {
    	uint8_t device_id;
		error = lis2dh12_device_id_get(&lis2dh12_ctxt, &device_id); // Device ID of sensor
        if (!error)
        {
        	if(device_id == LIS2DH12_ID)
            {
            	*(uint8_t *)out = device_id;  // Copy Device ID to out variable
            }
            else    // If Device ID is not matching as per datasheet value
            {
            	error = true;
            }
      	}
        else     // If fails to communicate with Sensor
        {
        	error = true;
        }
#ifdef LIS2DH12_DEBUG_ENABLE
    printf("Device ID = %d\n", *(uint8_t *)out);
#endif
    }
	
	if (parameter == LIS2DH12_MEAS_TYPE_TEMP)   // Parameter to get temperature value from Sensor
	{
    	uint16_t raw_temp = 0;                    // Variable to hold raw temperature value
	    float temperature;                        // Variable to hold temperature value in Degree Celsius
    	uint32_t temp_data = 0;                   // Temperature data in degree celsius

		error = lis2dh12_temperature_raw_get(&lis2dh12_ctxt, (uint8_t *)&raw_temp); // raw temperature data - 2 bytes
        if(!error)
       	{
       		temperature = lis2dh12_accel_rawtemp_to_celsius(raw_temp); // Convert raw temperature to degree celsius format
     	   	temp_data = (uint32_t)(temperature);
            *(uint32_t *) out = temp_data;   // Copy temp in deg celsius to out variable
        }
        else    // If fails to communicate with Sensor
        {
        	error = true;
        }
#ifdef LIS2DH12_DEBUG_ENABLE
	printf("Temprature = %2.2f\n", *(float *)&raw_temp);
#endif
	}

	if (parameter == LIS2DH12_MEAS_TYPE_ACCEL)    // Parameter to get acceleration (x, y & z axis) values from Sensor
	{          
		
		static int16_t data_raw_acceleration[3];  // Variable to hold raw acceleration values of X, Y & Z axis
		static float acceleration_mg[3];		  // Variable to hold acceleration values of X, Y & Z axis in mg type
		int16_t accel_data[3];					  // Acceleration data in mg * 100

		/* Read acceleration data */
    	memset(data_raw_acceleration, 0x00, 3 * sizeof(int16_t));
	    error = polling_for_acc_data(data_raw_acceleration); // raw acceleration data - 6 bytes
        //error = lis2dh12_acceleration_raw_get(&lis2dh12_ctxt, (uint8_t *)data_raw_acceleration); // raw acceleration data - 6 bytes
		
        if (!error)
        {
	        /* Convert raw acceleartion data to mg format as per the sensitivity */ 
    	    acceleration_mg[0] = lis2dh12_accel_rawdata_to_mg(data_raw_acceleration[0]);
        	acceleration_mg[1] = lis2dh12_accel_rawdata_to_mg(data_raw_acceleration[1]);
        	acceleration_mg[2] = lis2dh12_accel_rawdata_to_mg(data_raw_acceleration[2]);

			//TODO:update the out variable with this acceleration data - HP		
			accel_data[0] = (uint16_t)(acceleration_mg[0] * 100);
			accel_data[1] = (uint16_t)(acceleration_mg[1] * 100);
			accel_data[2] = (uint16_t)(acceleration_mg[2] * 100);

			((uint16_t*)out)[0] = accel_data[0];
			((uint16_t*)out)[1] = accel_data[1];
			((uint16_t*)out)[2] = accel_data[2];

        }
		else    // If fails to communicate with Sensor
        {
        	error = true;
        }
#ifdef LIS2DH12_DEBUG_ENABLE
            printf("Acceleration on X axis = %2.2f\n", acceleration_mg[0]);
            printf("Acceleration on Y axis = %2.2f\n", acceleration_mg[1]);
            printf("Acceleration on Z axis = %2.2f\n", acceleration_mg[2]);
#endif 
	}
    
    return error;
}

/*! \brief LIS2DH12 sensor sequencer config function.
 *
 *  This function will be called by the scheduler to configure the sensor parameters
 *   
 *
 *  \param parameter    ID of configuration parameter to be changed.
 *  \param in           Pointer to new value for configuration parameter.
 *
 *  \return 0 if successful. Any other value otherwise.
 */
int LIS2DH12_SeqConfig_Set(uint16_t parameter, const void *in)
{
    bool error = false;
	uint8_t ctrl_reg;
    uint8_t val_u8 = *(uint8_t *)in;

    switch (parameter) 
    {
        case LIS2DH12_SET_DATA_RATE: // Set Data Rate
            if (val_u8 <= LIS2DH12_ODR_5kHz376_LP_1kHz344_NM_HP)
            {
                error = lis2dh12_data_rate_set(&lis2dh12_ctxt, (lis2dh12_odr_t)val_u8);
            } 
            else 
            {
                error = true;
            }
            break;

		case LIS2DH12_SET_OPERATING_MODE: // Set Power Mode //Operating Mode
			if (val_u8 <= LIS2DH12_LP_8bit)
			{
				error = lis2dh12_operating_mode_set(&lis2dh12_ctxt, (lis2dh12_op_md_t)val_u8);
			} 
			else 
			{
				error = true;
			}
			break;

        case LIS2DH12_SET_FULL_SCALE: // Set Full Scale
            if (val_u8 <= LIS2DH12_16g)
            {
                 error = lis2dh12_full_scale_set(&lis2dh12_ctxt, (lis2dh12_fs_t)val_u8);
            } 
            else 
            {
                 error = true;
            }
            break;

		case LIS2DH12_SET_TAP_THRHLD: // Set Tap threshold
			 if (val_u8 <= LIS2DH12_TAP_THRHLD_AXIS_MAX)
			 {
				 error = lis2dh12_tap_threshold_set(&lis2dh12_ctxt, val_u8);
			 } 
			 else 
			 {
				 error = true;
			 }
			 break;

    	case LIS2DH12_SET_SELF_TEST: // Set Self Test
        	if (val_u8 <= LIS2DH12_ST_NEGATIVE)
        	{
          		error = lis2dh12_self_test_set(&lis2dh12_ctxt, (lis2dh12_st_t)val_u8);
        	} 
        	else 
       		{
            	error = true;
            }
            break;

        case LIS2DH12_SET_INT1_NOTIFICATION: // Set INterrupt1 Notification
            if (val_u8 <= LIS2DH12_INT1_LATCHED)
            {
                error = lis2dh12_int1_pin_notification_mode_set(&lis2dh12_ctxt, (lis2dh12_lir_int1_t)val_u8);
            } 
            else 
            {
                error = true;
            }
            break;

		case LIS2DH12_SET_INT2_NOTIFICATION: // Set INterrupt2 Notification
			if (val_u8 <= LIS2DH12_INT2_LATCHED)
			{
				error = lis2dh12_int2_pin_notification_mode_set(&lis2dh12_ctxt, (lis2dh12_lir_int2_t)val_u8);
			} 
			else 
			{
				error = true;
			}
			break;

        case LIS2DH12_SET_DATA_BLOCK_MODE: // Set Data Block Mode
            if (val_u8 <= LIS2DH12_no_update_until)
            {
                error = lis2dh12_block_data_update_set(&lis2dh12_ctxt, (lis2dh12_dbm_t)val_u8);
            } 
            else 
            {
                error = true;
            }
            break;

		case LIS2DH12_SET_CTRL_REG1:	
			ctrl_reg = val_u8;
			error = lis2dh12_write_reg(&lis2dh12_ctxt, LIS2DH12_CTRL_REG1, &ctrl_reg, 1);
			break;

		case LIS2DH12_SET_CTRL_REG2:
			ctrl_reg = val_u8;
			error = lis2dh12_write_reg(&lis2dh12_ctxt, LIS2DH12_CTRL_REG2, &ctrl_reg, 1);
			break;

		case LIS2DH12_SET_CTRL_REG3:
			ctrl_reg = val_u8;
			error = lis2dh12_write_reg(&lis2dh12_ctxt, LIS2DH12_CTRL_REG3, &ctrl_reg, 1);
			break;

		case LIS2DH12_SET_CTRL_REG4:
			ctrl_reg = val_u8;
			error = lis2dh12_write_reg(&lis2dh12_ctxt, LIS2DH12_CTRL_REG4, &ctrl_reg, 1);
			break;

		case LIS2DH12_SET_CTRL_REG5:
			ctrl_reg = val_u8;
			error = lis2dh12_write_reg(&lis2dh12_ctxt, LIS2DH12_CTRL_REG5, &ctrl_reg, 1);
			break;

		case LIS2DH12_SET_CTRL_REG6:
			ctrl_reg = val_u8;
			error = lis2dh12_write_reg(&lis2dh12_ctxt, LIS2DH12_CTRL_REG6, &ctrl_reg, 1);
			break;

		case LIS2DH12_SET_INT1_THS:
            error = lis2dh12_int1_gen_threshold_set(&lis2dh12_ctxt, val_u8);
			break;

		case LIS2DH12_SET_INT1_DURATION:
            error = lis2dh12_int1_gen_duration_set(&lis2dh12_ctxt, val_u8);
			break;

		case LIS2DH12_SET_INT1_CFG:
            error = lis2dh12_int1_gen_conf_set(&lis2dh12_ctxt, (lis2dh12_int1_cfg_t*)&val_u8);
			break;

		case LIS2DH12_SET_INT2_THS:
            error = lis2dh12_int2_gen_threshold_set(&lis2dh12_ctxt, val_u8);
			break;

		case LIS2DH12_SET_INT2_DURATION:
            error = lis2dh12_int2_gen_duration_set(&lis2dh12_ctxt, val_u8);
			break;

		case LIS2DH12_SET_INT2_CFG:
            error = lis2dh12_int2_gen_conf_set(&lis2dh12_ctxt, (lis2dh12_int2_cfg_t*)&val_u8);
			break;
			

        default:
            /* Should not reach here */
            error = true;
    }

    return error;
}

/*! \brief LIS2DH12 sensor sequencer I2C Write API.   
 *
 *  \param intf_ptr    Pointer to I2C channel port
 *  \param reg_addr    register Address to write to.
 *  \param reg_data    Pointer to register data which caller wants to write to reg_addr
 *  \param len         len of bytes of reg_data that needs to be written to reg_addr
 *
 *  \return 0 if successful. Any other value otherwise.
 */
static LIS2DH12_INTF_RET_TYPE lis2dh12_write_i2c(void *intf_ptr, uint8_t reg_addr, const uint8_t *reg_data, uint16_t len) 
{
    uint8_t txbuf[100] = {0};

    if (len > sizeof(txbuf) / sizeof(txbuf[0]) - 1) 
    {
        return true; // Data too long for transfer buffer.
    }
    txbuf[0] = reg_addr;
    memcpy(&txbuf[1], reg_data, len);

    return hal_i2c_master_write(*(uint8_t *)intf_ptr, (LIS2DH12_I2C_ADD_H >> 1), txbuf, len + 1);
}

/*! \brief LIS2DH12 sensor sequencer I2C Read API.   
 *
 *  \param intf_ptr    Pointer to I2C channel port
 *  \param reg_addr    register Address to read from.
 *  \param reg_data    Pointer to register data which caller wants to store the data read from reg_addr
 *  \param len         len of bytes of reg_data that needs to be read from reg_addr
 *
 *  \return 0 if successful. Any other value otherwise.
 */
static LIS2DH12_INTF_RET_TYPE lis2dh12_read_i2c(void *intf_ptr, uint8_t reg_addr, uint8_t *reg_data, uint16_t len) 
{
    return hal_i2c_master_writeread(*(uint8_t *)intf_ptr, (LIS2DH12_I2C_ADD_H >> 1), &reg_addr, 1, reg_data, len);
}

#endif //DEVICE_LIS2DH12
