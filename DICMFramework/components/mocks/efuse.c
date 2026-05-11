/*
 * efuse.c
 *
 *  Created on: 2 okt. 2023
 *      Author: Andlun
 */
#include <stdint.h>

int esp_efuse_mac_get_default(uint8_t* out)
{
    for (int i = 0; i < 6; i++)
    {
        out[i] = 0xFE;
    }
    return 0;
}
