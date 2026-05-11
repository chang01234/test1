/*! \file screen.c
 *	\brief Module for tracking and updating which graphic objects should be
 *	drawn on the HMI display.
 *
 *	The screen module provides three public functions. screen_change() is used
 *	to change the currently active screen. screen_update() is used to update the
 *	currently active screen when system state changes. screen_ticks_update() is
 *	used to keep track of system tick time to handle blinking or time delayed
 *	graphic objects.
 *
 * 	An implementation uses screen_change() to change which screen is currently
 *	displayed. The input parameter will be a pointer to a SCREEN_DEF struct that
 *	describes what should be shown.
 *
 *	An implementation must call screen_change() every time system state changes
 *	to keep displayed graphics synchronized with actual system state. And should
 *	call screen_ticks_update() frequently to ensure time-based graphic objects
 *	are updated as intended.
 *
 *	HMI graphics are organized into screens. Each screen usually consists of a
 *	single background, a single set of frame buffer coordinates, and zero or
 *	more sprites and/or strings that are to be displayed on top of this
 *	background. Each sprite or string in turn has zero or more conditions
 *	attached to it which describe when it should be shown. Screens additionally
 *	contain a list of all system state variables which are used by its strings
 *	and conditions. Screen updates check against this list to determine if a
 *	state change needs further evaluation to determine new screen state.
 *
 *	The actual graphic objects to be drawn on the display are either sprites,
 *	backgrounds, or strings.
 *
 *	The simplest type of graphic object is a sprite which consists of a bitmap,
 *	a set of coordinates at which this bitmap should be drawn, and a set of
 *	conditions which determine when the bitmap should be drawn.
 *
 *	Backgrounds are similar to sprites but contain no coordinates since they
 *	always cover the whole screen. They do contain lists of conditions which can
 *	be used to select from multiple backgrounds. This is intended for situations
 *	where a screen needs to have dynamic graphics outside of the area covered by
 *	the frame buffer. Unlike sprites or strings only one background can be
 *	active at any given time. Also unlike sprites or strings it is not possible
 *	to switch only the screen background. If system state changes lead to a
 *	change in active background the whole screen will be redrawn.
 *
 *	String objects are used to dynamically draw text or numbers on the display.
 *	Like sprites these contain a display position and a list of coordinates but
 *	instead of a bitmap have a font, type, and set of type-specific data. The
 *	string type is used to select a handler which in turn uses the included
 *	type-specific data to generate a set of glyph indexes which are then used by
 *	the draw module together with the font to draw the text or number on the
 *	display.
 *
 *	All object types can include a list of conditions all of which must be true
 *	for the object to be drawn on the screen. Objects contain a type to select
 *	the right handler and a set type-specific data which the handler uses to
 *	determine if the condition is true or not. Typically this will be dependent
 *	on either the value of a system state variable or the number of system ticks
 *	since the most recent screen change. If an object has no conditions it will
 *	always be drawn.
 *
 *	Make sure HMI_SCREEN_DEBUG_NAMES is defined globally when building fw and
 *	hmi_data for debugging screen names.
 */

/** Includes ******************************************************************/
#include "configuration.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ddm2_parameter_list.h"
#include "draw.h"
#include "hmi_dispatcher.h"
#include "screen.h"
#include "screen_data.h"
#include "sorted_container.h"
#include "varstate.h"

/** Defines *******************************************************************/
#define MAX_NUM_CONDITION_TIMERS 20
#define MS_TO_TICKS(delay)       ((pdMS_TO_TICKS(delay)) != 0 ? (pdMS_TO_TICKS(delay)) : 1)

//! Enumeration of all possible sprite/string states.
typedef enum OBJECT_STATE
{
    STATE_HIDDEN,
    STATE_SHOWN,
} OBJECT_STATE;

/** Private prototypes ********************************************************/
static int32_t brightness_conversion_default(uint8_t level);
static void redraw_display(void);
static void update_display(uint8_t var_index, const SCREEN_CONDITION *condition);
bool evalutate_string_item(const SCREEN_STRING *p_string, uint8_t var_index);
bool evalutate_sprite_item_by_index(const SCREEN_SPRITE *p_sprite, uint8_t var_index);
bool evalutate_sprite_item_by_condition(const SCREEN_SPRITE *p_sprite, const SCREEN_CONDITION *p_condition);
bool evaluate_conditions(const SCREEN_CONDITION *first_condition, bool *p_redraw_background);
const SCREEN_BACKGROUND *select_background(const SCREEN_BACKGROUND *background);
void generate_string(uint8_t *strbuf, const SCREEN_STRING *string);
void generate_string_from_unsigned(uint8_t *strbuf, const SCREEN_STRING *string);

/** Variables *****************************************************************/
static const SCREEN_DEF *current_screen = NULL;             //!< Active screen pointer.
static const SCREEN_BACKGROUND *current_background = NULL;  //!< Active background pointer.
static const SCREEN_DEF *previous_screen = NULL;            //!< Previous screen pointer.

static uint8_t screen_brightness;  //!< Brightness level used to save and restore from flash
static nvs_handle_t hmi_setting_nvs = 0;

static screen_hmi_settings_variable_t l_hmi_settings[] = {
    {.name = "hmi0backlight"},
    {.name = "hmi0timeformat"},
    {.name = "hmi0tempunit"},
    {.name = "hmi0sound"},
    {.name = "hmi0childlock"},
    {.name = "hmi0mute"},
    {.name = "hmi0screentimeout"},
};
//! State for all sprites and strings on current screen.
static EXT_RAM_ATTR OBJECT_STATE object_states[SCREEN_MAX_OBJECTS];

static screen_change_cb_t screen_update_callback = NULL;
static screen_display_brightness_conversion_cb_t screen_display_brightness_conversion = brightness_conversion_default;
static screen_hmi_settings_save_cb_t screen_hmi_settings_save_cb = NULL;

/*!< Structure that holds the data of the timer used for resolving the
    event(screen condition) for which the timer kicked off. It will be used
    by screen_timer_delay() once the connector_task is unblocked.
 */
typedef struct _DELAY_SCREEN_TIMER_DATA
{
    const SCREEN_CONDITION *screen_condition;  // Object's condition for which the timer kicked off
    const SCREEN_DEF *screen_definition;       // Active screen when the timer was created
    const SCREEN_SPRITE *p_sprite;
} DELAY_SCREEN_TIMER_DATA;

typedef struct _DELAY_SCREEN_TIMER
{
    TimerHandle_t timer;
    StaticTimer_t timer_buffer;
    const SCREEN_CONDITION *screen_condition;  // Object's condition for which a timer is created
    union
    {
        bool timer_tiggered;        // one-shot timer: true if triggered
        bool timer_on_time_status;  // periodic timer: true if flips on->off; false if flips off->on
    };
    uint8_t timer_type;  // SCREEN_DELAY_ON/OFF, SCREEN_DELAY_PERIODIC
} DELAY_SCREEN_TIMER;

//!< Structure that holds the definition of the timers
typedef struct _DELAY_SCREEN_TIMER_DEF
{
    const SCREEN_DEF *screen_definition;  // Active screen when the timer is created
    SORTED_CONTAINER *screen_timers;      // Timers container, that keeps all the timers used by the active screen
} DELAY_SCREEN_TIMER_DEF;

SORTED_CONTAINER__DECLARE_EXTRAM(screen_timers, MAX_NUM_CONDITION_TIMERS, DELAY_SCREEN_TIMER);  // Timers container that keeps all the timers used by the active screen
static EXT_RAM_ATTR DELAY_SCREEN_TIMER_DEF screen_timers_def;

COMPILE_TIME_ASSERT(sizeof(SORTED_LIST_KEY_TYPE) >= sizeof(SCREEN_CONDITION *));

//! \brief Timer callback for delayed on/off events
static void screen_delay_on_off_timer_callback(TimerHandle_t xTimer)
{
    DELAY_SCREEN_TIMER_DATA screen_timer_data;
    // Timer id refers to the screen condition for which the timer has fired
    uint32_t timer_id = (uint32_t)pvTimerGetTimerID(xTimer);

    screen_timer_data.screen_condition = (const SCREEN_CONDITION *)timer_id;
    screen_timer_data.screen_definition = screen_timers_def.screen_definition;

    // Unblock connector_task and process the screen condition in screen_timer_delay() function
    hmi_dispatch_event_to_connector(HMI_SET_MODULE_INTERNAL_EVENT(HMI_MODULE_TYPE_DISPLAY_MANAGER, HMI_TYPE_INTERNAL_EVENTS_SCREEN_TIMER_DELAY), &screen_timer_data, sizeof(screen_timer_data), portMAX_DELAY);
}

//! \brief Timer callback for delayed periodic events
static void screen_delay_periodic_timer_callback(TimerHandle_t xTimer)
{
    DELAY_SCREEN_TIMER_DATA screen_timer_data;
    // Timer id refers to the screen condition for which the timer has fired
    uint32_t timer_id = (uint32_t)pvTimerGetTimerID(xTimer);

    screen_timer_data.screen_condition = (const SCREEN_CONDITION *)timer_id;
    screen_timer_data.screen_definition = screen_timers_def.screen_definition;

    // Reference to the screen timer for which the timer callback has fired
    DELAY_SCREEN_TIMER *screen_timer = sorted_container__access(screen_timers_def.screen_timers, timer_id);
    if (screen_timer->screen_condition->period.on_time == pdTICKS_TO_MS(xTimerGetPeriod(xTimer)))  // Show object
    {
        xTimerChangePeriod(xTimer, pdMS_TO_TICKS(screen_timer->screen_condition->period.period - screen_timer->screen_condition->period.on_time), portMAX_DELAY);
    }
    else  // Hide object
    {
        xTimerChangePeriod(xTimer, pdMS_TO_TICKS(screen_timer->screen_condition->period.on_time), portMAX_DELAY);
    }

    // Unblock connector_task and process the screen condition in screen_timer_delay() function
    hmi_dispatch_event_to_connector(HMI_SET_MODULE_INTERNAL_EVENT(HMI_MODULE_TYPE_DISPLAY_MANAGER, HMI_TYPE_INTERNAL_EVENTS_SCREEN_TIMER_DELAY), &screen_timer_data, sizeof(screen_timer_data), portMAX_DELAY);
}

//! \brief Create timers for delayed screen conditions
static uint8_t create_object_timers(const SCREEN_CONDITION *const first_condition)
{
    uint32_t occupied;
    const SCREEN_CONDITION *condition = first_condition;

    while (condition)
    {
        switch (condition->type)
        {
        case SCREEN_DELAY_ON:
        // fallthrough
        case SCREEN_DELAY_OFF:
        {
            occupied = sorted_container__occupied(screen_timers_def.screen_timers);
            if (occupied >= MAX_NUM_CONDITION_TIMERS)
            {
                LOG(E, "Max number[%d] of time delayed objects exceeded", MAX_NUM_CONDITION_TIMERS);
                return 0;
            }

            DELAY_SCREEN_TIMER *timer;
            TRUE_CHECK_RETURN0(timer = sorted_container__new(screen_timers_def.screen_timers, (uint32_t)condition));
            TRUE_CHECK_RETURN0(timer->timer = xTimerCreateStatic(
                                   NULL,
                                   MS_TO_TICKS(condition->delay.length),
                                   false,
                                   (void *const)condition,  // Timer id refers to the screen condition for which the timer is created
                                   screen_delay_on_off_timer_callback,
                                   &timer->timer_buffer));

            timer->timer_tiggered = false;
            timer->screen_condition = condition;
            timer->timer_type = condition->type;

            break;
        }
        case SCREEN_DELAY_PERIODIC:
        {
            occupied = sorted_container__occupied(screen_timers_def.screen_timers);
            if (occupied >= MAX_NUM_CONDITION_TIMERS)
            {
                LOG(E, "Max number[%d] of time delayed objects exceeded", MAX_NUM_CONDITION_TIMERS);
                return 0;
            }

            DELAY_SCREEN_TIMER *timer;
            TRUE_CHECK_RETURN0(timer = sorted_container__new(screen_timers_def.screen_timers, (uint32_t)condition));
            TRUE_CHECK_RETURN0(timer->timer = xTimerCreateStatic(
                                   NULL,
                                   MS_TO_TICKS(condition->period.on_time),
                                   false,
                                   (void *const)condition,  // Timer id refers to the screen condition for which the timer is created
                                   screen_delay_periodic_timer_callback,
                                   &timer->timer_buffer));

            timer->timer_on_time_status = true;
            timer->screen_condition = condition;
            timer->timer_type = condition->type;

            break;
        }
        default:
            break;
        }

        condition = (condition->next_condition != NULL ? (const SCREEN_CONDITION *)HMI_DATA_ADDRESS(condition->next_condition) : NULL);
    }

    return 1;
}

//! \brief Create timers for delayed screen conditions defined by the active screen
static uint8_t create_screen_timers(const SCREEN_DEF *current_screen)
{
    screen_timers_def.screen_definition = current_screen;

    // Create timers for delayed background conditions
    const SCREEN_BACKGROUND *background = current_screen->first_background;
    while (background)
    {
        background = (const SCREEN_BACKGROUND *)HMI_DATA_ADDRESS(background);
        if (create_object_timers((background->first_condition != NULL ? (const SCREEN_CONDITION *)HMI_DATA_ADDRESS(background->first_condition) : NULL)) == 0)
        {
            return 0;
        }
        background = background->next_background;
    }

    // Create timers for delayed sprite conditions
    const SCREEN_SPRITE *sprite = current_screen->first_sprite;
    while (sprite)
    {
        sprite = (const SCREEN_SPRITE *)HMI_DATA_ADDRESS(sprite);
        if (create_object_timers((sprite->first_condition != NULL ? (const SCREEN_CONDITION *)HMI_DATA_ADDRESS(sprite->first_condition) : NULL)) == 0)
        {
            return 0;
        }
        sprite = sprite->next_sprite;
    }

    // Create timers for delayed string conditions
    const SCREEN_STRING *string = current_screen->first_string;
    while (string)
    {
        string = (const SCREEN_STRING *)HMI_DATA_ADDRESS(string);
        if (create_object_timers((string->first_condition != NULL ? (const SCREEN_CONDITION *)HMI_DATA_ADDRESS(string->first_condition) : NULL)) == 0)
        {
            return 0;
        }
        string = string->next_string;
    }

    return 1;
}

//! \brief Stop timers execution when switiching to a new screen
static void stop_screen_timers(void)
{
    uint32_t occupied = sorted_container__occupied(screen_timers_def.screen_timers);
    for (uint32_t i = 0; i < occupied; ++i)
    {
        uint32_t key;
        DELAY_SCREEN_TIMER *timer;

        sorted_container__iterate(screen_timers_def.screen_timers, i, (void **)&timer, &key);

        if (timer)
        {
            TRUE_CHECK(xTimerStop(timer->timer, portMAX_DELAY));
            TRUE_CHECK(xTimerDelete(timer->timer, portMAX_DELAY));
        }
    }

    sorted_container__delete_all(screen_timers_def.screen_timers);
}

//! \brief Start screen timers when switiching to a new screen
static void start_screen_timers(void)
{
    uint32_t ocuppied = sorted_container__occupied(screen_timers_def.screen_timers);
    for (uint32_t i = 0; i < ocuppied; ++i)
    {
        uint32_t key;
        DELAY_SCREEN_TIMER *timer;

        sorted_container__iterate(screen_timers_def.screen_timers, i, (void **)&timer, &key);

        if (timer)
        {
            TRUE_CHECK(xTimerStart(timer->timer, portMAX_DELAY));
        }
    }
}

//! \brief Initializes screen module.
void screen_init(void)
{
    current_screen = NULL;
    current_background = NULL;

    memset(&object_states[0], 0, sizeof(object_states));
    screen_timers_def.screen_timers = &screen_timers;

    // Load HMI settings from NVS
    if (nvs_open("hmi_setting", NVS_READWRITE, &hmi_setting_nvs) != ESP_OK)
    {
        LOG(E, "Failed to open NVS (hmi_setting).");
    }
    else
    {
        uint8_t value;

        if (nvs_get_u8(hmi_setting_nvs, "brightness", &value) != ESP_OK)
        {
            LOG(W, "Could not find (brightness)");
        }
        else
        {
            // Restore brightness value
            LOG(W, "Restore (brightness: %d)", value);
            screen_display_brightness_level_set(value);
            // How to load varstate with this value?
        }
    }
}

void screen_set_change_cb(screen_change_cb_t cb)
{
    screen_update_callback = cb;
}

void screen_set_display_brightness_cb(screen_display_brightness_conversion_cb_t cb)
{
    screen_display_brightness_conversion = cb;
}

/*! \brief Change currently displayed screen.
 *
 * 	\param screen   Pointer to new screen definition.
 */
void screen_change(const SCREEN_DEF *screen)
{
    // Only redraw if actually changing screen
    if (current_screen != screen)
    {
        current_screen = screen;

        stop_screen_timers();
        create_screen_timers(current_screen);
        start_screen_timers();

#ifdef HMI_SCREEN_DEBUG_NAMES
        LOG(D, "Redrawing %s", (const char *)HMI_DATA_ADDRESS(screen->name));
#endif
        if (NULL != screen_update_callback)
        {
            screen_update_callback(current_screen);
        }
        // Need to clear internal screen cache info here.
        memset(&object_states[0], 0, sizeof(object_states));
        redraw_display();
    }

    previous_screen = current_screen;
}

/*! \brief Check if screen needs to be updated when system state changes.
 *
 * 	Checks if current screen has state that depends on the provided state index
 * 	and if so reevaluates conditions for all screen objects and updates display
 * 	and screen module state.
 *
 * 	\param var_index    Index of changed system state variable.
 */
void screen_update(uint8_t var_index)
{
    if (current_screen != NULL)
    {
        const uint8_t *screen_indexes = current_screen->state_indexes;

        for (size_t i = 0; i < current_screen->num_state_indexes; ++i)
        {
            if (var_index == screen_indexes[i])
            {
                LOG(D, "var_index: %d triggers update_display()", var_index);
                update_display(var_index, NULL);
                return;
            }
        }
    }
}

/*! \brief Checks the state of timed conditions and update the screen.
 *
 *	This functions is called when timer event is provided by screen_delay_on_off_timer_callback()
 *	or screen_delay_periodic_timer_callback() callback functions. If the active screen has changed
 *	before the timer event is processed, the timer events will be discarded.
 *
 *	\param data          Holds the screen timer data structure
 *	\param data_size     Size of screen timer data structure
 */
void screen_timer_delay(const void *const data, size_t data_size)
{
    DELAY_SCREEN_TIMER *timer = NULL;
    DELAY_SCREEN_TIMER_DATA *timer_data = NULL;

    TRUE_CHECK_RETURN(data);
    TRUE_CHECK_RETURN(data_size == sizeof(DELAY_SCREEN_TIMER_DATA));

    timer_data = (DELAY_SCREEN_TIMER_DATA *)data;
    if (timer_data->screen_definition != current_screen)
    {
        LOG(W, "Screen has changed. Dismissing timer event.");
        return;
    }

    TRUE_CHECK(timer = sorted_container__access(screen_timers_def.screen_timers, (uint32_t)timer_data->screen_condition));
    switch (timer->timer_type)
    {
    case SCREEN_DELAY_OFF:
    // fallthrough
    case SCREEN_DELAY_ON:
    {
        timer->timer_tiggered = true;
        break;
    }
    case SCREEN_DELAY_PERIODIC:
    {
        timer->timer_on_time_status = !timer->timer_on_time_status;
        break;
    }
    default:
        LOG(E, "Unsupported timer type[%d]", timer->timer_type);
        return;
    }
    update_display(0xFF, timer->screen_condition);
}

/*! \brief Default brightness level conversion function.
 *
 *  Allowed default levels are [0..8]
 *  Can be overridden by product implementation
 */
static int32_t brightness_conversion_default(uint8_t level)
{
    assert(level <= 8);

    return (int32_t)(level)*CONNECTOR_HMI_BACKLIGHT_RESOLUTION / 8;
}

/*! \brief Reevaluate and redraw all screen elements.
 *
 *	Clears screen and frame buffer and then redraws everything based on data
 *	stored in current_screen. Also reinitializes all object_states.
 */
static void redraw_display(void)
{
    size_t state_index = 0;

    // Find active background and start full screen redraw.
    current_background = select_background((const SCREEN_BACKGROUND *)HMI_DATA_ADDRESS(current_screen->first_background));
    draw_screen_init(
        (current_screen->palette != NULL ? (const PALETTE_DEF *)HMI_DATA_ADDRESS(current_screen->palette) : NULL),
        (current_background->bitmap != NULL ? (const BITMAP_DEF *)HMI_DATA_ADDRESS(current_background->bitmap) : NULL),
        current_screen->fb_x_pos, current_screen->fb_y_pos, current_screen->fb_x_size, current_screen->fb_y_size, false);

    // Initialize state for all sprites and any draw active sprites to frame buffer.
    const SCREEN_SPRITE *sprite = current_screen->first_sprite;
    while (sprite != NULL)
    {
        sprite = (const SCREEN_SPRITE *)HMI_DATA_ADDRESS(sprite);
        if (state_index >= ELEMENTS(object_states))
        {
            LOG(W, "number of sprites in screen data exceedes " STR(ELEMENTS(object_states)) " supported by the firmware");
            break;
        }

        if (evaluate_conditions((sprite->first_condition != NULL ? (const SCREEN_CONDITION *)HMI_DATA_ADDRESS(sprite->first_condition) : NULL), NULL))
        {
            LOG(D, "object_state: %d to SHOWN", state_index);
            draw_sprite((const BITMAP_DEF *)HMI_DATA_ADDRESS(sprite->bitmap), sprite->x_pos, sprite->y_pos);
            object_states[state_index] = STATE_SHOWN;
        }
        else
        {
            if (object_states[state_index] == STATE_SHOWN)
            {
                LOG(D, "object_state: %d to HIDDEN", state_index);
            }
            object_states[state_index] = STATE_HIDDEN;
        }

        sprite = sprite->next_sprite;
        ++state_index;
    }

    // Initialize state for all strings and any draw active strings to frame buffer.
    const SCREEN_STRING *string = current_screen->first_string;
    while (string != NULL)
    {
        string = (const SCREEN_STRING *)HMI_DATA_ADDRESS(string);
        uint8_t strbuf[SCREEN_MAX_STRING_LENGTH] = {0};

        if (state_index >= ELEMENTS(object_states))
        {
            LOG(W, "number of strings in screen data exceedes " STR(ELEMENTS(object_states)) " supported by the firmware");
            break;
        }
        if (evaluate_conditions((string->first_condition != NULL ? (const SCREEN_CONDITION *)HMI_DATA_ADDRESS(string->first_condition) : NULL), NULL))
        {
            generate_string(strbuf, string);
            draw_string((const FONT_DEF *)HMI_DATA_ADDRESS(string->font), strbuf, string->x_pos, string->y_pos, string->x_size, string->y_size);
            object_states[state_index] = STATE_SHOWN;
        }
        else
        {
            object_states[state_index] = STATE_HIDDEN;
        }

        string = string->next_string;
        ++state_index;
    }

    if (state_index > SCREEN_MAX_OBJECTS)
    {
        LOG(E, ": size exceed'");
        assert(0);
    }
    // Finalize full screen redraw by updating screen from frame buffer.
    draw_screen();
}

/*! \brief Update current screen state.
 *
 * 	Rechecks which sprites, strings, and backgrounds should be displayed.
 * 	Updates values for all displayed strings. Calls for a full screen redraw if
 * 	background changes.
 *
 *  \param[in] condition Condition that triggered call. NULL if all conditions to be checked
 *  \param[in] var_index Variable that triggered call, 0xFF if all variables to be checked
 */
static void update_display(uint8_t var_index, const SCREEN_CONDITION *condition)
{
    size_t state_index = 0;

    // Check that background hasn't changed. Force a full screen redraw if it has.
    const SCREEN_BACKGROUND *active_background = select_background((const SCREEN_BACKGROUND *)HMI_DATA_ADDRESS(current_screen->first_background));

    if (active_background != current_background)
    {
        redraw_display();
        return;
    }

    // Draw all sprites that have gone from hidden to shown.
    const SCREEN_SPRITE *sprite = current_screen->first_sprite;
    while (sprite != NULL)
    {
        sprite = (const SCREEN_SPRITE *)HMI_DATA_ADDRESS(sprite);
        bool b_redraw_bg = false;
        if (state_index >= ELEMENTS(object_states))
        {
            LOG(W, "number of sprites (%d) in screen data exceedes " STR(ELEMENTS(object_states)) " supported by the firmware", ELEMENTS(object_states));
            break;
        }
        if (evalutate_sprite_item_by_index(sprite, var_index) || evalutate_sprite_item_by_condition(sprite, condition))
        {
            // Should test to draw item
            if (evaluate_conditions((sprite->first_condition != NULL ? (const SCREEN_CONDITION *)HMI_DATA_ADDRESS(sprite->first_condition) : NULL), &b_redraw_bg))
            {
                if (object_states[state_index] == STATE_HIDDEN)
                {
                    LOG(D, "Sprite object_state: %d to SHOWN", state_index);
                    draw_sprite((const BITMAP_DEF *)HMI_DATA_ADDRESS(sprite->bitmap), sprite->x_pos, sprite->y_pos);
                    object_states[state_index] = STATE_SHOWN;
                }
            }
            else
            {
                if (object_states[state_index] == STATE_SHOWN)
                {
                    if (b_redraw_bg)
                    {
                        draw_clear_area(sprite->x_pos, sprite->y_pos, ((const BITMAP_DEF *)HMI_DATA_ADDRESS(sprite->bitmap))->x_size, ((const BITMAP_DEF *)HMI_DATA_ADDRESS(sprite->bitmap))->y_size);
                    }
                    LOG(D, "Sprite object_state: %d to HIDDEN", state_index);
                }
                object_states[state_index] = STATE_HIDDEN;
            }
        }

        sprite = sprite->next_sprite;
        ++state_index;
    }

    /* Draw all strings that should be shown. Unlike with sprites we redraw any
     * strings that were already shown before update in case their contents
     * have changed.
     */
    const SCREEN_STRING *string = current_screen->first_string;
    while (string != NULL)
    {
        string = (const SCREEN_STRING *)HMI_DATA_ADDRESS(string);
        uint8_t strbuf[SCREEN_MAX_STRING_LENGTH] = {0};

        if (state_index >= ELEMENTS(object_states))
        {
            LOG(W, "number of strings in screen data exceedes " STR(ELEMENTS(object_states)) " supported by the firmware");
            break;
        }
        if (evalutate_string_item(string, var_index))
        {
            // Draw string
            if (evaluate_conditions((string->first_condition != NULL ? (const SCREEN_CONDITION *)HMI_DATA_ADDRESS(string->first_condition) : NULL), NULL))
            {
                if (object_states[state_index] == STATE_HIDDEN)
                {
                    LOG(D, "String object_state: %d first to SHOWN", state_index);
                }
                LOG(D, "String object_state: %d to SHOWN", state_index);
                generate_string(strbuf, string);
                draw_string((const FONT_DEF *)HMI_DATA_ADDRESS(string->font), strbuf, string->x_pos, string->y_pos, string->x_size, string->y_size);
                object_states[state_index] = STATE_SHOWN;
            }
            else
            {
                object_states[state_index] = STATE_HIDDEN;
                LOG(D, "String object_state: %d to HIDDEN", state_index);
            }
        }

        string = string->next_string;
        ++state_index;
    }
}

/*! \brief Evaluate a sprite item to see if it could be affected by a
 *  given var_index
 *
 *  \param[in] p_string Pointer to first string item
 *  \param[in] var_index Variable to check
 *
 *  \return true if item is affected by var_index else false.
 */
bool evalutate_sprite_item_by_index(const SCREEN_SPRITE *p_sprite, uint8_t var_index)
{
    bool ret_val = false;
    // Check if item value is affected by var_index
    if (var_index != 0xFF)
    {
        // Continue and check conditions
        const SCREEN_CONDITION *cond = (p_sprite->first_condition != NULL ? (const SCREEN_CONDITION *)HMI_DATA_ADDRESS(p_sprite->first_condition) : NULL);

        while (cond && !ret_val)
        {
            switch (cond->type)
            {
            case SCREEN_DELAY_PERIODIC:
            // fallthrough
            case SCREEN_DELAY_OFF:
            // fallthrough
            case SCREEN_DELAY_ON:
                break;
            default:
                if (cond->compare.var_index == var_index)
                {
                    ret_val = true;
                }
                break;
            }
            cond = (cond->next_condition != NULL ? (const SCREEN_CONDITION *)HMI_DATA_ADDRESS(cond->next_condition) : NULL);
        }
    }
    return ret_val;
}

/*! \brief Evaluate a sprite item to see if it could be affected by a
 *  given condition
 *
 *  \param[in] p_string Pointer to first string item
 *  \param[in] p_condition Condition to check
 *
 *  \return true if item is affected by condition else false.
 */
bool evalutate_sprite_item_by_condition(const SCREEN_SPRITE *p_sprite, const SCREEN_CONDITION *p_condition)
{
    bool ret_val = false;
    // Check if item value is affected by condition
    if (p_condition)
    {
        // Continue and check conditions
        const SCREEN_CONDITION *cond = (p_sprite->first_condition != NULL ? (const SCREEN_CONDITION *)HMI_DATA_ADDRESS(p_sprite->first_condition) : NULL);

        while (cond && !ret_val)
        {
            if (cond == p_condition)
            {
                // Item found
                ret_val = true;
            }
            cond = (cond->next_condition != NULL ? (const SCREEN_CONDITION *)HMI_DATA_ADDRESS(cond->next_condition) : NULL);
        }
    }
    return ret_val;
}

/*! \brief Evaluate a string item to see if it could be affected by a
 *  given /a var_index
 *
 *  \param[in] p_string Pointer to first string item
 *  \param[in] var_index Variable to check
 *
 *  \return true if string is affected by var_index else false.
 */
bool evalutate_string_item(const SCREEN_STRING *p_string, uint8_t var_index)
{
    bool ret_val = false;
    // Check if item value is affected by var_index
    if (p_string->type == SCREEN_STRING_TYPE_VALUE)
    {
        if (p_string->value.var_index == var_index)
        {
            ret_val = true;
        }
    }
    else
    {
        if (p_string->value_offset.var_index == var_index)
        {
            ret_val = true;
        }
    }
    if ((var_index != 0xFF) && !ret_val)
    {
        // Continue and check conditions
        const SCREEN_CONDITION *cond = (p_string->first_condition != NULL ? (const SCREEN_CONDITION *)HMI_DATA_ADDRESS(p_string->first_condition) : NULL);

        while (cond && !ret_val)
        {
            switch (cond->type)
            {
            case SCREEN_DELAY_PERIODIC:
            // fallthrough
            case SCREEN_DELAY_OFF:
            // fallthrough
            case SCREEN_DELAY_ON:
                break;
            default:
                if (cond->compare.var_index == var_index)
                {
                    ret_val = true;
                }
                break;
            }
            cond = (cond->next_condition != NULL ? (const SCREEN_CONDITION *)HMI_DATA_ADDRESS(cond->next_condition) : NULL);
        }
    }
    return ret_val;
}

/*! \brief Evaluate a list of screen object conditions starting at
 * 	first_condition and return true if entire list is valid.
 *
 * 	\param first_condition  Pointer to first condition in condition list.
 *
 * 	\return true if all conditions in list are valid else false.
 */
bool evaluate_conditions(const SCREEN_CONDITION *first_condition, bool *p_redraw_background)
{
    const SCREEN_CONDITION *cond = first_condition;
    if (p_redraw_background)
    {
        *p_redraw_background = false;
    }
    /* Evaluate conditions until no more conditions. If any condition evaluates
     * to false return false. Note that first condition could be NULL in which
     * case return true.
     */
    while (cond != NULL)
    {
        DDM2_TYPE_ENUM type;
        (void)varstate_get_parameter_type(cond->compare.var_index, &type);
        bool is_struct = false;
        bool is_valid = false;
        // Get data
        int32_t data = 0;
        uint8_t offset = 0;
        uint8_t len = 0;
        switch (cond->type)
        {
        case SCREEN_COMPARE_EQUAL_WITH_OFFSET:
        // fallthrough
        case SCREEN_COMPARE_NOT_EQUAL_WITH_OFFSET:
        // fallthrough
        case SCREEN_COMPARE_LESS_THAN_WITH_OFFSET:
        // fallthrough
        case SCREEN_COMPARE_GREATER_THAN_WITH_OFFSET:
        // fallthrough
        case SCREEN_COMPARE_LESS_THAN_OR_EQUAL_WITH_OFFSET:
        // fallthrough
        case SCREEN_COMPARE_GREATER_THAN_OR_EQUAL_WITH_OFFSET:
            offset = cond->compare_offset.offset;
            len = cond->compare_offset.len;
        // fallthrough
        case SCREEN_COMPARE_EQUAL:
        // fallthrough
        case SCREEN_COMPARE_NOT_EQUAL:
        // fallthrough
        case SCREEN_COMPARE_LESS_THAN:
        // fallthrough
        case SCREEN_COMPARE_GREATER_THAN:
        // fallthrough
        case SCREEN_COMPARE_LESS_THAN_OR_EQUAL:
        // fallthrough
        case SCREEN_COMPARE_GREATER_THAN_OR_EQUAL:
            is_valid = varstate_get_validated_data(&is_struct, cond->compare.var_index, &data, offset, len);
            if (is_valid == false)
            {
                // No data to compare
                return false;
            }
            break;
        default:
            // Do nothing
            break;
        }
        switch (cond->type)
        {
        case SCREEN_COMPARE_EQUAL:
        // fallthrough
        case SCREEN_COMPARE_EQUAL_WITH_OFFSET:
            if (data != cond->compare.value)
            {
                return false;
            }
            break;

        case SCREEN_COMPARE_NOT_EQUAL:
        // fallthrough
        case SCREEN_COMPARE_NOT_EQUAL_WITH_OFFSET:
            if (data == cond->compare.value)
            {
                return false;
            }
            break;

        case SCREEN_COMPARE_LESS_THAN:
        // fallthrough
        case SCREEN_COMPARE_LESS_THAN_WITH_OFFSET:
            if (data >= cond->compare.value)
            {
                return false;
            }
            break;

        case SCREEN_COMPARE_GREATER_THAN:
        // fallthrough
        case SCREEN_COMPARE_GREATER_THAN_WITH_OFFSET:
            if (data <= cond->compare.value)
            {
                return false;
            }
            break;

        case SCREEN_COMPARE_LESS_THAN_OR_EQUAL:
        // fallthrough
        case SCREEN_COMPARE_LESS_THAN_OR_EQUAL_WITH_OFFSET:
            if (data > cond->compare.value)
            {
                return false;
            }
            break;

        case SCREEN_COMPARE_GREATER_THAN_OR_EQUAL:
        // fallthrough
        case SCREEN_COMPARE_GREATER_THAN_OR_EQUAL_WITH_OFFSET:
            if (data < cond->compare.value)
            {
                return false;
            }
            break;

        case SCREEN_DELAY_ON:
        {
            DELAY_SCREEN_TIMER *timer = sorted_container__access(screen_timers_def.screen_timers, (uint32_t)cond);
            if (timer)
            {
                // Show object only when timer kicks off
                if (timer->timer_tiggered == true)
                {
                    return true;
                }
            }
            break;
        }
        case SCREEN_DELAY_OFF:
        {
            DELAY_SCREEN_TIMER *timer = sorted_container__access(screen_timers_def.screen_timers, (uint32_t)cond);
            if (timer)
            {
                // Show object only before timer kicks off
                if (timer->timer_tiggered == true)
                {
                    return false;
                }
            }
            break;
        }
        case SCREEN_DELAY_PERIODIC:
        {
            DELAY_SCREEN_TIMER *timer = sorted_container__access(screen_timers_def.screen_timers, (uint32_t)cond);
            if (timer)
            {
                // We want to be able to redraw background in case of false
                if (p_redraw_background)
                {
                    // In case of false we force redraw of the background
                    *p_redraw_background = true;
                }

                // Show object when timer flips from on to off
                if (timer->timer_on_time_status == false)
                {
                    return false;
                }
            }
            break;
        }
        default:
            break;
        }
        cond = (cond->next_condition != NULL ? (const SCREEN_CONDITION *)HMI_DATA_ADDRESS(cond->next_condition) : NULL);
    }

    // If all conditions evaluated to true return true.
    return true;
}

/*! \brief Evaluate a linked list of backgrounds starting at background and
 *  return first background which evaluates to true.
 *
 *  Note that finding no valid background in the list is a fatal error.
 *
 *  \param background    Pointer to first background in linked list of backgrounds.
 *
 *  \returns Pointer to first valid background.
 */
const SCREEN_BACKGROUND *select_background(const SCREEN_BACKGROUND *background)
{
    while (!evaluate_conditions((background->first_condition != NULL ? (const SCREEN_CONDITION *)HMI_DATA_ADDRESS(background->first_condition) : NULL), NULL))
    {
        if (background->next_background == NULL)
        {
            assert(0);  // Screen must always have at least one valid background.
        }
        background = (background->next_background != NULL ? (const SCREEN_BACKGROUND *)HMI_DATA_ADDRESS(background->next_background) : NULL);
    }

    return background;
}

/*! \brief Set display brightness level.
 *  Calls installed conversion function to transform to low level brightness values.
 *
 *  \param brightness Displayed brightness level. Levels are product dependent.
 */
void screen_display_brightness_level_set(uint8_t brightness)
{
    LOG(I, "Setting brightness to %d.", brightness);
    screen_brightness = brightness;
    draw_set_brightness(screen_display_brightness_conversion(brightness));
}

/*! \brief Get display brightness.
 *  Returns the brightness level
 *
 *  \return Current brightness level
 */
uint8_t screen_display_brightness_level_get(void)
{
    return screen_brightness;
}

/*! \brief Set display brightness in %.
 *  Calls installed conversion function to transform to low level brightness values.
 *
 *  \param brightness Displayed brightness level. Levels are product dependent.
 */
void screen_display_brightness_set(uint8_t brightness)
{
    draw_set_brightness((int32_t)brightness * (int32_t)CONNECTOR_HMI_BACKLIGHT_RESOLUTION / 100);
}

/*! \brief Get display brightness.
 *  Returns the brightness level in %
 *
 *  \return Current brightness level [%]
 */
uint8_t screen_display_brightness_get(void)
{
    return (screen_display_brightness_conversion(screen_brightness) * 100) / CONNECTOR_HMI_BACKLIGHT_RESOLUTION;
}

/*! \brief Saves HMI settings to NVS
 *
 */
void screen_hmi_settings_save(void)
{
    if (hmi_setting_nvs != 0)
    {
        // Start with restoring internal hmi settings
        for (uint32_t i = 0; i < ELEMENTS(l_hmi_settings); i++)
        {
            uint32_t parameter = 0;
            if (ddm2_parse_parameter_string(&parameter, l_hmi_settings[i].name, strlen(l_hmi_settings[i].name)) != -1)
            {
                uint8_t var_index = varstate_get_var_index_both(parameter);
                if (var_index != VARSTATE_INVALID_INDEX)
                {
                    l_hmi_settings[i].value = varstate_get(var_index);
                    if (nvs_set_u32(hmi_setting_nvs, l_hmi_settings[i].name, l_hmi_settings[i].value) != ESP_OK)
                    {
                        LOG(W, "Could not save (%s:%d)", l_hmi_settings[i].name, l_hmi_settings[i].value);
                    }
                    else
                    {
                        LOG(D, "Saved (%s:%d) from var_index:%d", l_hmi_settings[i].name, l_hmi_settings[i].value, var_index);
                    }
                }
            }
        }
        if (nvs_set_u8(hmi_setting_nvs, "brightness", screen_brightness) != ESP_OK)
        {
            LOG(W, "Could not save (brightness:%d)", screen_brightness);
        }
        else
        {
            LOG(D, "Saved (brightness:%d)", screen_brightness);
        }
        // Call product specific handle if more are to be saved
        if (NULL != screen_hmi_settings_save_cb)
        {
            screen_hmi_settings_variable_t *setting;
            uint32_t num_settings = 0;
            // Get product specific value to save
            screen_hmi_settings_save_cb(&setting, &num_settings);
            for (uint32_t i = 0; i < num_settings; i++, setting++)
            {
                if (nvs_set_u32(hmi_setting_nvs, setting->name, setting->value) != ESP_OK)
                {
                    LOG(W, "Could not save (%s:%d)", setting->name, setting->value);
                }
                else
                {
                    LOG(D, "Saved (%s:%d)", setting->name, setting->value);
                }
            }
        }
    }
}

/*! \brief Set HMI settings save callback
 *
 *  \param cb    Product specific callback that will handle save of hmi settings to flash
 */
void screen_set_hmi_settings_save_cb(screen_hmi_settings_save_cb_t cb)
{
    screen_hmi_settings_save_cb = cb;
}

/*! \brief Set HMI settings save callback
 *
 *  \param cb   Product specific callback that will handle save of hmi settings to flash
 */
void screen_load_hmi_settings(screen_hmi_settings_variable_t *settings, uint32_t num_settings)
{
    bool is_changed = false;
    // Start with restoring internal hmi settings
    for (uint32_t i = 0; i < ELEMENTS(l_hmi_settings); i++)
    {
        if (nvs_get_u32(hmi_setting_nvs, l_hmi_settings[i].name, &l_hmi_settings[i].value) != ESP_OK)
        {
            LOG(W, "Could not load (%s)", l_hmi_settings[i].name);
        }
        else
        {
            uint32_t parameter = 0;
            if (ddm2_parse_parameter_string(&parameter, l_hmi_settings[i].name, strlen(l_hmi_settings[i].name)) != -1)
            {
                uint8_t var_index = varstate_get_var_index_both(parameter);
                if (var_index != VARSTATE_INVALID_INDEX)
                {
                    varstate_set_changed(var_index, true);
                    is_changed = true;
                    varstate_set(var_index, l_hmi_settings[i].value);
                    // Set the ddmp subscribe value into ddmp set and subscribe array
                    p_ddmp_set_sub_array[var_index] = l_hmi_settings[i].value;
                    LOG(D, "Loaded (%s:%d) into var_index:%d", l_hmi_settings[i].name, l_hmi_settings[i].value, var_index);
                }
            }
        }
    }
    for (uint32_t i = 0; i < num_settings; i++, settings++)
    {
        if (nvs_get_u32(hmi_setting_nvs, settings->name, &settings->value) != ESP_OK)
        {
            LOG(W, "Could not load (%s:%d)", settings->name, settings->value);
        }
        else
        {
            LOG(D, "Loaded (%s:%d)", settings->name, settings->value);
        }
    }
    if (is_changed)
    {
        // Trigger change events
        hmi_dispatch_event_to_connector(HMI_TYPE_SET_MODULE_FIELD(HMI_MODULE_TYPE_UI_ENGINE) | HMI_TYPE_SET_GROUP_FIELD(HMI_TYPE_DDMP2), NULL, 0, pdMS_TO_TICKS(200));
    }
}

/*! \brief Fill strbuf with a string generated based on the supplied string
 * 	object.
 *
 *  \param strbuf    Buffer into which string will be written.
 *  \param string    String object definition.
 */
void generate_string(uint8_t *strbuf, const SCREEN_STRING *string)
{
    const VARSTATE_DEF *def;
    uint8_t offset = 0;
    uint8_t len = 0;
    switch (string->type)
    {
    case SCREEN_STRING_TYPE_VALUE_WITH_OFFSET:
        offset = string->value_offset.offset;
        len = string->value_offset.len;
        // fallthrough
    case SCREEN_STRING_TYPE_VALUE:
        def = varstate_def_get(string->value.var_index);
        if (def != NULL)
        {
            int32_t data = 0;
            bool is_struct = false;
            bool is_valid = varstate_get_validated_data(&is_struct, string->value.var_index, &data, offset, len);
            if (is_valid)
            {
                const char *p_format = (const char *)HMI_DATA_ADDRESS(string->value.format_string);
                // Check format string
                if (strchr((char *)p_format, 'f'))
                {
                    double value;
                    value = ((double)data) * (double)def->step;
                    snprintf((char *)strbuf, SCREEN_MAX_STRING_LENGTH, (char *)p_format, (double)value);
                }
                else if (strchr(p_format, 'd'))
                {
                    int32_t value = data;
                    value = (int32_t)((double)value * (double)def->step);
                    snprintf((char *)strbuf, SCREEN_MAX_STRING_LENGTH, p_format, value);
                    LOG(D, "String: %s ( %d %04x )", strbuf, value, value);
                }
                else if (strchr(p_format, 'x') || strchr(p_format, 'X'))
                {
                    int32_t value = data;
                    value = (int32_t)((double)data * (double)def->step);
                    snprintf((char *)strbuf, SCREEN_MAX_STRING_LENGTH, p_format, value);
                    LOG(D, "String: %s ( %d %04x )", strbuf, value, value);
                }
                else
                {
                    // Constant string
                    snprintf((char *)strbuf, SCREEN_MAX_STRING_LENGTH, p_format);
                }
            }
            else
            {
                // Don't render string'
                strbuf[0] = '\0';
            }
        }
        else
        {
            strbuf[0] = '\0';
        }
        break;
    }
}
