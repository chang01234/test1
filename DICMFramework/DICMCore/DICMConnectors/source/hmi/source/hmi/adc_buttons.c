/*! \file input_manager.c
*
*/


#include "adc_buttons.h"

#if defined(CONNECTOR_HMI_ADC_USE_RANGE_DETECTION_MECHANISM)
#include <string.h>

/* Button range manager */
static bool button_range_manager_initialize(BUTTON_RANGE_MANAGER * brm, BUTTON_RANGE * buttons_ranges, uint32_t buttons_ranges_elements);
static BUTTON_RANGE * button_range_manager_find_range_by_value(BUTTON_RANGE_MANAGER * brm, int32_t in_range);
static void button_range_manager_add_range(BUTTON_RANGE_MANAGER * brm, BUTTON_RANGE * button_range);

/* Button range */
static BUTTON_RANGE * button_range_create(BUTTON_RANGE_MANAGER * brm);
static int32_t button_range_get_upper_limit(const BUTTON_RANGE * br);
static int32_t button_range_get_lower_limit(const BUTTON_RANGE * br);
static void button_range_add_adc_button(BUTTON_RANGE * br, const ADC_BUTTON * button);
static bool button_range_is_value_in_range(const BUTTON_RANGE * br, int32_t value);
static void button_range_dump(const BUTTON_RANGE * br);

static BUTTON_RANGE_MANAGER button_range_manager;
static BUTTON_RANGE buttons_ranges[CONNECTOR_HMI_BUTTON_RANGE_INSTANCES];

static bool button_range_manager_initialize(BUTTON_RANGE_MANAGER * brm, BUTTON_RANGE * buttons_ranges, uint32_t buttons_ranges_elements)
{
    if (ELEMENTS(brm->ranges) < buttons_ranges_elements)
    {
        LOG(E, "Insufficient buttons ranges memory[%u<%u] declaration!", ELEMENTS(brm->ranges), buttons_ranges_elements);
        return false;
    }

    memset(brm, 0, sizeof(*brm));
    memset(buttons_ranges, 0, buttons_ranges_elements * sizeof(*buttons_ranges));

    return true;
}

static BUTTON_RANGE * button_range_manager_find_range_by_value(BUTTON_RANGE_MANAGER * brm, int32_t in_range)
{
    for (int32_t i = 0; i < brm->ranges_instances; i++)
    {
        if (button_range_is_value_in_range(brm->ranges[i], in_range))
        {
            return brm->ranges[i];
        }
    }
    return NULL;
}

static BUTTON_RANGE * button_range_manager_find_range_by_defined_ranges(BUTTON_RANGE_MANAGER * brm, int32_t lower_range, int32_t upper_range)
{
    for (int32_t i = 0; i < brm->ranges_instances; i++)
    {
        if ((lower_range == brm->ranges[i]->lower_range) && (upper_range == brm->ranges[i]->upper_range))
        {
            return brm->ranges[i];
        }
    }
    return NULL;
}

static void button_range_manager_add_range(BUTTON_RANGE_MANAGER * brm, BUTTON_RANGE * button_range)
{
    brm->ranges[brm->ranges_instances++] = button_range;

    /* Sort ranges by accending thresold values */
    for (int32_t i = 1; i < brm->ranges_instances; i++)
    {
        for (int32_t j = i;
            (j > 0) && (button_range_get_lower_limit(brm->ranges[j - 1]) > button_range_get_lower_limit(brm->ranges[j]));
            j--)
        {
            BUTTON_RANGE * tmp;

            tmp = brm->ranges[j - 1];
            brm->ranges[j - 1] = brm->ranges[j];
            brm->ranges[j] = tmp;
        }
    }

    /* Check for overlaps */
    for (int32_t i = 1; i < brm->ranges_instances; i++)
    {
        int32_t prev_upper_range;
        int32_t curr_lower_range;

        prev_upper_range = button_range_get_upper_limit(brm->ranges[i - 1]);
        curr_lower_range = button_range_get_lower_limit(brm->ranges[i]);

        if (prev_upper_range >= curr_lower_range)
        {
            LOG(E, "Range limits are overlapping for ranges %u(%u:%u) and %u(%u:%u)",
                i-1, brm->ranges[i - 1u]->lower_range, brm->ranges[i - 1u]->upper_range,
                i, brm->ranges[i]->lower_range, brm->ranges[i]->upper_range);
            button_range_dump(brm->ranges[i - 1u]);
            button_range_dump(brm->ranges[i]);
        }
    }
}

static void button_range_manager_dump(const BUTTON_RANGE_MANAGER * brm)
{
    LOG(I, "Defined %u ranges", brm->ranges_instances);
    for (int32_t i = 0; i < brm->ranges_instances; i++)
    {
        button_range_dump(brm->ranges[i]);
    }
}

static BUTTON_RANGE * button_range_create(BUTTON_RANGE_MANAGER * brm)
{
    int32_t button_range_instance = brm->ranges_instances;
    if (button_range_instance >= CONNECTOR_HMI_BUTTON_RANGE_INSTANCES)
    {
        LOG(E, "Insufficient memory for requrired buttons combinations");
        return NULL;
    }

    BUTTON_RANGE * br = &buttons_ranges[button_range_instance];

    for (uint32_t i = 0u; i < ELEMENTS(br->index); i++)
    {
        br->index[i] = EVENT_NUM_BUTTONS;
    }

    br->index_num = 0u;
    br->lower_range = 0;
    br->upper_range = 0;
    return br;
}

static void button_range_add_adc_button(BUTTON_RANGE * br, const ADC_BUTTON * button)
{
    if (button->lower_range > button->upper_range)
    {
        LOG(E,"button %u invalid ranges: lower[%u] > upper[%u]", button->index, button->lower_range, button->upper_range);
        return;
    }

    br->index[br->index_num++] = button->index;
    br->lower_range = button->lower_range;
    br->upper_range = button->upper_range;
}

static void button_range_dump(const BUTTON_RANGE * br)
{
    char event_buff[100];
    int event_buff_i;

    event_buff_i = 0;
    for (uint32_t i = 0u; i < br->index_num; i++)
    {
        event_buff_i += snprintf(&event_buff[event_buff_i], sizeof(event_buff), " %u", br->index[i]);
    }

    LOG(I, "Range [MIN] %5u [MAX] %5u with %u button event(s): %s",
        button_range_get_lower_limit(br),
        button_range_get_upper_limit(br),
        br->index_num,
        event_buff);
}

static bool button_range_is_value_in_range(const BUTTON_RANGE * br, int32_t value_mv)
{
    if ((value_mv >= br->lower_range) && (value_mv <= br->upper_range))
    {
        return true;
    }
    else
    {
        return false;
    }
}

static void initialize_button_range(const ADC_BUTTON * button)
{
    BUTTON_RANGE * range;

    LOG(I, "Searching for range with limits %u:%u mV", button->lower_range, button->upper_range);
    range = button_range_manager_find_range_by_defined_ranges(&button_range_manager, button->lower_range, button->upper_range);

    // Is there already a range covering this threshold? If not, create it.
    if (range != NULL)
    {
        LOG(I, "Add button event %u to range within limits %u:%u mV", button->index, button->lower_range, button->upper_range);
        button_range_add_adc_button(range, button);
    }
    else
    {
        LOG(I, "No range found with limits %u:%u mV, creating new one", button->lower_range, button->upper_range);
        range = button_range_create(&button_range_manager);
        TRUE_CHECK_RETURN(range);
        button_range_add_adc_button(range, button);
        button_range_manager_add_range(&button_range_manager, range);
    }
}

static int32_t button_range_get_upper_limit(const BUTTON_RANGE * br)
{
    return br->upper_range;
}

static int32_t button_range_get_lower_limit(const BUTTON_RANGE * br)
{
    return br->lower_range;
}

void adc_buttons_initialize_ranges(const ADC_BUTTON * buttons, uint32_t button_elements)
{
    bool is_brm_init = button_range_manager_initialize(&button_range_manager, buttons_ranges, ELEMENTS(buttons_ranges));
    TRUE_CHECK_RETURN(is_brm_init);

    for (uint32_t i = 0; i < button_elements; i++)
    {
        const ADC_BUTTON * adc_button = &buttons[i];

        initialize_button_range(adc_button);
    }

    button_range_manager_dump(&button_range_manager);
}

BUTTON_RANGE * adc_buttons_find_range_by_value(uint32_t adc_val)
{
    return button_range_manager_find_range_by_value(&button_range_manager, adc_val);
}

void adc_buttons_dump_range_and_value(const BUTTON_RANGE * range, uint32_t adc_val)
{
    char event_buff[100];
    int event_buff_i;

    event_buff_i = 0;
    for (uint32_t i = 0u; i < range->index_num; i++)
    {
        event_buff_i += snprintf(&event_buff[event_buff_i], sizeof(event_buff), " %u", range->index[i]);
    }

    LOG(I, "Range: [MIN] %5u : [ADC_VAL] %5u : [MAX] %5u with %u button event(s): %s",
        button_range_get_lower_limit(range),
        adc_val,
        button_range_get_upper_limit(range),
        range->index_num,
        event_buff);
}

#endif /* CONNECTOR_HMI_ADC_USE_RANGE_DETECTION_MECHANISM */
