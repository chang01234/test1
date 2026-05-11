#include <stdint.h>
#include "hal_system.h"
#include "esp_system.h"

/*! \brief HAL function to restart the system
	\param void
 */
void hal_restart_system(void)
{
    esp_restart();
}

