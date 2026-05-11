#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include "event.h"

enum NUM_BUT_HOLD
{
	ONE_BUTTON_HOLD = 1,
	TWO_BUTTON_HOLD,
	THREE_BUTTON_HOLD
};

enum NUM_ROTARY_DIRECTION
{
    ROTARY_DIRECTION_CW,
    ROTARY_DIRECTION_CCW,
    ROTARY_NUM_DIRECTIONS,
};

#ifdef CONNECTOR_HMI_ADC_BUTTONS
//! Holds button to ADC value assignments.
typedef struct ADC_BUTTON {
#if defined(CONNECTOR_HMI_ADC_USE_RANGE_DETECTION_MECHANISM)
	enum EVENT_BUTTON_INDEX index;	//!< Button to trigger
	int lower_range;
	int upper_range;
#else // CONNECTOR_HMI_ADC_USE_RANGE_DETECTION_MECHANISM
	int index;	//!< Button to trigger
	int adcval;	//!< ADC value to trigger button at
#endif // !CONNECTOR_HMI_ADC_USE_RANGE_DETECTION_MECHANISM
} ADC_BUTTON;
#endif // CONNECTOR_HMI_ADC_BUTTONS

/************ KEY RELEASE ************/
//<! Holds runtime state for a key release event.
typedef struct _KEY_RELEASE_STATE
{
	bool pushed;                            //!< True if user has pushed an event buttons.
	bool pushed_once;                       //!< True if user has pushed once but not yet released event buttons.
} KEY_RELEASE_STATE;

//<! Holds static data for a key release event.
typedef struct _KEY_RELEASE_DATA
{
    uint8_t state_index;                    //!< Index for accessing event runtime state that will be provided to the ui_engine
    const EVENT_KEY_RELEASE * key_release;  //!< Pointer to static event data for a key release event.
} KEY_RELEASE_DATA;

//<! Holds the static data for a key release event and the current state of the button.
typedef struct _KEY_RELEASE_DEF
{
    KEY_RELEASE_DATA key_release_data;
    KEY_RELEASE_STATE key_release_state;
} KEY_RELEASE_DEF;

//<! Key release definition structure that holds array of key release events.
typedef struct _KEY_RELEASE_EVENT_DEF
{
    uint8_t num_key_release_events;
    KEY_RELEASE_DEF key_release_event[EVENT_MAX_KEY_RELEASE];
} KEY_RELEASE_EVENT_DEF;
/************ KEY PRESS END ************/


/************ KEY HOLD ************/
//<! Holds runtime state for a key hold event.
typedef struct KEY_HOLD_STATE
{
    uint32_t trigger_time;              //!< Time a which the event should next be triggered.
    uint8_t	repeat_count;               //!< Number of times the event has been triggered since user pressed correct buttons.
} KEY_HOLD_STATE;

//<! Holds static data for a key hold event.
typedef struct _KEY_HOLD_DATA
{
    uint8_t state_index;                //!< Index for accessing event runtime state that will be provided to the ui_engine
    const EVENT_KEY_HOLD * key_hold;    //!< Pointer to static event data for a key hold event.
} KEY_HOLD_DATA;

//<! Holds the static data for a key hold event and the current state of the buttons.
typedef struct _KEY_HOLD_DEF
{
    KEY_HOLD_DATA key_hold_data;
    KEY_HOLD_STATE key_hold_state;
} KEY_HOLD_DEF;

//<! Key hold definition structure that holds array of key hold events.
typedef struct _KEY_HOLD_EVENT_DEF
{
    uint8_t num_key_hold_events;
    KEY_HOLD_DEF key_hold_event[EVENT_MAX_KEY_HOLD];
} KEY_HOLD_EVENT_DEF;
/************ KEY HOLD END ************/


/************ KEY PRESS ************/
//<! Holds current state of the button.
typedef struct _KEY_PRESS_STATE
{
    bool pushed;                        //!< True if user has pushed but not yet released event buttons defined by the key_press event.
} KEY_PRESS_STATE;

//<! Holds static data for a key press event.
typedef struct _KEY_PRESS_DATA
{
    uint8_t state_index;                //!< Index for accessing event runtime state that will be provided to the ui_engine
    const EVENT_KEY_PRESS * key_press;  //!< Pointer to static event data for a key press event.
} KEY_PRESS_DATA;

//<! Holds the static data for a key press event and the current state of the button.
typedef struct _KEY_PRESS_DEF
{
    KEY_PRESS_DATA key_press_data;
    KEY_PRESS_STATE key_press_state;
} KEY_PRESS_DEF;

//<! Key press definition structure that holds array of key press events.
typedef struct _KEY_PRESS_EVENT_DEF
{
    uint8_t num_key_press_events;
    KEY_PRESS_DEF key_press_event[EVENT_MAX_KEY_PRESS];
} KEY_PRESS_EVENT_DEF;
/************ KEY PRESS END ************/


/************ KEY ROTARY ************/
typedef struct ROTARY_STATE
{
    uint32_t last_change;
    int8_t steps;
    int8_t prev_position;
} ROTARY_STATE;

typedef struct _ROTARY_DATA
{
    uint8_t state_index;                 //!< Index for accessing event runtime state that will be provided to the ui_engine
    const EVENT_ROTATION * rotation;
} ROTARY_DATA;

//! Holds runtime state for a connected rotary encoder.
typedef struct _ROTARY_EVENT_DEF
{
    ROTARY_STATE rotary_state;
    ROTARY_DATA rotary_data[ROTARY_NUM_DIRECTIONS];
} ROTARY_EVENT_DEF;

typedef struct ROTARY_DEF
{
    ROTARY_EVENT_DEF rotary[EVENT_NUM_ROTARIES];
} ROTARY_DEF;
/************ KEY ROTARY END ************/

void input_manager_init(void);
void input_manager_init_ulp(void);
void input_manager_process_event(uint32_t event_id, const uint8_t * const data, size_t data_size);
#endif //INPUT_MANAGER_H
