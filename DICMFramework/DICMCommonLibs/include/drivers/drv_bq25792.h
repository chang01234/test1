/*****************************************************************************
 * \file       drv_bq25792.h
 * \brief      PMIC Driver Header File
 *
 *****************************************************************************/
#ifndef DRV_BQ25792_H_
#define DRV_BQ25792_H_

#include <stdint.h>
#include <stdbool.h>

#define BQ25792_MANUFACTURER		            "Texas Instruments"
#define BQ25792_CHIP_NAME			            "BQ25792"
#define BQ25792_PART_NUMBER                     1u
#define DEV_PMIC_BQ25792_I2C_SLAVE_ADDR         0x6B

#define ARR_ELEMENTS(x)					        (sizeof(x) / sizeof(x[0]))	//!< \~ Element count of array
#define BQ25792_REGISTER_SIZE_IN_BYTES          1u

#define BQ25792_REG18_TS_COOL_BIT_CLEAR         0xC0
#define BQ25792_REG18_TS_WARM_BIT_CLEAR         0x30

#define BQ25792_REG18_TS_COOL_71_1              0x00
#define BQ25792_REG18_TS_COOL_68_4              0x40
#define BQ25792_REG18_TS_COOL_65_5              0x80
#define BQ25792_REG18_TS_COOL_62_4              0xC0

#define BQ25792_REG18_TS_WARM_48_4              0x00
#define BQ25792_REG18_TS_WARM_44_8              0x10
#define BQ25792_REG18_TS_WARM_41_2              0x20
#define BQ25792_REG18_TS_WARM_37_7              0x30



#define BQ25792_REG16_TREG_BIT_CLEAR           0xC0
#define BQ25792_REG16_TSHUT_BIT_CLEAR          0x30 
#define BQ25792_REG16_TSHUT_85C                0x30
#define BQ25792_REG16_TREG_65C                 0x00
#define CURRENT_LIMIT_REG_BIT_STEP_SIZE_MILLI_AMP   10
#define BQ25792_BATTERY_CHARGE_CURRENT              50      // 50       *      10        +    0       =  500 mA
#define BQ25792_BATTERY_LIMITED_CURRENT             10      // 10       *      10        +    0       =  100 mA
#define VRECHG_FIXED_OFFSET_MILLIVOLT               50
#define TS_BIT_STEP_SIZE                            ((float)  0.0976563f)
#define TDIE_BIT_STEP_SIZE                          ((float)        0.5f)
#define ONE_BYTE                                    ((uint16_t)  1u)
#define TWO_BYTE                                    ((uint16_t)  2u)
#define MINIMUM_SYS_VOLT_REG_OFFSET_MV              2500
#define MINIMUM_SYS_VOLT_REG_BIT_STEP_SIZE_MV       250

/* FAULT STATUS 0 */
#define VAC1_FAULT_STAT_DEV_IN_OVP                       1 
#define VAC2_FAULT_STAT_DEV_IN_OVP                       1 
#define CONV_FAULT_STAT_DEV_IN_OCP                       1
#define IBAT_FAULT_STAT_DEV_IN_OCP                       1
#define IBUS_FAULT_STAT_DEV_IN_OCP                       1
#define VBAT_FAULT_STAT_DEV_IN_OVP                       1
#define VBUS_FAULT_STAT_DEV_IN_OVP                       1
#define IBAT_FAULT_STAT_DEV_IN_BAT_DISCHARG_CR           1

/* FAULT STATUS 1 */
#define TSHUT_FAULT_STAT_DEV_IN_THER_SHUTDOWN_PROTEC     1
#define OTG_FAULT_STAT_DEV_OTG_UNDER_VOLTAGE             1
#define OTG_FAULT_STAT_DEV_OTG_OVER_VOLTAGE              1
#define VSYS_FAULT_STAT_DEV_IN_SYS_OVP                   1
#define VSYS_FAULT_STAT_DEV_IN_SYS_SHORT_CIRCUIT_PROTEC  1

/* FAULT FLAG 0 */
#define FAULT_FLAG0_VAC1_ENTER_OVP                       1
#define FAULT_FLAG0_VAC2_ENTER_OVP                       1
#define FAULT_FLAG0_CONV_ENTER_OCP                       1
#define FAULT_FLAG0_IBAT_ENTER_DISCHARGED_OCP            1
#define FAULT_FLAG0_IBUS_ENTER_OCP                       1
#define FAULT_FLAG0_VBAT_ENTER_OVP                       1
#define FAULT_FLAG0_VBUS_ENTER_OVP                       1
#define FAULT_FLAG0_IBAT_ENTER_OR_EXIT_REGULATION        1

/* FAULT FLAG 1 */
#define FAULT_FLAG1_TSHUT                                        1
#define FAULT_FLAG1_STOP_OTG_DUE_TO_VBUS_UNDER_VOLTAGE           1
#define FAULT_FLAG1_STOP_OTG_DUE_TO_VBUS_OVER_VOLTAGE            1
#define FAULT_FLAG1_VSYS_STOP_SWITCH_DUE_TO_SYS_OVER_VOLTAGE     1
#define FAULT_FLAG1_STOP_SWITCH_DUE_TO_SYS_SHORT                 1


#define MINIMUM_SYSTEM_VOLTAGE_VALUE_5v     10          // The voltage diveder value need to be confirmed by hardware team.

                                                        // Reg Value | Bit Step Size  | Fixed Offset 
#define MINIMUM_SYSTEM_VOLTAGE_VALUE        37          //  37       *      250       +   2500     = 9000 + 2500 = 11750 (11.75 Volt)

//for testing
//#define MINIMUM_SYSTEM_VOLTAGE_VALUE      MINIMUM_SYSTEM_VOLTAGE_VALUE_5v 


#define CHARGE_VOLTAGE_LIMIT_VALUE          1380        // 1380      *      10        +    0       =  13800 (13.8 Volt)
#define CHARGE_CURRENT_LIMIT_VALUE          50          // 50       *      10        +    0       =  500 mA
#define INPUT_VOLTAGE_LIMIT_VALUE           100         // 100       *      100       +    0       =  10000mv = 10 Volt
#define IIDPM_CURRENT_LIMIT_VALUE           20

#define CHARGE_CURRENT_500MA                50          // 50       *      10        +    0       =  500 mA
#define CHARGE_CURRENT_300MA                30          // 30       *      10        +    0       =  300 mA
#define CHARGE_CURRENT_200MA                20          // 20       *      10        +    0       =  200 mA
#define CHARGE_CURRENT_100MA                10          // 10       *      10        +    0       =  100 mA

#define SOLAR_VOLT_14                       14
#define SOLAR_VOLT_12                       12
#define SOLAR_VOLT_10                       10
#define SOLAR_VOLT_08                       8
#define SOLAR_VOLT_06                       6


#define RES_PASS                0
#define RES_FAIL               -1

#define BIT0_MASK            0x01
#define BIT1_MASK            0x02
#define BIT2_MASK            0x04
#define BIT3_MASK            0x08
#define BIT4_MASK            0x10
#define BIT5_MASK            0x20
#define BIT6_MASK            0x40
#define BIT7_MASK            0x80
#define BIT1_AND_BIT2_MASK   0x06

#define IS_VAC1_AVAIL(x)                 ( x & BIT1_MASK )
#define IS_VAC2_AVAIL(x)                 ( x & BIT2_MASK )
#define IS_VALID_BAT_PWR_AVAIL(x)        ( x & BIT0_MASK )
#define IS_VAC1_VAC2_STAT_CHANGED(x)     ( x & BIT1_AND_BIT2_MASK )

#define BATT_CH_STATE_IC_INIT             0
#define BATT_CH_STATE_IC_INIT_DONE        1

typedef int32_t error_type;

//!< enum for bus configuration type
typedef enum drv_bus_conf_type
{
    SEQ_BUS_CONF_TYPE_I2C = 0,
    SEQ_BUS_CONF_TYPE_SPI
} drv_bus_conf_type;

//!< enum for VAC Over voltage protection thresholds
typedef enum __vac_ovp_thresholds
{
    THRESHOLD_VOLTAGE_26V = 0,
    THRESHOLD_VOLTAGE_18V = 1,
    THRESHOLD_VOLTAGE_12V = 2,
    THRESHOLD_VOLTAGE_7V  = 3
}vac_ovp_thresholds;

//!< enum for watchdog timer reset
typedef enum __wd_timer_reset
{
    WD_TIMER_NORMAL_DEF = 0,
    WD_TIMER_RESET      = 1
}wd_timer_reset;

//!< enum for enable/disable termination
typedef enum __fast_charge_tmr_setting
{
    FIVE_HOURS          = 0,
    EIGHT_HOURS         = 1,
    TWELVE_HOURS        = 2,
    TWENTY_FOUR_HOURS   = 3
}fast_charge_tmr_setting;

typedef enum __cell_config
{
    SINGLE_CELL = 0,
    TWO_CELL    = 1,
    THREE_CELL  = 2,
    FOUR_CELL   = 3
}cell_config;

typedef enum __charger_enable_config
{
    CHARGER_DISABLE     = 0,
    CHARGER_ENABLE      = 1
}charger_enable_config;

typedef enum __hiz_mode
{
    HIZ_MODE_DISABLE     = 0,
    HIZ_MODE_ENABLE      = 1
}hiz_mode;

typedef enum __wd_reset_time
{
    WD_DISABLE_TIME        = 0,
    WD_RST_TIME_500_MSEC   = 1,
    WD_RST_TIME_1SEC       = 2,
    WD_RST_TIME_2SEC       = 3,
    WD_RST_TIME_20SEC      = 4,
    WD_RST_TIME_40SEC      = 5,
    WD_RST_TIME_80SEC      = 6,
    WD_RST_TIME_160SEC     = 7
}wd_reset_time;

typedef enum __adc_control
{
    DISABLE = 0,
    ENABLE  = 1
}adc_control;

typedef enum __adc_ch_en_dis
{
    ADC_CH_ENABLE  = 0,
    ADC_CH_DISABLE = 1
}adc_ch_en_dis;

typedef enum __adc_conv_rate_ctrl
{
    CONT_CONVERSION      = 0,
    ONE_SHOT_CONVERSION  = 1
}adc_conv_rate_ctrl;

typedef enum __adc_sample_speed
{
    RESOLUTION_15_BIT   = 0,
    RESOLUTION_14_BIT   = 1,
    RESOLUTION_13_BIT   = 2,
    RESOLUTION_12_BIT   = 3
}adc_sample_speed;

typedef enum __adc_avg_ctrl
{
    SINGLE_VALUE   = 0,
    RUNNING_VALUE  = 1
}adc_avg_ctrl;

typedef enum __adc_avg_init
{
    START_AVG_USING_EX_REG_VAL    = 0,
    START_AVG_USING_NEW_ADC_CONV  = 1
}adc_avg_init;

typedef enum __chg_status
{
    NOT_CHARGING                  = 0,
    TRICKLE_CHARGE                = 1,
    PRE_CHARGE                    = 2,
    FAST_CHARGE                   = 3,
    TAPER_CHARGE                  = 4,
    RESERVED                      = 5,
    TOP_OFF_TIMER_ACTIVE_CHARGING = 6,
    CHARGING_TERMINATION_DONE     = 7
}chg_status;

typedef enum __bq25792_reg_addr
{
    MINIMUM_SYSTEM_VOLTAGE_REG0H  = 0x00,
    CHARGE_VOLTAGE_LIMIT_REG01H   = 0x01,
    CHARGE_CURRENT_LIMIT_REG03H   = 0x03,
    INPUT_VOLTAGE_LIMIT_REG05H    = 0x05,
    INPUT_CURRENT_LIMIT_REG06H    = 0x06,
    PRECHARGE_CONTROL_REG08H      = 0x08,
    TERMINATION_CONTROL_REG09H    = 0x09,
    RECHARGE_CONTROL_REG0AH       = 0x0A,
    TIMER_CONTROL_REG0EH          = 0x0E,
    CHARGE_CONTROL_0_REG0FH       = 0x0F,
    CHARGE_CONTROL_1_REG10H       = 0x10,
    CHARGE_CONTROL_2_REG11H       = 0x11,
    CHARGE_CONTROL_3_REG12H       = 0x12,
    CHARGE_CONTROL_4_REG13H       = 0x13,
    CHARGE_CONTROL_5_REG14H       = 0x14,
    TEMPERATURE_CONTROL_REG16H    = 0x16,
    NTC_CONTROL_0_REG17H          = 0x17,
    NTC_CONTROL_1_REG18H          = 0x18,
    ICO_CURRENT_LIMIT_REG19H      = 0x19,
    CHARGE_STATUS_0_REG1BH        = 0x1B,
    CHARGE_STATUS_1_REG1CH        = 0x1C,
    CHARGE_STATUS_2_REG1DH        = 0x1D,
    CHARGE_STATUS_3_REG1EH        = 0x1E,
    CHARGE_STATUS_4_REG1FH        = 0x1F,
    FAULT_STATUS_0_REG20H         = 0x20,
    FAULT_STATUS_1_REG21H         = 0x21,
    CHARGER_FLAG_0_REG22H         = 0x22,
    CHARGER_FLAG_1_REG23H         = 0x23,
    CHARGER_FLAG_2_REG24H         = 0x24,
    CHARGER_FLAG_3_REG25H         = 0x25,
    FAULT_FLAG_0_REG26H           = 0x26,
    FAULT_FLAG_1_REG27H           = 0x27,
    CHARGER_MASK_0_REG28H         = 0x28,
    CHARGER_MASK_1_REG29H         = 0x29,
    CHARGER_MASK_2_REG2AH         = 0x2A,
    CHARGER_MASK_3_REG2BH         = 0x2B,
    FAULT_MASK_0_REG2CH           = 0x2C,
    FAULT_MASK_1_REG2DH           = 0x2D,
    ADC_CONTROL_REG2EH            = 0x2E,
    ADC_FUNCTION_DISABLE_0_REG2FH = 0x2F,
    ADC_FUNCTION_DISABLE_1_REG30H = 0x30,
    IBUS_ADC_REG31H               = 0x31,
    IBAT_ADC_REG33H               = 0x33,
    VBUS_ADC_REG35H               = 0x35,
    VAC1_ADC_REG37H               = 0x37,
    VAC2_ADC_REG39H               = 0x39,
    VBAT_ADC_REG3BH               = 0x3B,
    VSYS_ADC_REG3DH               = 0x3D,
    TS_ADC_REG3FH                 = 0x3F,
    TDIE_ADC_REG41H               = 0x41,
    D_PLUS_REG43H                 = 0x43,
    D_MINUS_REG45H                = 0x45,
    DPDM_DRIVER_REG47H            = 0x47,
    PART_INFORMATION_REG48H       = 0x48,
    BQ25792_MAX_NUM_REG           = 0x49
}bq25792_reg_addr;

typedef union
{
	uint8_t byte;                     // BIT 0  to BIT 7

}REG00_MINIMAL_SYS_VOLTAGE;

typedef union
{
	uint16_t byte2;
    struct 
	{
		uint16_t VREG                : 11; // BIT 0  to BIT 10
        uint8_t  RESREVED            :  5; // BIT 11 to BIT 15
	} __attribute__((packed));
    
}REG01_CHARGE_VOLTAGE_LIMIT;

typedef union
{
	uint16_t byte2;

    struct 
	{
		uint16_t ICHG                :  9; // BIT 0  to BIT 9
        uint8_t  RESREVED            :  7; // BIT 10 to BIT 15
	} __attribute__((packed));
    
}REG03_CHARGE_CURRENT_LIMIT;

typedef union
{
	uint8_t byte;
    
}REG05_INPUT_VOLTAGE_LIMIT;

typedef union
{
	uint16_t byte2;

    struct 
	{
		uint16_t IINDPM              :  9; // BIT 0  to BIT 9
        uint8_t  RESREVED            :  7; // BIT 10 to BIT 15
	} __attribute__((packed));
    
}REG06_INPUT_CURRENT_LIMIT;

typedef union
{
	uint8_t byte;

    struct 
	{
		uint8_t IPRECHG             :  6; // BIT 0 to BIT 5
        uint8_t VBAT_LOWV           :  2; // BIT 6 to BIT 7
	} __attribute__((packed));
    
}REG08_PRECHARGE_CONTROL_REG;

typedef union
{
	uint8_t byte;
    
    struct 
	{
		uint8_t ITERM             :  5; // BIT 0 to BIT 4
        uint8_t RESERVED_5        :  1; // BIT 5
        uint8_t REG_RST           :  1; // BIT 6
        uint8_t RESERVED_7        :  1; // BIT 7
	} __attribute__((packed));
    
}REG08_TERMINATION_CONTROL_REG;

typedef union
{
	uint8_t byte;
	struct 
	{
		uint8_t B0_TO_B2_WA_TIME    : 3;
        uint8_t B3_WD_RST           : 1;
        uint8_t B4_B5_VAC_OVP       : 2;
        uint8_t B6_B7_RESERVED      : 2;
	} __attribute__((packed));
}REG10_CHARGER_CTRL_1;

typedef union
{
	uint8_t byte;
	struct 
	{
		uint8_t B0_RESERVED         : 1;
        uint8_t B1_EN_TERM          : 1;
        uint8_t B2_EN_HIZ           : 1;
        uint8_t B3_FORCE_ICO        : 1;
        uint8_t B4_EN_ICO           : 1;
        uint8_t B5_EN_CHG           : 1;
        uint8_t B6_FORCE_IBATDIS    : 1;
        uint8_t B7_EN_AUTO_IBATDIS  : 1;
	} __attribute__((packed));
}REG0F_CHARGER_CTRL_0;

typedef union
{
	uint8_t byte;
	struct 
	{
		uint8_t B0_EN_IBUS_OCP      : 1;
        uint8_t B1_FORCE_VINDPM_DET : 1;
        uint8_t B2_DIS_VOTG_UVP     : 1;
        uint8_t B3_DIS_VSYS_SHORT   : 1;
        uint8_t B4_DIS_STAT         : 1;
        uint8_t B5_PWM_FREQ         : 1;
        uint8_t B6_EN_ACDRV1        : 1;
        uint8_t B7_EN_ACDRV2        : 1;
	} __attribute__((packed));
}REG13_CHARGER_CTRL_4;

//REG14_Charger_Control_5 Register (Offset = 14h) [reset = 16h]
typedef union
{
	uint8_t byte;
	struct 
	{
		uint8_t EN_BATOC            : 1;
        uint8_t EN_EXTILIM          : 1;
        uint8_t EN_IINDPM           : 1;
        uint8_t IBAT_REG            : 2;
        uint8_t EN_IBAT             : 1;
        uint8_t RESERVED            : 1;
        uint8_t SFET_PRESENT        : 1;
	} __attribute__((packed));
}REG14_CHARGER_CTRL5_REG;

typedef union
{
	uint8_t byte;

    struct 
	{
		uint8_t DEV_REV             : 3;  // BIT 0 to BIT 2
        uint8_t PART_NUMBER         : 3;  // BIT 3 to BIT 5
        uint8_t RESERVED            : 2;  // BIT 6 and 7
	} __attribute__((packed));
}REG48_PART_INFO_REG;

typedef union
{
	uint8_t byte;

    struct 
	{
		uint8_t RESERVED            : 2;  // BIT 0 and 1
        uint8_t ADC_AVG_INIT        : 1;  // BIT 2
        uint8_t ADC_AVG             : 1;  // BIT 3
        uint8_t ADC_SAMPLE          : 2;  // BIT 4 and 5
        uint8_t ADC_RATE            : 1;  // BIT 6
        uint8_t ADC_EN              : 1;  // BIT 7
	} __attribute__((packed));
}REG2E_ADC_CONTROL_REG;

typedef union
{
	uint8_t byte;

    struct 
	{
		uint8_t BC1_2_DONE_MASK     : 1;  // BIT 0 
        uint8_t VBAT_PRESENT_MASK   : 1;  // BIT 1
        uint8_t TREG_MASK           : 1;  // BIT 2
        uint8_t RESERVED_3          : 1;  // BIT 3 
        uint8_t VBUS_MASK           : 1;  // BIT 4
        uint8_t RESERVED_5          : 1;  // BIT 5
        uint8_t ICO_MASK            : 1;  // BIT 6
        uint8_t CHG_MASK            : 1;  // BIT 7

	} __attribute__((packed));
}REG29_CHARGER_1_MASK;


typedef union
{
	uint8_t byte;

    struct 
	{
		uint8_t RESERVED            : 1;  // BIT 0
        uint8_t TDIE_ADC_DIS        : 1;  // BIT 1
        uint8_t TS_ADC_DIS          : 1;  // BIT 2
        uint8_t VSYS_ADC_DIS        : 1;  // BIT 3
        uint8_t VBAT_ADC_DIS        : 1;  // BIT 4
        uint8_t VBUS_ADC_DIS        : 1;  // BIT 5
        uint8_t IBAT_ADC_DIS        : 1;  // BIT 6
        uint8_t IBUS_ADC_DIS        : 1;  // BIT 7
	} __attribute__((packed));
}REG2F_ADC_FUNC_DISABLE_0_REG;

typedef union
{
	uint8_t byte;

    struct 
	{
		uint8_t RESERVED            : 4;  // BIT 0 to BIT 3
        uint8_t VAC1_ADC_DIS        : 1;  // BIT 4
        uint8_t VAC2_ADC_DIS        : 1;  // BIT 5
        uint8_t DM_ADC_DIS          : 1;  // BIT 6
        uint8_t DP_ADC_DIS          : 1;  // BIT 7
	} __attribute__((packed));
}REG30_ADC_FUNC_DISABLE_1_REG;

typedef union
{
	uint8_t byte;

    struct 
	{
		uint8_t TMR2X_EN            : 1;  // BIT 0
        uint8_t CHG_TMR             : 2;  // BIT 1 and 2
        uint8_t EN_CHG_TMR          : 1;  // BIT 3
        uint8_t EN_PRECHG_TMR       : 1;  // BIT 4
        uint8_t EN_TRICHG_TMR       : 1;  // BIT 5
        uint8_t TOPOFF_TMR          : 2;  // BIT 6 and 7
	} __attribute__((packed));
}REG0E_TIMER_CONTROL_REG;

typedef union
{
	uint8_t byte;
    
    struct 
	{
        uint8_t VRECHG              : 4;  // BIT 0 to BIT 3
		uint8_t TRECHG              : 2;  // BIT 4 and 5
        uint8_t CELL                : 2;  // BIT 6 and 7
	} __attribute__((packed));
}REG0A_RECHARGE_CTRL_REG;

typedef union
{
	uint8_t byte[2];
    
    struct 
	{
        int16_t ibus_adc            : 16;  // BIT 0 to BIT 16
	} __attribute__((packed));
}REG31_IBUS_ADC_REG;

typedef union
{
	uint8_t byte[2];
    
    struct 
	{
        int16_t ibat_adc            : 16;  // BIT 0 to BIT 16
	} __attribute__((packed));
}REG33_IBAT_ADC_REG;

typedef union
{
	uint8_t byte[2];
    
    struct 
	{
        uint16_t ICHG                : 9;  // BIT 0 to BIT 8
        uint8_t  RESERVED            : 7;  // BIT 9 to BIT 15
	} __attribute__((packed));
}REG03_CURRENT_LIMIT_REG;

typedef union
{
	uint8_t byte;
    
    struct 
	{
        uint8_t VAC1_OVP_STAT  : 1;  // BIT 0
        uint8_t VAC2_OVP_STAT  : 1;  // BIT 1
        uint8_t CONV_OCP_STAT  : 1;  // BIT 2
        uint8_t IBAT_OCP_STAT  : 1;  // BIT 3
        uint8_t IBUS_OCP_STAT  : 1;  // BIT 4
        uint8_t VBAT_OVP_STAT  : 1;  // BIT 5
        uint8_t VBUS_OVP_STAT  : 1;  // BIT 6
        uint8_t	IBAT_REG_STAT  : 1;  // BIT 7
    } __attribute__((packed));
}REG20_FAULT_STATUS_0_REG;

typedef union
{
	uint8_t byte;
    
    struct 
	{
        uint8_t RESERV_0_1       : 2;  // BIT 0 and BIT 1
        uint8_t TSHUT_STAT       : 1;  // BIT 2
        uint8_t RESERV_3         : 1;  // BIT 3
        uint8_t OTG_UVP_STAT     : 1;  // BIT 4
        uint8_t OTG_OVP_STAT     : 1;  // BIT 5
        uint8_t VSYS_OVP_STAT    : 1;  // BIT 6
        uint8_t VSYS_SHORT_STAT  : 1;  // BIT 7
    } __attribute__((packed));
}REG21_FAULT_STATUS_1_REG;

typedef union
{
	uint8_t byte;
    
    struct 
	{
        uint8_t VBUS_PRESENT_FLAG: 1;   // BIT 0
        uint8_t VAC1_PRESENT_FLAG : 1;  // BIT 1
        uint8_t VAC2_PRESENT_FLAG : 1;  // BIT 2
        uint8_t PG_FLAG           : 1;  // BIT 3
        uint8_t POORSRC_FLAG      : 1;  // BIT 4
        uint8_t WD_FLAG           : 1;  // BIT 5
        uint8_t VINDPM_FLAG       : 1;  // BIT 6
        uint8_t IINDPM_FLAG       : 1;  // BIT 7
    } __attribute__((packed));
}REG22_CHARGER_FLAG_0_REG;

typedef union
{
	uint8_t byte;
    
    struct 
	{
        uint8_t VBAT_PRESENT_FLAG  : 1;  // BIT 0
        uint8_t DPDM_FLAG          : 1;  // BIT 1
        uint8_t TREG_FLAG          : 1;  // BIT 2
        uint8_t RESERVED           : 3;  // BIT 3 to BIT 5
        uint8_t ICO_FLAG           : 2;  // BIT 6 to BIT 7
    } __attribute__((packed));
}REG23_CHARGER_FLAG_1_REG;

typedef union
{
	uint8_t byte;
    
    struct 
	{
        uint8_t VAC1_OVP_FLAG  : 1;  // BIT 0
        uint8_t VAC2_OVP_FLAG  : 1;  // BIT 1
        uint8_t CONV_OCP_FLAG  : 1;  // BIT 2
        uint8_t IBAT_OCP_FLAG  : 1;  // BIT 3
        uint8_t IBUS_OCP_FLAG  : 1;  // BIT 4
        uint8_t VBAT_OVP_FLAG  : 1;  // BIT 5
        uint8_t VBUS_OVP_FLAG  : 1;  // BIT 6
        uint8_t	IBAT_REG_FLAG  : 1;  // BIT 7
    } __attribute__((packed));
}REG26_FAULT_FLAG_0_REG;

typedef union
{
	uint8_t byte;
    
    struct 
	{
        uint8_t RESERV_0_1       : 2;  // BIT 0 and BIT 1
        uint8_t TSHUT_FLAG       : 1;  // BIT 2
        uint8_t RESERV_3         : 1;  // BIT 3
        uint8_t OTG_UVP_FLAG     : 1;  // BIT 4
        uint8_t OTG_OVP_FLAG     : 1;  // BIT 5
        uint8_t VSYS_OVP_FLAG    : 1;  // BIT 6
        uint8_t VSYS_SHORT_FLAG  : 1;  // BIT 7
    } __attribute__((packed));
}REG27_FAULT_FLAG_1_REG;

typedef union
{
	uint8_t byte;
    
    struct 
	{
        uint8_t VAC1_OVP_MASK  : 1;  // BIT 0
        uint8_t VAC2_OVP_MASK  : 1;  // BIT 1
        uint8_t CONV_OCP_MASK  : 1;  // BIT 2
        uint8_t IBAT_OCP_MASK  : 1;  // BIT 3
        uint8_t IBUS_OCP_MASK  : 1;  // BIT 4
        uint8_t VBAT_OVP_MASK  : 1;  // BIT 5
        uint8_t VBUS_OVP_MASK  : 1;  // BIT 6
        uint8_t	IBAT_REG_MASK  : 1;  // BIT 7
    } __attribute__((packed));
}REG2C_FAULT_MASK_0_REG;

typedef union
{
	uint8_t byte;
    
    struct 
	{
        uint8_t RESERV_0_1       : 2;  // BIT 0 and BIT 1
        uint8_t TSHUT_MASK       : 1;  // BIT 2
        uint8_t RESERV_3         : 1;  // BIT 3
        uint8_t OTG_UVP_MASK     : 1;  // BIT 4
        uint8_t OTG_OVP_MASK     : 1;  // BIT 5
        uint8_t VSYS_OVP_MASK    : 1;  // BIT 6
        uint8_t VSYS_SHORT_MASK  : 1;  // BIT 7
    } __attribute__((packed));
}REG2D_FAULT_MASK_1_REG;

typedef union
{
	uint8_t byte;
    
    struct 
	{
        uint8_t VBUS_PRESENT_STAT : 1;  // BIT 0
        uint8_t VAC1_PRESENT_STAT : 1;  // BIT 1
        uint8_t VAC2_PRESENT_STAT : 1;  // BIT 2
        uint8_t PG_STAT           : 1;  // BIT 3
        uint8_t POORSRC_STAT      : 1;  // BIT 4
        uint8_t WD_STAT           : 1;  // BIT 5
        uint8_t VINDPM_STAT       : 1;  // BIT 6
        uint8_t IINDPM_STAT       : 1;  // BIT 7
    } __attribute__((packed));
}REG1B_CHARGER_STATUS_0_REG;

typedef union
{
	uint8_t byte;
    
    struct 
	{
        uint8_t BC1_2_DONE_STAT  : 1;  // BIT 0
        uint8_t VBUS_STAT        : 4;  // BIT 1 to BIT 4
        uint8_t CHG_STAT         : 3;  // BIT 5 to BIT 7
    } __attribute__((packed));
}REG1C_CHARGER_STATUS_1_REG;

typedef union
{
	uint8_t byte;
    
    struct 
	{
        uint8_t VBAT_PRESENT_STAT  : 1;  // BIT 0
        uint8_t DPDM_STAT          : 1;  // BIT 1
        uint8_t TREG_STAT          : 1;  // BIT 2
        uint8_t RESERVED           : 3;  // BIT 3 to BIT 5
        uint8_t ICO_STAT           : 2;  // BIT 6 to BIT 7
    } __attribute__((packed));
}REG1D_CHARGER_STATUS_2_REG;

typedef union
{
	uint8_t byte;
    
    struct 
	{
        uint8_t RESERVED           : 1;  // BIT 0
        uint8_t PRECHG_TMR_STAT    : 1;  // BIT 1
        uint8_t TRICHG_TMR_STAT    : 1;  // BIT 2
        uint8_t CHG_TMR_STAT       : 1;  // BIT 3
        uint8_t VSYS_STAT          : 1;  // BIT 4
        uint8_t ADC_DONE_STAT      : 1;  // BIT 5
        uint8_t ACRB1_STAT         : 1;  // BIT 6
        uint8_t ACRB2_STAT         : 1;  // BIT 7
    } __attribute__((packed));
}REG1E_CHARGER_STATUS_3_REG;

typedef union
{
	uint8_t byte;
    
    struct 
	{
        uint8_t TS_HOT_STAT        : 1;  // BIT 0
        uint8_t TS_WARM_STAT       : 1;  // BIT 1
        uint8_t TS_COOL_STAT       : 1;  // BIT 2
        uint8_t TS_COLD_STAT       : 1;  // BIT 3
        uint8_t VBATOTG_LOW_STAT   : 1;  // BIT 4
        uint8_t RESERVED           : 3;  // BIT 5 to BIT 7
    } __attribute__((packed));
}REG1F_CHARGER_STATUS_4_REG;


//!< Structure to initiaize the BQ25792 driver 
typedef struct __bq25792_reg_config
{
    bq25792_reg_addr     addr;
    uint8_t*             data;
}bq25792_reg_config;

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
typedef struct drv_bus_conf
{
    drv_bus_conf_type type;
    union 
    {
        i2c_conf i2c;
        spi_conf spi;
    };
} drv_bus_conf;

/*!
 * @brief UC1510C device structure
 */
typedef struct	__bq25792_dev 
{
	/*! Chip Id */
	uint8_t chip_id;
	/*! Device Id */
	uint8_t dev_id;
    /*! slave address */
	uint8_t slave_address;
}bq25792_dev;

error_type bq25792_init(const drv_bus_conf* const bus_conf);
error_type bq25792_wd_reset(void);
error_type bq25792_read_reg(uint8_t reg_addr, uint8_t *const data, uint16_t len);
error_type bq25792_start_adc_conversion(void);
error_type bq25792_stop_adc_conversion(void);
error_type bq25792_wd_disable(void);
error_type bq25792_set_charging_current_limit(uint16_t chg_current);
error_type bq25792_write_reg(uint8_t reg_addr, uint8_t *data, uint16_t len);
uint8_t bq25792_get_chip_id(void);

#endif /* DRV_UC1510C_H_ */
