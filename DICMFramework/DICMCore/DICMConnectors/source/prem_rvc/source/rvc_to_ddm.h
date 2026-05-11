#ifndef RVC_TO_DDM_H
#define RVC_TO_DDM_H

#include <stdint.h>

int convert_rvc_to_ddm_system_value(int32_t variable, int32_t* i32Value);
int convert_ddm_system_value_to_rvc_value(uint32_t parameter, int32_t* i32Value);
int32_t get_parameter_by_pimage_index(uint32_t index);

#endif //RVC_TO_DDM_H
