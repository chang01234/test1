#ifndef RVC_TO_DDM_H
#define RVC_TO_DDM_H

#include "PImage.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    RVC_PART_UINT8,
    RVC_INST_UINT8,
    RVC_DEGC_UINT8,
    RVC_DEGC_UINT16,
    RVC_VACDC_UINT8,
    RVC_VACDC_UINT16,
    RVC_AACDC_UINT8,
    RVC_AACDC_UINT16,
    RVC_AACDC_UINT32,
    RVC_HZ_UINT8,
    RVC_W_UINT16,
    RVC_AMPHOUR_UINT16,
    RVC_STD_UINT8,
    RVC_STD_UINT8_GAIN1000,
    RVC_STD_UINT16,
    RVC_STD_UINT32
} rvc_data_type_t;

int convert_rvc_to_ddm_system_value(const uint32_t parameter, int32_t *i32Value);
int convert_ddm_system_value_to_rvc_value(const uint32_t parameter, int32_t *i32Value, bool is_set);
int convert_rvc_to_ddm_rvc_params(const uint32_t parameter, void *value, rvc_data_type_t type);
int convert_rvc_to_ddm_rvc_params_gain(const uint32_t parameter, void *value, rvc_data_type_t type, int32_t user_rvc_gain);
int convert_ddm_to_rvc_value(const uint32_t parameter, void *value, rvc_data_type_t type);
#endif  // RVC_TO_DDM_H
