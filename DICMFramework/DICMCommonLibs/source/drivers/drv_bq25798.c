/*****************************************************************************
 * \file       drv_bq25798.c
 * \brief      PMIC Driver File
 *
 *****************************************************************************/

#include "iGeneralDefinitions.h"

#ifdef DEVICE_BQ25798
#include "hal_i2c_master.h"
#include "drv_bq25798.h"
#include "hal_cpu.h"
#include "ddm2.h"
#include <string.h>
/*****************************************************************************
 * Local defines
 *****************************************************************************/
#define DRV_BQ25798_DEBUG 0
static bq25798_dev bq25798_dev_inst;

/*****************************************************************************
 * Static Function declarations
 *****************************************************************************/

static REG10_CHARGER_CTRL_1 chrg_ctrl1_reg10 = 
{
    .B0_TO_B2_WA_TIME = WD_RST_TIME_40SEC     ,
    .B3_WD_RST        = WD_TIMER_NORMAL_DEF   ,
    .B4_B5_VAC_OVP    = THRESHOLD_VOLTAGE_26V ,
    .B6_B7_RESERVED   = 0                     
};

static REG2E_ADC_CONTROL_REG adc_ctrl_reg;
static uint16_t ichg = 0;

#if DRV_BQ25798_DEBUG
bq25798_reg_addr reg_arr[] =
{
    MINIMUM_SYSTEM_VOLTAGE_REG0H ,
    CHARGE_VOLTAGE_LIMIT_REG01H  ,
    CHARGE_CURRENT_LIMIT_REG03H  ,
    INPUT_VOLTAGE_LIMIT_REG05H   ,
    INPUT_CURRENT_LIMIT_REG06H   ,
    PRECHARGE_CONTROL_REG08H     ,
    TERMINATION_CONTROL_REG09H   ,
    RECHARGE_CONTROL_REG0AH      ,
    CHARGE_CONTROL_0_REG0FH      ,
    CHARGE_CONTROL_1_REG10H      ,
    CHARGE_CONTROL_2_REG11H      ,
    CHARGE_CONTROL_3_REG12H      ,
    CHARGE_CONTROL_4_REG13H      ,
    CHARGE_CONTROL_5_REG14H      ,
    RESERVED_REG15H              ,
    TEMPERATURE_CONTROL_REG16H   ,
    NTC_CONTROL_0_REG17H         ,
    NTC_CONTROL_1_REG18H         ,
    ICO_CURRENT_LIMIT_REG19H     ,
    CHARGE_STATUS_0_REG1BH       ,
    CHARGE_STATUS_1_REG1CH       ,
    CHARGE_STATUS_2_REG1DH       ,
    CHARGE_STATUS_3_REG1EH       ,
    CHARGE_STATUS_4_REG1FH       ,
    FAULT_STATUS_0_REG20H        ,
    FAULT_STATUS_1_REG21H        ,
    CHARGER_FLAG_0_REG22H        ,
    CHARGER_FLAG_1_REG23H        ,
    CHARGER_FLAG_2_REG24H        ,
    CHARGER_FLAG_3_REG25H        ,
    FAULT_FLAG_0_REG26H          ,
    FAULT_FLAG_1_REG27H          ,
    CHARGER_MASK_0_REG28H        ,
    CHARGER_MASK_1_REG29H        ,
    CHARGER_MASK_2_REG2AH        ,
    CHARGER_MASK_3_REG2BH        ,
    FAULT_MASK_0_REG2CH          ,
    FAULT_MASK_1_REG2DH          ,
    ADC_CONTROL_REG2EH           ,
    ADC_FUNCTION_DISABLE_0_REG2FH,
    ADC_FUNCTION_DISABLE_1_REG30H,
    IBUS_ADC_REG31H              ,
    IBAT_ADC_REG33H              ,
    VBUS_ADC_REG35H              ,
    VAC1_ADC_REG37H              ,
    VAC2_ADC_REG39H              ,
    VBAT_ADC_REG3BH              ,
    VSYS_ADC_REG3DH              ,
    TS_ADC_REG3FH                ,
    TDIE_ADC_REG41H              ,
    D_PLUS_REG43H                ,
    D_MINUS_REG45H               ,
    DPDM_DRIVER_REG47H           ,
    PART_INFORMATION_REG48H      
};
#endif

/*! \brief Initialize BQ25798 LCD Driver IC
 *  \param bus_conf Pointer to bus configuration data.
 *  \return 0 if successful. Any other value otherwise.
 */
error_type bq25798_init(const drv_bus_conf* const bus_conf)
{
    error_type              result = RES_FAIL;
    error_type              result_chipid = RES_FAIL;
    error_type              final_result = RES_FAIL;
    REG48_PART_INFO_REG     bqic_part_info;
    REG0A_RECHARGE_CTRL_REG rechg_ctrl_reg;
    REG0E_TIMER_CONTROL_REG timer_ctrl_reg;
    uint8_t                 byte;
    uint16_t                byte2;

    /* Set the device ID */
    bq25798_dev_inst.dev_id = bus_conf->i2c.port;
    /* Set the slave address */
    bq25798_dev_inst.slave_address = DEV_PMIC_BQ25798_I2C_SLAVE_ADDR;

    /* Wait for 1000 msec before accessing BQ25798 I2C registers.. ( Recommended as per TI datasheeet is 25 msec ) */
    hal_cpu_wait_us(1000*1000);
    
    /* Read the BQ25798 IC part number */
    result_chipid = bq25798_read_reg(PART_INFORMATION_REG48H, &bqic_part_info.byte, ONE_BYTE);
    if ( RES_PASS == result_chipid )
    {
        if( (bqic_part_info.PART_NUMBER == BQ25798_PART_NUMBER) && (bqic_part_info.DEV_REV == BQ25798_DEV_NUMBER ) )
        {
            LOG(W,"[%s found]",BQ25798_CHIP_NAME);
            bq25798_dev_inst.chip_id = BQ25798_PART_NUMBER;
            result_chipid = RES_PASS;
        }
        else
        {
            result_chipid = RES_FAIL;
        }

        LOG(W, "BQ25798 PART_NUMBER : %d chip_id : %d", bqic_part_info.PART_NUMBER, bq25798_dev_inst.chip_id );

        /* Reset the watchdog timer */
        result = bq25798_wd_reset();

        /* Battery cell configuartion read */
        result = bq25798_read_reg(RECHARGE_CONTROL_REG0AH, &rechg_ctrl_reg.byte, ONE_BYTE);
        LOG(W, "Before config RECHARGE_CONTROL_REG0AH = 0x%x result = %d", rechg_ctrl_reg.byte, result);

        rechg_ctrl_reg.CELL = THREE_CELL;
        result = bq25798_write_reg(RECHARGE_CONTROL_REG0AH, &rechg_ctrl_reg.byte, ONE_BYTE);

    #if DRV_BQ25798_DEBUG
        result = bq25798_read_reg(RECHARGE_CONTROL_REG0AH, &rechg_ctrl_reg.byte, ONE_BYTE);
        LOG(W, "After config RECHARGE_CONTROL_REG0AH = 0x%x result = %d", rechg_ctrl_reg.byte, result);
    #endif

        /* Set charging current limit */
        #ifdef EMC_TEST_CASE
           result = bq25798_set_charging_current_limit(CHARGE_CURRENT_500MA);   /*Charging Current setting for EMC testing*/      
        #else
            
            result = bq25798_set_charging_current_limit(CHARGE_CURRENT_LIMIT_VALUE);

        #endif
        
        
        byte = MINIMUM_SYSTEM_VOLTAGE_VALUE;
        /* send the data to bq25798 */
        result = bq25798_write_reg(MINIMUM_SYSTEM_VOLTAGE_REG0H, &byte, ONE_BYTE);

    #if DRV_BQ25798_DEBUG
        byte = 0;
        result = bq25798_read_reg(MINIMUM_SYSTEM_VOLTAGE_REG0H, &byte, ONE_BYTE);
        LOG(W, "MINIMUM_SYSTEM_VOLTAGE_REG0H = 0x%x result = %d", byte, result);
    #endif

        byte2 = SWAP2(CHARGE_VOLTAGE_LIMIT_VALUE);
        /* Send the data to bq25798 */
        result = bq25798_write_reg(CHARGE_VOLTAGE_LIMIT_REG01H, (uint8_t*)&byte2, TWO_BYTE);

    #if DRV_BQ25798_DEBUG
        byte2 = 0;
        result = bq25798_read_reg(CHARGE_VOLTAGE_LIMIT_REG01H, (uint8_t*)&byte2, TWO_BYTE);
        LOG(W, "CHARGE_VOLTAGE_LIMIT_REG01H = 0x%x result = %d", SWAP2(byte2), result);
    #endif

    #if DRV_BQ25798_DEBUG
        byte2 = 0;
        result = bq25798_read_reg(CHARGE_CURRENT_LIMIT_REG03H, (uint8_t*)&byte2, TWO_BYTE);
        LOG(W, "CHARGE_CURRENT_LIMIT_REG03H = 0x%x result = %d", SWAP2(byte2), result);
    #endif

        result = bq25798_read_reg(TIMER_CONTROL_REG0EH, (uint8_t*)&timer_ctrl_reg.byte, ONE_BYTE);
        LOG(W, "Before config TIMER_CONTROL_REG0EH = 0x%x result = %d", timer_ctrl_reg.byte, result);

        //Set TSHUT  cut off temperature
        result = bq25798_read_reg(NTC_CONTROL_1_REG18H, (uint8_t*)&byte2, ONE_BYTE);
        byte2 = byte2 & (~BQ25798_REG18_TS_COOL_BIT_CLEAR);
        byte2 = byte2 & (~BQ25798_REG18_TS_WARM_BIT_CLEAR);

        LOG(W, "Before TEMP_CTRL_REG18H = 0x%x result = %d", byte2, result);
        byte2 = byte2 | BQ25798_REG18_TS_COOL_71_1 ; 
        byte2 = byte2 | BQ25798_REG18_TS_WARM_37_7;
        result = bq25798_write_reg(NTC_CONTROL_1_REG18H, (uint8_t*)&byte2, ONE_BYTE);
        
        //Set TSHUT  cut off temperature
        result = bq25798_read_reg(TEMPERATURE_CONTROL_REG16H, (uint8_t*)&byte2, ONE_BYTE);
        LOG(W, "Before config TEMPERATURE_CONTROL_REG16H = 0x%x result = %d", byte2, result);
        byte2 = byte2 & (~BQ25798_REG16_TSHUT_BIT_CLEAR);
        byte2 = byte2 & (~BQ25798_REG16_TREG_BIT_CLEAR);

        byte2 = byte2 | BQ25798_REG16_TSHUT_85C ; //set Thut cut off to 85 degree C value = 0x30;
        byte2 = byte2 | BQ25798_REG16_TREG_65C;
        result = bq25798_write_reg(TEMPERATURE_CONTROL_REG16H, (uint8_t*)&byte2, ONE_BYTE);

        timer_ctrl_reg.CHG_TMR = TWELVE_HOURS;
        /* send the data to bq25798 */
        result = bq25798_write_reg(TIMER_CONTROL_REG0EH, &timer_ctrl_reg.byte, ONE_BYTE);
        LOG(W, "After config TIMER_CONTROL_REG0EH = 0x%x result = %d", timer_ctrl_reg.byte, result);

        byte = INPUT_VOLTAGE_LIMIT_VALUE;
        result = bq25798_write_reg(INPUT_VOLTAGE_LIMIT_REG05H, &byte, ONE_BYTE);

    #if DRV_BQ25798_DEBUG
        byte = 0;
        result = bq25798_read_reg(INPUT_VOLTAGE_LIMIT_REG05H, (uint8_t*)&byte, ONE_BYTE);
        LOG(W, "Before config INPUT_VOLTAGE_LIMIT_REG05H = 0x%x result = %d", byte, result);
    #endif
        /* Start ADC conversion */
        result = bq25798_start_adc_conversion();
    }
    else
    {
        LOG(E, "BATTERY IC BQ25798 INIT FAILED error = %d", result);
    }

    if((result_chipid == RES_PASS) && (result == RES_PASS))
    {
        final_result = RES_PASS;
    }
    else
    {
        final_result = RES_FAIL; 
    }

    return final_result;
}

uint8_t bq25798_get_chip_id(void)
{
    return bq25798_dev_inst.chip_id;
}
 
/*! \brief  This function used to set the battery charging current limit
 *  \param  void
 *  \return result 0 - Pass / -1 - Fail
 */
error_type bq25798_set_charging_current_limit(uint16_t chg_current)
{
    error_type result = RES_FAIL;
    uint16_t data2 = SWAP2(chg_current);

    if ( ichg != data2 )
    {
        ichg = data2;

        result = bq25798_write_reg(CHARGE_CURRENT_LIMIT_REG03H, (uint8_t*)&data2, 2u);
    }
    else
    {
        /* Already configured with the same charging current limit */
        result  = RES_PASS;
    }
    
    return result;
}

/*! \brief  This function used to reset the internal watch dog timer of the battery charger IC BQ25798 before it get expires
 *  \param  void
 *  \return result 0 - Pass / -1 - Fail
 */
error_type bq25798_wd_reset(void)
{
    error_type result;

    /* set the WD reset bit */
    chrg_ctrl1_reg10.B0_TO_B2_WA_TIME = WD_RST_TIME_40SEC;
    chrg_ctrl1_reg10.B3_WD_RST        = WD_TIMER_RESET;

    /* send the data to bq25798 */
    result = bq25798_write_reg(CHARGE_CONTROL_1_REG10H, &chrg_ctrl1_reg10.byte, ONE_BYTE);

    return result;
}

error_type bq25798_wd_disable(void)
{
    error_type result;

    /* set the WD reset bit */
    chrg_ctrl1_reg10.B3_WD_RST        = WD_TIMER_RESET;
    chrg_ctrl1_reg10.B0_TO_B2_WA_TIME = WD_DISABLE_TIME;

    /* send the data to bq25798 */
    result = bq25798_write_reg(CHARGE_CONTROL_1_REG10H, &chrg_ctrl1_reg10.byte, ONE_BYTE);

    return result;
}

/*! \brief  This function used to start the one shot ADC conversion
 *  \param  void
 *  \return result 0 - Pass / -1 - Fail
 */
error_type bq25798_start_adc_conversion(void)
{
    error_type result;
    REG2F_ADC_FUNC_DISABLE_0_REG adc_func_dis0_reg;
    REG30_ADC_FUNC_DISABLE_1_REG adc_func_dis1_reg;

    adc_ctrl_reg.ADC_EN       = ENABLE;
    adc_ctrl_reg.ADC_AVG_INIT = START_AVG_USING_NEW_ADC_CONV;
    adc_ctrl_reg.ADC_AVG      = SINGLE_VALUE;
    adc_ctrl_reg.ADC_SAMPLE   = RESOLUTION_15_BIT;
    adc_ctrl_reg.ADC_RATE     = ONE_SHOT_CONVERSION;

    /* Start ADC conversion */
    result = bq25798_write_reg(ADC_CONTROL_REG2EH, &adc_ctrl_reg.byte, ONE_BYTE);

    adc_func_dis0_reg.TDIE_ADC_DIS = ADC_CH_ENABLE;
    adc_func_dis0_reg.IBAT_ADC_DIS = ADC_CH_ENABLE;
    adc_func_dis0_reg.IBUS_ADC_DIS = ADC_CH_ENABLE;
    adc_func_dis0_reg.TS_ADC_DIS   = ADC_CH_ENABLE;
    adc_func_dis0_reg.VSYS_ADC_DIS = ADC_CH_ENABLE;
    adc_func_dis0_reg.VBAT_ADC_DIS = ADC_CH_ENABLE;
    adc_func_dis0_reg.VBUS_ADC_DIS = ADC_CH_ENABLE;

    result = bq25798_write_reg(ADC_FUNCTION_DISABLE_0_REG2FH, &adc_func_dis0_reg.byte, ONE_BYTE);

    adc_func_dis1_reg.DM_ADC_DIS   = ADC_CH_ENABLE;
    adc_func_dis1_reg.DP_ADC_DIS   = ADC_CH_ENABLE;
    adc_func_dis1_reg.VAC1_ADC_DIS = ADC_CH_ENABLE;
    adc_func_dis1_reg.VAC2_ADC_DIS = ADC_CH_ENABLE;
    
    result = bq25798_write_reg(ADC_FUNCTION_DISABLE_1_REG30H, &adc_func_dis1_reg.byte, ONE_BYTE);

    return result;
}

/*! \brief  This function used to start the one shot ADC conversion
 *  \param  void
 *  \return result 0 - Pass / -1 - Fail
 */
error_type bq25798_stop_adc_conversion(void)
{
    error_type result;
    REG2F_ADC_FUNC_DISABLE_0_REG adc_func_dis0_reg;
    REG30_ADC_FUNC_DISABLE_1_REG adc_func_dis1_reg;

    adc_ctrl_reg.ADC_EN       = DISABLE;
    adc_ctrl_reg.ADC_AVG_INIT = START_AVG_USING_NEW_ADC_CONV;
    adc_ctrl_reg.ADC_AVG      = SINGLE_VALUE;
    adc_ctrl_reg.ADC_SAMPLE   = RESOLUTION_15_BIT;
    adc_ctrl_reg.ADC_RATE     = ONE_SHOT_CONVERSION;

    /* send the data to bq25798 */
    result = bq25798_write_reg(ADC_CONTROL_REG2EH, &adc_ctrl_reg.byte, ONE_BYTE);

    adc_func_dis0_reg.TDIE_ADC_DIS = ADC_CH_DISABLE;
    adc_func_dis0_reg.IBAT_ADC_DIS = ADC_CH_DISABLE;
    adc_func_dis0_reg.IBUS_ADC_DIS = ADC_CH_DISABLE;
    adc_func_dis0_reg.TS_ADC_DIS   = ADC_CH_DISABLE;
    adc_func_dis0_reg.VSYS_ADC_DIS = ADC_CH_DISABLE;
    adc_func_dis0_reg.VBAT_ADC_DIS = ADC_CH_DISABLE;
    adc_func_dis0_reg.VBUS_ADC_DIS = ADC_CH_DISABLE;

    result = bq25798_write_reg(ADC_FUNCTION_DISABLE_0_REG2FH, &adc_func_dis0_reg.byte, ONE_BYTE);

    adc_func_dis1_reg.DM_ADC_DIS   = ADC_CH_DISABLE;
    adc_func_dis1_reg.DP_ADC_DIS   = ADC_CH_DISABLE;
    adc_func_dis1_reg.VAC1_ADC_DIS = ADC_CH_DISABLE;
    adc_func_dis1_reg.VAC2_ADC_DIS = ADC_CH_DISABLE;

    result = bq25798_write_reg(ADC_FUNCTION_DISABLE_1_REG30H, &adc_func_dis1_reg.byte, ONE_BYTE);

    return result;
}

/*! \brief  This function provides write functionality to the Battery charger IC BQ25798.
 *  \param reg_addr	Register address to write the value to.
 *  \param reg_data	Register data to write to the reg_addr.
 *  \param len      Length of bytes to be written.
 *
 *  \return 0 if successful. Any other value otherwise.
 */
error_type bq25798_write_reg(uint8_t reg_addr, uint8_t *data, uint16_t len)
{
    error_type result;
    uint8_t txbuf[10] = { 0 };
    uint8_t retry_count = 0;

    if ( len <  ( ARR_ELEMENTS( txbuf ) - 1) )
    {
        txbuf[0] = reg_addr;
        memcpy(&txbuf[1], data, len);

        do
        {
            result = hal_i2c_master_acquire(bq25798_dev_inst.dev_id);
            if (result == HAL_E_OK)
            {
                result = hal_i2c_master_write(bq25798_dev_inst.dev_id, bq25798_dev_inst.slave_address, txbuf, len + ONE_BYTE);

                hal_i2c_master_release(bq25798_dev_inst.dev_id);
            }
        }while( ( ++retry_count <= 5 ) && ( result != RES_PASS ) );
    }
    else
    {
        result = RES_FAIL;   // Data to long for transfer buffer.
    }

    return result;
}

/*! \brief  This function provides read functionality to the Battery charger IC BQ25798.
 *  \param reg_addr	Register address to read the value from.
 *  \param reg_data	pointer to the data which has been read from sensor.
 *  \param len      Length of bytes to be read.
 *
 *  \return 0 if successful. Any other value otherwise.
 */
error_type bq25798_read_reg(uint8_t reg_addr, uint8_t *const data, uint16_t len)
{   
    error_type result;
    uint8_t retry_count = 0;

    do
    {
        result = hal_i2c_master_acquire(bq25798_dev_inst.dev_id);
        if (result == HAL_E_OK)
        {
            result = hal_i2c_master_writeread(bq25798_dev_inst.dev_id, bq25798_dev_inst.slave_address, &reg_addr, BQ25798_REGISTER_SIZE_IN_BYTES, data, len);
            
            hal_i2c_master_release(bq25798_dev_inst.dev_id);
        }
    }while( ( ++retry_count <= 5 ) && ( result != RES_PASS ) );

    return result;
}

/**
 * @brief Enable MPPT Charging feature
 * @return error_type 
 */
error_type bq25798_enable_mppt()
{
    error_type result;
    uint8_t mppt_data;
    uint8_t mppt_data_r;

    mppt_data = ENABLE_MPPT;
    result = bq25798_write_reg(MPPT_CONTROL_REG15H, (uint8_t*) &mppt_data, ONE_BYTE);
    if ( result != RES_FAIL )
    {
        
        result = bq25798_read_reg(MPPT_CONTROL_REG15H, &mppt_data_r, 1u);
        LOG(W, "[MPPT Enabled = %d]", mppt_data_r);
    }
    return result;
}

/**
 * @brief Disable MPPT Charging feature
 * @return error_type 
 */
error_type bq25798_disable_mppt()
{
    error_type result;
    uint8_t mppt_data;

    mppt_data = DISABLE_MPP;
    result = bq25798_write_reg(MPPT_CONTROL_REG15H, (uint8_t*)&mppt_data, ONE_BYTE);
    if ( result != RES_FAIL )
    {
        LOG(W, "[MPPT Disabled]]");
    }

    return result;
}

#endif
