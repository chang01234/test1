/*****************************************************************************
 * \file       drv_uc1510c.
 * \brief      Driver code for the Touch and LCD Controller Driver IC UC1510C can able to drive any LCD upto 4 Backplanes and 40 Segment.
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
#ifdef DEVICE_UC1510C
/*****************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "hal_i2c_master.h"
#include "hal_gpio.h"
#include "osal.h"
#include "drv_uc1510c.h"
#include "iGeneralDefinitions.h"

extern HAL_GPIO_RESULT_ENUM LCD_RESET_RSTB(int level);
/*****************************************************************************
 * Local defines
 *****************************************************************************/

static uc510c_dev uc1510c_dev_inst;

#define UC1510C_TOUCH_PANEL_DETECT_TIME_MS       ((uint8_t)      10u)  //!< Time for Touch panel detection in milliseconds
#define UC1510C_DISPLAY_CLOCK_KHZ                ((float)     13.888) 
#define CALC_TP_TIME                             ((uint8_t)(UC1510C_TOUCH_PANEL_DETECT_TIME_MS * UC1510C_DISPLAY_CLOCK_KHZ))

#define UC1510C_TOUCH_PANEL_COM_SELECTION        ((uint8_t)      2u)  //!< default value 4

#define ENABLE_DRV_UC1510C_DEBUG_LOGS             0u

//!< UC1510C Mode control Register Configuration
static MODE_SET mode_set_reg = 
{   
    .D7_PWR_SAV = OSCILLATOR_ENABLE, 
    .D6_UNUSED 	= 0, 
	.D5_I 		= IP_BANK_RAM_BIT0_AND_BIT1, 
    .D4_O 		= OP_BANK_RAM_BIT0_AND_BIT1, 
    .D3_E 		= DISP_STATUS_ENABLED, 
    .D2_B 		= ONE_BY_TWO_BIAS, 
    .D0_D1_M 	= STATIC_BP0
};

//!< UC1510C Frame control Register Configuration
static FRAME_CONTROL frame_control = 
{
    .D7_RPT_IRQ_FLAG    = 0,
    .D6_RPT_FN_KEY      = FUNCTION_KEY_ENABLED,
    .D4_D5_RPT_GESTURE  = SINGLE_BUTTON_TOUCH,
    .D3_UNUSED          = 0,
    .D2_UNUSED          = 0,
    .D1_UNUSED          = 0,
    .D0_UNUSED          = 0
};

//!< UC1510C Touch Panel Time Configuration
static TP_TIME tp_time = 
{
    .D0_TO_D7_TP_TIME = 0x06,
};

//!< UC1510C Touch COM Panel Configuration
TP_COM tp_com = 
{
    .D0_TO_D7_TP_COM = UC1510C_TOUCH_PANEL_COM_SELECTION,
};

//!< Array list with UC1510C Regiter configurations
static UC1510C_REG uc1510c_reg[] =
{
    {.addr = MODE_SET_REG_ADDR  ,    .value = &mode_set_reg.byte    },
    {.addr = FRAME_CTRL_REG_ADDR,    .value = &frame_control.byte   },
    {.addr = TP_TIME_REG_ADDR   ,    .value = &tp_time.byte         },
    {.addr = TP_COM_REG_ADDR    ,    .value = &tp_com.byte          }
};


#ifdef LCD_DRIVER_IC_INIT_VERSION_200623

uint16_t  lcd_init_len = 195;

static const uint8_t InitTable[390] = {   
0x00,  0xB9,  0x00,  0xB9,  0x29,  0x02,  0x17,  0x7F,  0x1A,  0x04,
0x1A,  0x04,  0x17,  0x00,  0x1F,  0x0B,  0x1F,  0x07,  0x1F,  0x0E,
0x1F,  0x0D,  0x33,  0x00,  0x38,  0x00,  0x30,  0x80,  0x07,  0x24,
0x07,  0x24,  0x02,  0x4D,  0x16,  0x01,  0x15,  0x28,  0x15,  0x28,
0x05,  0x03,  0x34,  0x00,  0x35,  0x06,  0x38,  0x00,
0x28,  0x00,  0x09,  0x80,  0x0A,  0x0F,  0x0A,  0x10,  0x0A,  0x11,
0x0A,  0x00,  0x0A,  0x00,  0x0A,  0x00,  0x0A,  0x00,  0x0A,  0x00,
0x0A,  0x00,  0x0A,  0x00,  0x0A,  0x00,  0x0A,  0x00,  0x0A,  0x00,
0x0A,  0x00,  0x0A,  0x00,  0x0A,  0x00,  0x09,  0x00,
0x24,  0x10,  0x09,  0x80,  0x25,  0x10,  0x25,  0x10,  0x25,  0x10,
0x25,  0x10,  0x25,  0x10,  0x25,  0x10,  0x25,  0x10,  0x25,  0x10,
0x25,  0x10,  0x25,  0x10,  0x25,  0x10,  0x25,  0x10,  0x25,  0x10,
0x25,  0x10,  0x25,  0x10,  0x25,  0x10,  0x09,  0x00,
0x24,  0x20,  0x09,  0x80,  0x25,  0x64,  0x25,  0x46,  0x25,  0x46,
0x25,  0x46,  0x25,  0x46,  0x25,  0x3C,  0x25,  0x46,  0x25,  0x46,
0x25,  0x46,  0x25,  0x46,  0x25,  0x46,  0x25,  0x46,  0x25,  0x46,
0x25,  0x46,  0x25,  0x46,  0x25,  0x46,  0x09,  0x00,
0x11,  0x00,  0x09,  0x80,  0x12,  0x1F,  0x12,  0x1D,  0x12,  0x1D,
0x12,  0x16,  0x12,  0x14,  0x12,  0x14,  0x12,  0x16,  0x12,  0x16,
0x12,  0x16,  0x12,  0x16,  0x12,  0x16,  0x12,  0x16,  0x12,  0x16,
0x12,  0x14,  0x12,  0x14,  0x12,  0x14,  0x09,  0x00,
0x13,  0x00,  0x09,  0x80,  0x14,  0x14,  0x14,  0x14,  0x14,  0x14,
0x14,  0x14,  0x14,  0x14,  0x14,  0x14,  0x14,  0x14,  0x14,  0x14,
0x14,  0x14,  0x14,  0x14,  0x14,  0x14,  0x14,  0x14,  0x14,  0x0A,
0x14,  0x0A,  0x14,  0x0A,  0x14,  0x14,  0x09,  0x00,
0x18,  0x00,  0x09,  0x80,  0x19,  0x08,  0x19,  0x08,  0x19,  0x08,
0x19,  0x0C,  0x19,  0x0C,  0x19,  0x0C,  0x19,  0x0C,  0x19,  0x0C,
0x19,  0x0C,  0x19,  0x0C,  0x19,  0x0C,  0x19,  0x0C,  0x19,  0x0C,
0x19,  0x0C,  0x19,  0x0C,  0x19,  0x0C,  0x09,  0x00,
0x30,  0x89,  0x04,  0xD3,  0x36,  0x03,  0x37,  0x06,  0x39,  0x24,
0x3A,  0x22,  0x3B,  0x02,  0x3C,  0x43,  0x3E,  0x00,  0x3F,  0x00,
0x3E,  0x01,  0x3F,  0x00,  0x01,  0x00,  0x03,  0x02,  0x06,  0x30,
0x08,  0x02,  0x0B,  0x08,  0x0C,  0x00,  0x0D,  0xB0,  0x0E,  0xE0,
0x0F,  0x00,
0x1A,  0x04,  0x09,  0x80,  0x22,  0x00,  0x23,  0x00,  0x23,  0x00,
0x23,  0x00,  0x23,  0x00,  0x23,  0x04,  0x23,  0x08,  0x23,  0x08,
0x23,  0x01,  0x23,  0x10,  0x23,  0x06,  0x23,  0x33,  0x23,  0x74,
0x23,  0x00,  0x23,  0x00,  0x23,  0x04,  0x23,  0x00,  0x23,  0x00,
0x23,  0x00,  0x23,  0x24,  0x23,  0x00,  0x09,  0x00,
0x1C,  0x00,  0x1D,  0x00,  0x20,  0x27,  0x26,  0x41,  0x27,  0x02,
0x28,  0x00,  0x2A,  0x0C,  0x2B,  0x70,  0x2E,  0x03,  0x21,  0x24,
0x3D,  0x71,  0x29,  0x01
};
#endif


/*! \brief Initialize UC1510C LCD Driver IC
 *
 *  \param bus_conf Pointer to bus configuration data.
 *
 *  \return 0 if successful. Any other value otherwise.
 */
error_type init_uc1510c(const uc_drv_bus_conf* const bus_conf)
{
    error_type result = 0;
    uint16_t   index  = 0;
    uint8_t    data   = 0;
    uint8_t    addr   = 0;

    /* After Wait for atleast 10 msec before set high on RSTB Line */
    osal_task_delay(1);

    /* Set RSTB line to high */
    LCD_RESET_RSTB(1);

    /* Wait for atleast 1 micro second */
    osal_task_delay(1);

    /* Set the device ID */
    uc1510c_dev_inst.dev_id = bus_conf->i2c.port;
    /* Set the slave address */
    uc1510c_dev_inst.slave_address = UC1510C_DRIVER_IC_SECONDARY_ADDR;

    if ( STATIC_BP0 == mode_set_reg.D0_D1_M )
    {
        uc1510c_dev_inst.back_plane = 0;
    }
    
    for ( index = 0; index < ARR_ELEMENTS(uc1510c_reg); index++ )
    {
#if ENABLE_DRV_UC1510C_DEBUG_LOGS
        LOG(I, "Reg Addr = 0x%x Reg Value = 0x%x", uc1510c_reg[index].addr, *uc1510c_reg[index].value);
#endif
        result = uc1510c_write_reg(uc1510c_reg[index].addr, uc1510c_reg[index].value, UC1510C_REGISTER_SIZE_IN_BYTES);
    }

    /* Configure the LCD registers */
    for ( index = 0; index < 195; index++ )
    {
        addr = InitTable[index * 2];
        data = InitTable[(index * 2) + 1];
		uc1510c_write_reg(addr, &data, UC1510C_REGISTER_SIZE_IN_BYTES);
    }

    addr = DEVICE_SELECT_REG_ADDR;
    data = 0x00;
    uc1510c_write_reg(addr, &data, UC1510C_REGISTER_SIZE_IN_BYTES);

    addr = TP_TIME_REG_ADDR;  // Touch panel detection time
    data = TP_TIME_REG_VAL;
    uc1510c_write_reg(addr, &data, UC1510C_REGISTER_SIZE_IN_BYTES);

    addr = ANA_REG5_ADDR;  // Touch panel Cap sense gain
    data = REG5_CAP_GAIN_1;
    uc1510c_write_reg(addr, &data, UC1510C_REGISTER_SIZE_IN_BYTES);
    
    osal_task_delay(100);

    return result;
}

/*! \brief Function to set the segment in the 
 *
 *  \param segment_num Pointer to bus configuration data.
 *  \param enable_segement Pointer to bus configuration data.
 *
 *  \return 0 if successful. Any other value otherwise.
 */
error_type  uc1510c_set_segment(uint8_t segment_num, bool enable_segement)
{
    uint8_t    data_pointer_value = 0u;
    uint8_t    addr = 0;
    uint8_t    ram_bit = 0;
    error_type result = RES_FAIL;
    RAM_DATA   ram_data;
    DATA_POINT data_point;

    /* Find the data pointer value from the segment number */
    addr               = segment_num / UC1510C_RAM_DATA_MAX_NUM_BITS;
    data_pointer_value = addr * UC1510C_RAM_DATA_MAX_NUM_BITS;
    
    /* Calculate the RAM bit address index */
    ram_bit = segment_num % UC1510C_RAM_DATA_MAX_NUM_BITS;

    /* 
    UC1510C RAM filling order is given below
    RAM Bit position     | 0     1    2   3   4   5      6       7
    RAM Filling Order    | 7     6    5   4   3   2      1       0     (Data)
                            n   n+1  n+2  n+3 n+4 n+5     n+6     n+7    
                                                    SEG2    SEG1    SEG0  (Segment)                  

    It means the MSB will get stored at 0th bit and so on.
    To acheive this bit position, the calculated ram_bit should be reversed
    */
    ram_bit = (UC1510C_RAM_DATA_MAX_NUM_BITS - 1) - ram_bit; 

    /* Fill the data pointer structure */
    data_point.D0_TO_D5_DATA_POINT = data_pointer_value & DATA_POINT_MASK;
    data_point.D6_RD_MEM_EN        = WRITE_MEMORY_ENABLE;

#if ENABLE_DRV_UC1510C_DEBUG_LOGS
    LOG(I, "addr = %d segment_num = %d, data_pointer_value = %d, ram_bit = %d, data_point = 0x%x", 
        addr, segment_num, data_pointer_value, ram_bit, data_point.byte);
#endif

    /* Send the data pointer value via I2C to UC1510C controller */
    result = uc1510c_write_reg(DATA_POINT_REG_ADDR, &data_point.byte, UC1510C_REGISTER_SIZE_IN_BYTES);

    if ( RES_FAIL != result )
    {
        if ( FALSE == enable_segement )
        {
            /* Clear the RAM bit data segment */
            uc1510c_dev_inst.ram_data[uc1510c_dev_inst.back_plane][addr] &= ~( 1 << ram_bit );
        }
        else
        {
            /* Set the RAM bit data segment */
            uc1510c_dev_inst.ram_data[uc1510c_dev_inst.back_plane][addr] |= ( 1 << ram_bit );
        }

        /* Set the RAM data in the register */
        ram_data.D0_TO_D7_RAM_DATA = uc1510c_dev_inst.ram_data[uc1510c_dev_inst.back_plane][addr];

#if ENABLE_DRV_UC1510C_DEBUG_LOGS
        LOG(I, "RAM_DATA = 0x%x ram_data.byte = 0x%x", uc1510c_dev_inst.ram_data[uc1510c_dev_inst.back_plane][addr], ram_data.byte);
#endif

        /* Send the segment num to the LCD RAM register */
        result = uc1510c_write_reg(RAM_DATA_REG_ADDR, &ram_data.byte, UC1510C_REGISTER_SIZE_IN_BYTES);
    }
    else
    {
        LOG(E, "I2C Bus acces error");
    }

    return result;
}

/*! \brief Function to get touch panel status
 *
 *  \param touch_status Pointer to store the read touch panel status
 *
 *  \return 0 if successful. Any other value otherwise.
 */
error_type uc1510c_get_touch_panel_status(uint8_t* touch_status)
{
    /* Read the report register */
    error_type result = uc1510c_read_reg((REPORT_REG_ADDR), (uint8_t* const)touch_status, UC1510C_REGISTER_SIZE_IN_BYTES);

	return result;
}

/*! \brief  This function provides write functionality to the LCD Driver IC UC1510C.
 *
 *
 *  \param reg_addr	Register address to write the value to.
 *  \param reg_data	Register data to write to the reg_addr.
 *  \param len      Length of bytes to be written.
 *  \param dev_id   Device ID
 *
 *  \return 0 if successful. Any other value otherwise.
 */
error_type uc1510c_write_reg(uint8_t reg_addr, const uint8_t *data, uint16_t len)
{
    error_type result;
    uint8_t txbuf[10] = { 0 };
    
    result = hal_i2c_master_acquire(uc1510c_dev_inst.dev_id);
    if (result == HAL_E_OK)
    {
        if ( len <  ( ARR_ELEMENTS( txbuf ) - 1) )
        {
            txbuf[0] = reg_addr;
            memcpy(&txbuf[1], data, len);
            result = hal_i2c_master_write(uc1510c_dev_inst.dev_id, uc1510c_dev_inst.slave_address, txbuf, len + 1);
        }
        else
        {
            result = RES_FAIL;   // Data to long for transfer buffer.
        }
        hal_i2c_master_release(uc1510c_dev_inst.dev_id);
    }

    return result;
}

/*! \brief  This function provides read functionality to the LCD Driver IC UC1510C.
 *
 *
 *  \param reg_addr	Register address to read the value from.
 *  \param reg_data	pointer to the data which has been read from sensor.
 *  \param len      Length of bytes to be read.
 *  \param dev_id   Device ID
 *
 *  \return 0 if successful. Any other value otherwise.
 */
error_type uc1510c_read_reg(uint8_t reg_addr, uint8_t *const data, uint16_t len)
{   
    error_type result;

    result = hal_i2c_master_acquire(uc1510c_dev_inst.dev_id);
    if (result == HAL_E_OK)
    {
        result = hal_i2c_master_writeread(uc1510c_dev_inst.dev_id, uc1510c_dev_inst.slave_address , &reg_addr, UC1510C_REGISTER_SIZE_IN_BYTES, data, len);

        hal_i2c_master_release(uc1510c_dev_inst.dev_id);
    }

    return result;
}

#endif
