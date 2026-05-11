/*! \file connector_uart_ddm.c
    \brief DDMP2<->DDM UART connector
*/
#include <stdint.h>
#include <string.h>

#include "configuration.h"
#include "connector_uart_ddm.h"
#include "ddm2_parameter_list.h"

#include "esp_attr.h"
#include "esp_idf_version.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 3, 0)
#include "driver/uart.h"  // UART port and configuration definition
#elif ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include "driver/myuart.h"  // UART port and configuration definition
#else
#include "driver/uart.h"  // UART port and configuration definition
#endif

#include "ddmp.h"
#include "driver/gpio.h"

#include "broker.h"
#include "ddm2.h"
#include "esp_log.h"

#ifndef CONNECTOR_UART_DDM_CPU_CORE
#define CONNECTOR_UART_DDM_CPU_CORE ((BaseType_t)tskNO_AFFINITY)
#endif
#ifndef CONNECTOR_UART_DDM_PROCESS_CPU_CORE
#define CONNECTOR_UART_DDM_PROCESS_CPU_CORE CONNECTOR_UART_DDM_CPU_CORE
#endif

#define IF_UART_1             0
#define CC_PARAMETER_COUNT    40
#define DDMP2_PARAMETER_COUNT 23
#define COMPART_NUM0          0
#define COMPART_NUM1          1
#define MEASURED_TEMP0        1
#define MEASURED_TEMP1        2
#define SET_TEMP0             3
#define SET_TEMP1             4
#define COMP_POWER            5
#define COOLER_POWER          6
#define C0_POWER              7
#define C1_POWER              8
#define CONVERSION_POW        4
#define CONVERSION_MESURETEMP 6
#define CONVERSION_SETTEMP    8
#define CONVERSION_OFFTEMP    17
#define COUNT_RANGE_16BIT     0xFFFF
#define COUNT_RANGE_MAX       0x10000
#define COUNT_RANGE_32BIT     0x00000000FFFFFFFF
#define MOVE_16BIT            16
#define MOVE_32BIT            32
#define TEMP_OUT_RANGE_4C     40
#define TEMP_NEG_40C          -400

#if CONFIG_UART_ISR_IN_IRAM
#warning "Placing UART ISR in IRAM section is forbidden when caching is enabled"
#endif

#ifndef CONNECTOR_UART_DDM_EXTENDED_LOG
#define CONNECTOR_UART_DDM_EXTENDED_LOG 0
#endif

#if CONNECTOR_UART_DDM_EXTENDED_LOG
#define UART_DDM_LOG(level, format, ...)               LOG(level, format, __VA_ARGS__)
#define UART_DDM_HEXDUMP(tag, buffer, buff_len, level) ESP_LOG_BUFFER_HEXDUMP(tag, buffer, buff_len, level)
#else  // CONNECTOR_UART_DDM_EXTENDED_LOG
#define UART_DDM_LOG(level, format, ...)               ((void)0)
#define UART_DDM_HEXDUMP(tag, buffer, buff_len, level) ((void)0)
#endif  // CONNECTOR_UART_DDM_EXTENDED_LOG

#ifndef CONNECTOR_UART_DDM_DRIVER_QUEUE_SIZE
#define CONNECTOR_UART_DDM_DRIVER_QUEUE_SIZE 8
#endif

typedef struct DDM_CONVERSION_DATA
{
    uint32_t ddm_parameter;   //!< \~ DDM Parameter ID
    uint32_t ddm2_parameter;  //!< \~ DDM Parameter ID
    DDMP_TYPES_ENUM type;     //!< \~ Parameter value type
} DDM_CONVERSION_DATA;

typedef struct DDM2_STRUCT_DATA
{
    int32_t struct_data0;
    int32_t struct_data1;
} DDM2_STRUCT_DATA;

typedef struct DDM2_STRUCT_RANGE_DATA
{
    int64_t struct_range_data0;
    int64_t struct_range_data1;
} DDM2_STRUCT_RANGE_DATA;

typedef enum
{
    POWER_COVER,
    MEASURED_TEMP_COVER,
    SET_TEMP_COVER,
    DOOR_COVER,
    TEMP_OFFSET_COVER,
    COVER_COUNT,
} STRUCT_DATA_DDM2;

typedef enum
{
    TEMP_RANG_COVER,
    RECOMME_RANG_COVER,
    RANGECOVER_COUNT,
} STRUCT_RANGEDATA_DDM2;

typedef enum
{
    NTC0_OPEN,
    NTC0_SHORT,
    NTC1_OPEN,
    NTC1_SHORT,
    SOLEN_VALVE,
    FAN_OVERCUR,
    COMP_STAR_FAIL,
    COMP_SPEED,
    COMP_OVERTEMP,
    TEMP_ALERT,
    DOOR_ALERT,
    VOL_ALERT,
    ERROR_COUNT,
} STRUCT_ERROR_DDM2;

typedef struct TEMP_ALERT_STRUCT
{
    bool b_temp_alert_step1;
    bool b_temp_alert_step2;
    bool b_temp_alert_step3;
    uint8_t temp_alert_form;
    uint8_t temp_alert_form_cache;
    int16_t temp_alert_cache;
} TEMP_ALERT_STRUCT;

static QueueHandle_t uart_queue;                              //!< \~ Handles to UART1 event queue
static TimerHandle_t timeout_timers[DDMP_INTERFACE_COUNT];    //!< \~ DDMP communication timeout timers
static SemaphoreHandle_t ddmp_mutexes[DDMP_INTERFACE_COUNT];  //!< \~ Mutexes to protect ddmp stacks
static SemaphoreHandle_t global_ddmp_mutex;                   //!< \~ Mutex to protect data store
static int cfxcc_instance = -1;                               //!< \~ Class instance from broker
static int cfxprod_instance = -1;                             //!< \~ Class instance from broker

static uint8_t ddm2_nocpt = 0;
static DDM2_STRUCT_DATA struct_data_send[COVER_COUNT];
static DDM2_STRUCT_RANGE_DATA struct_rangedata_send[RANGECOVER_COUNT];
static uint16_t error_value_send[ERROR_COUNT] = {0};
static uint16_t error_value[ERROR_COUNT] = {0};

static const char *cfxprod_descr = "CFX5 compressor controller board";
static const char *cfxprod_manuf = "Dometic";

static const SORTED_LIST_ENTRY Ddm_parameter_lookup_data[CC_PARAMETER_COUNT] = {
    //!< \~ DDM parameter ID -> conversion table index
    {DDM_PRODUCT_MODEL_NUMBER, 0},
    {DDM_PRODUCT_SERIAL_NUMBER, 1},
    {DDM_PRODUCT_TYPE, 2},
    {DDM_PRODUCT_HINGE_POSITION, 3},
    {DDM_C0_POWER, 4},
    {DDM_C1_POWER, 5},
    {DDM_C0_MEASURED_TEMPERATURE, 6},
    {DDM_C1_MEASURED_TEMPERATURE, 7},
    {DDM_C0_SET_TEMPERATURE, 8},
    {DDM_C1_SET_TEMPERATURE, 9},
    {DDM_ACTIVE_COMPARTMENT, 10},
    {DDM_C0_DOOR_OPEN, 11},
    {DDM_C1_DOOR_OPEN, 12},
    {DDM_C0_TEMPERATURE_RANGE, 13},
    {DDM_C1_TEMPERATURE_RANGE, 14},
    {DDM_C0_RECOMMENDED_RANGE, 15},
    {DDM_C1_RECOMMENDED_RANGE, 16},
    {DDM_C0_TEMPERATURE_OFFSET, 17},
    {DDM_C1_TEMPERATURE_OFFSET, 18},
    {DDM_COOLER_POWER, 19},
    {DDM_BATTERY_VOLTAGE_LEVEL, 20},
    {DDM_BATTERY_PROTECTION_LEVEL, 21},
    {DDM_COMPRESSOR_POWER, 22},
    {DDM_DC_CURRENT_LEVEL, 23},
    {DDM_POWER_SOURCE, 24},
    {DDM_ICEMAKER_POWER, 25},
    {DDM_C0_NTC_OPEN_ERROR, 26},
    {DDM_C0_NTC_SHORT_ERROR, 27},
    {DDM_SOLENOID_VALVE_ERROR, 28},
    {DDM_C1_NTC_OPEN_ERROR, 29},
    {DDM_C1_NTC_SHORT_ERROR, 30},
    {DDM_FAN_OVERCURRENT_ERROR, 31},
    {DDM_COMPRESSOR_START_FAIL_ERROR, 32},
    {DDM_COMPRESSOR_SPEED_ERROR, 33},
    {DDM_CONTROLLER_OVERTEMP_ERROR, 34},
    {DDM_TEMPERATURE_ALERT_CC, 35},
    {DDM_DOOR_ALERT, 36},
    {DDM_VOLTAGE_ALERT, 37},
    {DDM_CC_SERIAL_NUMBER, 38},
    {DDM_CC_FIRMWARE_VERSION, 39},
};

static const SORTED_LIST_ENTRY Ddm2_parameter_lookup_data[DDMP2_PARAMETER_COUNT] = {
    //!< \~ DDM2 parameter ID -> conversion table index
    {MCCC0PTYPE, 2},
    {MCCC0NOCPT, 2},
    {MCCC0CPOW, 4},
    {MCCC0CTEMP, 6},
    {MCCC0CSETTEMP, 8},
    {MCCC0ACPT, 10},
    {MCCC0CDOOR, 11},
    {MCCC0CTEMPRNG, 13},
    {MCCC0CRECDRNG, 15},
    {MCCC0CTEMPOFS, 17},
    {MCCC0COOLERPOW, 19},
    {MCCC0V, 20},
    {MCCC0BATPROTLVL, 21},
    {MCCC0COMPPOW, 22},
    {MCCC0I, 23},
    {MCCC0POWSRC, 24},
    {MCCC0ICEPOW, 25},
    {MCCC0ERRST, 26},
    {MCCC0SN, 38},
    //    {MCCC0SKU, 1},
    {MCCC0FWVER, 39},
    {PROD0SN, 1},
    {PROD0FWVER, 39},
    {PROD0MDL, 0},
};

static const SORTED_LIST Ddm_parameter_lookup_list = {
    //!< \~ Sorted list for DDM parameter lookup
    .pdata = (SORTED_LIST_ENTRY *)Ddm_parameter_lookup_data,
    .capacity = CC_PARAMETER_COUNT,
    .entry_count = CC_PARAMETER_COUNT,
};

static const SORTED_LIST Ddm2_parameter_lookup_list = {
    //!< \~ Sorted list for DDM2 parameter lookup
    .pdata = (SORTED_LIST_ENTRY *)Ddm2_parameter_lookup_data,
    .capacity = DDMP2_PARAMETER_COUNT,
    .entry_count = DDMP2_PARAMETER_COUNT,
};

static const DDM_CONVERSION_DATA Ddm_conversion_data[DDM_PARAMETER_COUNT] = {
    //!< \~ DDM<->DDM2 parameter and type conversion table
    {
        DDM_PRODUCT_MODEL_NUMBER,
        PROD0MDL,
        DDMP_TYPE_STRING,
    },
    {
        DDM_PRODUCT_SERIAL_NUMBER,
        PROD0SN,
        DDMP_TYPE_STRING,
    },
    {
        DDM_PRODUCT_TYPE,
        MCCC0PTYPE,
        DDMP_TYPE_UINT8,
    },
    {
        DDM_PRODUCT_HINGE_POSITION,
        MCCC0SKU,
        DDMP_TYPE_UINT8,
    },
    {
        DDM_C0_POWER,
        MCCC0CPOW,
        DDMP_TYPE_UINT8,
    },
    {
        DDM_C1_POWER,
        MCCC0CPOW,
        DDMP_TYPE_UINT8,
    },
    {
        DDM_C0_MEASURED_TEMPERATURE,
        MCCC0CTEMP,
        DDMP_TYPE_INT16,
    },
    {
        DDM_C1_MEASURED_TEMPERATURE,
        MCCC0CTEMP,
        DDMP_TYPE_INT16,
    },
    {
        DDM_C0_SET_TEMPERATURE,
        MCCC0CSETTEMP,
        DDMP_TYPE_INT16,
    },
    {
        DDM_C1_SET_TEMPERATURE,
        MCCC0CSETTEMP,
        DDMP_TYPE_INT16,
    },
    {
        DDM_ACTIVE_COMPARTMENT,
        MCCC0ACPT,
        DDMP_TYPE_UINT8,
    },
    {
        DDM_C0_DOOR_OPEN,
        MCCC0CDOOR,
        DDMP_TYPE_UINT8,
    },
    {
        DDM_C1_DOOR_OPEN,
        MCCC0CDOOR,
        DDMP_TYPE_UINT8,
    },
    {
        DDM_C0_TEMPERATURE_RANGE,
        MCCC0CTEMPRNG,
        DDMP_TYPE_ARRAY,
    },
    {
        DDM_C1_TEMPERATURE_RANGE,
        MCCC0CTEMPRNG,
        DDMP_TYPE_ARRAY,
    },
    {
        DDM_C0_RECOMMENDED_RANGE,
        MCCC0CRECDRNG,
        DDMP_TYPE_ARRAY,
    },
    {
        DDM_C1_RECOMMENDED_RANGE,
        MCCC0CRECDRNG,
        DDMP_TYPE_ARRAY,
    },
    {
        DDM_C0_TEMPERATURE_OFFSET,
        MCCC0CTEMPOFS,
        DDMP_TYPE_INT16,
    },
    {
        DDM_C1_TEMPERATURE_OFFSET,
        MCCC0CTEMPOFS,
        DDMP_TYPE_INT16,
    },
    {
        DDM_COOLER_POWER,
        MCCC0COOLERPOW,
        DDMP_TYPE_UINT8,
    },
    {
        DDM_BATTERY_VOLTAGE_LEVEL,
        MCCC0V,
        DDMP_TYPE_INT16,
    },
    {
        DDM_BATTERY_PROTECTION_LEVEL,
        MCCC0BATPROTLVL,
        DDMP_TYPE_UINT8,
    },
    {
        DDM_COMPRESSOR_POWER,
        MCCC0COMPPOW,
        DDMP_TYPE_UINT8,
    },
    {
        DDM_DC_CURRENT_LEVEL,
        MCCC0I,
        DDMP_TYPE_INT16,
    },
    {
        DDM_POWER_SOURCE,
        MCCC0POWSRC,
        DDMP_TYPE_UINT8,
    },
    {
        DDM_ICEMAKER_POWER,
        MCCC0ICEPOW,
        DDMP_TYPE_UINT8,
    },
    {
        DDM_C0_NTC_OPEN_ERROR,
        MCCC0ERRST,
        DDMP_TYPE_UINT8,
    },
    {
        DDM_C0_NTC_SHORT_ERROR,
        MCCC0ERRST,
        DDMP_TYPE_UINT8,
    },
    {
        DDM_SOLENOID_VALVE_ERROR,
        MCCC0ERRST,
        DDMP_TYPE_UINT8,
    },
    {
        DDM_C1_NTC_OPEN_ERROR,
        MCCC0ERRST,
        DDMP_TYPE_UINT8,
    },
    {
        DDM_C1_NTC_SHORT_ERROR,
        MCCC0ERRST,
        DDMP_TYPE_UINT8,
    },
    {
        DDM_FAN_OVERCURRENT_ERROR,
        MCCC0ERRST,
        DDMP_TYPE_UINT8,
    },
    {
        DDM_COMPRESSOR_START_FAIL_ERROR,
        MCCC0ERRST,
        DDMP_TYPE_UINT8,
    },
    {
        DDM_COMPRESSOR_SPEED_ERROR,
        MCCC0ERRST,
        DDMP_TYPE_UINT8,
    },
    {
        DDM_CONTROLLER_OVERTEMP_ERROR,
        MCCC0ERRST,
        DDMP_TYPE_UINT8,
    },
    {
        DDM_TEMPERATURE_ALERT_CC,
        MCCC0ERRST,
        DDMP_TYPE_UINT8,
    },
    {
        DDM_DOOR_ALERT,
        MCCC0ERRST,
        DDMP_TYPE_UINT8,
    },
    {
        DDM_VOLTAGE_ALERT,
        MCCC0ERRST,
        DDMP_TYPE_UINT8,
    },
    {
        DDM_CC_SERIAL_NUMBER,
        MCCC0SN,
        DDMP_TYPE_STRING,
    },
    {
        DDM_CC_FIRMWARE_VERSION,
        MCCC0FWVER,
        DDMP_TYPE_STRING,
    },
};

static void connector_uart_process_task(void *Parameter);
static int ddmp_send_uart(int intf, void *data, size_t size);  //!< \~ UART send callback prototype

//! \~ DDMP interface definition list
const DDMP_INTERFACE interfaces[DDMP_INTERFACE_COUNT] = {
    [IF_UART_1] = {
        .type = DDMP_INTF_UART,
        .send_cb = ddmp_send_uart,
        .handle = UART_NUM_1,
    },
};

//! \~ Callback to start and stop frame timeout timer
void ddmp_timer_cb(int intf, DDMP_TIMER_ACTION_ENUM timer_action)
{
    BOUNDED(intf, DDMP_INTERFACE_COUNT);
    BOUNDED(timer_action, DDMP_TIMER_ACTION_COUNT);

    switch (timer_action)
    {
    case DDMP_TIMER_START:
        TRUE_CHECK(xTimerStart(timeout_timers[intf], portMAX_DELAY));
        break;
    case DDMP_TIMER_STOP:
        TRUE_CHECK(xTimerStop(timeout_timers[intf], portMAX_DELAY));
        break;
    default:
        break;
    }
}

static void error_check_send(uint32_t conver_parameter, uint8_t fram_value, STRUCT_ERROR_DDM2 error_num)
{
    uint8_t error_cnt = 0;
    if (fram_value)
    {
        switch (error_num)
        {
        case NTC0_OPEN:
            error_value[error_num] = MC_NTC_OPEN_CIRCUIT_ERROR;
            break;

        case NTC0_SHORT:
            error_value[error_num] = MC_NTC_SHORT_CIRCUIT_ERROR;
            break;

        case NTC1_OPEN:
            error_value[error_num] = MC_NTC1_OPEN_CIRCUIT_ERROR;
            break;

        case NTC1_SHORT:
            error_value[error_num] = MC_NTC1_SHORT_CIRCUIT_ERROR;
            break;

        case SOLEN_VALVE:
            error_value[error_num] = GENERIC_SOLENOID_VALVE_ERROR;
            break;

        case FAN_OVERCUR:
            error_value[error_num] = MC_COMPRESSOR_FAN_OVER_CURRENT_ERROR;
            break;

        case COMP_STAR_FAIL:
            error_value[error_num] = MC_COMPRESSOR_START_FAIL_ERROR;
            break;

        case COMP_SPEED:
            error_value[error_num] = MC_COMPRESSOR_SPEED_LOW_ERROR;
            break;

        case COMP_OVERTEMP:
            error_value[error_num] = MC_COMPRESSOR_OVER_TEMPERATURE_ERROR;
            break;

        case TEMP_ALERT:
            error_value[error_num] = GENERIC_TEMPERATURE_OUT_OF_RANGE_ERROR;
            break;

        case DOOR_ALERT:
            error_value[error_num] = GENERIC_DOOR_OPENED_ERROR;
            break;

        case VOL_ALERT:
            error_value[error_num] = GENERIC_DC_UNDER_VOLTAGE_ERROR;
            break;

        default:
            break;
        }
    }
    else
    {
        error_value[error_num] = 0;
    }

    for (int i = 0; i < ERROR_COUNT; i++)
    {
        if (error_value[i] != 0)
        {
            error_value_send[error_cnt] = error_value[i];
            error_cnt++;
        }
    }

    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, conver_parameter | DDM2_PARAMETER_INSTANCE(cfxcc_instance), &error_value_send, error_cnt * 2, connector_uart_ddm.connector_id, portMAX_DELAY);
}

static void ddm2_struct_send(uint32_t conver_parameter, uint8_t *value, uint8_t ddm2_nocpt_load, STRUCT_DATA_DDM2 struct_param, uint8_t noctpt_num, uint8_t factor_num)
{
    static uint8_t ddm2_send_size;
    ddm2_send_size = 4 * ddm2_nocpt_load;  // all DDM2 values are int32, hence 4 bytes
    if (noctpt_num == 0)
    {
        struct_data_send[struct_param].struct_data0 = factor_num * INT16(value);  // all DDM values are *10 and all DDM2 values are *1000, hence a factor 100 difference
    }
    else if (noctpt_num == 1)
    {
        struct_data_send[struct_param].struct_data1 = factor_num * INT16(value);  // all DDM values are *10 and all DDM2 values are *1000, hence a factor 100 difference
    }
    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, conver_parameter | DDM2_PARAMETER_INSTANCE(cfxcc_instance), &struct_data_send[struct_param], ddm2_send_size, connector_uart_ddm.connector_id, portMAX_DELAY);
}

static void ddm_to_ddm2_other(uint32_t conver_parameter, uint8_t *value, uint8_t ddm2_nocpt_load, STRUCT_RANGEDATA_DDM2 struct_param, uint8_t noctpt_num, uint8_t factor_num)
{
    int32_t value_range_min = 0;
    int64_t value_range_max = 0;
    int32_t value_range;
    static uint8_t ddm2_value_send_size;

    ddm2_value_send_size = 8 * ddm2_nocpt_load;
    value_range = INT32(value);
    value_range_min = ((value_range & COUNT_RANGE_16BIT) - (COUNT_RANGE_MAX)) * factor_num;
    value_range_max = ((value_range >> MOVE_16BIT) & COUNT_RANGE_16BIT) * factor_num;
    struct_rangedata_send[struct_param].struct_range_data0 = (value_range_min & COUNT_RANGE_32BIT) | (value_range_max << MOVE_32BIT);
    struct_rangedata_send[struct_param].struct_range_data1 = (value_range_min & COUNT_RANGE_32BIT) | (value_range_max << MOVE_32BIT);

    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, conver_parameter | DDM2_PARAMETER_INSTANCE(cfxcc_instance), &struct_rangedata_send[struct_param], ddm2_value_send_size, connector_uart_ddm.connector_id, portMAX_DELAY);
}

static void temp_alert_check_cx(int16_t temp_measure, int16_t temp_set, int32_t compressor_power, bool b_temp_alarm_reset, uint8_t cx_power)
{
    uint8_t temp_alert_send = 0;
    static TEMP_ALERT_STRUCT c0_alart_value, c1_alart_value;

    if (b_temp_alarm_reset)
    {
        b_temp_alarm_reset = false;
        c0_alart_value.b_temp_alert_step1 = false;
        c0_alart_value.b_temp_alert_step2 = false;
        c0_alart_value.b_temp_alert_step3 = false;
        c0_alart_value.temp_alert_form = 0;
        c1_alart_value.b_temp_alert_step1 = false;
        c1_alart_value.b_temp_alert_step2 = false;
        c1_alart_value.b_temp_alert_step3 = false;
        c1_alart_value.temp_alert_form = 0;
    }
    else
    {
        if (compressor_power)
        {
            if (temp_measure > temp_set)
            {
                if (cx_power == COMPART_NUM0)
                {
                    c0_alart_value.b_temp_alert_step1 = true;
                    c0_alart_value.temp_alert_cache = temp_measure - temp_set;
                }
                else
                {
                    c1_alart_value.b_temp_alert_step1 = true;
                    c1_alart_value.temp_alert_cache = temp_measure - temp_set;
                }
            }
        }
        else
        {
            if (temp_measure <= TEMP_NEG_40C)
            {
                if (cx_power == COMPART_NUM0)
                {
                    c0_alart_value.b_temp_alert_step2 = false;
                    c0_alart_value.temp_alert_form = 0;
                }
                else
                {
                    c1_alart_value.b_temp_alert_step2 = false;
                    c1_alart_value.temp_alert_form = 0;
                }
            }
            else
            {
                if (temp_measure <= temp_set)
                {
                    if (cx_power == COMPART_NUM0)
                    {
                        c0_alart_value.b_temp_alert_step2 = true;
                        c0_alart_value.temp_alert_cache = temp_set - temp_measure;
                    }
                    else
                    {
                        c1_alart_value.b_temp_alert_step2 = true;
                        c1_alart_value.temp_alert_cache = temp_set - temp_measure;
                    }
                }
            }

            if (cx_power == COMPART_NUM0)
            {
                if (c0_alart_value.b_temp_alert_step1)
                {
                    c0_alart_value.b_temp_alert_step3 = true;
                }
            }
            else
            {
                if (c1_alart_value.b_temp_alert_step1)
                {
                    c1_alart_value.b_temp_alert_step3 = true;
                }
            }
        }

        if (cx_power == COMPART_NUM0)
        {
            if (c0_alart_value.b_temp_alert_step2 && c0_alart_value.b_temp_alert_step3)
            {
                if (c0_alart_value.temp_alert_cache > TEMP_OUT_RANGE_4C)
                {
                    c0_alart_value.temp_alert_form = 1;
                }
                else
                {
                    c0_alart_value.temp_alert_form = 0;
                }
            }
        }
        else
        {
            if (c1_alart_value.b_temp_alert_step2 && c1_alart_value.b_temp_alert_step3)
            {
                if (c1_alart_value.temp_alert_cache > TEMP_OUT_RANGE_4C)
                {
                    c1_alart_value.temp_alert_form = 1;
                }
                else
                {
                    c1_alart_value.temp_alert_form = 0;
                }
            }
        }
    }

    if ((c0_alart_value.temp_alert_form_cache != c0_alart_value.temp_alert_form) || (c1_alart_value.temp_alert_form_cache != c1_alart_value.temp_alert_form))
    {
        c0_alart_value.temp_alert_form_cache = c0_alart_value.temp_alert_form;
        c1_alart_value.temp_alert_form_cache = c1_alart_value.temp_alert_form;

        if ((c0_alart_value.temp_alert_form_cache == 1) || (c1_alart_value.temp_alert_form_cache == 1))
        {
            temp_alert_send = 1;
        }
        else
        {
            temp_alert_send = 0;
        }
        error_check_send(MCCC0ERRST, temp_alert_send, TEMP_ALERT);
    }
}

static void temp_alert_check(uint8_t *input_data, uint8_t data_type)
{
    static int16_t compare_measure_temp0 = 0;
    static int16_t compare_measure_temp1 = 0;
    static int16_t compare_set_temp0 = 0;
    static int16_t compare_set_temp1 = 0;
    static int16_t comp_power = 0;
    static int16_t c0_power_val = 0;
    static int16_t c1_power_val = 0;
    static bool temp_alert_reset = false;

    switch (data_type)
    {
    case MEASURED_TEMP0:
        compare_measure_temp0 = INT16(input_data);
        temp_alert_reset = false;
        break;

    case MEASURED_TEMP1:
        compare_measure_temp1 = INT16(input_data);
        temp_alert_reset = false;
        break;

    case SET_TEMP0:
        compare_set_temp0 = INT16(input_data);
        temp_alert_reset = true;
        break;

    case SET_TEMP1:
        compare_set_temp1 = INT16(input_data);
        temp_alert_reset = true;
        break;

    case COMP_POWER:
        comp_power = INT16(input_data);
        temp_alert_reset = false;
        break;

    case COOLER_POWER:
        temp_alert_reset = true;
        break;

    case C0_POWER:
        c0_power_val = INT16(input_data);
        temp_alert_reset = true;
        break;

    case C1_POWER:
        c1_power_val = INT16(input_data);
        temp_alert_reset = true;
        break;
    }

    if (c0_power_val == 1)
    {
        temp_alert_check_cx(compare_measure_temp0, compare_set_temp0, comp_power, temp_alert_reset, COMPART_NUM0);
    }

    if (c1_power_val == 1)
    {
        temp_alert_check_cx(compare_measure_temp1, compare_set_temp1, comp_power, temp_alert_reset, COMPART_NUM1);
    }
}

//! \~ Incoming DDM frame callback: receive, convert and forward frames to broker
void ddmp_frame_cb(int intf, DDMP_FRAME *frame)
{
    BOUNDED(intf, DDMP_INTERFACE_COUNT);
    ASSERT(frame);

    SORTED_LIST_VALUE_TYPE conversion_table_index;
    SORTED_LIST_RETURN_VALUE ddm_lookup_result;
    size_t ddm2_value_size = 0;
    uint8_t ddm2_value_buffer[15];

    switch (frame->control)
    {
    case DDMP_ACTION_SUBSCRIBE:  // ignore subscriptions, CC will receive any and all PUB that are converted from SET
        break;
    case DDMP_ACTION_PUBLISH:                                                                                                                 // convert data and publish to broker
        ddm_lookup_result = sorted_list_unique_get(&conversion_table_index, (SORTED_LIST *)&Ddm_parameter_lookup_list, frame->parameter, 0);  // lookup DDM parameter

        if (ddm_lookup_result == SORTED_LIST_FAIL)
        {
            LOG(E, "Unrecognized Publish to DDM parameter %08x!", frame->parameter);
            return;
        }

        const DDM_CONVERSION_DATA *const Conversion_data = &Ddm_conversion_data[conversion_table_index];

        switch (Conversion_data->type)
        {
        case DDMP_TYPE_INT16:
            if (frame->int16 != MISSING_int16)  // filter out any MISSING DDM value as there is no equivalent in DDM2
            {
                switch (Conversion_data->ddm_parameter)
                {
                case DDM_BATTERY_VOLTAGE_LEVEL:
                case DDM_DC_CURRENT_LEVEL:
                    ddm2_value_size = 4;                                   // all DDM2 values are int32, hence 4 bytes
                    INT32(ddm2_value_buffer) = 100 * INT16(frame->value);  // all DDM values are *10 and all DDM2 values are *1000, hence a factor 100 difference
                    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, Conversion_data->ddm2_parameter | DDM2_PARAMETER_INSTANCE(cfxcc_instance), ddm2_value_buffer, ddm2_value_size, connector_uart_ddm.connector_id, portMAX_DELAY);
                    break;

                case DDM_C0_MEASURED_TEMPERATURE:
                    ddm2_struct_send(Conversion_data->ddm2_parameter, frame->value, ddm2_nocpt, MEASURED_TEMP_COVER, COMPART_NUM0, 100);
                    temp_alert_check(frame->value, MEASURED_TEMP0);
                    break;

                case DDM_C0_SET_TEMPERATURE:
                    ddm2_struct_send(Conversion_data->ddm2_parameter, frame->value, ddm2_nocpt, SET_TEMP_COVER, COMPART_NUM0, 100);
                    temp_alert_check(frame->value, SET_TEMP0);
                    break;

                case DDM_C1_MEASURED_TEMPERATURE:
                    ddm2_struct_send(Conversion_data->ddm2_parameter, frame->value, ddm2_nocpt, MEASURED_TEMP_COVER, COMPART_NUM1, 100);
                    temp_alert_check(frame->value, MEASURED_TEMP1);
                    break;

                case DDM_C1_SET_TEMPERATURE:
                    ddm2_struct_send(Conversion_data->ddm2_parameter, frame->value, ddm2_nocpt, SET_TEMP_COVER, COMPART_NUM1, 100);
                    temp_alert_check(frame->value, SET_TEMP1);
                    break;

                case DDM_C0_TEMPERATURE_OFFSET:
                    ddm2_struct_send(Conversion_data->ddm2_parameter, frame->value, ddm2_nocpt, TEMP_OFFSET_COVER, COMPART_NUM0, 100);
                    break;

                case DDM_C1_TEMPERATURE_OFFSET:
                    ddm2_struct_send(Conversion_data->ddm2_parameter, frame->value, ddm2_nocpt, TEMP_OFFSET_COVER, COMPART_NUM1, 100);
                    break;

                default:
                    break;
                }
            }
            break;

        case DDMP_TYPE_UINT8:
            switch (Conversion_data->ddm_parameter)
            {
            case DDM_PRODUCT_TYPE:
                ddm2_value_size = 4;
                INT32(ddm2_value_buffer) = UINT8(frame->value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, Conversion_data->ddm2_parameter | DDM2_PARAMETER_INSTANCE(cfxcc_instance), ddm2_value_buffer, ddm2_value_size, connector_uart_ddm.connector_id, portMAX_DELAY);
                if (UINT8(frame->value) == 3)
                {
                    ddm2_nocpt = 2;
                }
                else
                {
                    ddm2_nocpt = 1;
                }
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, MCCC0NOCPT | DDM2_PARAMETER_INSTANCE(cfxcc_instance), &ddm2_nocpt, ddm2_value_size, connector_uart_ddm.connector_id, portMAX_DELAY);
                break;

            case DDM_COMPRESSOR_POWER:
                temp_alert_check(frame->value, COMP_POWER);
                ddm2_value_size = 4;
                INT32(ddm2_value_buffer) = UINT8(frame->value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, Conversion_data->ddm2_parameter | DDM2_PARAMETER_INSTANCE(cfxcc_instance), ddm2_value_buffer, ddm2_value_size, connector_uart_ddm.connector_id, portMAX_DELAY);
                break;

            case DDM_COOLER_POWER:
                temp_alert_check(frame->value, COOLER_POWER);
                ddm2_value_size = 4;
                INT32(ddm2_value_buffer) = UINT8(frame->value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, Conversion_data->ddm2_parameter | DDM2_PARAMETER_INSTANCE(cfxcc_instance), ddm2_value_buffer, ddm2_value_size, connector_uart_ddm.connector_id, portMAX_DELAY);
                break;

            case DDM_BATTERY_PROTECTION_LEVEL:
                // fallthrough
            case DDM_ACTIVE_COMPARTMENT:
                // fallthrough
            case DDM_POWER_SOURCE:
                // fallthrough
            case DDM_ICEMAKER_POWER:
                ddm2_value_size = 4;
                INT32(ddm2_value_buffer) = UINT8(frame->value);
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, Conversion_data->ddm2_parameter | DDM2_PARAMETER_INSTANCE(cfxcc_instance), ddm2_value_buffer, ddm2_value_size, connector_uart_ddm.connector_id, portMAX_DELAY);
                break;

            case DDM_C0_POWER:
                ddm2_struct_send(Conversion_data->ddm2_parameter, frame->value, ddm2_nocpt, POWER_COVER, COMPART_NUM0, 1);
                temp_alert_check(frame->value, C0_POWER);
                break;

            case DDM_C0_DOOR_OPEN:
                ddm2_struct_send(Conversion_data->ddm2_parameter, frame->value, ddm2_nocpt, DOOR_COVER, COMPART_NUM0, 1);
                break;

            case DDM_C1_POWER:
                ddm2_struct_send(Conversion_data->ddm2_parameter, frame->value, ddm2_nocpt, POWER_COVER, COMPART_NUM1, 1);
                temp_alert_check(frame->value, C1_POWER);
                break;

            case DDM_C1_DOOR_OPEN:
                ddm2_struct_send(Conversion_data->ddm2_parameter, frame->value, ddm2_nocpt, DOOR_COVER, COMPART_NUM1, 1);
                break;

            case DDM_C0_NTC_OPEN_ERROR:
                error_check_send(Conversion_data->ddm2_parameter, *(frame->value), NTC0_OPEN);
                break;

            case DDM_C0_NTC_SHORT_ERROR:
                error_check_send(Conversion_data->ddm2_parameter, *(frame->value), NTC0_SHORT);
                break;

            case DDM_SOLENOID_VALVE_ERROR:
                error_check_send(Conversion_data->ddm2_parameter, *(frame->value), SOLEN_VALVE);
                break;

            case DDM_C1_NTC_OPEN_ERROR:
                error_check_send(Conversion_data->ddm2_parameter, *(frame->value), NTC1_OPEN);
                break;

            case DDM_C1_NTC_SHORT_ERROR:
                error_check_send(Conversion_data->ddm2_parameter, *(frame->value), NTC1_SHORT);
                break;

            case DDM_FAN_OVERCURRENT_ERROR:
                error_check_send(Conversion_data->ddm2_parameter, *(frame->value), FAN_OVERCUR);
                break;

            case DDM_COMPRESSOR_START_FAIL_ERROR:
                error_check_send(Conversion_data->ddm2_parameter, *(frame->value), COMP_STAR_FAIL);
                break;

            case DDM_COMPRESSOR_SPEED_ERROR:
                error_check_send(Conversion_data->ddm2_parameter, *(frame->value), COMP_SPEED);
                break;

            case DDM_CONTROLLER_OVERTEMP_ERROR:
                error_check_send(Conversion_data->ddm2_parameter, *(frame->value), COMP_OVERTEMP);
                break;

            case DDM_TEMPERATURE_ALERT_CC:
                error_check_send(Conversion_data->ddm2_parameter, *(frame->value), TEMP_ALERT);
                break;

            case DDM_DOOR_ALERT:
                error_check_send(Conversion_data->ddm2_parameter, *(frame->value), DOOR_ALERT);
                break;

            case DDM_VOLTAGE_ALERT:
                error_check_send(Conversion_data->ddm2_parameter, *(frame->value), VOL_ALERT);
                break;

            default:
                break;
            }
            break;

        case DDMP_TYPE_ARRAY:
            switch (Conversion_data->ddm_parameter)
            {
            case DDM_C0_TEMPERATURE_RANGE:
                ddm_to_ddm2_other(Conversion_data->ddm2_parameter, frame->value, ddm2_nocpt, TEMP_RANG_COVER, COMPART_NUM0, 100);
                break;

            case DDM_C1_TEMPERATURE_RANGE:
                ddm_to_ddm2_other(Conversion_data->ddm2_parameter, frame->value, ddm2_nocpt, TEMP_RANG_COVER, COMPART_NUM1, 100);
                break;

            case DDM_C0_RECOMMENDED_RANGE:
                ddm_to_ddm2_other(Conversion_data->ddm2_parameter, frame->value, ddm2_nocpt, RECOMME_RANG_COVER, COMPART_NUM0, 100);
                break;

            case DDM_C1_RECOMMENDED_RANGE:
                ddm_to_ddm2_other(Conversion_data->ddm2_parameter, frame->value, ddm2_nocpt, RECOMME_RANG_COVER, COMPART_NUM1, 100);
                break;
            }
            break;

        default:
            ddm2_value_size = frame->size - DDMP_SIZE_PARAMETER;  // no conversion needed for non-numeric data
            switch (Conversion_data->ddm2_parameter)
            {
            case PROD0MDL:
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, Conversion_data->ddm2_parameter | DDM2_PARAMETER_INSTANCE(cfxprod_instance), frame->value, ddm2_value_size, connector_uart_ddm.connector_id, portMAX_DELAY);
                break;
            case PROD0SN:
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, Conversion_data->ddm2_parameter | DDM2_PARAMETER_INSTANCE(cfxprod_instance), frame->value, ddm2_value_size, connector_uart_ddm.connector_id, portMAX_DELAY);
                break;
            default:
                if (Conversion_data->ddm_parameter == DDM_CC_FIRMWARE_VERSION)
                {
                    // Also send to prod<x>fwver
                    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, PROD0FWVER | DDM2_PARAMETER_INSTANCE(cfxprod_instance), frame->value, ddm2_value_size, connector_uart_ddm.connector_id, portMAX_DELAY);
                }
                connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, Conversion_data->ddm2_parameter | DDM2_PARAMETER_INSTANCE(cfxcc_instance), frame->value, ddm2_value_size, connector_uart_ddm.connector_id, portMAX_DELAY);
                break;
            }
            break;
        }
        break;
    case DDMP_ACTION_ACK:
        UART_DDM_LOG(I, "ACK%c", '!');
        break;
    case DDMP_ACTION_PING:
        UART_DDM_LOG(I, "PING%c", '!');
        break;
    case DDMP_ACTION_HELLO:
        UART_DDM_LOG(I, "HELLO%c", '!');
        break;
    default:
        return;
    }
}

//! \~ Error callback, print error message and disable CC class instance
void ddmp_error_cb(int intf, DDMP_ERROR_ENUM error, const uint8_t *error_string)
{
    RANGE(intf, DDMP_INTERFACE_NONE, DDMP_INTERFACE_COUNT);
    BOUNDED(error, DDMP_ERROR_COUNT);
    ASSERT(error_string);

    LOG(W, "DDMP interface CC error %u: %s", error, error_string);
    if (error == DDMP_ERROR_TIMEOUTS_EXCEEDED)
    {
        if (cfxcc_instance != -1)
        {
            TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, MCCC0 | DDM2_PARAMETER_INSTANCE(cfxcc_instance), &Zero, sizeof(Zero), connector_uart_ddm.connector_id, portMAX_DELAY));
            cfxcc_instance = -1;
        }
        if (cfxprod_instance != -1)
        {
            TRUE_CHECK(connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, PROD0 | DDM2_PARAMETER_INSTANCE(cfxprod_instance), &Zero, sizeof(Zero), connector_uart_ddm.connector_id, portMAX_DELAY));
            cfxprod_instance = -1;
        }
    }
}

//! \~ Connected callback, print interface that was connected and enable CC class instance
void ddmp_connect_cb(int intf)
{
    BOUNDED(intf, DDMP_INTERFACE_COUNT);

    if (cfxcc_instance == -1)
    {
        uint32_t cfxcc0 = MCCC0;  // request CC instance
        NONNEG_CHECK(cfxcc_instance = broker_register_instance(&cfxcc0, connector_uart_ddm.connector_id));
    }
    if (cfxprod_instance == -1)
    {
        uint32_t cfxprod = PROD0;  // request PROD instance
        NONNEG_CHECK(cfxprod_instance = broker_register_instance(&cfxprod, connector_uart_ddm.connector_id));
    }

    LOG(I, "DDMP Connection established for interface CC, instance %d!", cfxcc_instance);
    LOG(I, "Mapped CC instance %d to prod instance %d", cfxcc_instance, cfxprod_instance);
}

//! \~ Global lock callback, lock any other context from accessing critical section
void ddmp_global_lock_cb(int intf)
{
    TRUE_CHECK(xSemaphoreTake(global_ddmp_mutex, portMAX_DELAY));
}

//! \~ Global unlock callback
void ddmp_global_unlock_cb(int intf)
{
    TRUE_CHECK(xSemaphoreGive(global_ddmp_mutex));
}

//! \~ Local lock callback, lock any other context from accessing interface queue
void ddmp_lock_cb(int intf)
{
    BOUNDED(intf, DDMP_INTERFACE_COUNT);

    TRUE_CHECK(xSemaphoreTake(ddmp_mutexes[intf], portMAX_DELAY));
}

//! \~ Local unlock callback
void ddmp_unlock_cb(int intf)
{
    BOUNDED(intf, DDMP_INTERFACE_COUNT);

    TRUE_CHECK(xSemaphoreGive(ddmp_mutexes[intf]));
}

//! \~ Assert callback, print error message
void ddmp_assert_cb(int line, const char *func, char *assertion, int value)
{
    LOG(E, "%s:%d %s==%d", func, line, assertion, value);
}

//! \~ Structure collating DDMP callbacks
DDMP_CALLBACKS callbacks = {
    .frame_cb = ddmp_frame_cb,
    .error_cb = ddmp_error_cb,
    .timer_cb = ddmp_timer_cb,
    .connect_cb = ddmp_connect_cb,
    .global_lock_cb = ddmp_global_lock_cb,
    .global_unlock_cb = ddmp_global_unlock_cb,
    .lock_cb = ddmp_lock_cb,
    .unlock_cb = ddmp_unlock_cb,
    .assert_cb = ddmp_assert_cb,
};

/**
 * @brief   UART initialization configuration
 * Configure UART. Note that REF_TICK is used so that the baud rate remains
 * correct while APB frequency is changing in light sleep mode.
 */

static const uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
#if CONFIG_FREERTOS_USE_TICKLESS_IDLE
#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2
    .source_clk = UART_SCLK_REF_TICK,
#else
    .source_clk = UART_SCLK_XTAL,
#endif
#endif
};

//! \~ UART initialization
static void initialize_uart(void)
{
    LOG(I, "Tx: GPIO%d Rx: GPIO%d", CONNECTOR_UART_DDM_TX, CONNECTOR_UART_DDM_RX);
    ZERO_CHECK(uart_param_config(CONNECTOR_UART_DDM_NUM, &uart_config));
    ZERO_CHECK(uart_set_pin(CONNECTOR_UART_DDM_NUM, CONNECTOR_UART_DDM_TX, CONNECTOR_UART_DDM_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ZERO_CHECK(uart_driver_install(CONNECTOR_UART_DDM_NUM, CONNECTOR_UART_DDM_BUF_SIZE, CONNECTOR_UART_DDM_BUF_SIZE, CONNECTOR_UART_DDM_DRIVER_QUEUE_SIZE, &uart_queue, 0));
};

//! \~ Receive data from UART event queue
static void uart_queue_task(void *pvParameter)
{
    int uart_num = (int)pvParameter;
    uart_event_t event;
    static EXT_RAM_ATTR uint8_t rx_buffer[CONNECTOR_UART_DDM_BUF_SIZE];

    // Init uart in task
    initialize_uart();

    TRUE_CHECK(xTaskCreatePinnedToCore(connector_uart_process_task, (char *)connector_uart_ddm.name, 3072, NULL, xTASK_PRIORITY_NORMAL, NULL, CONNECTOR_UART_DDM_PROCESS_CPU_CORE));
    ZERO_CHECK(ddmp_connect(IF_UART_1));

    while (1)
    {
        TRUE_CHECK(xQueueReceive(uart_queue, (void *)&event, (TickType_t)portMAX_DELAY));

        switch (event.type)
        {
        case UART_DATA:
            NONNEG_CHECK(uart_read_bytes(uart_num, rx_buffer, event.size, portMAX_DELAY));
            UART_DDM_HEXDUMP("DDM RX", rx_buffer, event.size, ESP_LOG_INFO);
            ddmp_receive_uart(IF_UART_1, rx_buffer, event.size, IF_UART_1);
            break;
        case UART_FIFO_OVF:
            LOG(W, "HW FIFO overflow, flushing!");
            uart_flush_input(uart_num);
            break;
        case UART_BUFFER_FULL:
            LOG(W, "Ring buffer full, flushing!");
            uart_flush_input(uart_num);
            break;
        case UART_BREAK:
            UART_DDM_LOG(I, "UART brea%c", 'k');
            break;
        case UART_PARITY_ERR:
            LOG(W, "UART parity error");
            break;
        case UART_FRAME_ERR:
            LOG(W, "UART frame error");
            break;
        default:
            LOG(W, "Unknown UART event type %d!", event.type);
            break;
        }
    }
}

//! \~ Callback to send data out of a UART interface
static int ddmp_send_uart(int intf, void *data, size_t size)
{
    int result;

    BOUNDED(intf, DDMP_INTERFACE_COUNT);
    ASSERT(data);
    ASSERT(size);

    UART_DDM_HEXDUMP("DDM TX", data, size, ESP_LOG_INFO);
    NONNEG_CHECK(result = uart_write_bytes(interfaces[intf].handle, (const char *)data, size));
    return (result >= 0);
}

//! \~ Process frames from broker, convert and forward to DDM
static void connector_uart_process_task(void *Parameter)
{
    SORTED_LIST_VALUE_TYPE conversion_table_index;
    SORTED_LIST_RETURN_VALUE ddm2_lookup_result;
    DDMP2_FRAME *pframe;
    size_t frame_size;
    const DDM_CONVERSION_DATA *conversion_data;
    const DDM_CONVERSION_DATA *conversion_data1;
    size_t ddm_value_size;
    uint8_t ddm_value_buffer[15];

    int32_t uart_value_c0 = 0;
    int32_t uart_value_c1 = 0;

    while (1)
    {
        TRUE_CHECK(pframe = xRingbufferReceive(connector_uart_ddm.to_connector, &frame_size, portMAX_DELAY));

        switch (pframe->frame.control)
        {
        case DDMP2_CONTROL_SUBSCRIBE:  // convert DDM2 SUB to DDM SUB
            ddm2_lookup_result = sorted_list_unique_get(&conversion_table_index, (SORTED_LIST *)&Ddm2_parameter_lookup_list, DDM2_PARAMETER_BASE_INSTANCE(pframe->frame.subscribe.parameter), 0);

            if (ddm2_lookup_result == SORTED_LIST_FAIL)
            {
                // Special handling of prod class here
                if (DDM2_PARAMETER_BASE_INSTANCE(pframe->frame.subscribe.parameter) == PROD0DESCRIPTION)
                {
                    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, pframe->frame.subscribe.parameter, cfxprod_descr, strlen(cfxprod_descr), connector_uart_ddm.connector_id, portMAX_DELAY);
                }
                else if (DDM2_PARAMETER_BASE_INSTANCE(pframe->frame.subscribe.parameter) == PROD0CLIST)
                {
                    uint32_t clist = MCCC0 | DDM2_PARAMETER_INSTANCE(cfxcc_instance);
                    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, pframe->frame.subscribe.parameter, &clist, sizeof(clist), connector_uart_ddm.connector_id, portMAX_DELAY);
                }
                else if (DDM2_PARAMETER_BASE_INSTANCE(pframe->frame.subscribe.parameter) == PROD0MANUF)
                {
                    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, pframe->frame.subscribe.parameter, cfxprod_manuf, strlen(cfxprod_manuf), connector_uart_ddm.connector_id, portMAX_DELAY);
                }
                else
                {
                    LOG(E, "Unrecognized subscribe to DDM2 parameter %08x!", pframe->frame.subscribe.parameter);
                }
                break;
            }

            conversion_data = &Ddm_conversion_data[conversion_table_index];

            if (conversion_table_index == CONVERSION_POW || conversion_table_index == CONVERSION_MESURETEMP || conversion_table_index == CONVERSION_SETTEMP || conversion_table_index == CONVERSION_OFFTEMP)
            {
                conversion_data1 = &Ddm_conversion_data[conversion_table_index + 1];
                ddmp_subscribe(IF_UART_1, conversion_data1->ddm_parameter);
                // LOG(W, "conversion_data parameter %08x!", conversion_data1->ddm_parameter);
            }

            ddmp_subscribe(IF_UART_1, conversion_data->ddm_parameter);
            break;
        case DDMP2_CONTROL_SET:  // convert DDM2 SET to DDM PUB + SUB
            ddm2_lookup_result = sorted_list_unique_get(&conversion_table_index, (SORTED_LIST *)&Ddm2_parameter_lookup_list, pframe->frame.publish.parameter, 0);

            if (ddm2_lookup_result == SORTED_LIST_FAIL)
            {
                LOG(E, "Unrecognized publish to DDM2 parameter %08x!", pframe->frame.publish.parameter);
                break;
            }

            if (conversion_table_index == CONVERSION_SETTEMP || conversion_table_index == CONVERSION_OFFTEMP)
            {
                conversion_data1 = &Ddm_conversion_data[conversion_table_index + 1];
                ddm_value_size = 2;
                uart_value_c1 = *((int32_t *)&pframe->frame.set.value.raw[4]) / 100;
                INT16(ddm_value_buffer) = uart_value_c1;
                ddmp_publish(IF_UART_1, conversion_data1->ddm_parameter, ddm_value_buffer, ddm_value_size);
                ddmp_subscribe(IF_UART_1, conversion_data1->ddm_parameter);  // trigger transmission of updated value
            }
            else if (conversion_table_index == CONVERSION_POW)
            {
                conversion_data1 = &Ddm_conversion_data[conversion_table_index + 1];
                ddm_value_size = 1;
                uart_value_c1 = *((int32_t *)&pframe->frame.set.value.raw[4]);
                INT8(ddm_value_buffer) = uart_value_c1;
                ddmp_publish(IF_UART_1, conversion_data1->ddm_parameter, ddm_value_buffer, ddm_value_size);
                ddmp_subscribe(IF_UART_1, conversion_data1->ddm_parameter);  // trigger transmission of updated value
            }

            conversion_data = &Ddm_conversion_data[conversion_table_index];

            switch (conversion_data->type)
            {
            case DDMP_TYPE_INT16:
                ddm_value_size = 2;
                uart_value_c0 = *((int32_t *)&pframe->frame.set.value.raw[0]) / 100;
                INT16(ddm_value_buffer) = uart_value_c0;
                ddmp_publish(IF_UART_1, conversion_data->ddm_parameter, ddm_value_buffer, ddm_value_size);
                break;
            case DDMP_TYPE_UINT8:
                ddm_value_size = 1;
                INT8(ddm_value_buffer) = pframe->frame.set.value.int32;
                ddmp_publish(IF_UART_1, conversion_data->ddm_parameter, ddm_value_buffer, ddm_value_size);
                break;
            default:
                ddm_value_size = ddmp2_value_size(pframe);
                ddmp_publish(IF_UART_1, conversion_data->ddm_parameter, pframe->frame.set.value.raw, ddm_value_size);
                break;
            }

            ddmp_subscribe(IF_UART_1, conversion_data->ddm_parameter);  // trigger transmission of updated value
            break;
        default:
            LOG(W, "UART connector recived UNHANDLED frame %02x from broker!", pframe->frame.control);
            break;
        }
        vRingbufferReturnItem(connector_uart_ddm.to_connector, pframe);
    }
}

//! \~ Timeout function for timer, retransmit frame
void timeout(TimerHandle_t xTimer)
{
    uint32_t intf = (uint32_t)pvTimerGetTimerID(xTimer);

    ddmp_retransmit(intf);
};

//! \~ Initialize connector
static int initialize_connector_uart_ddm(void)
{
    TRUE_CHECK(ddmp_mutexes[IF_UART_1] = xSemaphoreCreateMutex());
    TRUE_CHECK(global_ddmp_mutex = xSemaphoreCreateMutex());
    TRUE_CHECK(timeout_timers[IF_UART_1] = xTimerCreate(NULL, pdMS_TO_TICKS(DDMP_TIMEOUT), pdFALSE, (void *)IF_UART_1, timeout));

    LOG(I, "Initializing DDMP " DDMPLIB_VERSION ", %d interfaces", DDMP_INTERFACE_COUNT);
    ZERO_CHECK(ddmp_initialize(&callbacks, interfaces, ELEMENTS(interfaces), 0));

    TRUE_CHECK(xTaskCreatePinnedToCore(uart_queue_task, "uart" STR(CONNECTOR_UART_DDM_NUM), 3072, (void *)CONNECTOR_UART_DDM_NUM, xTASK_PRIORITY_NORMAL, NULL, CONNECTOR_UART_DDM_CPU_CORE));

    return 1;
}

//! \~ Connector structure
CONNECTOR connector_uart_ddm = {
    .name = "DDM UART connector",
    .initialize = initialize_connector_uart_ddm,
};
