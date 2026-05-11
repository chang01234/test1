/*! \file screen.h
 *	\brief Screen module public defines and function headers.
 */
 
#ifndef SCREEN_H_
#define SCREEN_H_

/** Includes ******************************************************************/
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "draw.h"
#include "hmi_data_def.h"

/** Defines *******************************************************************/
#define SCREEN_MAX_OBJECTS          200 //!< Maximum number of sprites and strings allowed per screen.
#define SCREEN_MAX_STRING_LENGTH    100	//!< Maximum length for screen string object.

typedef struct _screen_hmi_settings_variable_t
{
    const char *name;
    uint32_t value;
} screen_hmi_settings_variable_t;

typedef void (*screen_change_cb_t)(const SCREEN_DEF *);
typedef int32_t (*screen_display_brightness_conversion_cb_t)(uint8_t);
typedef void (*screen_hmi_settings_save_cb_t)(screen_hmi_settings_variable_t **settings, uint32_t *num_settings);

/** Function prototypes *******************************************************/
void screen_init(void);
void screen_set_change_cb(screen_change_cb_t cb);
void screen_set_display_brightness_cb(screen_display_brightness_conversion_cb_t cb);
void screen_set_hmi_settings_save_cb(screen_hmi_settings_save_cb_t cb);
void screen_load_hmi_settings(screen_hmi_settings_variable_t *settings, uint32_t num_settings);
void screen_change(const SCREEN_DEF *screen);
void screen_update(uint8_t var_index);
void screen_display_brightness_level_set(uint8_t brightness);
uint8_t screen_display_brightness_level_get(void);
void screen_display_brightness_set(uint8_t brightness);
uint8_t screen_display_brightness_get(void);
void screen_hmi_settings_save(void);
void screen_timer_delay(const void * const data, size_t data_size);

#endif /* SCREEN_H_ */
