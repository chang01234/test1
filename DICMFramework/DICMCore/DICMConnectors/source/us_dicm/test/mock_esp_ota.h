#ifndef MOCK_ESP_OTA_H
#define MOCK_ESP_OTA_H

#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    /* configurable behavior */
    esp_err_t begin_ret;
    esp_err_t write_ret;
    esp_err_t abort_ret;
    esp_err_t end_ret;
    esp_err_t set_boot_ret;
    bool next_partition_valid;

    /* observation */
    bool get_part_called;
    bool begin_called;
    bool write_called;
    bool abort_called;
    bool end_called;
    bool set_boot_called;
    int abort_count;

    size_t total_written;
} ota_mock_t;

extern ota_mock_t ota_mock;

/* Test control functions - match what test file expects */
void mock_esp_ota_reset(void);
void mock_esp_ota_set_next_partition_valid(bool valid);
void mock_esp_ota_set_begin_fail(bool fail);
void mock_esp_ota_set_write_fail(bool fail);
void mock_esp_ota_set_end_fail(bool fail);
void mock_esp_ota_set_boot_partition_fail(bool fail);
bool mock_esp_ota_was_aborted(void);
void mock_esp_ota_reset_abort_count(void);
int mock_esp_ota_get_abort_count(void);

#ifdef __cplusplus
}
#endif
#endif  // MOCK_ESP_OTA_H
