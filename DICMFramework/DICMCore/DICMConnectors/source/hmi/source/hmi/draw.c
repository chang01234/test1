/*! \file draw.c
 *	\brief Functions for managing internal frame buffer and updating HMI display.
 */

#include "configuration.h"

/** Includes ******************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "esp_heap_caps.h"
#include <string.h>
#include "draw.h"
#include "st7789v.h"
#include "hmi_dispatcher.h"
#if CONFIG_PM_ENABLE
#include "esp_pm.h"
#endif /* CONFIG_PM_ENABLE */

/** Defines *******************************************************************/

#define X_PIXELS            (240u)
#define Y_PIXELS            (240u)
#define BUF_ROWS            (8)
#define BUF_PIXELS          (X_PIXELS * BUF_ROWS)
#define BUF_LINES           (2u)

#ifndef CONNECTOR_HMI_LCD_BCKL_PIN
#define CONNECTOR_HMI_LCD_BCKL_PIN (-1) // Do not control backlight pin. PWM controlled by HAL.
#endif

#ifndef CONNECTOR_HMI_LCD_RST_PIN
#define CONNECTOR_HMI_LCD_RST_PIN (-1) // Do not use TE (sync) pin.
#endif

#ifndef CONNECTOR_HMI_LCD_TE_PIN
#define CONNECTOR_HMI_LCD_TE_PIN (-1) // Do not use TE (sync) pin.
#endif

#ifndef CONNECTOR_HMI_LCD_DMA_CHAN
#define CONNECTOR_HMI_LCD_DMA_CHAN (0) // Do not use DMA.
#endif

#ifndef CONNECTOR_HMI_LCD_BCKL_PWM_CHANNEL
#define CONNECTOR_HMI_LCD_BCKL_PWM_CHANNEL (-1) // Do not use. PWM controlled by HAL.
#endif

#ifndef CONNECTOR_HMI_LCD_BCKL_PWM_TIMER
#define CONNECTOR_HMI_LCD_BCKL_PWM_TIMER (-1) // Do not use. PWM controlled by HAL.
#endif

// Specifies framebuffer size. Should be able to contain all non-background objects on screen.
#define FRAMEBUFFER_SIZE    (X_PIXELS * Y_PIXELS)

typedef struct DRAW_STATE
{
    const BITMAP_DEF *background;
    uint16_t palette_data[256];
    uint16_t dma_buffer[BUF_LINES][BUF_PIXELS];
    uint8_t *send_dma_buffer;
    uint8_t *fb_pixel_data;
    uint8_t fb_x_start;
    uint8_t fb_y_start;
    uint8_t fb_x_end;
    uint8_t fb_y_end;
    bool init_called;
    DRAW_DIRECTION_ENUM draw_direction;
} DRAW_STATE;

typedef struct BITMAP_UNPACK_STATE
{
    const uint16_t *data_pointer;
    uint16_t data_count;
    uint8_t pixel_cache;
} BITMAP_UNPACK_STATE;

/** Variables *****************************************************************/
static EXT_RAM_ATTR DRAW_STATE ds;
volatile uint16_t display_te_counter;
static uint8_t display_suspended = 0;
static int32_t display_brightness = 0;
#if CONFIG_PM_ENABLE
static esp_pm_lock_handle_t hmi_pm_lock = (esp_pm_lock_handle_t)0;
#endif /* CONFIG_PM_ENABLE */

/** Private prototypes ********************************************************/
static void draw_refresh_framebuffer(uint8_t x_pos, uint8_t y_pos, uint8_t x_size, uint8_t y_size);
static void bitmap_unpack_init(const BITMAP_DEF *bitmap, BITMAP_UNPACK_STATE *state);
static inline uint8_t bitmap_unpack_pixel(BITMAP_UNPACK_STATE *state) __attribute__((always_inline));
static inline void bitmap_unpack_skip(BITMAP_UNPACK_STATE *state, uint16_t pixels) __attribute__((always_inline));


/*! \brief Init screen drawing functions.
 *
 *  Initializes screen refresh timer and global state.
 *
 *  \param draw_direction Specifies screen rotation.
 */
void draw_init(DRAW_DIRECTION_ENUM draw_direction)
{
	static const st7789v_init_data_t init_data = {
        .spi_host = CONNECTOR_HMI_LCD_SPI_HOST,
        .dma_chan = CONNECTOR_HMI_LCD_DMA_CHAN,
        .mosi_pin = CONNECTOR_HMI_LCD_MOSI_PIN,
        .sclk_pin = CONNECTOR_HMI_LCD_SCLK_PIN,
        .cs_pin = CONNECTOR_HMI_LCD_CS_PIN,
        .dc_pin = CONNECTOR_HMI_LCD_DC_PIN,
        .rst_pin = CONNECTOR_HMI_LCD_RST_PIN,
        .te_pin = CONNECTOR_HMI_LCD_TE_PIN,
        .bckl_pin = CONNECTOR_HMI_LCD_BCKL_PIN,
        .bckl_pwm_channel = CONNECTOR_HMI_LCD_BCKL_PWM_CHANNEL,
        .bckl_pwm_timer = CONNECTOR_HMI_LCD_BCKL_PWM_TIMER,
        .res_x = X_PIXELS,
        .res_y = Y_PIXELS,
        .transfer_size = BUF_PIXELS * sizeof(uint16_t),
	};

#if CONFIG_PM_ENABLE
    int retval;
    /* Create Lock to disable PM when screen is ON */
    retval = esp_pm_lock_create(ESP_PM_APB_FREQ_MAX, 0, "hmi", &hmi_pm_lock);
    TRUE_CHECK_RETURN(retval == ESP_OK);
#endif /* CONFIG_PM_ENABLE */

    // Initialize internal state.
    memset(&ds, 0, sizeof(DRAW_STATE));
    ds.draw_direction = draw_direction;
    TRUE_CHECK_RETURN(ds.send_dma_buffer = heap_caps_malloc(BUF_PIXELS * 2, MALLOC_CAP_DMA));	// Max we send per DMA request
    TRUE_CHECK_RETURN(ds.fb_pixel_data = heap_caps_malloc(FRAMEBUFFER_SIZE, MALLOC_CAP_SPIRAM));
    display_suspended = 1;
    display_brightness = 0;

    tft_bckl_set_duty(0);
    // Initialize display
    st7789v_initialize(&init_data);	// Initialize the display controller
    draw_set_suspend(HMI_SLEEP);
    draw_set_brightness(255);
}

/*! \brief Initializes screen data and framebuffer.
 *
 * 	Configures the screen background and anitializes the framebuffer. Sprites
 * 	are drawn only to the framebuffer until \p draw_screen is called. Use
 * 	\p draw_screen to start a slow refresh of the entire screen.
 *
 * 	\param palette		Pointer to screen palette definition.
 * 	\param background	Pointer to screen background bitmap definition.
 * 	\param fb_x_pos		Frame buffer top left corner X coordinate.
 * 	\param fb_y_pos		Frame buffer top left corner Y coordinate.
 * 	\param fb_x_size	Frame buffer size in X direction.
 * 	\param fb_y_size	Frame buffer size in Y direction.
 * 	\param no_redraw	Reconfigure framebuffer only, do not wait for \p draw_screen.
 *
 * 	\see draw_screen
 * 	\see draw_sprite
 */
void draw_screen_init(const PALETTE_DEF *palette, const BITMAP_DEF *background, uint8_t fb_x_pos, uint8_t fb_y_pos, uint8_t fb_x_size, uint8_t fb_y_size, bool no_redraw)
{
    // Store screen palette and background for later.

    assert(palette != NULL);
    assert(background != NULL);

    ds.background = background;

    // Fetch palette data into RAM
    memcpy(ds.palette_data, (const uint16_t*)HMI_DATA_ADDRESS(palette->color_data), 512);

    // Set framebuffer size.

    uint16_t fb_pixels = fb_x_size * fb_y_size;
    assert(fb_pixels <= FRAMEBUFFER_SIZE);

    ds.fb_x_start = fb_x_pos;
    ds.fb_y_start = fb_y_pos;
    ds.fb_x_end = fb_x_pos + fb_x_size - 1;
    ds.fb_y_end = fb_y_pos + fb_y_size - 1;

    // Fill framebuffer with background pixel data.

    BITMAP_UNPACK_STATE bus;
    bitmap_unpack_init(ds.background, &bus);

    uint8_t *fb_data = ds.fb_pixel_data;
    for (uint_fast8_t y = 0; y < Y_PIXELS; y++)
    {
        for (uint_fast8_t x = 0; x < X_PIXELS; x++)
        {
            uint8_t pixel = bitmap_unpack_pixel(&bus);

            if ((y >= ds.fb_y_start) && (y <= ds.fb_y_end) && (x >= ds.fb_x_start) && (x <= ds.fb_x_end))
            {
                *fb_data++ = pixel;
            }
        }
    }

    // Framebuffer preparation complete.

    if (no_redraw)
    {
        ds.init_called = false;
    }
    else
    {
        ds.init_called = true;
    }
}

/*! \brief Complete a full screen redraw and update display.
 *
 * 	Redraws the entire screen. Calls to \p draw_sprite will update smaller
 * 	areas of the screen directly after this call.
 *
 * 	\see draw_screen_init
 * 	\see draw_sprite
 */
void draw_screen(void)
{
    uint8_t bf;
    BITMAP_UNPACK_STATE bus;
    uint8_t *fb_data;
    uint8_t pixel;
    uint8_t y;

    // Set draw area.
    st7789v_open_window(0, 0, X_PIXELS, Y_PIXELS);

    // Prepare buffers for decompression.
    bf = 0;
    fb_data = ds.fb_pixel_data;
    bitmap_unpack_init(ds.background, &bus);
    y = 0;

    // Fill first buffer.
    for (uint8_t r = 0; r < BUF_ROWS; r++)
    {
        for (uint8_t x = 0; x < X_PIXELS; x++)
        {
            pixel = bitmap_unpack_pixel(&bus);

            if ((y >= ds.fb_y_start) && (y <= ds.fb_y_end) && (x >= ds.fb_x_start) && (x <= ds.fb_x_end))
            {
                pixel = *fb_data++;
            }

            ds.dma_buffer[bf][r * X_PIXELS + x] = ds.palette_data[pixel];
        }
        y++;
    }

    // Wait for start of frame.
    st7789v_wait_vsync();
    //while(display_te_counter != 0)
    //	;

    // Set slow refresh
    st7789v_set_slow_sync();

    // Start first DMA transfer.
    memcpy(ds.send_dma_buffer, (uint8_t*)&(ds.dma_buffer[bf]), BUF_PIXELS * 2);

    st7789v_write_data(ds.send_dma_buffer, BUF_PIXELS * 2);

    // Transmit remaining pixel data.
    while (y < Y_PIXELS)
    {
    	// Swap buffers.
    	bf = !bf;

        for (uint8_t r = 0; r < BUF_ROWS; r++)
        {
            for (uint8_t x = 0; x < X_PIXELS; x++)
            {
                pixel = bitmap_unpack_pixel(&bus);

                if ((y >= ds.fb_y_start) && (y <= ds.fb_y_end) && (x >= ds.fb_x_start) && (x <= ds.fb_x_end))
                {
                    pixel = *fb_data++;
                }

                ds.dma_buffer[bf][r * X_PIXELS + x] = ds.palette_data[pixel];
            }
            y++;
        }

        // Wait for previous DMA to finish.
        st7789v_write_data_wait();

        // Wait for screen refresh to reach this line.
        //while(display_te_counter < (y - BUF_ROWS))
        //	;
        memcpy(ds.send_dma_buffer, (uint8_t*)&(ds.dma_buffer[bf]), BUF_PIXELS * 2);

        // Start DMA transfer.
        st7789v_write_data(ds.send_dma_buffer, BUF_PIXELS * 2);
    }

    // Wait until last transfer to display is finished before sending new commands.
    st7789v_write_data_wait();

    // Set fast refresh.
    st7789v_set_fast_sync();

    // Full screen refresh complete.
    ds.init_called = false;
}

/*! \brief Draw a sprite.
 *
 * 	Draws a sprite on the framebuffer. Also refreshes the affected area on the
 * 	display if \p draw_screen has been called previously.
 *
 * 	\param sprite   Pointer to sprite bitmap definition.
 * 	\param x_pos    Sprite top left corner X coordinate.
 * 	\param y_pos    Sprite top left corner Y coordinate.
 *
 * 	\see draw_screen_init
 * 	\see draw_screen
 */
void draw_sprite(const BITMAP_DEF *sprite, uint8_t x_pos, uint8_t y_pos)
{
    BITMAP_UNPACK_STATE bus;
    uint8_t *fb_data;
    uint8_t pixel;

    assert(ds.background != NULL);
    assert(sprite != NULL);

    // Translate coordinates from screen to framebuffer.
    x_pos -= ds.fb_x_start;
    y_pos -= ds.fb_y_start;

    // Do not draw sprites that are outside the framebuffer.
    assert(((x_pos + sprite->x_size) <= (ds.fb_x_end - ds.fb_x_start + 1)) && ((y_pos + sprite->y_size) <= (ds.fb_y_end - ds.fb_y_start + 1)));

    bitmap_unpack_init(sprite, &bus);

    for (uint8_t y = y_pos; y < (y_pos + sprite->y_size); y++)
    {
        // Find start of current row on framebuffer.
        fb_data = &ds.fb_pixel_data[(y * (ds.fb_x_end - ds.fb_x_start + 1)) + x_pos];

        for (uint8_t x = x_pos; x < (x_pos + sprite->x_size); x++)
        {
            pixel = bitmap_unpack_pixel(&bus);

            // Only draw non-zero pixels.
            if (pixel != 0)
            {
                *fb_data = pixel;
            }

            fb_data++;
        }
    }

    // Perform a fast update of the affected screen area if draw_screen_init has not been called.
    if (!ds.init_called)
    {
        draw_refresh_framebuffer(x_pos, y_pos, sprite->x_size, sprite->y_size);
    }
}

/*! \brief Draw contents of a string using provided font.
 *
 * 	Draws a string using provided font by drawing a set of sprites. As with
 * 	\p draw_sprite, if \p draw_screen is called, this function updates the
 * 	display directly.
 *
 * 	\param font     Pointer to font definition.
 * 	\param string   Pointer to string.
 * 	\param x_pos    Sprite top left corner X coordinate.
 * 	\param y_pos    Sprite top left corner Y coordinate.
 *
 * 	\see draw_sprite
 */
void draw_string(const FONT_DEF *font, uint8_t *string, uint8_t x_pos, uint8_t y_pos, uint8_t x_size, uint8_t y_size)
{
    assert(font != NULL);
    assert(string != NULL);

    // Calculate text area in pixels.
    uint16_t text_x_size = 0;
    uint16_t text_y_size = 0;

    for (uint16_t i = 0; string[i] != 0; i++)
    {
#if FONT_NUM_GLYPHS < UINT8_MAX
        if (string[i] < FONT_NUM_GLYPHS)
#endif
        {
            const BITMAP_DEF *glyph = ((const BITMAP_DEF **)HMI_DATA_ADDRESS(font->glyph))[string[i]];
            if (glyph != NULL)
            {
                glyph = (const BITMAP_DEF *)HMI_DATA_ADDRESS(glyph);
                if ((ds.draw_direction == DRAW_DIRECTION_UP) || (ds.draw_direction == DRAW_DIRECTION_DOWN))
                {
                    text_x_size += glyph->x_size;
                    text_y_size = glyph->y_size;
                }
                else if ((ds.draw_direction == DRAW_DIRECTION_RIGHT) || (ds.draw_direction == DRAW_DIRECTION_LEFT))
                {
                    text_x_size = glyph->x_size;
                    text_y_size += glyph->y_size;
                }
            }
        }
    }

    // Align text within the text area.
    uint16_t text_x_pos = x_pos;
    uint16_t text_y_pos = y_pos;

    if ((text_x_size <= x_size) && (text_y_size <= y_size))
    {
        if (ds.draw_direction == DRAW_DIRECTION_UP)
        {
            text_x_pos = x_pos + (x_size / 2) - (text_x_size / 2); // Drawing glyphs in positive X direction.
            text_y_pos = y_pos + (y_size / 2) - (text_y_size / 2);
        }
        else if (ds.draw_direction == DRAW_DIRECTION_RIGHT)
        {
            text_x_pos = x_pos + (x_size / 2) - (text_x_size / 2);
            text_y_pos = y_pos + (y_size / 2) - (text_y_size / 2); // Drawing glyphs in positive Y direction.
        }
        else if (ds.draw_direction == DRAW_DIRECTION_DOWN)
        {
            text_x_pos = x_pos + (x_size / 2) + (text_x_size / 2); // Drawing glyphs in negative X direction.
            text_y_pos = y_pos + (y_size / 2) - (text_y_size / 2);
        }
        else if (ds.draw_direction == DRAW_DIRECTION_LEFT)
        {
            text_x_pos = x_pos + (x_size / 2) - (text_x_size / 2);
            text_y_pos = y_pos + (y_size / 2) + (text_y_size / 2); // Drawing glyphs in negative Y direction.
        }
    }

    // Save init flag and set it to avoid multiple LCD updates.
    bool old_init_called = ds.init_called;
    ds.init_called = true;

    // Clear text area to background image.
    draw_clear_area(x_pos, y_pos, x_size, y_size);

    // Draw glyphs.
    for (uint16_t i = 0; string[i] != 0; i++)
    {
#if FONT_NUM_GLYPHS < UINT8_MAX
        if (string[i] < FONT_NUM_GLYPHS)
#endif
        {
            const BITMAP_DEF *glyph = ((const BITMAP_DEF **)HMI_DATA_ADDRESS(font->glyph))[string[i]];
            if (glyph != NULL)
            {
                glyph = (const BITMAP_DEF *)HMI_DATA_ADDRESS(glyph);
                if (ds.draw_direction == DRAW_DIRECTION_UP)
                {
                    if ((text_x_pos >= x_pos) && ((text_x_pos + glyph->x_size) <= (x_pos + x_size)))
                    {
                        draw_sprite(glyph, text_x_pos, text_y_pos);
                    }
                    text_x_pos += glyph->x_size;
                }
                else if (ds.draw_direction == DRAW_DIRECTION_RIGHT)
                {
                    if ((text_y_pos >= y_pos) && ((text_y_pos + glyph->y_size) <= (y_pos + y_size)))
                    {
                        draw_sprite(glyph, text_x_pos, text_y_pos);
                    }
                    text_y_pos += glyph->y_size;
                }
                else if (ds.draw_direction == DRAW_DIRECTION_DOWN)
                {
                    text_x_pos -= glyph->x_size;

                    if ((text_x_pos >= x_pos) && ((text_x_pos + glyph->x_size) <= (x_pos + x_size)))
                    {
                        draw_sprite(glyph, text_x_pos, text_y_pos);
                    }
                }
                else if (ds.draw_direction == DRAW_DIRECTION_LEFT)
                {
                    text_y_pos -= glyph->y_size;

                    if ((text_y_pos >= y_pos) && ((text_y_pos + glyph->y_size) <= (y_pos + y_size)))
                    {
                        draw_sprite(glyph, text_x_pos, text_y_pos);
                    }
                }
            }
        }
    }

    // Restore saved init flag.
    ds.init_called = old_init_called;

    // Update entire text area of LCD if requested.
    if (!ds.init_called)
    {
        draw_refresh_framebuffer(x_pos - ds.fb_x_start, y_pos - ds.fb_y_start, x_size, y_size);
    }
}

/*! \brief Controls display power save mode.
 *
 * 	Controls the power save mode of the display controller. Also keeps the
 *  backlight turned off while in power save mode.
 *
 * 	\param suspend  1 to enter power save mode or 0 to re-enable the display.
 *
 * 	\see draw_set_suspend
 */
void draw_set_suspend(uint8_t suspend)
{
    if (suspend != display_suspended)
    {
        display_suspended = suspend;
 
        if (suspend)
        {
            LOG(I, "LCD suspended.");
            tft_bckl_set_duty(0);
            st7789v_suspend();
#if CONFIG_PM_ENABLE
            esp_pm_lock_release(hmi_pm_lock);
#endif /* CONFIG_PM_ENABLE */
        }
        else
        {
#if CONFIG_PM_ENABLE
            esp_pm_lock_acquire(hmi_pm_lock);
#endif /* CONFIG_PM_ENABLE */
            st7789v_resume();
            tft_bckl_set_duty(display_brightness);
            LOG(I, "LCD active.");
        }
 
        // Inform input manger's ULP component that LCD state has changed
        hmi_dispatch_event_to_module(HMI_SET_MODULE_INTERNAL_EVENT(HMI_MODULE_TYPE_INPUT_MANAGER, HMI_TYPE_INTERNAL_EVENTS_SCREEN_STATE_UPDATED), &suspend, sizeof(suspend));
    }
}

/*! \brief Controls display brightness.
 *
 * 	Updates display brightness if it is enabled. Otherwise the value is saved
 *  until the display is enabled.
 *
 * 	\param brightness   Display brightness.
 *
 * 	\see draw_set_suspend
 */
void draw_set_brightness(int32_t brightness)
{
    display_brightness = brightness;

    if (!display_suspended)
    {
        tft_bckl_set_duty(display_brightness);
        LOG(I, "LCD brightness set to %d.", display_brightness);
    }
}

/*! \brief Clears area to background image.
 *
 * 	Repaints area occupied by the specified sprite with the background image.
 * 	Also refreshes the affected area on the display if \p draw_screen has been
 * 	called previously.
 *
 * 	\param x_pos    Area top left corner X coordinate.
 * 	\param y_pos    Area top left corner Y coordinate.
 * 	\param x_size   Area X size.
 * 	\param y_size   Area Y size.
 *
 * 	\see draw_screen_init
 * 	\see draw_screen
 * 	\see draw_sprite
 */
void draw_clear_area(uint8_t x_pos, uint8_t y_pos, uint8_t x_size, uint8_t y_size)
{
    BITMAP_UNPACK_STATE bus;
    uint8_t *fb_data;
    uint8_t pixel;
    uint16_t bg_data_cur;
    uint16_t bg_data_next;

    assert(ds.background != NULL);

    // Translate coordinates from screen to framebuffer.
    x_pos -= ds.fb_x_start;
    y_pos -= ds.fb_y_start;

    // The area must be inside the framebuffer.
    assert(((x_pos + x_size) <= (ds.fb_x_end - ds.fb_x_start + 1)) && ((y_pos + y_size) <= (ds.fb_y_end - ds.fb_y_start + 1)));

    bitmap_unpack_init(ds.background, &bus);
    bg_data_cur = 0;
    bg_data_next = 0;

    for (uint8_t y = y_pos; y < (y_pos + y_size); y++)
    {
        // Calculate background data location and fast forward if needed.
        bg_data_next = (y + ds.fb_y_start) * X_PIXELS + (x_pos + ds.fb_x_start);
        bitmap_unpack_skip(&bus, bg_data_next - bg_data_cur);
        bg_data_cur = bg_data_next;

        // Find start of current row on framebuffer.
        fb_data = &ds.fb_pixel_data[(y * (ds.fb_x_end - ds.fb_x_start + 1)) + x_pos];

        // Repaint row.
        for (uint8_t x = x_pos; x < (x_pos + x_size); x++)
        {
            pixel = bitmap_unpack_pixel(&bus);

            *fb_data = pixel;

            fb_data++;
            bg_data_cur++;
        }
    }

    // Perform a fast update of the affected screen area if draw_screen_init has not been called.
    if (!ds.init_called)
    {
        draw_refresh_framebuffer(x_pos, y_pos, x_size, y_size);
    }
}

/*! \brief Refreshes an area of the LCD.
 *
 * 	Refreshes the selected area on the display. Coordinates are specified
 * 	relative to the framebuffer.
 *
 * 	\param sprite   Pointer to sprite bitmap definition.
 * 	\param x_pos    X coordinate of area to update.
 * 	\param y_pos    Y coordinate of area to update.
 * 	\param x_size   X size of area to update.
 * 	\param y_size   Y size of area to update.
 *
 * 	\see draw_screen_init
 * 	\see draw_screen
 * 	\see draw_sprite
 */
static void draw_refresh_framebuffer(uint8_t x_pos, uint8_t y_pos, uint8_t x_size, uint8_t y_size)
{
    uint8_t bf;
    uint8_t *fb_data;

    bf = 0;

    // Set draw area.
    st7789v_open_window(ds.fb_x_start + x_pos, ds.fb_y_start + y_pos, x_size, y_size);

    // Fill first buffer.
    fb_data = &ds.fb_pixel_data[(y_pos * (ds.fb_x_end - ds.fb_x_start + 1)) + x_pos];
    for (uint8_t x = 0; x < x_size; x++)
    {
        ds.dma_buffer[bf][x] = ds.palette_data[*fb_data++];
    }

    // Start first DMA transfer.
    memcpy(ds.send_dma_buffer, (uint8_t*)&(ds.dma_buffer[bf]), x_size * 2);

    st7789v_write_data(ds.send_dma_buffer, x_size * 2);

	// Transmit remaining pixel data.
    for (uint16_t y = 1; y < y_size; y++)
    {
        // Swap buffers.
        bf = !bf;

        fb_data = &ds.fb_pixel_data[((y_pos + y) * (ds.fb_x_end - ds.fb_x_start + 1)) + x_pos];
        for (uint8_t x = 0; x < x_size; x++)
        {
            ds.dma_buffer[bf][x] = ds.palette_data[*fb_data++];
        }

        // Wait for previous DMA to finish.
        st7789v_write_data_wait();

		memcpy(ds.send_dma_buffer, (uint8_t*)&(ds.dma_buffer[bf]), x_size * 2);

        // Start DMA transfer.
        st7789v_write_data(ds.send_dma_buffer, x_size * 2);
    }

    // Wait for previous DMA to finish.
    st7789v_write_data_wait();
}

/*! \brief Initializes a bitmap unpacker.
 *
 * 	Initializes an object that is used to decompress RLE-encoded pixel data.
 *
 * 	\param bitmap   Pointer to sprite bitmap definition.
 * 	\param state    Pointer to object state data.
 */
static void bitmap_unpack_init(const BITMAP_DEF *bitmap, BITMAP_UNPACK_STATE *state)
{
    assert(bitmap != NULL);
    assert(state != NULL);

    state->data_pointer = (const uint16_t*)HMI_DATA_ADDRESS(bitmap->pixel_data);
    state->data_count = (*state->data_pointer >> 8) + 1;
    state->pixel_cache = *state->data_pointer & 0x00ff;
}

/*! \brief Unpacks a pixel.
 *
 * 	Performs decompression of pixel data. Returns a single pixel value.
 *
 * 	\param state    Pointer to object state data.
 * 	\return         Decompressed pixel value.
 */
static inline uint8_t bitmap_unpack_pixel(BITMAP_UNPACK_STATE *state)
{
    assert(state != NULL);

    if (state->data_count == 0)
    {
        state->data_pointer++;
        state->data_count = *state->data_pointer >> 8;
        state->pixel_cache = *state->data_pointer & 0x00ff;
    }
    else
    {
        state->data_count--;
    }

    return state->pixel_cache;
}

/*! \brief Skips pixels in the unpacking process.
 *
 * 	Fast forwards through pixels in compressed data.
 *
 * 	\param state    Pointer to object state data.
 * 	\param pixels   Number of pixels to skip.
 */
static inline void bitmap_unpack_skip(BITMAP_UNPACK_STATE *state, uint16_t pixels)
{
    // Fast forward through symbols.
    while (pixels > state->data_count)
    {
        pixels -= state->data_count + 1;

        state->data_pointer++;
        state->data_count = *state->data_pointer >> 8;
        state->pixel_cache = *state->data_pointer & 0x00ff;
    }

    // Remove remaining pixels from current symbol.
    state->data_count -= pixels;
}
