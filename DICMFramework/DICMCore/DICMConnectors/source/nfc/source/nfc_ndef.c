/**
  ******************************************************************************
  * @file    nfc_ndef.c
  * @author  Felix Qin
  * @date    27-Apr-2025
  * @brief   This file help to manage NDEF file.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>
#include "configuration.h"
#include "lib_ndef.h"

#define BLE_MAC_LENGTH     6  // BLE MAC Address Length
#define BLE_PAYLOAD_LENGTH 8

/**
  * @brief  This function write the text in the TAG.
  * @param  text_in : Text to write.
  * @param  ndef_out: The generated NDEF record.
  * @return NDEF record length.
  */
uint16_t generate_text_ndef_record(char *text_in, uint8_t *ndef_out)
{
  // uint16_t status = NDEF_ERROR;
  uint16_t text_size, ndef_len = 0;

  /* TEXT : 1+en+message */
  text_size = TEXT_TYPE_STRING_LENGTH + ISO_ENGLISH_CODE_STRING_LENGTH + strlen(text_in);

  /* TEXT header */
  ndef_out[ndef_len] = MB_Mask | TNF_WellKnown;
  if (text_size <= 0xFF) 
  {
    ndef_out[ndef_len] |= SR_Mask;  // Set the SR bit
  }
  ndef_len++;

  ndef_out[ndef_len++] = TEXT_TYPE_STRING_LENGTH;
  if (text_size > 0xFF)
  {
    ndef_out[ndef_len++] = (text_size & 0xFF000000) >> 24;
    ndef_out[ndef_len++] = (text_size & 0x00FF0000) >> 16;
    ndef_out[ndef_len++] = (text_size & 0x0000FF00) >> 8;
    ndef_out[ndef_len++] = text_size & 0x000000FF;
  }
  else
  {
    ndef_out[ndef_len++] = (uint8_t)text_size;
  }
  memcpy(&ndef_out[ndef_len], TEXT_TYPE_STRING, TEXT_TYPE_STRING_LENGTH);
  ndef_len += TEXT_TYPE_STRING_LENGTH;

  /* TEXT payload */
  ndef_out[ndef_len++] = ISO_ENGLISH_CODE_STRING_LENGTH;
  memcpy(&ndef_out[ndef_len], ISO_ENGLISH_CODE_STRING, ISO_ENGLISH_CODE_STRING_LENGTH);
  ndef_len += ISO_ENGLISH_CODE_STRING_LENGTH;

  memcpy(&ndef_out[ndef_len], text_in, strlen(text_in));
  ndef_len += strlen(text_in);

  return ndef_len;
}

/**
 * @brief Reverses the byte order of a MAC address or any byte array.
 *
 * This function takes a pointer to a byte array (e.g., a MAC address) and reverses
 * the order of its bytes. It is commonly used to convert between big-endian and 
 * little-endian representations of MAC addresses or other data.
 *
 * @param[in,out] mac Pointer to the byte array to be reversed.
 *                    The array is modified in place.
 * @param[in] length  The length of the byte array.
 *
 * @return void.
 */
void reverse_mac_address(uint8_t *mac, size_t length)
{
    for (size_t i = 0; i < length / 2; i++) 
    {
        uint8_t temp = mac[i];
        mac[i] = mac[length - 1 - i];
        mac[length - 1 - i] = temp;
    }
}

/**
  * @brief  This function generate an NDEF record for BLE OOB pairing
  * @param  mac_address : Ble mac address, should be 6 bytes.
  * @param  ndef_out: The generated NDEF record.
  * @return NDEF record length.
  */
uint16_t generate_ble_oob_ndef_record(uint8_t *mac_address_in, uint8_t *ndef_out)
{
    const char *type_name = "application/vnd.bluetooth.ep.oob";  // MIME type for BLE OOB
    uint8_t type_length = strlen(type_name);
    uint16_t ndef_len = 0;

    // NDEF Header
    ndef_out[ndef_len++] = SR_Mask | TNF_MediaType;

    // Type Length
    ndef_out[ndef_len++] = type_length;

    // Payload Length
    ndef_out[ndef_len++] = BLE_PAYLOAD_LENGTH;

    // Type payload/name
    memcpy(&ndef_out[ndef_len], type_name, type_length);
    ndef_len += type_length;

    ndef_out[ndef_len++] = BLE_PAYLOAD_LENGTH;
    ndef_out[ndef_len++] = 0x00;
    // Payload (BLE MAC Address)
    reverse_mac_address(mac_address_in, BLE_MAC_LENGTH);
    memcpy(&ndef_out[ndef_len], mac_address_in, BLE_MAC_LENGTH);
    ndef_len += BLE_MAC_LENGTH;

    // Total Record Length
    return ndef_len;
}

/**
  * @brief  This function generate an NDEF record for URI.
  * @param  uri_in: URI link(Example: "dometic.com" (with "https://www." prefix)).
  * @param  ndef_out: The generated NDEF record.
  * @return NDEF record length.
  */
uint16_t generate_uri_ndef_record(const char *uri_in, uint8_t *ndef_out)
{
    // URI Identifier Code for "https://www."
    const uint8_t uri_identifier_code = URI_ID_0x02;
    uint16_t ndef_len = 0;

    // Calculate URI length
    uint8_t uri_length = strlen(uri_in);

    // NDEF Header: TNF = Well-Known Type (0x01), SR = 1 (Short Record)
    ndef_out[ndef_len++] = SR_Mask | TNF_WellKnown;

    // Type Length: 1 byte (Type = "U" for URI)
    ndef_out[ndef_len++] = URI_TYPE_STRING_LENGTH;

    // Payload Length: URI Identifier Code (1 byte) + URI Length
    ndef_out[ndef_len++] = URI_TYPE_STRING_LENGTH + uri_length;

    // Type: "U" (URI Record Type)
    memcpy(&ndef_out[ndef_len], URI_TYPE_STRING, URI_TYPE_STRING_LENGTH);
    ndef_len += URI_TYPE_STRING_LENGTH;

    // URI Identifier Code
    ndef_out[ndef_len++] = uri_identifier_code;

    // URI (e.g., "google.com")
    memcpy(&ndef_out[ndef_len], uri_in, uri_length);
    ndef_len += uri_length;

    // Total Record Length
    return ndef_len;
}

/**
  * @brief  This function generate an NDEF record for Android AAR.
  * @param  app_name_in: App name(Example: "com.dometic.dev").
  * @param  ndef_out: The generated NDEF record.
  * @return NDEF record length.
  */
uint16_t generate_android_aar_ndef_record(const char *app_name_in, uint8_t *ndef_out)
{
    const char *type_name = "android.com:pkg";  // External Type for Android AAR
    const char type_length = strlen(type_name);
    uint16_t ndef_len = 0;

    // Calculate package name length
    uint8_t package_name_length = strlen(app_name_in);

    // NDEF Header: ME = 1, SR = 1 (Short Record), TNF = External Type (0x04)
    ndef_out[ndef_len++] = ME_Mask | SR_Mask | TNF_NFCForumExternal;

    // Type Length
    ndef_out[ndef_len++] = type_length;

    // Payload Length: Package Name Length
    ndef_out[ndef_len++] = package_name_length;

    // Type payload
    memcpy(&ndef_out[ndef_len], type_name, type_length);
    ndef_len += type_length;

    // Payload: Package Name (e.g., "com.tencent.mm" for WeChat)
    memcpy(&ndef_out[ndef_len], app_name_in, package_name_length);
    ndef_len += package_name_length;

    // Total Record Length
    return ndef_len;
}