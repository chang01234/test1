/*! \file draw.h
 *	\brief Draw module public defines and function headers.
 */
 
#ifndef DRAW_H_
#define DRAW_H_

/** Includes ******************************************************************/
#include <stdint.h>
#include <stdbool.h>
/** Defines *******************************************************************/
#define FONT_NUM_GLYPHS     (256u)      //!< Number of glyphs in a font.
#define HMI_SLEEP           (1u)        //!< HMI display is turned OFF and LCD controller goes to sleep mode
#define HMI_WAKEUP          (0u)        //!< HMI display is ON and LCD controller Wake up

#include "hmi_data_def.h"


/** Variables *****************************************************************/

// Counter variable which increments from the TE display signal at every hsync.
extern volatile uint16_t display_te_counter;

/** Function prototypes *******************************************************/
void draw_init(DRAW_DIRECTION_ENUM draw_direction);
void draw_clear_area(uint8_t x_pos, uint8_t y_pos, uint8_t x_size, uint8_t y_size);
void draw_screen_init(const PALETTE_DEF *palette, const BITMAP_DEF *background, uint8_t fb_x_pos, uint8_t fb_y_pos, uint8_t fb_x_size, uint8_t fb_y_size, bool no_redraw);
void draw_screen(void);
void draw_sprite(const BITMAP_DEF *sprite, uint8_t x_pos, uint8_t y_pos);
void draw_string(const FONT_DEF *font, uint8_t *string, uint8_t x_pos, uint8_t y_pos, uint8_t x_size, uint8_t y_size);
void draw_set_suspend(uint8_t suspend);
void draw_set_brightness(int32_t brightness);

#endif /* DRAW_H_ */
