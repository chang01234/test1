/*! \file st7789v.h
 *	\brief Display driver function headers.
 */

#ifndef ST7789V_H_
#define ST7789V_H_

/** Includes ******************************************************************/
#include <stdint.h>
#include <stddef.h>

#include "hal/spi_types.h"
#include "hal/gpio_types.h"
#include "hal/timer_types.h"
#include "hal/ledc_types.h"

/** Defines *******************************************************************/
#define ST7789V_SLPIN		0x10
#define ST7789V_SLPOUT		0x11
#define ST7789V_MADCTL		0x36
#define ST7789V_COLMOD		0x3a
#define ST7789V_INVON		0x21
#define ST7789V_CASET		0x2a
#define ST7789V_RASET		0x2b
#define ST7789V_PORCTRL		0xb2
#define ST7789V_GCTRL		0xb7
#define ST7789V_VCOMS		0xbb
#define ST7789V_LCMCTRL		0xc0
#define ST7789V_VDVVRHEN	0xc2
#define ST7789V_VRHS		0xc3
#define ST7789V_VDVS		0xc4
#define ST7789V_FRCTRL2		0xc6
#define ST7789V_PWCTRL1		0xd0
#define ST7789V_PVGAMCTRL	0xe0
#define ST7789V_NVGAMCTRL	0xe1
#define ST7789V_DISPON		0x29
#define ST7789V_DISPOFF		0x28
#define ST7789V_RAMWR		0x2c
#define ST7789V_WRMEMC		0x3c
#define ST7789V_TEON		0x35
#define ST7789V_FRCTRL1		0xb3
#define ST7789V_FRCTRL2		0xc6
#define ST7789V_PTLAR		0x30
#define ST7789V_PTLON		0x12
#define ST7789V_GSCAN		0x45
#define ST7789V_RDDID		0x04
#define ST7789V_RDID1		0xda
#define ST7789V_RDID2		0xdb
#define ST7789V_RDID3		0xdc
#define ST7789V_NOP			0x00
#define ST7789V_SWRESET		0x01

#define FRCTRL1_DIV1		0x00
#define FRCTRL1_DIV2		0x01
#define FRCTRL1_DIV4		0x02
#define FRCTRL1_DIV16		0x03

#define FRCTRL2_111HZ		0x01
#define FRCTRL2_90HZ		0x05
#define FRCTRL2_75HZ		0x09
#define FRCTRL2_60HZ		0x0f
#define FRCTRL2_50HZ		0x15
#define FRCTRL2_40HZ		0x1e

// 45 & 22,5 Hz
/*#define FR					FRCTRL2_90HZ
#define DIV_FAST			FRCTRL1_DIV2
#define DIV_SLOW			FRCTRL1_DIV4*/

// 37,5 & 18,8 Hz
/*#define FR					FRCTRL2_75HZ
#define DIV_FAST			FRCTRL1_DIV2
#define DIV_SLOW			FRCTRL1_DIV4*/

// 60 & 15 Hz
#define FR					FRCTRL2_60HZ
#define DIV_FAST			FRCTRL1_DIV1
#define DIV_SLOW			FRCTRL1_DIV4

// 50 & 12.5 Hz
/*#define FR					FRCTRL2_50HZ
#define DIV_FAST			FRCTRL1_DIV1
#define DIV_SLOW			FRCTRL1_DIV4*/

// 40 & 10 Hz
/*#define FR					FRCTRL2_40HZ
#define DIV_FAST			FRCTRL1_DIV1
#define DIV_SLOW			FRCTRL1_DIV4*/

// 55,5 & 7 Hz
/*#define FR					FRCTRL2_111HZ
#define DIV_FAST			FRCTRL1_DIV2
#define DIV_SLOW			FRCTRL1_DIV16*/

// 45 & 5,5 Hz
/*#define FR					FRCTRL2_90HZ
#define DIV_FAST			FRCTRL1_DIV2
#define DIV_SLOW			FRCTRL1_DIV16*/

// 37,5 & 4,7 Hz
/*#define FR					FRCTRL2_75HZ
#define DIV_FAST			FRCTRL1_DIV2
#define DIV_SLOW			FRCTRL1_DIV16*/

// 30 & 3,75 Hz
/*#define FR					FRCTRL2_60HZ
#define DIV_FAST			FRCTRL1_DIV2
#define DIV_SLOW			FRCTRL1_DIV16*/

/*#define FR					FRCTRL2_75HZ
#define DIV_FAST			FRCTRL1_DIV4
#define DIV_SLOW			FRCTRL1_DIV4*/

#define MADCTL_MY			0x80
#define MADCTL_MX			0x40
#define MADCTL_MV			0x20
#define MADCTL_ML			0x10
#define MADCTL_RGB			0x08
#define MADCTL_MH			0x04

#define TE_MODE_1			0x00
#define TE_MODE_2			0x01

// 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
// G2 G1 G0 B4 B3 B2 B1 B0 R4 R3 R2 R1 R0 G5 G4 G3
#define RGB555(R, G, B)	(((R << 3) & 0x00f8) | \
						(((G << 14) | (G >> 2)) & 0xc007) | \
						((B << 8) & 0x1f00))

#define RGB565(R, G, B)	(((R << 3) & 0x00f8) | \
						(((G << 13) | (G >> 3)) & 0xe007) | \
						((B << 8) & 0x1f00))

typedef struct {
	spi_host_device_t spi_host;			//!< SPI bus.
	int dma_chan;						//!< DMA channel 1 or 2. 0 to disable DMA.
	gpio_num_t mosi_pin;				//!< MOSI/SDI/SDA pin.
	gpio_num_t sclk_pin;				//!< SCK pin.
	gpio_num_t cs_pin;					//!< CS pin.
	gpio_num_t dc_pin;					//!< DC/RS pin.
	gpio_num_t rst_pin;					//!< Reset pin.
	gpio_num_t bckl_pin;				//!< Backlight pin. -1 to disable.
	gpio_num_t te_pin;					//!< TE pin number for sync detection. -1 to disable.
	ledc_channel_t bckl_pwm_channel;	//!< Backlight PWM channel (0 - 7).
	ledc_timer_t bckl_pwm_timer;		//!< Backlight PWM timer (0 - 3).
	int16_t res_x;						//!< Horizontal resolution.
	int16_t res_y;						//!< Vertical resolution.
	int16_t transfer_size;				//!< Maximum transfer length (max 4094).
} st7789v_init_data_t;

/** Function prototypes *******************************************************/
void st7789v_initialize(const st7789v_init_data_t *initdata);
void st7789v_open_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void st7789v_write_data(void *data, size_t len);
void st7789v_write_data_wait(void);
void st7789v_set_slow_sync(void);
void st7789v_set_fast_sync(void);
void st7789v_suspend(void);
void st7789v_resume(void);
void st7789v_wait_vsync(void);
void st7789v_set_brightness(uint8_t brightness);

#endif /* ST7789V_H_ */
