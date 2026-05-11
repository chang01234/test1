/*! \file st7789v.c
 *	\brief Display driver communication functions.
 *
 *	Provides functions for working with the external st7789v display driver.
 */

/** Includes ******************************************************************/
#include "configuration.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "st7789v.h"

//#define CFX3_DISPLAY_JT00704_02

/** Variables *****************************************************************/
static const uint8_t st7789v_init_data[] =	//command, number of arguments, argument list
{
		ST7789V_NOP,		0x00,									// 00
		ST7789V_SWRESET, 	0x00,
		ST7789V_DISPOFF,	0x00,								// 28
		ST7789V_SLPIN,		0x00,									// 10
//		ST7789V_SLPOUT,		0x00,									// 11
		ST7789V_MADCTL,		0x01, 	0x00, 							// 36 00
		ST7789V_COLMOD,		0x01, 	0x55, 							// 3a 55
		ST7789V_INVON,		0x00, 									// 21
		ST7789V_CASET,		0x04, 	0x00, 0x00, 0x00, 0xef, 		// 2a 00 00 00 ef
		ST7789V_RASET,		0x04, 	0x00, 0x00, 0x00, 0xef,			// 2b 00 00 00 ef
		ST7789V_PORCTRL,	0x05, 	0x0c, 0x0c, 0x00, 0x33, 0x33,	// b2 0c 0c 00 33 33
#ifdef CFX3_DISPLAY_JT00704_02
		ST7789V_GCTRL,		0x01, 	0x35,							// b7 35
		ST7789V_VCOMS,		0x01, 	0x1f,							// bb 1f
#else // JT60233_01_
		ST7789V_GCTRL,		0x01, 	0x00,							// b7 00 (VGH=12.2V,VGL=-7.16V)
		ST7789V_VCOMS,		0x01, 	0x36,							// bb 36 (VCOM=1.45V)
#endif
		ST7789V_LCMCTRL,	0x01, 	0x2c,							// c0 2c
		ST7789V_VDVVRHEN,	0x01, 	0x01,							// c2 01
#ifdef CFX3_DISPLAY_JT00704_02
		ST7789V_VRHS,		0x01, 	0x12,							// c3 12
#else // JT60233_01_
		ST7789V_VRHS,		0x01, 	0x13,							// c3 13 (GVDD=4.5+(VCOM+VCOM OFFSET+VDV)=4.8V)
		0xd6,				0x01,	0xa1,							// d6 a1 (when sleep in��set VGH=GND for power saving)
#endif
		ST7789V_VDVS,		0x01, 	0x20,							// c3 20
		ST7789V_FRCTRL2,	0x01, 	0x0f,							// c6 0f
		ST7789V_PWCTRL1,	0x02, 	0xa4, 0xa1,						// d0 a4 a1
#ifdef CFX3_DISPLAY_JT00704_02
		ST7789V_PVGAMCTRL,	0x0e, 	0xd0, 0x08, 0x11, 0x08, 0x0c, 0x15, 0x39, 0x33, 0x50, 0x36, 0x13, 0x14, 0x29, 0x2d, // e0 d0 08 11 08 0c 15 39 33 50 36 13 14 29 2d
		ST7789V_NVGAMCTRL,	0x0e, 	0xd0, 0x08, 0x10, 0x08, 0x06, 0x06, 0x39, 0x44, 0x51, 0x0b, 0x16, 0x14, 0x2f, 0x31, // e1 d0 08 10 08 06 06 39 44 51 0b 16 14 2f 31
#else // JT60233_01_
		ST7789V_PVGAMCTRL,	0x0e, 	0xf0, 0x08, 0x0e, 0x09, 0x08, 0x04, 0x2f, 0x33, 0x45, 0x36, 0x13, 0x12, 0x2a, 0x2d, 
		ST7789V_NVGAMCTRL,	0x0e, 	0xf0, 0x0e, 0x12, 0x0c, 0x0a, 0x15, 0x2e, 0x32, 0x44, 0x39, 0x17, 0x18, 0x2b, 0x2f,
#endif
//		ST7789V_DISPON,		0x00,									// 29
		ST7789V_DISPOFF,	0x00,								// 28
		ST7789V_CASET,		0x04, 	0x00, 0x00, 0x00, 0xef,			// 2a 00 00 00 ef
		ST7789V_RASET,		0x04, 	0x00, 0x00, 0x00, 0xef,			// 2b 00 00 00 ef
		ST7789V_TEON,		0x01,	TE_MODE_1,						// 35 01
		ST7789V_FRCTRL1,	0x01,	DIV_FAST,						// b3 01
		ST7789V_FRCTRL2,	0x01,	FR,								// c6 0f
		ST7789V_RAMWR,		0x00,									// 2c
};

/** Defines *******************************************************************/
#define LCD_SPI_FREQ (40000000ULL)
#define BCKL_PWM_FREQ (1000)

static const st7789v_init_data_t *lcdcfg = NULL;
static spi_device_handle_t spi;
static bool controller_asleep = false;
static volatile TaskHandle_t vsync_notify_task = NULL;

static void spi_pre_transfer_callback(spi_transaction_t *t)
{
	int dc = (int)t->user;
	gpio_set_level(lcdcfg->dc_pin, dc);
}

static void lcd_cmd(spi_device_handle_t spi, const uint8_t cmd)
{
	esp_err_t ret;
	spi_transaction_t t;

	memset(&t, 0, sizeof(t));
	t.length = 8;
	t.tx_buffer = &cmd;
	t.user = (void *)0;

	ret = spi_device_polling_transmit(spi, &t);
	assert(ret == ESP_OK);

	t.user = (void *)1;
}

static void lcd_data(spi_device_handle_t spi, const uint8_t* data, size_t len)
{
	esp_err_t ret;
	spi_transaction_t t;

	if (len == 0)
		return;

	memset(&t, 0, sizeof(t));
	t.length = len * 8;
	t.tx_buffer = data;
	t.user = (void *)1;

	ret = spi_device_polling_transmit(spi, &t);
	assert(ret == ESP_OK);
}

static void IRAM_ATTR te_isr_handler(void *args)
{
	(void)args;

	BaseType_t higher_priority_task_woken = pdFALSE;

	if (vsync_notify_task != NULL)
	{
		vTaskNotifyGiveFromISR(vsync_notify_task, &higher_priority_task_woken);
		vsync_notify_task = NULL;
	}

	if (higher_priority_task_woken == pdTRUE)
	{
		portYIELD_FROM_ISR();
	}
}

void st7789v_initialize(const st7789v_init_data_t *initdata)
{
	esp_err_t ret;

	lcdcfg = initdata;

	spi_bus_config_t buscfg = {
		.miso_io_num = -1,
		.mosi_io_num = lcdcfg->mosi_pin,
		.sclk_io_num = lcdcfg->sclk_pin,
		.quadwp_io_num = -1,
		.quadhd_io_num = -1,
		.max_transfer_sz = lcdcfg->transfer_size,
		//.flags = SPICOMMON_BUSFLAG_IOMUX_PINS, // Required for sclk speeds above 40MHz.
	};

	spi_device_interface_config_t devcfg = {
		.clock_speed_hz = LCD_SPI_FREQ,
		.mode = 0,
		.spics_io_num = lcdcfg->cs_pin,
		.queue_size = 1,
		.pre_cb = spi_pre_transfer_callback,
		.flags = SPI_DEVICE_NO_DUMMY,
	};

	// Initialize the SPI bus.
	ret = spi_bus_initialize(lcdcfg->spi_host, &buscfg, lcdcfg->dma_chan);
	ESP_ERROR_CHECK(ret);
	ret = spi_bus_add_device(lcdcfg->spi_host, &devcfg, &spi);
	ESP_ERROR_CHECK(ret);

	// Initialize non-SPI GPIOs.
	gpio_set_direction(lcdcfg->dc_pin, GPIO_MODE_OUTPUT);
	if (lcdcfg->rst_pin != -1)
	{
		gpio_set_direction(lcdcfg->rst_pin, GPIO_MODE_OUTPUT);
	}

	if (lcdcfg->te_pin != -1)
	{
		gpio_set_direction(lcdcfg->te_pin, GPIO_MODE_INPUT);
#if 0
		ret = gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
		ESP_ERROR_CHECK(ret);
#endif
		ret = gpio_isr_handler_add(lcdcfg->te_pin, te_isr_handler, NULL);
		ESP_ERROR_CHECK(ret);
		ret = gpio_set_intr_type(lcdcfg->te_pin, GPIO_INTR_POSEDGE);
		ESP_ERROR_CHECK(ret);
		ret = gpio_intr_enable(lcdcfg->te_pin);
		ESP_ERROR_CHECK(ret);
	}

	if(lcdcfg->bckl_pin != -1)
	{
		ledc_timer_config_t ledc_timer = {
			.duty_resolution = LEDC_TIMER_8_BIT,
			.freq_hz = BCKL_PWM_FREQ,
			.speed_mode = LEDC_LOW_SPEED_MODE,
			.timer_num = lcdcfg->bckl_pwm_timer,
			.clk_cfg = LEDC_AUTO_CLK,
		};

		ledc_channel_config_t ledc_channel = {
			.channel = lcdcfg->bckl_pwm_channel,
			.duty = 0,
			.gpio_num = lcdcfg->bckl_pin,
			.speed_mode = LEDC_LOW_SPEED_MODE,
			.hpoint = 0,
			.timer_sel = lcdcfg->bckl_pwm_timer,
		};

		ledc_timer_config(&ledc_timer);
		ledc_channel_config(&ledc_channel);
		ledc_fade_func_install(0);
	}

	// Disable backlight.
	st7789v_set_brightness(0);

	// Reset the display.
	if (lcdcfg->rst_pin != -1)
	{
		gpio_set_level(lcdcfg->rst_pin, 0);
		vTaskDelay(pdMS_TO_TICKS(100));
		gpio_set_level(lcdcfg->rst_pin, 1);
		vTaskDelay(pdMS_TO_TICKS(100));
	}

	// Send init commands.
	for (size_t i = 0; i < sizeof(st7789v_init_data);)
	{
		lcd_cmd(spi, st7789v_init_data[i]);

		if(st7789v_init_data[i] == ST7789V_SLPOUT)
			vTaskDelay(pdMS_TO_TICKS(30));
		if(st7789v_init_data[i] == ST7789V_SLPIN)
			vTaskDelay(pdMS_TO_TICKS(30));
		if(st7789v_init_data[i] == ST7789V_SWRESET)
			vTaskDelay(pdMS_TO_TICKS(10));
		i++;

		if(st7789v_init_data[i] > 0)
		{
			lcd_data(spi, &st7789v_init_data[i + 1], st7789v_init_data[i]);
		}

		i += st7789v_init_data[i] + 1;
	}

	// The display will display garbage data on startup. Either clear it or
	// wait for it to be initialized before turning on the backlight.
#if LCD_CLEAR_ON_INIT
	st7789v_open_window(0, 0, lcdcfg->res_x, lcdcfg->res_y);
	for (size_t i = 0; i < lcdcfg->res_x * lcdcfg->res_y; i++)
	{
		uint16_t p = RGB555(0, 0, 0);
		lcd_data(spi, (uint8_t *)&p, 2);
	}
#endif
}

void st7789v_open_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
	uint16_t x2 = x + w - 1;
	uint16_t y2 = y + h - 1;

	uint8_t cols[4] = { x >> 8, x, x2 >> 8, x2 };
	uint8_t rows[4] = { y >> 8, y, y2 >> 8, y2 };

	if ((x > lcdcfg->res_x) || (w > lcdcfg->res_x) || ((x + w) > lcdcfg->res_x))
		return;
	if ((y > lcdcfg->res_y) || (h > lcdcfg->res_y) || ((y + h) > lcdcfg->res_y))
		return;

	lcd_cmd(spi, ST7789V_NOP);

	lcd_cmd(spi, ST7789V_RASET);
	lcd_data(spi, rows, 4);

	lcd_cmd(spi, ST7789V_CASET);
	lcd_data(spi, cols, 4);

	lcd_cmd(spi, ST7789V_RAMWR);
}

void st7789v_write_data(void *data, size_t len)
{
	static spi_transaction_t trans;
	esp_err_t ret;

	memset(&trans, 0, sizeof(spi_transaction_t));
	trans.tx_buffer = data;
	trans.length = 8 * len;
	trans.user = (void *)1;

	ret = spi_device_queue_trans(spi, &trans, portMAX_DELAY);
	ZERO_CHECK(ret);
	assert(ret == ESP_OK);
}

void st7789v_write_data_wait(void)
{
	spi_transaction_t *trans;
	esp_err_t ret;

	ret = spi_device_get_trans_result(spi, &trans, portMAX_DELAY);
	assert(ret == ESP_OK);
}

void st7789v_set_slow_sync(void)
{
	uint8_t div = DIV_SLOW; // Set slow refresh.

	lcd_cmd(spi, ST7789V_NOP); // To prevent from missing the first byte of the command.
	lcd_cmd(spi, ST7789V_FRCTRL1); // Update refresh.
	lcd_data(spi, &div, 1);
	lcd_cmd(spi, ST7789V_RAMWR); // Set RAM write mode.
}

void st7789v_set_fast_sync(void)
{
	uint8_t div = DIV_FAST; // Set fast refresh.

	lcd_cmd(spi, ST7789V_NOP); // To prevent from missing the first byte of the command.
	lcd_cmd(spi, ST7789V_FRCTRL1); // Update refresh.
	lcd_data(spi, &div, 1);
	lcd_cmd(spi, ST7789V_RAMWR); // Set RAM write mode.
}

void st7789v_suspend(void)
{
	controller_asleep = true;

	lcd_cmd(spi, ST7789V_NOP); // To prevent from missing the first byte of the command.
	lcd_cmd(spi, ST7789V_DISPOFF); // Set display output off
	lcd_cmd(spi, ST7789V_SLPIN); // Set display controller to sleep mode.

	vTaskDelay(pdMS_TO_TICKS(30)); // Wait for sleep mode to take before returning.
}

void st7789v_resume(void)
{
	lcd_cmd(spi, ST7789V_NOP); // To prevent from missing the first byte of the command.
	lcd_cmd(spi, ST7789V_SLPOUT); // Wake display controller from sleep mode.
	vTaskDelay(pdMS_TO_TICKS(30)); // Wait for sleep mode to end before returning.
	lcd_cmd(spi, ST7789V_DISPON); // Set display output on

	controller_asleep = false;
}

void st7789v_wait_vsync(void)
{
	if ((lcdcfg->te_pin != -1) && !controller_asleep)
	{
		// Wait for vsync, or at most 100ms.
		vsync_notify_task = xTaskGetCurrentTaskHandle();
		ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100));
	}
}

void st7789v_set_brightness(uint8_t brightness)
{
	if (lcdcfg->bckl_pin != -1)
	{
		if (brightness == 0)
		{
			ledc_stop(LEDC_LOW_SPEED_MODE, lcdcfg->bckl_pwm_channel, 0);
		} 
		else
		{
			// Fade to desired level during 200 ms.
			ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE, lcdcfg->bckl_pwm_channel, brightness, 200, LEDC_FADE_NO_WAIT);
		}
	}
}
