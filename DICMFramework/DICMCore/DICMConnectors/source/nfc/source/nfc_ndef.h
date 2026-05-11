/**
  ******************************************************************************
  * @file    nfc_ndef.h
  * @author  Felix Qin
  * @date    27-Apr-2025
  * @brief   This file help to manage Text NDEF file.
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef LIB_NDEF_TEXT_H_
#define LIB_NDEF_TEXT_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
 
uint16_t generate_text_ndef_record(char *text_in, uint8_t *ndef_out);
uint16_t generate_ble_oob_ndef_record(uint8_t *mac_address, uint8_t *ndef_record);
uint16_t generate_uri_ndef_record(const char *uri_in, uint8_t *ndef_out);
uint16_t generate_android_aar_ndef_record(const char *app_name_in, uint8_t *ndef_out);

#ifdef __cplusplus
}
#endif

#endif /* LIB_NDEF_TEXT_H_ */
