#include "configuration.h"

#if defined(CONNECTOR_LIN) || defined(CONNECTOR_LIN_SERVER) 

#include "dometic_func.h"

//-----------------------------------------------------------------------------
// Function:    dometic_ac_function_mode_set
//-----------------------------------------------------------------------------
// Description: Set Dometic AC function mode to LIN value.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
int32_t dometic_ac_mode_set(pstore_table_t eIndex, int32_t i32Value)
{
    /* In: Cool=0; Heat=1; Fan=2; Auto=3; Dry=4; */
    int32_t mode_a = 0;
    int32_t mode_b = 0;

	if ((i32Value >= 0) && (i32Value <= 4))
	{
        switch (i32Value)
        {
            case 4:
                /* Dry: Mode A = 0, Mode B = 0 */
            break;
            case 3:
                /* Auto: Mode A = 1, Mode B = 0 */
                mode_a = 1;
            break;
            case 0:
                /* Cool:  Mode A = 1, Mode B = 1 */
                mode_a = 1;
                mode_b = 1;
            break;
            case 1:
                /* Heat: Mode A = 1, Mode B = 2 */
                mode_a = 1;
                mode_b = 2;
            break;
            default:
                /* Fan: Mode A = 1, Mode B = 3 */
                mode_a = 1;
                mode_b = 3;
            break;
        }

        // TODO: Only works for bus 0.
        if (eIndex == VAR_LIN0_DO_AC_MODE_CTRL_C)
        {
            /* Store the overall mode setting, not going out on LIN bus */
            pstore_SetValue(eIndex, i32Value);

            /* Store the mode parameters */
            pstore_SetValue(VAR_LIN0_DO_AC_MODE_A_C, mode_a);
            pstore_SetValue(VAR_LIN0_DO_AC_MODE_B_C, mode_b);

            if (i32Value == 3)
            {
                // When mode auto, force fan speed to auto
		        pstore_SetValue(VAR_LIN0_DO_AC_FAN_MODE_C, 0);
            }
            else if (i32Value == 2)
            {
                // Fan mode can not have auto fan speed
                pstore_SetValue(VAR_LIN0_DO_AC_FAN_MODE_C, 1);
            }
        }

        return 1;
	}

    return 0;
}

//-----------------------------------------------------------------------------
// Function:    dometic_ac_function_mode_get
//-----------------------------------------------------------------------------
// Description: Get Dometic AC function mode to LIN value.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
int32_t dometic_ac_mode_get(void)
{
    /* Out: Cool=0; Heat=1; Fan=2; Auto=3; Dry=4; */
    uint8_t mode_a;
    uint8_t mode_b;
    int32_t res = 0;

    // TODO: Only works for bus 0.
    mode_a = pstore_u8GetValue(VAR_LIN0_DO_AC_MODE_A_I);
    mode_b = pstore_u8GetValue(VAR_LIN0_DO_AC_MODE_B_I);

    if ((mode_a == 0) && (mode_b == 0))
    {
        res = 4; // Dry
    }
    else if ((mode_a == 1) && (mode_b == 0))
    {
        res = 3; // Auto
    }
    else if ((mode_a == 1) && (mode_b == 1))
    {
        res = 0; // Cool
    }
    else if ((mode_a == 1) && (mode_b == 2))
    {
        res = 1; // Heat
    }
    else if ((mode_a == 1) && (mode_b == 3))
    {
        res = 2; // Fan
    }

    return res;
}

//-----------------------------------------------------------------------------
// Function:    dometic_ac_target_temp_set
//-----------------------------------------------------------------------------
// Description: Set Dometic AC target temperature to LIN value.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
int32_t dometic_ac_target_temp_set(pstore_table_t eIndex, int32_t i32Value)
{
    /* Frame value = Target Temperature - 16 */

	if ((i32Value >= 16) && (i32Value <= 31))
	{
		pstore_SetValue(eIndex, (i32Value - 16));
        return 1;
	}

    return 0;
}

//-----------------------------------------------------------------------------
// Function:    dometic_ac_fan_speed_set
//-----------------------------------------------------------------------------
// Description: Set Dometic AC target temperature to LIN value.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
int32_t dometic_ac_fan_speed_set(pstore_table_t eIndex, int32_t i32Value)
{
    int32_t operating_mode;
    int32_t result = 0;
    
    operating_mode = dometic_ac_mode_get();

	switch (i32Value)
	{
        case 0:
        case 1:
        case 2:
        case 3:
            // Only allow changes for speed when not in auto mode
            if (operating_mode != 3)
            {
                pstore_SetValue(VAR_LIN0_DO_AC_FAN_SPEED_C, i32Value);
                pstore_SetValue(VAR_LIN0_DO_AC_FAN_MODE_C, 1);
                result = 1;
            }
            break;
        case 5:
            // Only allow a change to auto when not in fan mode
            if (operating_mode != 2)
            {
		        pstore_SetValue(VAR_LIN0_DO_AC_FAN_MODE_C, 0);
                result = 1;
            }
            break;
        default:
            break;
	}

    return result;
}

//-----------------------------------------------------------------------------
// Function:    dometic_ac_fan_speed_get
//-----------------------------------------------------------------------------
// Description: Get Dometic AC fan speed setting.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
int32_t dometic_ac_fan_speed_get(pstore_table_t eIndex, DDM2_TYPE_ENUM type)
{
    /* Get both mode and speed */
	uint8_t speed = pstore_u8GetValue(VAR_LIN0_DO_AC_FAN_SPEED_I);
	uint8_t mode = pstore_u8GetValue(VAR_LIN0_DO_AC_FAN_MODE_I);

    (void) type;

    if (mode == 0)
    {
        // This means that fans are not possible to control.
        // Return DDM fan speed auto mode.
        return 5;
    }
    else
    {
        return speed;
    }
}

//-----------------------------------------------------------------------------
// Function:    dometic_ac_target_temp_convert
//-----------------------------------------------------------------------------
// Description: Convert Dometic AC target temperature to system value.
//-----------------------------------------------------------------------------
// Return:      Converted value.
//-----------------------------------------------------------------------------
int32_t dometic_ac_target_temp_convert(int32_t i32Value)
{
    /* System temp = i32Value + 16 */

    return i32Value + 16;
}

#endif