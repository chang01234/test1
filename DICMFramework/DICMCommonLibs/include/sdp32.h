#ifndef SDP32_H_
#define SDP32_H_

/*! CPP guard */
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>


// convert two 8 bit values to one word
#define BIU16(data, start) (((uint16_t)(data)[start]) << 8 | ((data)[start + 1]))

// data length of result from I2C
#define COMMAND_DATA_LENGTH 			2
#define PRODUCT_IDENTIFIER_LENGTH		5
#define PRODUCT_NUMBER_LENGTH			4
#define SENSOR_DATA_LENGTH 			9

#define DEFAULT_SDP3X_I2C_ADDRESS  		0x21
#define WITH_REG1_SDP3X_I2C_ADDRESS  		0x22  //ADDR connected with 1.2kOhm to GND
#define WITH_REG2_SDP3X_I2C_ADDRESS  		0x23  //ADDR connected with 2.7kOhm to GND
#define SDP32_PRODUCT_NUMBER			0x03010201

//-----------------------------------------------------------------------------
/// \brief Enumeration to configure the temperature compensation for
///        measurement.
typedef enum {
  SDP32_TEMPCOMP_MASS_FLOW,
  SDP32_TEMPCOMP_DIFFERNTIAL_PRESSURE
} Sdp32TempComp;

//-----------------------------------------------------------------------------
/// \brief Enumeration to configure the averaging for measurement.
typedef enum {
  SDP32_AVERAGING_NONE,
  SDP32_AVERAGING_TILL_READ
} Sdp32Averaging;

// Sensor Commands
typedef enum {
  /// Undefined dummy command.
  COMMAND_UNDEFINED                       	= 0x0000,
  /// Start continous measurement                     	\n
  /// Temperature compensation: Mass flow             	\n
  /// Averaging: Average till read
  COMMAND_START_CONT_MEASURMENT_MF_AVERAGE     	= 0x3603,
  /// Start continous measurement                     	\n
  /// Temperature compensation: Mass flow             	\n
  /// Averaging: None - Update rate 1ms
  COMMAND_START_CONT_MEASURMENT_MF_NONE        	= 0x3608,
  /// Start continous measurement                     	\n
  /// Temperature compensation: Differential pressure 	\n
  /// Averaging: Average till read
  COMMAND_START_CONT_MEASURMENT_DP_AVERAGE     	= 0x3615,
  /// Start continous measurement                     	\n
  /// Temperature compensation: Differential pressure 	\n
  /// Averaging: None - Update rate 1ms
  COMMAND_START_CONT_MEASURMENT_DP_NONE        	= 0x361E,
  // Stop continuous measurement.						
  COMMAND_STOP_CONTINOUS_MEASUREMENT      	= 0x3FF9,
  /// Start Triggered measurement		  	\n
  /// Temperature compensation: Mass flow             	\n
  /// Clock stretching: No
  COMMAND_START_TRIG_MEASUREMENT_MF_NONE	= 0X3624,
  /// Start Triggered measurement		  	\n
  /// Temperature compensation: Mass flow             	\n
  /// Clock stretching: Yes
  COMMAND_START_TRIG_MEASUREMENT_MF_CS		= 0X3626,
    /// Start Triggered measurement		  	\n
  /// Temperature compensation: Differential pressure  	\n
  /// Clock stretching: No
  COMMAND_START_TRIG_MEASUREMENT_DP_NONE	= 0X362F,
  /// Start Triggered measurement		  	\n
  /// Temperature compensation: Differential pressure  	\n
  /// Clock stretching: Yes
  COMMAND_START_TRIG_MEASUREMENT_DP_CS		= 0X362D,
  // Soft RESET command.					
  COMMAND_SOFT_RESET      			= 0x0006,
  // Entering Sleep mode command.					
  COMMAND_ENTER_SLEEP_MODE 			= 0x3677,
  // Commands for reading Product Identifier						
  COMMAND_READ_PRODUCT_IDENTIFIER_1		= 0x367C,
  COMMAND_READ_PRODUCT_IDENTIFIER_2		= 0xE102,
} sdp32_command;

typedef enum SDP32_INTF_RET_TYPE
{
    /**\name API success code */
    SDP32_SUCCESS             = INT8_C(0),

    /**\name API error codes */
    SDP32_E_NULL_PTR          = INT8_C(-1),
    SDP32_E_DEV_NOT_FOUND     = INT8_C(-2),
    SDP32_E_INVALID_COMMAND   = INT8_C(-3),
    SDP32_E_COMM_FAIL         = INT8_C(-4)
}SDP32_INTF_RET_TYPE;

/*!
 * @brief Bus communication function pointer which should be mapped to
 * the platform specific read functions of the user
 *
 * @param[in] command        : Command to be sent to Sensor
 * @param[out] reg_data      : Pointer to data buffer where read data is stored.
 * @param[in] len            : Number of bytes of data to be read.
 * @param[in, out] intf_ptr  : Void pointer that can enable the linking of descriptors
 *                                  for interface related call backs.
 *
 * @retval   0 -> Success.
 * @retval Non zero value -> Fail.
 *
 */
typedef SDP32_INTF_RET_TYPE (*sdp32_read_fptr_t)(uint8_t *data, uint32_t len, void *intf_ptr);
/*!
 * @brief Bus communication function pointer which should be mapped to
 * the platform specific read functions of the user
 *
 * @param[in] command        : Command to be sent to Sensor
 * @param[in, out] intf_ptr  : Void pointer that can enable the linking of descriptors
 *                                  for interface related call backs.
 *
 * @retval   0 -> Success.
 * @retval Non zero value -> Fail.
 *
 */
typedef SDP32_INTF_RET_TYPE (*sdp32_write_fptr_t)(sdp32_command command, void *intf_ptr);

/*!
 * @brief Delay function pointer which should be mapped to
 * delay function of the user
 *
 * @param[in] period              : Delay in microseconds.
 * @param[in, out] intf_ptr       : Void pointer that can enable the linking of descriptors
 *                                  for interface related call backs (Not used)
 *
 */
typedef void (*sdp32_delay_us_fptr_t)(uint32_t period, void *intf_ptr);

/*!
 * @brief SDP32 device structure
 */
typedef struct sdp32_dev
{
    /*< Interface function pointer used to enable the device address for I2C and chip selection for SPI */
    void *intf_ptr;

    /* Product Number of the chip */
    uint32_t prod_num;

    /*< Read function pointer */
    sdp32_read_fptr_t read;

    /*< Write function pointer */
    sdp32_write_fptr_t write;

    /*< Delay function pointer */
    sdp32_delay_us_fptr_t delay_us;

    /*< Variable to store result of read/write function */
    SDP32_INTF_RET_TYPE intf_rslt;
}SDP32_DEV;

int8_t sdp32_init(SDP32_DEV *dev);

int8_t sdp32_send_command(sdp32_command command, SDP32_DEV *dev);

int8_t sdp32_get_data(uint8_t *data, uint32_t len, SDP32_DEV *dev);

int8_t sdp32_SoftReset(SDP32_DEV *dev);

#ifdef __cplusplus
}
#endif /* End of CPP guard */
#endif /* SDP32_H_ */
/** @}*/
