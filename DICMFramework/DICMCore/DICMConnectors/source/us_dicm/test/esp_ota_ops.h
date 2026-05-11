#ifndef ESP_OTA_OPS_H
#define ESP_OTA_OPS_H

#include "esp_err.h"
#include "esp_partition.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OTA_SIZE_UNKNOWN 0xffffffff

typedef uint32_t esp_ota_handle_t;

const esp_partition_t *esp_ota_get_next_update_partition(const esp_partition_t *start_from);
esp_err_t esp_ota_begin(const esp_partition_t *partition, size_t image_size, esp_ota_handle_t *out_handle);
esp_err_t esp_ota_write(esp_ota_handle_t handle, const void *data, size_t size);
esp_err_t esp_ota_end(esp_ota_handle_t handle);
esp_err_t esp_ota_abort(esp_ota_handle_t handle);
esp_err_t esp_ota_set_boot_partition(const esp_partition_t *partition);
const esp_partition_t *esp_ota_get_running_partition(void);
const esp_partition_t *esp_ota_get_boot_partition(void);

#ifdef __cplusplus
}
#endif

#endif /* ESP_OTA_OPS_H */
