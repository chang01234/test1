/*! \file drv_lis2dh12.h
 *	\brief LIS2DH12 driver header file.
 */

#ifndef LIS2DH12_SEQ_H_
#define LIS2DH12_SEQ_H_

#include <stdint.h>
#include <stdbool.h>

/* LIS2DH12 sensor sequencer ID */
#define LIS2DH12_SEQ_ID                           0x23

/**\name API success code */
#define LIS2DH12_OK                               INT8_C(0)

/**\name API error codes */
#define LIS2DH12_E_NULL_PTR                       INT8_C(-1)
#define LIS2DH12_E_DEV_NOT_FOUND                  INT8_C(-2)
#define LIS2DH12_E_INVALID_LEN                    INT8_C(-3)
#define LIS2DH12_E_COMM_FAIL                      INT8_C(-4)

/* Axis Threshold range */
#define LIS2DH12_TAP_THRHLD_AXIS_MIN                          0  // Same for all 3 axis (X, Y & Z)
#define LIS2DH12_TAP_THRHLD_AXIS_MAX                          0x1F // Same for all 3 axis (X, Y & Z)

/* Axis Tap Recognition range */
#define LIS2DH12_TAP_RECOG_ALL_AXIS_MIN                      0
#define LIS2DH12_TAP_RECOG_ALL_AXIS_MAX                      0xE  //recognize all exis

/* INTERRUPT for event range */
#define LIS2DH12_INT1_EVENT_ENABLE_MIN                        0     //interrupt enabled reg
#define LIS2DH12_INT1_EVENT_ENABLE_MAX                        0x1F

/* WAKEUP Threshold range */
#define LIS2DH12_WAKEUP_THRESHOLD_MIN                         0  // Same for all 3 axis (X, Y & Z)
#define LIS2DH12_WAKEUP_THRESHOLD_MAX                         0x3F // Same for all 3 axis (X, Y & Z)

/* WAKEUP Duration range - Wakeup duration. 1 LSB = 1 *1/ODR */
#define LIS2DH12_WAKEUP_DURATION_MIN                          0 
#define LIS2DH12_WAKEUP_DURATION_MAX                          0x03 //TODO: Review the Max value again

/* FREEFALL Duration range - Freefall duration. 1 LSB = 1 *1/ODR */
#define LIS2DH12_FREEFALL_DURATION_MIN                        0
#define LIS2DH12_FREEFALL_DURATION_MAX                        0xF8

/* Double tap interval/latency duration range - Double Tap */
#define LIS2DH12_DT_INTERVAL_DURATION_MIN                     0     //latency time set for double tap
#define LIS2DH12_DT_INTERVAL_DURATION_MAX                     0x0F

/* Double tap shock duration range - Double Tap */
#define LIS2DH12_DT_SHOCK_DURATION_MIN                        0     //shock duration set for double tap
#define LIS2DH12_DT_SHOCK_DURATION_MAX                        0x03

/* Double tap shock duration range - Double Tap */
#define LIS2DH12_DT_QUIET_TIME_MIN                            0     //quiet time set for double tap
#define LIS2DH12_DT_QUIET_TIME_MAX                            0x03

/**
 * LIS2DH12_INTF_RET_TYPE is the read/write interface return type which can be overwritten by the build system.
 */
#ifndef LIS2DH12_INTF_RET_TYPE
#define LIS2DH12_INTF_RET_TYPE                      int32_t
#endif

/* Soft reset acts as reset for all control registers, then goes to 0.
 * Default value: 0 (0: disabled; 1: enabled)
 */
#define LIS2DH12_SOFT_RESET_ENABLE         1
#define LIS2DH12_SOFT_RESET_DISABLE        0

/* lis2dh12 user configuration parameter structure */
typedef struct lis2dh12_config_values
{
	uint8_t  ctrl_reg1;
    uint8_t  data_rate;			//ODR set
	uint8_t  low_power_mode;
	uint8_t  enable_xyz_axis;
	uint8_t  ctrl_reg2;			//High Pass filter mode config
	uint8_t  ctrl_reg3;			//Enable Event on Interrupt 1
	uint8_t  ctrl_reg4;
	uint8_t  block_data_update;
    uint8_t  full_scale;
    uint8_t  operating_mode;
	uint8_t  ctrl_reg5;
	uint8_t  fifo_enable;
	uint8_t	 latch_int1;
	uint8_t  ctrl_reg6;			//Enable Event on Interrupt 2
	uint8_t  fifo_ctrl_reg;
	uint8_t  int1_cfg;
	uint8_t  int1_ths;
	uint8_t  int1_duration;

    uint8_t  tap_ths_set;
    uint8_t  fds_path;
    uint8_t  event_set;
    uint8_t  tilt_ths_set;
    uint8_t  tap_rec_axis;
    uint8_t  axis_priority;
    uint8_t  ff_ths_set;
    uint8_t  ff_dur_set;
    uint8_t  wkup_ths_set;
    uint8_t  wkup_dur_set;
    uint8_t  i2c_comm;
    uint8_t  enable_ST_DT;
    uint8_t  double_tap_dur;
    uint8_t  double_tap_quiet;
    uint8_t  double_tap_shock;
    uint8_t  Tap_mode;	
} lis2dh12_config_values;

/* lis2dh12 sensor configuration paramater TYPE identifiers */
typedef enum lis2dh12_config_types
{ 
    LIS2DH12_TYPE_SOFT_RESET = 0x30,
    LIS2DH12_TYPE_DATA_RATE,
    LIS2DH12_TYPE_LOW_POWER_MODE,
    LIS2DH12_TYPE_ENABLE_XYZ_AXIS,
    LIS2DH12_TYPE_BLOCK_DATA_UPDATE,
    LIS2DH12_TYPE_FULL_SCALE,
    LIS2DH12_TYPE_OPERATING_MODE,
    LIS2DH12_TYPE_FIFO_ENABLE,
    LIS2DH12_TYPE_CONF_CTRL_REG1,
    LIS2DH12_TYPE_CONF_CTRL_REG2,
    LIS2DH12_TYPE_CONF_CTRL_REG3,
    LIS2DH12_TYPE_CONF_CTRL_REG4,
    LIS2DH12_TYPE_CONF_CTRL_REG5,
    LIS2DH12_TYPE_CONF_CTRL_REG6,
    LIS2DH12_TYPE_FDS,
    LIS2DH12_TYPE_EVENT,
    LIS2DH12_TYPE_TILT_THS_SET,
    LIS2DH12_TYPE_TAP_THS_SET,
    LIS2DH12_TYPE_TAP_RECG_DIRECTION,
    LIS2DH12_TYPE_AXIS_PRIORITY,
    LIS2DH12_TYPE_FF_THS,
    LIS2DH12_TYPE_FF_DUR,
    LIS2DH12_TYPE_WAKEUP_THS,
    LIS2DH12_TYPE_WAKEUP_DUR,
    LIS2DH12_TYPE_I2C_COMM,
    LIS2DH12_TYPE_DOUBLE_TAP_DUR,
    LIS2DH12_TYPE_DT_QUIET_TIME,
    LIS2DH12_TYPE_DT_SHOCK_DURATION,
    LIS2DH12_TYPE_BOTH_SINGLE_DOUBLE_TAP,
}lis2dh12_config_types;

// List of events supported from LIS2DH12 sensor and posted to user upon sensing. 
typedef enum lis2dh12_param_edm_events
{
    LIS2DH12_PARAM_EDM_SINGLE_TAP = 1,        // Single Tap event
    LIS2DH12_PARAM_EDM_DOUBLE_TAP,            // Double Tap event (Currently it's not supported)
    LIS2DH12_PARAM_EDM_TILT,                  // Tilt event
    LIS2DH12_PARAM_EDM_TAMPERING,             // Tampering event: Same as ST but with high threshold
    LIS2DH12_PARAM_EDM_MOTION,                // Motion event
    LIS2DH12_PARAM_EDM_FREEFALL               // Free fall event
}lis2dh12_param_edm_events;  

/* Sensor specific enums which are not covered in reg.h */
typedef enum 
{
    LIS2DH12_conti_update    = 0,     // 0: continuous update
    LIS2DH12_no_update_until    = 1,  // 1: output registers not updated until MSB and LSB read
} lis2dh12_dbm_t;

typedef enum 
{
    LIS2DH12_tap_recogn_disable    = 0,      // 0: Disable
    LIS2DH12_tap_recogn_enable     = 1,     // 1: Enable
} lis2dh12_tap_recogn;

typedef enum 
{
    LIS2DH12_interrupt_disable    = 0,      // 0: Disable
    LIS2DH12_interrupt_enable     = 1,     // 1: Enable
} lis2dh12_interrupt_set;


// TILT event threshold settings
typedef enum 
{
    LIS2DH12_6D_THRHLD_80DEGREE    = 0,      // 0: 80 degree - 6 thresold decoding
    LIS2DH12_6D_THRHLD_70DEGREE,             // 1: 70 degree - 11 thresold decoding 
    LIS2DH12_6D_THRHLD_60DEGREE,             // 2: 60 degree - 16 thresold decoding
    LIS2DH12_6D_THRHLD_50DEGREE              // 3: 50 degree - 21 thresold decoding
} lis2dh12_6d_threshold;

// List of events supported from LIS2DH12 sensor. 
typedef enum lis2dh12_config_edm_events
{
    LIS2DH12_CFG_EDM_NO_EVENT = 0,      // No event
    LIS2DH12_CFG_EDM_SINGLE_TAP,        // Single Tap event
    LIS2DH12_CFG_EDM_DOUBLE_TAP,        // Double Tap event
    LIS2DH12_CFG_EDM_TILT,              // Tilt event
    LIS2DH12_CFG_EDM_TAMPERING,         // Tampering event
    LIS2DH12_CFG_EDM_MOTION,            // Motion event(wake up)
    LIS2DH12_CFG_EDM_FREEFALL,          // Free fall event
    LIS2DH12_CFG_EDM_ALL_EVENTS         // Enabling all events (ST or Tampering, TILT, Motion & FF)
} lis2dh12_config_edm_events;  

// List of events supported from LIS2DH12 sensor for connector.
typedef enum accelerometer_event
{
    None = 0,      	// No event
    Tampering,      // Tampering event
    Moving,         // Motion event(wake up)
    Fall_Detection, // Free fall event
    Single_Tapping, // Single Tap event
    Double_Tapping, // Double Tap event
    Tilting,        // Tilt event
    motion_and_tap  // Enabling Motion & ST
} acc_event_t;

// List of user readable parameters from LIS2DH12 sensor
typedef enum lis2dh12_read_params
{
    LIS2DH12_MEAS_TYPE_WHOAMI = 0,  // device ID of sensor
    LIS2DH12_MEAS_TYPE_TEMP,        // Temperature data in deg celsius from sensor
    LIS2DH12_MEAS_TYPE_ACCEL,       // Acceleration(X, Y & Z axis) data in mg from sensor
} lis2dh12_read_params;

//TODO: These are placeholders and should be removed once proper parameters are ready.
typedef enum lis2dh12_config_params
{
    LIS2DH12_SET_OPERATING_MODE = 0,
    LIS2DH12_SET_DATA_RATE,
    LIS2DH12_SET_FULL_SCALE,
    LIS2DH12_SET_FIFO_ENABLE,
    LIS2DH12_SET_CTRL_REG1,
    LIS2DH12_SET_CTRL_REG2,
    LIS2DH12_SET_CTRL_REG3,
    LIS2DH12_SET_CTRL_REG4,
    LIS2DH12_SET_CTRL_REG5,
    LIS2DH12_SET_CTRL_REG6,
    LIS2DH12_SET_INT1_THS,
    LIS2DH12_SET_INT1_DURATION,
    LIS2DH12_SET_INT1_CFG,
    LIS2DH12_SET_INT2_THS,
    LIS2DH12_SET_INT2_DURATION,
    LIS2DH12_SET_INT2_CFG,
    LIS2DH12_SET_OFFSET_WEIGHT,
    LIS2DH12_SET_SELF_TEST,
    LIS2DH12_SET_DATA_READYMODE,
    LIS2DH12_SET_FILTER_PATH,
    LIS2DH12_SET_FILTER_BANDWIDTH,
    LIS2DH12_SET_SPI_MODE,          // It's not supported in this version as sensor commn is I2C based
    LIS2DH12_SET_I2C_INTERFACE,
    LIS2DH12_SET_CS_MODE,           // It's not supported in this version as sensor commn is I2C based
    LIS2DH12_SET_PIN_POLARITY,
    LIS2DH12_SET_INT1_NOTIFICATION,
    LIS2DH12_SET_INT2_NOTIFICATION,
    LIS2DH12_SET_PIN_MODE,
    LIS2DH12_SET_WKUP_FEED_DATA,
    LIS2DH12_SET_ACT_MODE,
    LIS2DH12_SET_TAP_AXIS_PRIORITY,
    LIS2DH12_SET_TAP_MODE,
    LIS2DH12_SET_6D_FEED_DATA,
    LIS2DH12_SET_FF_THRESHOLD,
    LIS2DH12_SET_FIFO_MODE,
    LIS2DH12_SET_DATA_BLOCK_MODE,
    LIS2DH12_SET_INTR1_ROUTE_SET, // Added a seperate function itself
    LIS2DH12_SET_TAP_THRHLD,
    LIS2DH12_SET_WKUP_THRESHOLD,
    LIS2DH12_SET_WKUP_DURATION,
    LIS2DH12_SET_6D_THRESHOLD,  // TILT event threshold setting
    LIS2DH12_SET_FF_DURATION,   // FF Duration
    LIS2DH12_SET_DT_DURATION,   // DT interval
    LIS2DH12_SET_DT_QUIET_TIME,
    LIS2DH12_SET_DT_SHOCK_DURATION,
}lis2dh12_config_params;

/* LIS2DH12 sensor bus config type - I2C or SPI */
typedef enum lis2dh12_bus_conf_type
{
    LIS2DH12_BUS_CONF_TYPE_I2C,
    LIS2DH12_BUS_CONF_TYPE_SPI,
} lis2dh12_bus_conf_type;

/* LIS2DH12 sensor I2C configuration parameters */
typedef struct lis2dh12_i2c_conf
{
    uint8_t  port;
    uint32_t sda;
    uint32_t scl;
    uint32_t bitrate;
} lis2dh12_i2c_conf;

/* LIS2DH12 sensor GPIO configuration parameters - Required for interrupt */
typedef struct lis2dh12_gpio_conf
{
    int device;
    int port;
    int pin;
} lis2dh12_gpio_conf;

typedef struct lis2dh12_bus_conf
{
    lis2dh12_bus_conf_type type; // Bus type for sensor communication
    union 
    {
        lis2dh12_i2c_conf i2c; // Currently only I2C communication to LIS sensor is supported
      //bme280_spi_conf spi;
    };
    lis2dh12_gpio_conf gpio;  // GPIO config for interrupt
} lis2dh12_bus_conf;

/* LIS2DH12 sequencer function declarations - scheduler exposure */
typedef void (*LIS2DH12_SEQ_CB) (uint16_t seq_id, uint16_t parameter);

int lis2dh12_seq_init(const lis2dh12_bus_conf *bus_conf, LIS2DH12_SEQ_CB cb);

int lis2dh12_seq_read(lis2dh12_read_params parameter, void *out);

int lis2dh12_seq_conf(const uint8_t *data);

int lis2dh12_seq_reset(void);

/* LIS2DH12 sequencer function declarations - for internal usage */
int LIS2DH12_SeqConfig_Set(uint16_t parameter, const void *in);

uint8_t LIS2DH12_SeqConfig_Parser(uint8_t *p_payload); 

uint8_t LIS2DH12_User_Event_Check(uint8_t *sens_cfg_val);

uint8_t LIS2DH12_Default_SeqConfig();

/* Extra Function */
void enableInt1click(void);
void enableTapDetection();
bool isTapped(void);
void reading_status_reg();
int32_t polling_for_acc_data(int16_t *raw_data);
bool getInt1(void);
void enable_wakeup_event();
void enable_freefall_event();
void enable_tap_event();
void enable_tilt_event();
void enable_motion_and_tap_event();
void disable_all_event();

#endif /* LIS2DH12_SEQ_H_ */
