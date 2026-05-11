//------------------------------------------------------------------------------
// Module:      power_management.h
//
//------------------------------------------------------------------------------
// Description: Initialize system power management.
//              
//------------------------------------------------------------------------------
#ifndef POWER_MANAGEMENT_H
#define POWER_MANAGEMENT_H

extern uint32_t ulp_adc_button_threshold;
extern uint32_t ulp_adc_status;
extern uint32_t ulp_entry;
extern uint32_t ulp_exit;
extern uint32_t ulp_last_result;
extern uint32_t ulp_p1_wake;
extern uint32_t ulp_sample_counter;
extern uint32_t ulp_tdata_left;
extern uint32_t ulp_tdata_middle;
extern uint32_t ulp_tdata_right;
extern uint32_t ulp_wake_up;

void initialize_power_management(void);

#endif /* POWER_MANAGEMENT_H */
