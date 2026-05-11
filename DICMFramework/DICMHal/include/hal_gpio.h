/*! \file hal_gpio.h
	\brief GPIO Hardware Abstraction Layer
*/

#ifndef HAL_GPIO_H_
#define HAL_GPIO_H_
#include <stdint.h>

#define CB_TABLE_SIZE   32      //!< \~ Callback table size in pins

//! \~ General HAL GPIO return values
typedef enum _HAL_GPIO_RESULT_ENUM
{
    HAL_GPIO_OK,
    HAL_GPIO_ERROR_DEVICE_UNAVAILABLE,
    HAL_GPIO_ERROR_PORT_UNAVAILABLE,
    HAL_GPIO_ERROR_PIN_UNAVAILABLE,
} HAL_GPIO_RESULT_ENUM;

//! \~ Supported chips by GPIO HAL
typedef enum _HAL_GPIO_DEVICE_ENUM
{
    HAL_GPIO_DEVICE_SELF,
    HAL_GPIO_DEVICE_PCA9675,     // I2C
    HAL_GPIO_DEVICE_PCA9675_B,   // I2C
    HAL_GPIO_DEVICE_TCA6424A,    // I2C
    HAL_GPIO_DEVICE_TCA9554A,    // I2C
    HAL_GPIO_DEVICE_COUNT,
} HAL_GPIO_DEVICE_ENUM;

#define HAL_GPIO_DEVICE_PCA9673 HAL_GPIO_DEVICE_PCA9675 // PCA9673 is PCA9675 with async reset

//Defined for backwards compatibility with earlier HAL versions.
#define HAL_GPIO_DEVICE_ESP32 HAL_GPIO_DEVICE_SELF

//! \~ Supported GPIO HAL pin modes
typedef enum _HAL_GPIO_PINMODE_ENUM
{
    HAL_GPIO_PINMODE_DISABLE     = 0,
    HAL_GPIO_PINMODE_READ	 = 1,
    HAL_GPIO_PINMODE_WRITE       = 2,
    HAL_GPIO_PINMODE_READWRITE   = 3,
    HAL_GPIO_PINMODE_OPENDRAIN   = 4,
} HAL_GPIO_PINMODE_ENUM;
#define HAL_GPIO_PIN_MODE_MASK   (0x0F)

typedef enum _HAL_GPIO_PULLUP_DOWN_ENUM
{
    HAL_GPIO_PULLUP_DOWN_DISABLE    = 0x0,
    HAL_GPIO_PULLUP_ENABLE          = 0x10,
    HAL_GPIO_PULLDOWN_ENABLE        = 0x20
} HAL_GPIO_PULLUP_DOWN_ENUM;
#define HAL_GPIO_PIN_PULLUP_DOWN_MASK   (0xF0)

//! \~ GPIO HAL pin interrupt modes
typedef enum _HAL_GPIO_INTRMODE_ENUM
{   
    HAL_GPIO_INTRMODE_DISABLE,
    HAL_GPIO_INTRMODE_POSEDGE,
    HAL_GPIO_INTRMODE_NEGEDGE,
    HAL_GPIO_INTRMODE_ANYEDGE,
} HAL_GPIO_INTRMODE_ENUM;

//! \~ HAL GPIO pin interrupt callback routine
typedef void(*HAL_GPIO_ISR_CB)(int device,int port,int pin);

//! \~ HAL GPIO pinlist struct
//! \sa Gpio_pinlist
typedef struct _HAL_GPIO_PINLIST
{
    HAL_GPIO_DEVICE_ENUM device;
    int port;
    int pin;
    uint8_t pinmode;
    int level;
    HAL_GPIO_INTRMODE_ENUM intrmode;
    HAL_GPIO_ISR_CB cb;
} HAL_GPIO_PINLIST;

HAL_GPIO_RESULT_ENUM hal_gpio_init(void);
HAL_GPIO_RESULT_ENUM hal_gpio_pinsetup(const HAL_GPIO_DEVICE_ENUM device,const int port,const int pin,const uint8_t pinmode,const HAL_GPIO_INTRMODE_ENUM intrmode,const HAL_GPIO_ISR_CB cb);
HAL_GPIO_RESULT_ENUM hal_gpio_setlevel(const HAL_GPIO_DEVICE_ENUM device,const int port,const int pin,const int level);
HAL_GPIO_RESULT_ENUM hal_gpio_getlevel(const HAL_GPIO_DEVICE_ENUM device,const int port,const int pin,int * const level);

#endif /* HAL_GPIO_H_ */
