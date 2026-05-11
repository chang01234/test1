 /******************************************************************************
 * \file       drv_uc1510c.h
 * \brief      UC1510C Driver Header File.
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
#ifndef DRV_UC1510C_H_
#define DRV_UC1510C_H_

#include <stdint.h>
#include <stdbool.h>

typedef int32_t error_type;

#define RES_PASS   0
#define RES_FAIL  -1
#define INTERRUPT_MODE                        0
#define POLLING_MODE                          1
#define UC1510C_TOUCH_PANEL_READ_MODE       POLLING_MODE

#define STATIC_MODE                           0
#define MULTIPLEX_1_2_MODE                    1
#define MULTIPLEX_1_3_MODE                    2
#define MULTIPLEX_1_4_MODE                    3

#define LCD_MODE                              STATIC_MODE

#if   ( LCD_MODE == STATIC_MODE )
#define UC1510C_NUM_BACKPLANES              ((uint8_t)  1u)
#elif ( LCD_MODE == MULTIPLEX_1_2_MODE )
#define UC1510C_NUM_BACKPLANES              ((uint8_t)  2u)
#elif ( LCD_MODE == MULTIPLEX_1_3_MODE )
#define UC1510C_NUM_BACKPLANES              ((uint8_t)  3u)
#elif ( LCD_MODE == MULTIPLEX_1_4_MODE )
#define UC1510C_NUM_BACKPLANES              ((uint8_t)  4u)
#endif

#define UC1510C_RAM_SIZE                    ((uint8_t)  3u)
#define UC1510C_RAM_DATA_MAX_NUM_BITS       ((uint8_t)  8u)
#define ARR_ELEMENTS(x)					    (sizeof(x) / sizeof(x[0]))	//!< \~ Element count of array
#define UC1510C_REGISTER_SIZE_IN_BYTES      ((uint16_t)  1u)
#define UC1510C_TWO_BYTES_DATA              ((uint16_t)  2u)
#define UC1510C_MAX_SET_SEGEMENT            ((uint8_t)   8u)
#define DATA_POINT_MASK                     ((uint8_t) 0x3Fu)
#define READ_MEMORY_ENABLE                  ((uint8_t) 0x01u)
#define WRITE_MEMORY_ENABLE                 ((uint8_t) 0x00u)
#define TP_TIME_REG_VAL                     ((uint8_t) 0x06)    //Default value set is 0x06
#define REG5_CAP_GAIN_1                     ((uint8_t) 0x00)
#define REG5_CAP_GAIN_1B2                   ((uint8_t) 0x01)
#define REG5_CAP_GAIN_1B3                   ((uint8_t) 0x01)
#define REG5_CAP_GAIN_1B4                   ((uint8_t) 0x03)
#define REG5_CAP_GAIN_1B5                   ((uint8_t) 0x04)



//!< enum holding address's of UC1510C registers
typedef enum __uc1510c_regaddr
{
    MODE_SET_REG_ADDR       = 0x30,
    DATA_POINT_REG_ADDR     = 0x31,
    RAM_DATA_REG_ADDR       = 0x32,
    DEVICE_SELECT_REG_ADDR  = 0x33,
    BLINK_SELECT_REG_ADDR   = 0x34,
    CLK_DIVIDER_REG_ADDR    = 0x35,
    TP_COM_REG_ADDR         = 0x36,
    TP_TIME_REG_ADDR        = 0x37,
    CLK_CONTROL_REG_ADDR    = 0x38,
    FRAME_CTRL_REG_ADDR     = 0x29,
    REPORT_REG_ADDR         = 0x2F,
    ANA_REG5_ADDR           = 0x06
}UC1510C_REG_ADDR;

//!< enum for bus configuration type
typedef enum uc_drv_bus_conf_type
{
    UC_SEQ_BUS_CONF_TYPE_I2C = 0,
    UC_SEQ_BUS_CONF_TYPE_SPI
} uc_drv_bus_conf_type;

//!< struct type for I2C bus configration
typedef struct i2c_conf
{
    uint8_t  port;
    uint32_t sda;
    uint32_t scl;
    uint32_t bitrate;
} i2c_conf;

//!< struct type for SPI bus configration
typedef struct spi_conf
{
    uint8_t  port;
    uint32_t miso;
    uint32_t mosi;
    uint32_t cs;
    uint32_t bitrate;
} spi_conf;

//!< struct type for driver bus configration
typedef struct uc_drv_bus_conf
{
    uc_drv_bus_conf_type type;
    union 
    {
        i2c_conf i2c;
        spi_conf spi;
    };
} uc_drv_bus_conf;

/*!
 * @brief UC1510C device structure
 */
typedef struct	__uc510c_dev 
{
	/*! Chip Id */
	uint8_t chip_id;
	/*! Device Id */
	uint8_t dev_id;
    /*! slave address */
	uint8_t slave_address;
    /*! ram data 0 to 39 segment */
    uint8_t ram_data[UC1510C_NUM_BACKPLANES][UC1510C_RAM_SIZE];
    /*! ram data 0 to 39 segment */
    uint8_t rd_ram_data[UC1510C_NUM_BACKPLANES][UC1510C_RAM_SIZE];
    /*! backplane configuration */
	uint8_t back_plane;
    /*! RAM Bit counter */
    uint8_t ram_bit_counter;
}uc510c_dev;

//!< Structure to initiaize the UC1510C driver 
typedef struct __uc1510c_reg
{
    UC1510C_REG_ADDR     addr;
    uint8_t*            value;
}UC1510C_REG;

//!< enum for LCD driver mode selection
typedef enum __mode_set_lcd_drive_mode 
{
    RATIO_1_4_MULTIPLEX_BP0_BP1_BP2_BP3 = 0x0,
    STATIC_BP0                          = 0x1,
    RATIO_1_2_MULTIPLEX_BP0_BP1         = 0x2,
    RATIO_1_3_MULTIPLEX_BP0_BP1_BP2     = 0x3,
}MODE_SET_LCD_DRIVE_MODE;

//!< enum for mode setting bias configuration
typedef enum __mode_set_bias_config 
{
    ONE_BY_THREE_BIAS = 0,
    ONE_BY_TWO_BIAS   = 1
}MODE_SET_BIAS_CONFIG;

//!< enum for mode setting display status
typedef enum __mode_set_display_status
{
    DISP_STATUS_DISABLED = 0,
    DISP_STATUS_ENABLED  = 1
}MODE_SET_DISPLAY_STATUS;

//!< enum for mode setting output bank selection
typedef enum __mode_set_output_bank_sel
{
    OP_BANK_RAM_BIT0_AND_BIT1  = 0,
    OP_BANK_RAM_BIT2_AND_BIT3  = 1
}MODE_SET_OUTPUT_BANK_SEL;

//!< enum for mode setting input bank selection
typedef enum __mode_set_input_bank_sel
{
    IP_BANK_RAM_BIT0_AND_BIT1  = 0,
    IP_BANK_RAM_BIT2_AND_BIT3  = 1
}MODE_SET_INPUT_BANK_SEL;

//!< enum for mode setting oscilator selection for power saving
typedef enum __mode_set_power_save
{
    OSCILLATOR_ENABLE   = 0,
    OSCILLATOR_DISABLE  = 1
}MODE_SET_POWER_SAVE;

//!< enum for data point read/write memory enable
typedef enum __data_point_rd_mem_enable
{
    WRITE_DATA_POINT   = 0,
    READ_DATA_POINT    = 1
}DATA_POINT_READ_MEM_ENABLE;

//!< enum for device sync selection
typedef enum __device_sync_selection
{
    SYNC_DIABLED   = 0,
    SYNC_WITH_CASCADED_DEVCIE = 1
}DEVICE_SYNC_SELECTION;

//!< enum for blink mode selection
typedef enum __blink_mode_selection
{
    NORMAL_BLINKING        = 0,
    ALTERNATE_RAM_BLINKING = 1
}BLINK_MODE_SLECTION;

//!< enum for blink frequency selection
typedef enum __blink_freq_selection
{
    FREQ_SELC_OFF = 0,
    FREQ_SELC_1   = 1,
    FREQ_SELC_2   = 2,
    FREQ_SELC_3   = 3
}BLINK_FREQ_SLECTION;

//!< enum for source clock bypass
typedef enum __bypass_src_clk
{
    DIS_CLK_SELECTED_BY_0X35H_REG = 0,
    DIS_CLK_FROM_CLK_PORT         = 1,
}BYPASS_SRC_CLK;

//!< enum for reporting IRQ flag
typedef enum __frame_ctrl_rpt_irq_flag
{
    NO_IRQ          = 0,
    REPORT_READY    = 1,
}FRAME_CTRL_RPT_IRQ_FLAG;

typedef enum __frame_ctrl_rpt_function_key
{
    FUNCTION_KEY_DISABLED   = 0,
    FUNCTION_KEY_ENABLED    = 1,
}FRAME_CTRL_RPT_FUNCTION_KEY;

typedef enum __frame_ctrl_rpt_gesture
{
    SINGLE_BUTTON_TOUCH       = 0,
    GESTURE_TOUCH_LEFT        = 1,
    GESTURE_TOUCH_RIGHT       = 2,
    GESTURE_TOUCH_OTHER_CASES = 3,
}FRAME_CTRL_RPT_GESTURE;

typedef union
{
	uint8_t byte;
	struct 
	{
		uint8_t D0_D1_M     : 2;
        uint8_t D2_B        : 1;
        uint8_t D3_E        : 1;
        uint8_t D4_O        : 1;
        uint8_t D5_I        : 1;
        uint8_t D6_UNUSED   : 1;
        uint8_t D7_PWR_SAV  : 1;
	} __attribute__((packed));
} MODE_SET;

typedef union
{
	uint8_t byte;
	struct 
	{
		uint8_t D0_TO_D5_DATA_POINT     : 6;
        uint8_t D6_RD_MEM_EN            : 1;
        uint8_t D7_UNUSED               : 1;
	} __attribute__((packed));
} DATA_POINT;

typedef union
{
	uint8_t byte;
	struct 
	{
		uint8_t D0_TO_D7_RAM_DATA : 8;
	} __attribute__((packed));
} RAM_DATA;

typedef union
{
	uint8_t byte;
	struct 
	{
		uint8_t D0_TO_D2_DS     : 3;
        uint8_t D3_UNUSED       : 1;
        uint8_t D4_S_SEL        : 1;
        uint8_t D5_UNUSED       : 1;
        uint8_t D6_UNUSED       : 1;
        uint8_t D7_UNUSED       : 1;
	} __attribute__((packed));
} DEVICE_SELECT;

typedef union
{
	uint8_t byte;
	struct 
	{
		uint8_t D0_D1_BF     : 2;
        uint8_t D2_BS        : 1;
        uint8_t D3_UNUSED    : 1;
        uint8_t D4_UNUSED    : 1;
        uint8_t D5_UNUSED    : 1;
        uint8_t D6_UNUSED    : 1;
        uint8_t D7_UNUSED    : 1;
	} __attribute__((packed));
} BLINK_SELECT;

typedef union
{
	uint8_t byte;
	struct 
	{
		uint8_t D0_TO_D5_CLK_SEL    : 6;
        uint8_t D6_UNUSED           : 1;
        uint8_t D7_UNUSED           : 1;
	} __attribute__((packed));
} CLK_DIVIDER;

typedef union
{
	uint8_t byte;
	struct 
	{
		uint8_t D0_TO_D7_TP_COM   : 8;
	} __attribute__((packed));
} TP_COM;

typedef union
{
	uint8_t byte;
	struct 
	{
		uint8_t D0_TO_D7_TP_TIME   : 8;
	} __attribute__((packed));
} TP_TIME;

typedef union
{
	uint8_t byte;
	struct 
	{
		uint8_t D0_DISABLE_SRC_CLK   : 1;
        uint8_t D1_BYPASS_SRC_CLK    : 1;
        uint8_t D2_UNUSED            : 1;
        uint8_t D3_UNUSED            : 1;
        uint8_t D4_UNUSED            : 1;
        uint8_t D5_UNUSED            : 1;
        uint8_t D6_UNUSED            : 1;
        uint8_t D7_UNUSED            : 1;
	} __attribute__((packed));
} CLK_CONTROL;

typedef union
{
	uint8_t byte;
	struct 
	{
        uint8_t D0_UNUSED            : 1;
        uint8_t D1_UNUSED            : 1;
        uint8_t D2_UNUSED            : 1;
        uint8_t D3_UNUSED            : 1;
        uint8_t D4_D5_RPT_GESTURE    : 2;
        uint8_t D6_RPT_FN_KEY        : 1;
        uint8_t D7_RPT_IRQ_FLAG      : 1;
	} __attribute__((packed));
} FRAME_CONTROL;

typedef union
{
	uint8_t byte;
	struct 
	{
        uint8_t D0_TO_D5_RPT_CH      : 6;
        uint8_t D6_UNUSED            : 1;
        uint8_t D7_UNUSED            : 1;
	} __attribute__((packed));
} REPORT;

error_type init_uc1510c(const uc_drv_bus_conf* const bus_conf);

error_type uc1510c_set_segment(uint8_t segment_num, bool enable_segement);

error_type uc1510c_get_touch_panel_status(uint8_t* touch_status);

error_type uc1510c_write_reg(uint8_t reg_addr, const uint8_t *data, uint16_t len);

error_type uc1510c_read_reg(uint8_t reg_addr, uint8_t *const data, uint16_t len);

#if 0
error_type uc1510c_set_segment_blink(uint8_t segment_num, bool enable_segement);
#endif

#endif /* DRV_UC1510C_H_ */
