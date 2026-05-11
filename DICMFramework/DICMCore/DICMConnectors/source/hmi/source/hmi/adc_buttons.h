#ifndef ADC_BUTTON_TUNE_H_
#define ADC_BUTTON_TUNE_H_

#include "configuration.h"
#include "input_manager.h"

#define CONNECTOR_HMI_BUTTON_COMBINATION_MAX    2
#define CONNECTOR_HMI_BUTTON_RANGE_INSTANCES    4

typedef struct BUTTON_RANGE
{
    uint32_t index[CONNECTOR_HMI_BUTTON_COMBINATION_MAX];   //!< Button event indexes used for trigger
    uint32_t index_num; // Number of indexes to trigger (up to CONNECTOR_HMI_BUTTON_COMBINATION_MAX)
    int lower_range;
    int upper_range;
} BUTTON_RANGE;

typedef struct BUTTON_RANGE_MANAGER
{
    BUTTON_RANGE *ranges[CONNECTOR_HMI_BUTTON_RANGE_INSTANCES];
    int32_t ranges_instances;
} BUTTON_RANGE_MANAGER;


#if defined(CONNECTOR_HMI_ADC_USE_RANGE_DETECTION_MECHANISM)
void adc_buttons_initialize_ranges(const ADC_BUTTON * buttons, uint32_t button_elements);
BUTTON_RANGE * adc_buttons_find_range_by_value(uint32_t adc_val);
void adc_buttons_dump_range_and_value(const BUTTON_RANGE * range, uint32_t adc_val);
#endif
#endif //ADC_BUTTON_TUNE_H_
