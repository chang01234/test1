/*! \file rs485_master_battery.c
    \brief Mobile power battery device parameter definitions implementation
*/
#include "configuration.h"

// System includes
#include <stdbool.h>
#include <string.h>
// IDF includes
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
// Component includes
#include "esp_modbus_master.h"
// Framework includes
#include "broker.h"
#include "connector.h"
#include "connector_rs485_master.h"
#include "ddm2.h"
#include "product_database.h"
// Current module includes
#include "rs485_modbus_master.h"
#include "rs485_modbus_master_battery.h"

#define BATTERY_VOLTAGE_CHANGE_THRESHOLD_MV 100
#define BATTERY_CURRENT_CHANGE_THRESHOLD_MA 100
#define BATTERY_SOC_CHANGE_THRESHOLD        10
#define BATTERY_CAPREM_CHANGE_THRESHOLD     1000
#define BATTERY_SOC_PRECISION_DIVISOR       10
#define BATTERY_SOC_PRECISION_ROUNDING      5
#define BATTERY_SOC_MAX_VALUE               1000
#define BATTERY_SOH_PRECISION_DIVISOR       10
#define BATTERY_SOH_PRECISION_ROUNDING      5
#define BATTERY_SOH_MAX_VALUE               1000
#define BATTERY_CYCLE_MAX_VALUE             65535

#define OPTS(min_val, max_val, step_val)                   \
    {                                                      \
        .opt1 = min_val, .opt2 = max_val, .opt3 = step_val \
    }
#define INST_OFFSET(field) ((uint16_t)(offsetof(reg_params_t, field) + 1))

// Parameter descriptor table for mobile power battery, Input Registers
static const mb_parameter_descriptor_t s_battery_descriptor_table[] = {
    // { CID, Param Name, Units, Modbus Slave Addr, Modbus Reg Type, Reg Start, Reg Size,
    // Instance Offset, Data Type, Data Size, Parameter Options, Access Mode}
    // 30001-30002: Battery voltage
    {BATTERY_CID_VOLTAGE, "Battery_Voltage", "mV", BATTERY_SLAVE_ADDR_DEFAULT, MB_PARAM_INPUT, 30001, 2,
     INST_OFFSET(battery_voltage), PARAM_TYPE_U32, RS485_PARAM_VALUE_SIZE_U32, OPTS(0, 0xFFFFFFFF, 1), PAR_PERMS_READ},
    // 30003-30004: Total current
    {BATTERY_CID_CURRENT, "Total_Current", "mA", BATTERY_SLAVE_ADDR_DEFAULT, MB_PARAM_INPUT, 30003, 2,
     INST_OFFSET(total_current), PARAM_TYPE_U32, RS485_PARAM_VALUE_SIZE_U32, OPTS(0, 0xFFFFFFFF, 1), PAR_PERMS_READ},
    // 30005: SOC
    {BATTERY_CID_SOC, "SOC", "%", BATTERY_SLAVE_ADDR_DEFAULT, MB_PARAM_INPUT, 30005, 1,
     INST_OFFSET(soc), PARAM_TYPE_U16, RS485_PARAM_VALUE_SIZE_U16, OPTS(0, BATTERY_SOC_MAX_VALUE, 1), PAR_PERMS_READ},
    // 30006: SOH
    {BATTERY_CID_SOH, "SOH", "%", BATTERY_SLAVE_ADDR_DEFAULT, MB_PARAM_INPUT, 30006, 1,
     INST_OFFSET(soh), PARAM_TYPE_U16, RS485_PARAM_VALUE_SIZE_U16, OPTS(0, BATTERY_SOH_MAX_VALUE, 1), PAR_PERMS_READ},
    // 30041-30042: Full capacity
    {BATTERY_CID_FULL_CAPACITY, "Full_capacity", "mAh", BATTERY_SLAVE_ADDR_DEFAULT, MB_PARAM_INPUT, 30041, 2,
     INST_OFFSET(full_capacity), PARAM_TYPE_U32, RS485_PARAM_VALUE_SIZE_U32, OPTS(0, 0xFFFFFFFF, 1), PAR_PERMS_READ},
    // 30043-30044: Remaining capacity
    {BATTERY_CID_CAPREM, "Remain_Capacity", "mAh", BATTERY_SLAVE_ADDR_DEFAULT, MB_PARAM_INPUT, 30043, 2,
     INST_OFFSET(remain_capacity), PARAM_TYPE_U32, RS485_PARAM_VALUE_SIZE_U32, OPTS(0, 0xFFFFFFFF, 1), PAR_PERMS_READ},
    // 30045: Ambient temperature of acquisition board
    {BATTERY_CID_TEMP, "Ambient_Temp", "℃", BATTERY_SLAVE_ADDR_DEFAULT, MB_PARAM_INPUT, 30045, 1,
     INST_OFFSET(ambient_temp), PARAM_TYPE_U16, RS485_PARAM_VALUE_SIZE_U16, OPTS(0, 127, 1), PAR_PERMS_READ},
    // 30050: Charge detection status
    {BATTERY_CID_CHGDET, "Work_Status", "", BATTERY_SLAVE_ADDR_DEFAULT, MB_PARAM_INPUT, 30050, 1,
     INST_OFFSET(work_status), PARAM_TYPE_U16, RS485_PARAM_VALUE_SIZE_U16, OPTS(0, 2, 1), PAR_PERMS_READ},
    // 30051: Heating switch status
    {BATTERY_CID_HTR, "H_Switch_Status", "", BATTERY_SLAVE_ADDR_DEFAULT, MB_PARAM_INPUT, 30051, 1,
     INST_OFFSET(h_switch_status), PARAM_TYPE_U16, RS485_PARAM_VALUE_SIZE_U16, OPTS(0, 1, 1), PAR_PERMS_READ},
    // 30292: Charging voltage value
    {BATTERY_CID_DVOLT, "Charg_Voltage", "0.1V", BATTERY_SLAVE_ADDR_DEFAULT, MB_PARAM_INPUT, 30292, 1,
     INST_OFFSET(charg_voltage), PARAM_TYPE_U16, RS485_PARAM_VALUE_SIZE_U16, OPTS(0, 0xFFFF, 1), PAR_PERMS_READ},
    // 30293: Charging current limit value
    {BATTERY_CID_DCURR, "Charg_Current", "0.1A", BATTERY_SLAVE_ADDR_DEFAULT, MB_PARAM_INPUT, 30293, 1,
     INST_OFFSET(charg_current), PARAM_TYPE_U16, RS485_PARAM_VALUE_SIZE_U16, OPTS(0, 0xFFFF, 1), PAR_PERMS_READ},
    // 10001: Battery fault/error status part 1
    {BATTY_CID_ERROR_1, "Battery_Error_Status_1", "", BATTERY_SLAVE_ADDR_DEFAULT, MB_PARAM_DISCRETE, 10001, 16,
     INST_OFFSET(battery_error_status_1), PARAM_TYPE_U16, RS485_PARAM_VALUE_SIZE_U16, OPTS(0, 0xFFFF, 1), PAR_PERMS_READ},
    // 10002: Battery fault/error status part 2
    {BATTY_CID_ERROR_2, "Battery_Error_Status_2", "", BATTERY_SLAVE_ADDR_DEFAULT, MB_PARAM_DISCRETE, 10002, 16,
     INST_OFFSET(battery_error_status_2), PARAM_TYPE_U16, RS485_PARAM_VALUE_SIZE_U16, OPTS(0, 0xFFFF, 1), PAR_PERMS_READ},
    // 30007: Cycle times
    {BATTERY_CID_CYCLE_NUM, "Cycle_Times", "", BATTERY_SLAVE_ADDR_DEFAULT, MB_PARAM_INPUT, 30007, 1,
     INST_OFFSET(cycle_times), PARAM_TYPE_U16, RS485_PARAM_VALUE_SIZE_U16, OPTS(0, BATTERY_CYCLE_MAX_VALUE, 1), PAR_PERMS_READ},
    // 30206: Design capacity
    {BATTERY_CID_DESIGN_CAPACITY, "Design_Capacity", "mAh", BATTERY_SLAVE_ADDR_DEFAULT, MB_PARAM_INPUT, 30206, 2,
     INST_OFFSET(design_capacity), PARAM_TYPE_U32, RS485_PARAM_VALUE_SIZE_U32, OPTS(0, 0xFFFFFFFF, 1), PAR_PERMS_READ},
    // 40211: Product number
    {BATTERY_CID_PRODUCT_NUM_CODE, "Product_Number", "", BATTERY_SLAVE_ADDR_DEFAULT, MB_PARAM_HOLDING, 40211, 10,
     INST_OFFSET(product_number), PARAM_TYPE_U8, RS485_PARAM_VALUE_SIZE_STR, OPTS(0, 0, 0), PAR_PERMS_READ},
    // 40261: Serial number
    {BATTERY_CID_SERIAL_NUM, "Serial_Number", "", BATTERY_SLAVE_ADDR_DEFAULT, MB_PARAM_HOLDING, 40261, 10,
     INST_OFFSET(serial_number), PARAM_TYPE_U8, RS485_PARAM_VALUE_SIZE_STR, OPTS(0, 0, 0), PAR_PERMS_READ},
    // 40281: Firmware version
    {BATTERY_CID_FIRMWARE_VER, "Firmware_Version", "", BATTERY_SLAVE_ADDR_DEFAULT, MB_PARAM_HOLDING, 40281, 10,
     INST_OFFSET(firmware_version), PARAM_TYPE_U8, RS485_PARAM_VALUE_SIZE_STR, OPTS(0, 0, 0), PAR_PERMS_READ},
    // 40291: Hardware version
    {BATTERY_CID_HARDWARE_VER, "Hardware_Version", "", BATTERY_SLAVE_ADDR_DEFAULT, MB_PARAM_HOLDING, 40291, 10,
     INST_OFFSET(hardware_version), PARAM_TYPE_U8, RS485_PARAM_VALUE_SIZE_STR, OPTS(0, 0, 0), PAR_PERMS_READ}};
static const uint16_t s_battery_descriptor_table_size = sizeof(s_battery_descriptor_table) / sizeof(s_battery_descriptor_table[0]);
typedef struct
{
    rs485_battery_cid_t cid;   // Modbus CID
    uint32_t ddm2_param_base;  // DDM2 base parameter (without instance)
} rs485_battery_cid_to_ddm2_map_t;

// mapping (adjust according to the DDM2 parameter definitions):
static const rs485_battery_cid_to_ddm2_map_t s_cid_to_ddm2_map[] = {
    {BATTERY_CID_VOLTAGE, 0x1e020009},           // mbat0volt - DC voltage
    {BATTERY_CID_CURRENT, 0x1e02000a},           // mbat0curr - DC current
    {BATTERY_CID_SOC, 0x1e02000c},               // mbat0soc - State of charge
    {BATTERY_CID_SOH, 0x1e02000f},               // mbat0soh - State of health
    {BATTERY_CID_FULL_CAPACITY, 0x1e02001c},     // mbat0capacity - Full charge capacity
    {BATTERY_CID_CAPREM, 0x1e020010},            // mbat0caprem - Remaining capacity
    {BATTERY_CID_TEMP, 0x1e02000b},              // mbat0temp - Ambient temperature of acquisition board
    {BATTERY_CID_CHGDET, 0x1e02001a},            // mbat0chgdet - Charge detection status
    {BATTERY_CID_HTR, 0x1e02001f},               // mbat0htr - Heating switch status
    {BATTERY_CID_DVOLT, 0x1e020014},             // mbat0dvolt - Charging voltage value
    {BATTERY_CID_DCURR, 0x1e020015},             // mbat0dcurr - Charging current limit value
    {BATTY_CID_ERROR_1, 0x1e020021},             // mbat0status - Battery fault/error status part 1
    {BATTY_CID_ERROR_2, 0x1e020021},             // mbat0status - Battery fault/error status part 2
    {BATTERY_CID_CYCLE_NUM, 0x1e03000d},         // mbathist0cycles - The number of charge cycles
    {BATTERY_CID_DESIGN_CAPACITY, 0x1c000007},   // prod0mdl - Design capacity
    {BATTERY_CID_PRODUCT_NUM_CODE, 0x1c000004},  // prod0name - Product numeric code
    {BATTERY_CID_SERIAL_NUM, 0x1c000002},        // prod0sn - Serial number
    {BATTERY_CID_FIRMWARE_VER, 0x1c000005},      // prod0fwver - Firmware version
    {BATTERY_CID_HARDWARE_VER, 0x1c000006}       // prod0hwver - Hardware version
};

static const uint16_t s_cid_to_ddm2_map_size = sizeof(s_cid_to_ddm2_map) / sizeof(s_cid_to_ddm2_map[0]);

uint32_t rs485_battery_cid_to_ddm2_param(rs485_battery_cid_t cid, uint8_t instance)
{
    for (uint16_t i = 0; i < s_cid_to_ddm2_map_size; i++)
    {
        if (s_cid_to_ddm2_map[i].cid == cid)
        {
            return s_cid_to_ddm2_map[i].ddm2_param_base | DDM2_PARAMETER_INSTANCE(instance);
        }
    }
    return 0;
}

esp_err_t rs485_battery_ddm2_param_to_cid(uint32_t ddm2_param, rs485_battery_cid_t *cid)
{
    uint32_t base_param = DDM2_PARAMETER_BASE_INSTANCE(ddm2_param);

    for (uint16_t i = 0; i < s_cid_to_ddm2_map_size; i++)
    {
        if (s_cid_to_ddm2_map[i].ddm2_param_base == base_param)
        {
            if (cid != NULL)
            {
                *cid = s_cid_to_ddm2_map[i].cid;
            }
            return ESP_OK;
        }
    }
    return ESP_ERR_NOT_FOUND;
}

esp_err_t rs485_battery_ddm2_param_to_addr(uint32_t ddm2_param, uint8_t *addr)
{
    uint8_t instance = DDM2_PARAMETER_INSTANCE_PART(ddm2_param) >> 8;

    for (uint16_t i = 0; i < RS485_SLAVE_ADDR_MAX; i++)
    {
        if (s_slave_status[i].registered_mbat_instance == instance && s_slave_status[i].is_online)
        {
            if (addr != NULL)
            {
                *addr = s_slave_status[i].addr;
            }
            return ESP_OK;
        }
    }
    return ESP_ERR_NOT_FOUND;
}

/**
 * @brief Convert U32_CDAB byte order to normal uint32_t
 * U32_CDAB format: bytes are ordered as [C, D, A, B] where:
 * - A, B = bytes from first register (low address)
 * - C, D = bytes from second register (high address)
 * @param cdab_value Value in CDAB byte order
 * @return uint32_t Value in normal byte order [A, B, C, D]
 */
static uint32_t convert_u32_cdab_to_normal(uint32_t cdab_value)
{
    // Extract bytes from CDAB format: [C, D, A, B]
    uint8_t byte_c = (cdab_value >> 24) & 0xFF;
    uint8_t byte_d = (cdab_value >> 16) & 0xFF;
    uint8_t byte_a = (cdab_value >> 8) & 0xFF;
    uint8_t byte_b = cdab_value & 0xFF;

    // Reconstruct in normal order: [A, B, C, D]
    return ((uint32_t)byte_a << 24) | ((uint32_t)byte_b << 16) | ((uint32_t)byte_c << 8) | byte_d;
}

/**
 * @brief Convert paired bytes from "BA DC ..." ordering to "AB CD ..." ordering.
 * @param src Pointer to source buffer (read-only).
 * @param len Number of bytes to process from `src` (should be even).
 * @param dst Pointer to destination buffer, must have space for at least `len` bytes.
 */
static void convert_badc_to_abcd(const char *src, uint8_t len, char *dst)
{
    if (src == NULL || dst == NULL || len == 0 || (len % 2) != 0 || len > RS485_PARAM_VALUE_SIZE_STR)
    {
        return;
    }

    uint8_t dst_idx = 0;
    for (int i = 0; i < len; i += 2)
    {
        dst[dst_idx++] = src[i + 1];
        dst[dst_idx++] = src[i];
    }
    LOG(D, "dst_idx:%d", dst_idx);
}

/**
 * @brief Update product property fields for a battery device
 * @param prodnode_ddminst Product database node DDM2 instance
 * @param mbat_inst MBAT instance number
 */
static void update_prop_field(int prodnode_ddminst, uint8_t mbat_inst)
{
    prodxprop_type_t type = {0};
    type.type.cls = PRODXPROP_TYPE_CLASS_POWER;
    type.type.intf = PRODXPROP_TYPE_INTERFACE_RS485;
    ProdDBUpdateCache((const void *)&mbat_inst, sizeof(uint8_t), FIELD_PROP_INST, prodnode_ddminst);
    ProdDBUpdateCache((const void *)&type, sizeof(type), FIELD_PROP_TYPE, prodnode_ddminst);
    // Publish PROP
    ProdDBUpdateCache((const void *)&type, 0, FIELD_PROP, prodnode_ddminst);
}

esp_err_t rs485_battery_value_to_ddm2_value(rs485_battery_cid_t cid, uint8_t *value, uint8_t *size)
{
    if (value == NULL || size == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    int32_t ddm2_value;
    *size = RS485_PARAM_VALUE_SIZE_U32;  // Default size for DDM2 battery parameters

    switch (cid)
    {
    case BATTERY_CID_VOLTAGE:
    {
        // Input: uint32_t battery_voltage (mV, U32_CDAB byte order)
        // Output: int32_t mbat0volt (mV)
        uint32_t voltage_cdab = *(uint32_t *)value;
        uint32_t voltage_normal = convert_u32_cdab_to_normal(voltage_cdab);
        ddm2_value = (int32_t)voltage_normal;
        LOG(D, "Voltage Normal: %d mV", ddm2_value);
        memcpy(value, &ddm2_value, *size);
        break;
    }
    case BATTERY_CID_CURRENT:
    {
        // Input: int32_t total_current (mA, U32_CDAB byte order)
        // Output: int32_t mbat0curr (mA)
        uint32_t current_cdab = *(uint32_t *)value;
        uint32_t current_normal = convert_u32_cdab_to_normal(current_cdab);
        ddm2_value = (int32_t)current_normal;
        LOG(D, "Current Normal: %d mA", ddm2_value);
        memcpy(value, &ddm2_value, *size);
        break;
    }
    case BATTERY_CID_SOC:
    {
        // Input: uint16_t soc (0-1000, representing 0.0%-100.0% with 0.1% precision)
        // Output: int32_t mbat0soc (0-100, representing 0%-100% with 1% precision)
        uint16_t soc = *(uint16_t *)value;
        ddm2_value = (int32_t)((soc + BATTERY_SOC_PRECISION_ROUNDING) / BATTERY_SOC_PRECISION_DIVISOR);
        LOG(D, "SOC DDM2: %d (%%)", ddm2_value);
        memcpy(value, &ddm2_value, *size);
        break;
    }
    case BATTERY_CID_SOH:
    {
        // Input: uint16_t soh (0-1000, representing 0.0%-100.0% with 0.1% precision)
        // Output: int32_t mbat0soh (0-100, representing 0%-100% with 1% precision)
        uint16_t soh = *(uint16_t *)value;
        ddm2_value = (int32_t)((soh + BATTERY_SOH_PRECISION_ROUNDING) / BATTERY_SOH_PRECISION_DIVISOR);
        LOG(D, "SOH DDM2: %d (%%)", ddm2_value);
        memcpy(value, &ddm2_value, *size);
        break;
    }
    case BATTERY_CID_FULL_CAPACITY:
    {
        // Input: uint32_t capacity (mAh, U32_CDAB byte order)
        // Output: int32_t prod0mdl (Ah)
        uint32_t capacity_cdab = *(uint32_t *)value;
        uint32_t capacity_normal = convert_u32_cdab_to_normal(capacity_cdab);
        ddm2_value = (int32_t)(capacity_normal / 1000);  // Convert mAh to Ah
        LOG(D, "Capacity Normal: %d Ah", ddm2_value);
        memcpy(value, &ddm2_value, *size);
        break;
    }
    case BATTERY_CID_CAPREM:
    {
        // Input: uint32_t remain_capacity (mAh, U32_CDAB byte order)
        // Output: int32_t mbat0caprem (Ah)
        uint32_t caprem_cdab = *(uint32_t *)value;
        uint32_t caprem_normal = convert_u32_cdab_to_normal(caprem_cdab);
        ddm2_value = (int32_t)(caprem_normal / 1000);  // Convert mAh to Ah
        LOG(D, "Remaining Capacity Normal: %d Ah", ddm2_value);
        memcpy(value, &ddm2_value, *size);
        break;
    }
    case BATTERY_CID_TEMP:
    {
        // Input: int16_t ambient_temp (°C)
        // Output: int32_t mbat0temp (°C)
        uint16_t ambient_temp = *(uint16_t *)value;
        ddm2_value = (int32_t)ambient_temp;
        LOG(D, "Ambient Temp DDM2: %d ℃", ddm2_value);
        ddm2_value = ddm2_value * 1000;  // Convert to m°C
        memcpy(value, &ddm2_value, *size);
        break;
    }
    case BATTERY_CID_CHGDET:
    {
        // Input: uint16_t work_status (0 or 1 or 2)
        // Output: int32_t mbat0chgdet (0 or 1)
        uint16_t work_status = *(uint16_t *)value;
        if (work_status != 1)  // Charging: work_status = 1
        {
            work_status = 0;  // Idle(0) or Discharging(2)
        }
        ddm2_value = (int32_t)work_status;
        LOG(D, "Charge Detection Status DDM2: %d", ddm2_value);
        memcpy(value, &ddm2_value, *size);
        break;
    }
    case BATTERY_CID_HTR:
    {
        // Input: uint16_t h_switch_status (0 or 1)
        // Output: int32_t mbat0htr (0 or 1)
        uint16_t h_switch_status = *(uint16_t *)value;
        ddm2_value = (int32_t)h_switch_status;
        LOG(D, "Heating Switch Status DDM2: %d", ddm2_value);
        memcpy(value, &ddm2_value, *size);
        break;
    }
    case BATTERY_CID_DVOLT:
    {
        // Input: uint16_t charg_voltage (0.1V)
        // Output: int32_t mbat0dvolt (0.1V)
        uint16_t charg_voltage = *(uint16_t *)value;
        ddm2_value = (int32_t)charg_voltage;
        LOG(D, "Charging Voltage DDM2: %d (0.1V)", ddm2_value);
        ddm2_value = ddm2_value * 100;  // Convert 0.1V to mV
        memcpy(value, &ddm2_value, *size);
        break;
    }
    case BATTERY_CID_DCURR:
    {
        // Input: uint16_t charg_current (0.1A)
        // Output: int32_t mbat0dcurr (0.1A)
        uint16_t charg_current = *(uint16_t *)value;
        ddm2_value = (int32_t)charg_current;
        LOG(D, "Charging Current DDM2: %d (0.1A)", ddm2_value);
        ddm2_value = ddm2_value * 100;  // Convert 0.1A to mA
        memcpy(value, &ddm2_value, *size);
        break;
    }
    case BATTY_CID_ERROR_1:
    {
        // Input: uint16_t battery_fault_status (bits)
        // Output: uint16_t mbat0status[] (bits)
        uint16_t battery_fault_status = *(uint16_t *)value;
        uint16_t mbatxstatus[BIT_SIZE_U16] = {0};
        uint8_t index = 0;

        for (int i = 0; i < BIT_SIZE_U16; i++)
        {
            if (battery_fault_status & (1 << i))
            {
                LOG(D, "Battery Fault Bit %d Set", i);
                switch (i)
                {
                case 0:
                    mbatxstatus[index++] = MPS_BAT_CELL_FAULT;
                    break;
                case 1:
                    mbatxstatus[index++] = MPS_BAT_CELL_FAULT;
                    break;
                case 2:
                    mbatxstatus[index++] = MPS_DC_LOW_VOLT_LIMIT_REACHED;
                    break;
                case 3:
                    mbatxstatus[index++] = MPS_DC_HIGH_VOLT_LIMIT_REACHED;
                    break;
                case 4:
                    mbatxstatus[index++] = MPS_DC_HIGH_CURR_LIMIT_REACHED;
                    break;
                case 5:
                    mbatxstatus[index++] = MPS_DC_HIGH_CURR_LIMIT_REACHED;
                    break;
                case 12:
                    mbatxstatus[index++] = MPS_DC_LOW_TEMP_LIMIT_REACHED;
                    break;
                case 13:
                    mbatxstatus[index++] = MPS_DC_HIGH_TEMP_LIMIT_REACHED;
                    break;
                case 14:
                    mbatxstatus[index++] = MPS_DC_LOW_TEMP_LIMIT_REACHED;
                    break;
                case 15:
                    mbatxstatus[index++] = MPS_DC_HIGH_TEMP_LIMIT_REACHED;
                    break;
                default:
                    break;
                }
            }
        }
        *size = index * sizeof(uint16_t);
        memcpy(value, mbatxstatus, *size);
        break;
    }
    case BATTY_CID_ERROR_2:
    {
        // Input: uint16_t battery_fault_status (bits)
        // Output: uint16_t mbat0status[] (bits)
        uint16_t battery_fault_status = *(uint16_t *)value;
        uint16_t mbatxstatus[BIT_SIZE_U16] = {0};
        uint8_t index = 0;

        for (int i = 0; i < BIT_SIZE_U16; i++)
        {
            if (battery_fault_status & (1 << i))
            {
                LOG(D, "Battery Fault Bit %d Set", i + 16);
                switch (i)
                {
                case 0:
                    mbatxstatus[index++] = MPS_DC_LOW_STATE_CHARGE_LIMIT_REACHED;
                    break;
                case 5:
                    mbatxstatus[index++] = MPS_GEN_DEVICE_FAULT;
                    break;
                case 10:
                    mbatxstatus[index++] = MPS_GEN_DEVICE_FAULT;
                    break;
                default:
                    break;
                }
            }
        }

        *size = index * sizeof(uint16_t);
        memcpy(value, mbatxstatus, *size);
        break;
    }
    case BATTERY_CID_CYCLE_NUM:
    {
        // Input: uint16_t cycle_times
        // Output: int32_t mbathist0cycles
        uint16_t cycle_times = *(uint16_t *)value;
        ddm2_value = (int32_t)cycle_times;
        LOG(D, "Cycle Times DDM2: %d", ddm2_value);
        memcpy(value, &ddm2_value, *size);
        break;
    }
    case BATTERY_CID_DESIGN_CAPACITY:
    case BATTERY_CID_PRODUCT_NUM_CODE:
    case BATTERY_CID_SERIAL_NUM:
    case BATTERY_CID_FIRMWARE_VER:
    case BATTERY_CID_HARDWARE_VER:
        // These parameters are handled by rs485_battery_process_product_info
        // They don't need DDM2 value conversion, they update product database directly
        return ESP_ERR_NOT_SUPPORTED;
    default:
        return ESP_ERR_NOT_SUPPORTED;
    }

    return ESP_OK;
}

esp_err_t rs485_battery_process_product_info(rs485_battery_cid_t cid, void *value, uint8_t value_size, int instance)
{
    if (value == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    switch (cid)
    {
    case BATTERY_CID_DESIGN_CAPACITY:
    {
        // Input: uint32_t design_capacity mAh
        uint32_t design_capacity_cdab = *(uint32_t *)value;
        uint32_t design_capacity_normal = convert_u32_cdab_to_normal(design_capacity_cdab);
        int32_t ddm2_value = (int32_t)design_capacity_normal / 1000;  // Convert mAh to Ah
        char capacity_str[RS485_PARAM_VALUE_SIZE_STR];
        sprintf(capacity_str, "EPL-%dBT-12V-G3", ddm2_value);  // Example: EPL-100BT-12V-G3
        LOG(D, "Design Capacity DDM2: %s %d", capacity_str, instance);
        ProdDBUpdateCache(capacity_str, strlen(capacity_str), FIELD_MDL, instance);
        break;
    }
    case BATTERY_CID_PRODUCT_NUM_CODE:
    {
        // Input: uint8_t product_number[RS485_PARAM_VALUE_SIZE_STR]
        // Direct copy, ensure null-termination
        char result[RS485_PARAM_VALUE_SIZE_STR + 1] = {0};
        convert_badc_to_abcd((char *)value, RS485_PARAM_VALUE_SIZE_STR, result);
        LOG(D, "pnc: %s %d", result, instance);
        ProdDBUpdateCache(result, strlen(result), FIELD_PNC, instance);
        break;
    }
    case BATTERY_CID_SERIAL_NUM:
    {
        // Input: uint8_t serial_number[RS485_PARAM_VALUE_SIZE_STR]
        // Direct copy, ensure null-termination
        char result[RS485_PARAM_VALUE_SIZE_STR + 1] = {0};
        convert_badc_to_abcd((char *)value, RS485_PARAM_VALUE_SIZE_STR, result);
        LOG(D, "serial num: %s %d", result, instance);
        ProdDBUpdateCache(result, strlen(result), FIELD_SN, instance);
        break;
    }
    case BATTERY_CID_FIRMWARE_VER:
    {
        // Input: uint8_t firmware_version[RS485_PARAM_VALUE_SIZE_STR]
        // Direct copy, ensure null-termination
        char result[RS485_PARAM_VALUE_SIZE_STR + 1] = {0};
        char fw_version[7] = {0};
        char desc[40] = {0};
        convert_badc_to_abcd((char *)value, RS485_PARAM_VALUE_SIZE_STR, result);
        LOG(D, "firmware string: %s %d", result, instance);
        sprintf(desc, "Software Version: %s", result);
        ProdDBUpdateCache(desc, strlen(desc), FIELD_DESC, instance);
        // Format firmware version as x.y.z
        sprintf(fw_version, "%c%c.%c%c.%c%c", result[9], result[10], result[11], result[12], result[13], result[14]);
        ProdDBUpdateCache(fw_version, strlen(fw_version), FIELD_FWVER, instance);
        break;
    }
    case BATTERY_CID_HARDWARE_VER:
    {
        // Input: uint8_t hardware_version[RS485_PARAM_VALUE_SIZE_STR]
        // Direct copy, ensure null-termination
        char result[RS485_PARAM_VALUE_SIZE_STR + 1] = {0};
        convert_badc_to_abcd((char *)value, RS485_PARAM_VALUE_SIZE_STR, result);
        LOG(D, "prod string: %s %d", result, instance);
        ProdDBUpdateCache(result, strlen(result), FIELD_HWVER, instance);
        break;
    }
    default:
        return ESP_ERR_NOT_SUPPORTED;
    }

    return ESP_OK;
}

esp_err_t rs485_battery_get_descriptor_table(const void **descriptor_table,
                                             uint16_t *num_elements)
{
    if (descriptor_table == NULL || num_elements == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    *descriptor_table = (const void *)s_battery_descriptor_table;
    *num_elements = s_battery_descriptor_table_size;

    LOG(D, "Battery descriptor table: %d parameters", s_battery_descriptor_table_size);
    return ESP_OK;
}

bool rs485_battery_is_polling_parameter(rs485_battery_cid_t cid)
{
    return (cid >= BATTERY_CID_VOLTAGE && cid <= BATTERY_CID_CYCLE_NUM);
}

uint16_t rs485_battery_get_parameter_count(void)
{
    return s_battery_descriptor_table_size;
}

/**
 * @brief Check whether the parameter value changes are greater than the threshold setting.
 * @param cid Modbus CID identifying the parameter type
 * @param cached_value Pointer to cached value buffer
 * @param new_value Pointer to new value buffer
 * @return false: Change below threshold; true: The change is greater than the threshold or has no threshold limit
 */
static bool threshold_check(rs485_battery_cid_t cid,
                            const uint8_t *cached_value,
                            const uint8_t *new_value)
{
    bool result = true;

    switch (cid)
    {
    case BATTERY_CID_VOLTAGE:
    case BATTERY_CID_CURRENT:
    case BATTERY_CID_CAPREM:
    {
        uint32_t cached_cdab = *(const uint32_t *)cached_value;
        uint32_t new_cdab = *(const uint32_t *)new_value;

        // Convert from CDAB to normal format for comparison
        uint32_t cached_normal = convert_u32_cdab_to_normal(cached_cdab);
        uint32_t new_normal = convert_u32_cdab_to_normal(new_cdab);

        // Calculate absolute difference
        uint32_t diff;
        if (cached_normal > new_normal)
        {
            diff = cached_normal - new_normal;
        }
        else
        {
            diff = new_normal - cached_normal;
        }
        if (cid == BATTERY_CID_VOLTAGE && diff < BATTERY_VOLTAGE_CHANGE_THRESHOLD_MV)
        {
            result = false;  // Change below threshold
        }
        if (cid == BATTERY_CID_CURRENT && diff < BATTERY_CURRENT_CHANGE_THRESHOLD_MA)
        {
            result = false;  // Change below threshold
        }
        if (cid == BATTERY_CID_CAPREM && diff < BATTERY_CAPREM_CHANGE_THRESHOLD)
        {
            result = false;  // Change below threshold
        }
        break;
    }
    case BATTERY_CID_SOC:
    {
        uint16_t cached_soc = *(uint16_t *)cached_value;
        uint16_t new_soc = *(uint16_t *)new_value;
        uint16_t diff;
        if (cached_soc > new_soc)
        {
            diff = cached_soc - new_soc;
        }
        else
        {
            diff = new_soc - cached_soc;
        }
        if (diff < BATTERY_SOC_CHANGE_THRESHOLD)
        {
            result = false;  // Change below threshold
        }
        break;
    }
    default:
        break;
    }

    return result;
}

bool rs485_battery_check_value_changed(rs485_battery_cid_t cid,
                                       const uint8_t *cached_value,
                                       const uint8_t *new_value,
                                       uint8_t size,
                                       bool *is_valid)
{
    if (cached_value == NULL || new_value == NULL || is_valid == NULL || size == 0)
    {
        return false;
    }

    // If cache is invalid (first time reading), mark as valid and return true
    if (!(*is_valid))
    {
        *is_valid = true;
        return true;
    }

    // Special handling for VOLTAGE and CURRENT with threshold check
    if (!threshold_check(cid, cached_value, new_value))
    {
        return false;  // Change below threshold
    }

    if (memcmp(cached_value, new_value, size) != 0)
    {
        // Value changed
        return true;
    }

    // Value unchanged
    return false;
}

bool rs485_battery_publish_status(rs485_battery_cid_t cid,
                                  uint32_t ddm2_param,
                                  const void *rs485_value,
                                  uint8_t value_size,
                                  uint8_t connector_id)
{
    if (rs485_value == NULL || value_size == 0 || ddm2_param == 0 || value_size > RS485_PARAM_VALUE_SIZE_U32)
    {
        return false;
    }

    uint8_t publish_buffer[BIT_SIZE_U16 * 2] = {0};
    uint8_t publish_value_size = 0;

    memcpy(publish_buffer, rs485_value, value_size);

    // Convert rs485_battery value to DDM2 value (in-place conversion on temp buffer)
    esp_err_t err = rs485_battery_value_to_ddm2_value(cid, publish_buffer, &publish_value_size);
    if (err != ESP_OK)
    {
        LOG(E, "Failed to convert value for CID %d: %s", cid, esp_err_to_name(err));
        return false;
    }

    // Publish to Broker (always use size 4 for DDM2 int32_t values)
    bool result = connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                                 ddm2_param,
                                                 publish_buffer,
                                                 publish_value_size,
                                                 connector_id,
                                                 portMAX_DELAY);
    if (result)
    {
        LOG(D, "Published CID %d (DDM2: 0x%08x)", cid, ddm2_param);
    }
    else
    {
        LOG(W, "Failed to publish CID %d (DDM2: 0x%08x) to broker", cid, ddm2_param);
    }

    return result;
}

void slave_battery_device_login(uint8_t addr)
{
    // Register MBAT0
    uint32_t param = MBAT0;
    int instance = broker_register_instance(&param, connector_rs485_master.connector_id);

    if (instance == -1)
    {
        LOG(E, "Failed to register mbat instance for slave, addr %d", addr);
        return;
    }
    else
    {
        LOG(D, "Registered DDM2 mbat instance %d for slave %d successfuly", instance, addr);
        s_slave_status[ADDR_OFFSET(addr)].registered_mbat_instance = instance;
    }
    // Register MBATHIST0
    param = MBATHIST0;
    instance = broker_register_instance(&param, connector_rs485_master.connector_id);

    if (instance == -1)
    {
        LOG(E, "Failed to register mbathist instance for slave, addr %d", addr);
        return;
    }
    else
    {
        LOG(D, "Registered DDM2 mbathist instance %d for slave %d successfuly", instance, addr);
        s_slave_status[ADDR_OFFSET(addr)].registered_mbathist_instance = instance;

        param = MBAT0LINKED | DDM2_PARAMETER_INSTANCE(s_slave_status[ADDR_OFFSET(addr)].registered_mbat_instance);
        calculate_mbatxlinked_mbatxtype_param(param);
    }

    // Register product class
    prod_database_t *prod_data = hal_mem_malloc_prefer(sizeof(prod_database_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    if (prod_data != NULL)
    {
        memset(prod_data, 0, sizeof(prod_database_t));
        prod_data->sn[0] = addr;  // Init: Use slave address as part of serial number for uniqueness
        int instance = ProdDBProdClassNodeCreate(prod_data, sizeof(prod_database_t), connector_rs485_master.connector_id);

        if (instance == -1)
        {
            LOG(E, "Failed to register prod instance for slave, addr %d", addr);
            hal_mem_free(prod_data);
            return;
        }
        else
        {
            LOG(D, "Registered DDM2 prod instance %d for slave %d successfuly", instance, addr);
            ProdDBUpdateCache("B-TEC Battery", strlen("B-TEC Battery"), FIELD_NAME, instance);
            ProdDBUpdateCache("ENERDRIVE", strlen("ENERDRIVE"), FIELD_MANUF, instance);
            ProdDBUpdateCache("MB3", strlen("MB3"), FIELD_FWID, instance);
            uint32_t clist_value = MBAT0 | DDM2_PARAMETER_INSTANCE(s_slave_status[ADDR_OFFSET(addr)].registered_mbat_instance);
            ProdDBUpdateCache(&clist_value, sizeof(uint32_t), FIELD_CLIST, instance);
            clist_value = MBATHIST0 | DDM2_PARAMETER_INSTANCE(s_slave_status[ADDR_OFFSET(addr)].registered_mbat_instance);
            ProdDBUpdateCache(&clist_value, sizeof(uint32_t), FIELD_CLIST, instance);
            update_prop_field(instance, s_slave_status[ADDR_OFFSET(addr)].registered_mbat_instance);
            s_slave_status[ADDR_OFFSET(addr)].registered_prod_instance = instance;
        }
    }
}

void slave_battery_device_logout(uint8_t addr)
{
    // Use s_slave_status[ADDR_OFFSET(addr)] as the single source of truth
    rs485_slave_status_t *status = &s_slave_status[ADDR_OFFSET(addr)];
    ProdDBProdClassNodeDelete(status->registered_prod_instance);

    // Publish unavailable class: MBAT0 | DDM2_PARAMETER_INSTANCE(instance)
    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                   MBAT0 | DDM2_PARAMETER_INSTANCE(status->registered_mbat_instance),
                                   &Zero,
                                   sizeof(Zero),
                                   connector_rs485_master.connector_id,
                                   (TickType_t)portMAX_DELAY);

    // Publish unavailable class: MBATHIST0 | DDM2_PARAMETER_INSTANCE(instance)
    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                   MBATHIST0 | DDM2_PARAMETER_INSTANCE(status->registered_mbathist_instance),
                                   &Zero,
                                   sizeof(Zero),
                                   connector_rs485_master.connector_id,
                                   (TickType_t)portMAX_DELAY);

    // Reset status
    status->registered_mbat_instance = -1;
    status->registered_prod_instance = -1;
    status->registered_mbathist_instance = -1;
    status->product_info_read = false;
    LOG(D, "Slave %d went offline", addr);
}

void calculate_mbatxlinked_mbatxtype_param(uint32_t ddm2_param)
{
    uint32_t base_param = DDM2_PARAMETER_BASE_INSTANCE(ddm2_param);
    uint8_t addr = 0;
    uint32_t param_value = 0;

    esp_err_t err = rs485_battery_ddm2_param_to_addr(ddm2_param, &addr);
    if (err != ESP_OK)
    {
        return;
    }

    switch (base_param)
    {
    case MBAT0LINKED:
        param_value = MBATHIST0 | DDM2_PARAMETER_INSTANCE(s_slave_status[ADDR_OFFSET(addr)].registered_mbathist_instance);
        break;
    case MBAT0TYPE:
        /* code */
        param_value = 3;  // Lithium-Iron-Phosphate
        break;

    default:
        return;
    }
    bool result = connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH, ddm2_param, &param_value, sizeof(uint32_t), connector_rs485_master.connector_id, portMAX_DELAY);
    if (result)
    {
        LOG(D, "Published DDM2: 0x%08x, value: 0x%08x", ddm2_param, param_value);
    }
    else
    {
        LOG(W, "Failed to publish DDM2: 0x%08x, value: 0x%08x", ddm2_param, param_value);
    }
}
