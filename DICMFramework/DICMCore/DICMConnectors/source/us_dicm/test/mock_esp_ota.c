#include "mock_esp_ota.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include <string.h>

/* ---------------- Mock state ---------------- */

ota_mock_t ota_mock;

/* Reset between tests */
void mock_esp_ota_reset(void)
{
    memset(&ota_mock, 0, sizeof(ota_mock));

    ota_mock.begin_ret = ESP_OK;
    ota_mock.write_ret = ESP_OK;
    ota_mock.abort_ret = ESP_OK;
    ota_mock.end_ret = ESP_OK;
    ota_mock.set_boot_ret = ESP_OK;
    ota_mock.next_partition_valid = true;
    ota_mock.abort_count = 0;
}

void mock_esp_ota_set_next_partition_valid(bool valid)
{
    ota_mock.next_partition_valid = valid;
}

void mock_esp_ota_set_boot_partition_fail(bool fail)
{
    ota_mock.set_boot_ret = fail ? ESP_FAIL : ESP_OK;
}

void mock_esp_ota_set_begin_fail(bool fail)
{
    ota_mock.begin_ret = fail ? ESP_FAIL : ESP_OK;
}

void mock_esp_ota_set_write_fail(bool fail)
{
    ota_mock.write_ret = fail ? ESP_FAIL : ESP_OK;
}

void mock_esp_ota_set_end_fail(bool fail)
{
    ota_mock.end_ret = fail ? ESP_FAIL : ESP_OK;
}

bool mock_esp_ota_was_aborted(void)
{
    return ota_mock.abort_called;
}

void mock_esp_ota_reset_abort_count(void)
{
    ota_mock.abort_count = 0;
}

int mock_esp_ota_get_abort_count(void)
{
    return ota_mock.abort_count;
}

/* ---------------- Mocked ESP-IDF symbols ---------------- */

static esp_partition_t fake_partition = {
    .type = ESP_PARTITION_TYPE_APP,
    .subtype = ESP_PARTITION_SUBTYPE_APP_OTA_0,
    .address = 0x100000,
    .size = 0x200000,
    .label = "mock_ota"};

const esp_partition_t *
esp_ota_get_next_update_partition(const esp_partition_t *start)
{
    (void)start;
    ota_mock.get_part_called = true;

    if (!ota_mock.next_partition_valid)
    {
        return NULL;
    }

    return &fake_partition;
}

esp_err_t esp_ota_begin(const esp_partition_t *partition,
                        size_t image_size,
                        esp_ota_handle_t *out_handle)
{
    (void)partition;
    (void)image_size;
    ota_mock.begin_called = true;
    if (out_handle)
    {
        *out_handle = (esp_ota_handle_t)0x1234;
    }
    return ota_mock.begin_ret;
}

esp_err_t esp_ota_write(esp_ota_handle_t handle,
                        const void *data,
                        size_t size)
{
    (void)handle;
    (void)data;
    ota_mock.write_called = true;
    ota_mock.total_written += size;
    return ota_mock.write_ret;
}

esp_err_t esp_ota_abort(esp_ota_handle_t handle)
{
    (void)handle;
    ota_mock.abort_called = true;
    ota_mock.abort_count++;
    return ota_mock.abort_ret;
}

esp_err_t esp_ota_end(esp_ota_handle_t handle)
{
    (void)handle;
    ota_mock.end_called = true;
    return ota_mock.end_ret;
}

esp_err_t esp_ota_set_boot_partition(const esp_partition_t *partition)
{
    (void)partition;
    ota_mock.set_boot_called = true;
    return ota_mock.set_boot_ret;
}
