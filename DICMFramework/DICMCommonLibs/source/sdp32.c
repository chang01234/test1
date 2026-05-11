
#include "sdp32.h"


/*!
 * @brief This internal API is used to validate the device structure pointer for
 * null conditions.
 */
static int8_t null_ptr_check(const SDP32_DEV *dev)
{
    int8_t rslt;

    if ((dev == NULL) || (dev->read == NULL) || (dev->write == NULL) || (dev->delay_us == NULL))
    {
        /* Device structure pointer is not valid */
        rslt = SDP32_E_NULL_PTR;
    }
    else
    {
        /* Device structure is fine */
        rslt = SDP32_SUCCESS;
    }

    return rslt;
}

static int8_t sdp32_ValidateCommand(sdp32_command command)
{
    int8_t rslt;

    switch(command)
    {
	case COMMAND_START_CONT_MEASURMENT_MF_AVERAGE:
	case COMMAND_START_CONT_MEASURMENT_MF_NONE:
	case COMMAND_START_CONT_MEASURMENT_DP_AVERAGE:
	case COMMAND_START_CONT_MEASURMENT_DP_NONE:
	case COMMAND_STOP_CONTINOUS_MEASUREMENT:
	case COMMAND_START_TRIG_MEASUREMENT_MF_NONE:
	case COMMAND_START_TRIG_MEASUREMENT_MF_CS:
	case COMMAND_START_TRIG_MEASUREMENT_DP_NONE:
	case COMMAND_START_TRIG_MEASUREMENT_DP_CS:
	case COMMAND_SOFT_RESET:
	case COMMAND_ENTER_SLEEP_MODE:
	case COMMAND_READ_PRODUCT_IDENTIFIER_1:
	case COMMAND_READ_PRODUCT_IDENTIFIER_2:
            rslt = SDP32_SUCCESS;
            break;
	default:
            rslt = SDP32_E_INVALID_COMMAND;
            break;
    }

    return rslt;
}

int8_t sdp32_send_command(sdp32_command command, SDP32_DEV *dev)
{
    int8_t rslt;

    /* Check for null pointer in the device structure*/
    rslt = null_ptr_check(dev);
    
    if (rslt == SDP32_SUCCESS)
    {
        rslt = sdp32_ValidateCommand(command);
	if (rslt == SDP32_SUCCESS)
	{
            /* Write the command */
            dev->intf_rslt = dev->write(command, dev->intf_ptr);
	
            /* Check for communication error */
            if (dev->intf_rslt != SDP32_SUCCESS)
            {
		rslt = SDP32_E_COMM_FAIL;
            }
	}
	else
	{
            rslt = SDP32_E_INVALID_COMMAND;
	}
    }
    else
    {
	rslt = SDP32_E_NULL_PTR;
    }

    return rslt;
}

int8_t sdp32_get_data(uint8_t *data, uint32_t len, SDP32_DEV *dev)
{
    int8_t rslt;

    /* Check for null pointer in the device structure*/
    rslt = null_ptr_check(dev);

    if ((rslt == SDP32_SUCCESS) || (data != NULL))
    {
        /* Write the command */
	dev->intf_rslt = dev->read(data, len, dev->intf_ptr);

	/* Check for communication error */
	if (dev->intf_rslt != SDP32_SUCCESS)
	{
            rslt = SDP32_E_COMM_FAIL;
	}
    }
    else
    {
	rslt = SDP32_E_NULL_PTR;
    }

    return rslt;
}

uint32_t sdp32_get_prodnumber(uint8_t *data, uint8_t len)
{
    uint8_t prod_idt[PRODUCT_IDENTIFIER_LENGTH] = { 0 };
    uint32_t prod_number = 0x0;

    memcpy(prod_idt, data, len);
    /*********
	Byte1: Product number [31:24]
	Byte2: Product number [23:16]
	Byte3: CRC
	Byte4: Product number [15:8]
	Byte5: Product number [7:0]
	Byte6: CRC
    ************/
    
    prod_number = ((prod_idt[0] << 24) | (prod_idt[1] << 16) | (prod_idt[3] << 8) | (prod_idt[4]));

    return prod_number;
}

int8_t sdp32_init(SDP32_DEV *dev)
{
    /* Product Number read try count */
    uint8_t try_count = 5;
    int8_t rslt;
    uint8_t prod_idt[PRODUCT_IDENTIFIER_LENGTH] = { 0 };
    uint32_t prod_num = 0x0;

    /* Check for null pointer in the device structure*/
    rslt = null_ptr_check(dev);
	
    /* Proceed if null check is fine */
    if (rslt == SDP32_SUCCESS)
    {
        /* Stop the previous continuous measurement sensing */
        sdp32_send_command(COMMAND_STOP_CONTINOUS_MEASUREMENT, dev);
        /* Wait for 1 ms */
        dev->delay_us(1000, dev->intf_ptr);

        /* Soft reset of the sensor */
        //sdp32_SoftReset(dev);

	while (try_count)
        {
            /* Write the command */
            rslt = sdp32_send_command(COMMAND_READ_PRODUCT_IDENTIFIER_1, dev);
            if (rslt == SDP32_SUCCESS)
            {
                rslt = sdp32_send_command(COMMAND_READ_PRODUCT_IDENTIFIER_2, dev);

                if (rslt == SDP32_SUCCESS)
                {
                    /* Read Product Identifier */
                    rslt = sdp32_get_data(prod_idt, PRODUCT_IDENTIFIER_LENGTH, dev);
		
                    /* Check for chip id validity */
                    if (rslt == SDP32_SUCCESS)
                    {
                        prod_num = sdp32_get_prodnumber(prod_idt, PRODUCT_IDENTIFIER_LENGTH);
                        /* Verify Product Number is valid */
                        if(prod_num == SDP32_PRODUCT_NUMBER)
                        {
                            dev->prod_num = prod_num;
                            break;
                        }
                    }
                }
            }

            /* Wait for 1 ms */
            dev->delay_us(2000, dev->intf_ptr);
            --try_count;
	}

	/* Chip id check failed */
        if (!try_count)
        {
            rslt = SDP32_E_DEV_NOT_FOUND;
        }
    }

    return rslt;
}

int8_t sdp32_SoftReset(SDP32_DEV *dev)
{
    int8_t rslt;

    /* Check for null pointer in the device structure*/
    rslt = null_ptr_check(dev);

    if ((rslt == SDP32_SUCCESS))
    {
        /* Write the command */
        dev->intf_rslt = dev->write(COMMAND_SOFT_RESET, dev->intf_ptr);

	/* Check for communication error */
	if (dev->intf_rslt != SDP32_SUCCESS)
	{
            rslt = SDP32_E_COMM_FAIL;
	}
        else
        {
            /* Wait for 20 ms ----
            After the reset command the sensor will take maximum 20ms to reset. 
            During this time the sensor will not acknowledge its address nor accept commands.*/
            dev->delay_us(20000, dev->intf_ptr);
        }
    }
    else
    {
	rslt = SDP32_E_NULL_PTR;
    }

    return rslt;
}
