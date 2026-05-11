/*
 * test_connector_us_dicm.cpp
 *
 * Unit tests for connector_us_dicm
 * Created on: 28 Jan 2026
 * Author: Kire Janev
 */

extern "C" {
#include "broker.h"
#include "configuration.h"
#include "connector_unittest.h"
#include "connector_us_dicm.h"
#include "crc32.h"
#include "ddm2.h"
#include "ddm2_parameter_list.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mock_esp_ota.h"
#include "update_service_types.h"
}

#include "DICMFrameworkTestFixture.hpp"

using ::testing::Test;

class ConnectorUsDicmTestFixture : public DICMFrameworkTestFixture
{
  public:
    static ConnectorUsDicmTestFixture *getInstance()
    {
        return mTestInstance;
    }

    static inline ConnectorUsDicmTestFixture *mTestInstance = NULL;

    ConnectorUsDicmTestFixture() : DICMFrameworkTestFixture()
    {
        mTestInstance = this;
    }

  protected:
    void SetUp() override
    {
        mock_esp_ota_reset();
        DICMFrameworkTestFixture::SetUp();
        connector_unittest_enable(NULL, NULL);
        setConnectorId(&connector_us_dicm.connector_id);
        DICMFrameworkTestFixture::SetupFramework();
    }

    void TearDown() override
    {
        DICMFrameworkTestFixture::TearDown();
    }
};

/*
 * Test: Register USM class instance via broker to trigger GW0INV handler
 */
TEST_F(ConnectorUsDicmTestFixture, RegisterUSMInstanceTriggersGW0INV)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;

    vPortPauseScheduler();
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    // Register USM class instance via broker - this triggers GW0INV publish
    uint32_t device_class = USM0;
    broker_register_instance(&device_class, connector_unittest.connector_id);

    // Verify the connector subscribed to USM parameters after GW0INV
    bool found_usm_dd_subscription = false;
    bool found_usm_tbs_subscription = false;
    bool found_usm_stat_subscription = false;

    int num_frames = getNumSentDDMP2Frames();
    for (int i = 0; i < num_frames; i++)
    {
        int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
        if ((res == 0) && (myFrame.frame.control == DDMP2_CONTROL_SUBSCRIBE))
        {
            uint32_t param_base = DDM2_PARAMETER_BASE_INSTANCE(myFrame.frame.subscribe.parameter);
            if (param_base == USM0DD)
            {
                found_usm_dd_subscription = true;
            }
            else if (param_base == USM0TBS)
            {
                found_usm_tbs_subscription = true;
            }
            else if (param_base == USM0STAT)
            {
                found_usm_stat_subscription = true;
            }
        }
    }

    EXPECT_TRUE(found_usm_dd_subscription) << "Should have subscribed to USM0DD after GW0INV";
    EXPECT_TRUE(found_usm_tbs_subscription) << "Should have subscribed to USM0TBS after GW0INV";
    EXPECT_TRUE(found_usm_stat_subscription) << "Should have subscribed to USM0STAT after GW0INV";

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
}

/*
 * Test: Subscribe to US0LIST should return firmware ID list
 */
TEST_F(ConnectorUsDicmTestFixture, SubscribeToUS0LIST)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;
    int us_instance = 0;

    vPortPauseScheduler();
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                   (US0LIST | DDM2_PARAMETER_INSTANCE(us_instance)),
                                   NULL,
                                   0,
                                   connector_unittest.connector_id,
                                   (TickType_t)portMAX_DELAY);

    // Should receive a PUBLISH with the firmware ID list
    EXPECT_GE(getNumSentDDMP2Frames(), 1) << "Should have sent at least one DDMP2 frame";

    int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "Should have a frame available";
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_PUBLISH) << "Should have sent a PUBLISH";
    EXPECT_EQ(DDM2_PARAMETER_BASE_INSTANCE(myFrame.frame.publish.parameter), US0LIST)
        << "Should be US0LIST parameter";

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
}

/*
 * Test: Subscribe to US0STAT should return service status
 */
TEST_F(ConnectorUsDicmTestFixture, SubscribeToUS0STAT)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;
    int us_instance = 0;

    vPortPauseScheduler();
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                   (US0STAT | DDM2_PARAMETER_INSTANCE(us_instance)),
                                   NULL,
                                   0,
                                   connector_unittest.connector_id,
                                   (TickType_t)portMAX_DELAY);

    EXPECT_GE(getNumSentDDMP2Frames(), 1) << "Should have sent at least one DDMP2 frame";

    int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "Should have a frame available";
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_PUBLISH) << "Should have sent a PUBLISH";
    EXPECT_EQ(DDM2_PARAMETER_BASE_INSTANCE(myFrame.frame.publish.parameter), US0STAT)
        << "Should be US0STAT parameter";

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
}

/*
 * Test: Invalid firmware ID in download done
 */
TEST_F(ConnectorUsDicmTestFixture, InvalidFirmwareIdIgnored)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;
    int usm_instance = 0;

    us_dd_download_done_t dd;
    memset(&dd, 0, sizeof(dd));
    dd.size = 1024;
    dd.crc = 0x12345678;
    dd.service = 0x01;

    // Use invalid firmware ID
    const char *invalid_fwid = "INVALID_FIRMWARE_ID";
    size_t fwid_len = strlen(invalid_fwid);
    size_t dd_size = offsetof(us_dd_download_done_t, fwid) + fwid_len;

    uint8_t dd_buffer[256];
    memcpy(dd_buffer, &dd, offsetof(us_dd_download_done_t, fwid));
    memcpy(dd_buffer + offsetof(us_dd_download_done_t, fwid), invalid_fwid, fwid_len);

    vPortPauseScheduler();
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    // Subscribe to USM0DD before publishing to it
    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                   (USM0DD | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   NULL,
                                   0,
                                   getConnectorId(),
                                   (TickType_t)portMAX_DELAY);
    // Check that we got a SUBSCRIBE response for USM0DD
    bool found_subscribe = false;
    int num_frames = getNumSentDDMP2Frames();
    for (int i = 0; i < num_frames; i++)
    {
        int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
        if ((res == 0) && (myFrame.frame.control == DDMP2_CONTROL_SUBSCRIBE) &&
            (DDM2_PARAMETER_BASE_INSTANCE(myFrame.frame.subscribe.parameter) == USM0DD))
        {
            found_subscribe = true;
            break;
        }
    }
    EXPECT_TRUE(found_subscribe) << "Should have received SUBSCRIBE confirmation for USM0DD";

    clearSentDDMP2Frames();

    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                   (USM0DD | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   dd_buffer,
                                   dd_size,
                                   connector_unittest.connector_id,
                                   (TickType_t)portMAX_DELAY);

    // Should not send US0RRQ for invalid firmware ID
    bool found_rrq = false;
    num_frames = getNumSentDDMP2Frames();
    for (int i = 0; i < num_frames; i++)
    {
        int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
        if ((res == 0) && (DDM2_PARAMETER_BASE_INSTANCE(myFrame.frame.publish.parameter) == US0RRQ))
        {
            found_rrq = true;
            break;
        }
    }
    EXPECT_FALSE(found_rrq) << "Should not send US0RRQ for invalid firmware ID";

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
}

/*
 * Test: OTA preparation fails
 */
TEST_F(ConnectorUsDicmTestFixture, PrepareOTAFails)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;
    int usm_instance = 0;

    // Prepare download done data
    us_dd_download_done_t dd;
    memset(&dd, 0, sizeof(dd));
    dd.size = 1024;
    dd.crc = 0x12345678;
    dd.service = 0x01;  // Service bit 0

    // Copy firmware ID (use the actual firmware build ID from the connector)
    const char *fwid = FIRMWARE_BUILD_ID;
    size_t fwid_len = strlen(fwid);
    size_t dd_size = offsetof(us_dd_download_done_t, fwid) + fwid_len;

    uint8_t dd_buffer[256];
    memcpy(dd_buffer, &dd, offsetof(us_dd_download_done_t, fwid));
    memcpy(dd_buffer + offsetof(us_dd_download_done_t, fwid), fwid, fwid_len);

    // Set up mock to fail partition lookup and OTA begin fail
    mock_esp_ota_set_next_partition_valid(false);
    mock_esp_ota_set_begin_fail(true);

    vPortPauseScheduler();
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
    // Subscribe to USM0DD before publishing to it
    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                   (USM0DD | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   NULL,
                                   0,
                                   getConnectorId(),
                                   (TickType_t)portMAX_DELAY);
    // Check that we got a SUBSCRIBE response for USM0DD
    bool found_subscribe = false;
    int num_frames = getNumSentDDMP2Frames();
    for (int i = 0; i < num_frames; i++)
    {
        int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
        if ((res == 0) && (myFrame.frame.control == DDMP2_CONTROL_SUBSCRIBE) &&
            (DDM2_PARAMETER_BASE_INSTANCE(myFrame.frame.subscribe.parameter) == USM0DD))
        {
            found_subscribe = true;
            break;
        }
    }
    EXPECT_TRUE(found_subscribe) << "Should have received SUBSCRIBE confirmation for USM0DD";

    clearSentDDMP2Frames();

    // Simulate USM0DD publish
    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                   (USM0DD | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   dd_buffer,
                                   dd_size,
                                   connector_unittest.connector_id,
                                   (TickType_t)portMAX_DELAY);

    // Should have sent US0RRQ (read request) to start the update
    bool found_rrq = false;
    num_frames = getNumSentDDMP2Frames();
    for (int i = 0; i < num_frames; i++)
    {
        int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
        if ((res == 0) && (DDM2_PARAMETER_BASE_INSTANCE(myFrame.frame.publish.parameter) == US0RRQ))
        {
            found_rrq = true;
            break;
        }
    }
    EXPECT_TRUE(found_rrq) << "Should have sent US0RRQ to start update";

    // Check that US0DATA was sent with abort result since OTA preparation failed
    bool found_data = false;
    num_frames = getNumSentDDMP2Frames();
    for (int i = 0; i < num_frames; i++)
    {
        int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
        if ((res == 0) && (DDM2_PARAMETER_BASE_INSTANCE(myFrame.frame.publish.parameter) == US0DATA))
        {
            found_data = true;
            // Verify it's an abort message
            if (frame_size >= sizeof(us_data_read_ack_t))
            {
                us_data_read_ack_t *ack = (us_data_read_ack_t *)myFrame.frame.publish.value.raw;
                EXPECT_EQ(ack->result, US_ACK_RESULT_ABORT) << "US0DATA should contain abort result";
            }
            break;
        }
    }
    EXPECT_TRUE(found_data) << "Should send US0DATA with abort when OTA preparation fails";

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
}

/*
 * Test: Simulate successful OTA transfer
 */
TEST_F(ConnectorUsDicmTestFixture, SuccessfulOTATransfer)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;
    int usm_instance = 0;
    int us_instance = 0;

    // Test data for OTA transfer (2 blocks)
    const size_t block_size = 64;
    const size_t total_size = block_size * 2;
    uint8_t test_data[total_size];
    for (size_t i = 0; i < total_size; i++)
    {
        test_data[i] = i & 0xFF;
    }

    // Calculate expected CRC
    crc32_t expected_crc = crc32_init();
    expected_crc = crc32_update(expected_crc, test_data, total_size);
    expected_crc = crc32_finalize(expected_crc);

    // vPortPauseScheduler();
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    // Subscribe to USM0DD, USM0TBS, USM0STAT
    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                   (USM0DD | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   NULL, 0,
                                   getConnectorId(),
                                   portMAX_DELAY);

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                   (USM0TBS | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   NULL, 0,
                                   getConnectorId(),
                                   portMAX_DELAY);

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                   (USM0STAT | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   NULL, 0,
                                   getConnectorId(),
                                   portMAX_DELAY);

    clearSentDDMP2Frames();

    // Step 1: Publish USM0DD (download done)
    us_dd_download_done_t dd;
    memset(&dd, 0, sizeof(dd));
    dd.size = total_size;
    dd.crc = expected_crc;
    dd.service = 0x01;  // Service bit 0

    const char *fwid = FIRMWARE_BUILD_ID;
    size_t fwid_len = strlen(fwid);
    size_t dd_size = offsetof(us_dd_download_done_t, fwid) + fwid_len;

    uint8_t dd_buffer[256];
    memcpy(dd_buffer, &dd, offsetof(us_dd_download_done_t, fwid));
    memcpy(dd_buffer + offsetof(us_dd_download_done_t, fwid), fwid, fwid_len);

    mock_esp_ota_set_next_partition_valid(true);
    mock_esp_ota_set_begin_fail(false);

    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                   (USM0DD | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   dd_buffer, dd_size,
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(50));

    // Verify US0RRQ was published
    bool found_rrq = false;
    int num_frames = getNumSentDDMP2Frames();
    for (int i = 0; i < num_frames; i++)
    {
        int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
        if ((res == 0) && (DDM2_PARAMETER_BASE_INSTANCE(myFrame.frame.publish.parameter) == US0RRQ))
        {
            found_rrq = true;
            break;
        }
    }
    EXPECT_TRUE(found_rrq) << "Should have sent US0RRQ after USM0DD";

    clearSentDDMP2Frames();

    // Publish USM0TBS (transfer block size)
    int32_t transfer_block_size = block_size;
    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                   (USM0TBS | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   &transfer_block_size, sizeof(transfer_block_size),
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(50));
    clearSentDDMP2Frames();

    // Step 2: Send binary data in 2 blocks
    for (int block = 0; block < 2; block++)
    {
        // Send the raw data block directly
        uint8_t *data_to_send = &test_data[block * block_size];

        // Set US0DATA
        connector_send_frame_to_broker(DDMP2_CONTROL_SET,
                                       (US0DATA | DDM2_PARAMETER_INSTANCE(us_instance)),
                                       data_to_send, block_size,
                                       connector_unittest.connector_id,
                                       portMAX_DELAY);

        // Time for processing
        // vTaskDelay(pdMS_TO_TICKS(100));

        // Verify US0DATA publish (ACK OK)
        bool found_data_ack = false;
        num_frames = getNumSentDDMP2Frames();
        printf("Block %d: num_frames after US0DATA = %d\n", block, num_frames);
        for (int i = 0; i < num_frames; i++)
        {
            int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
            printf("Block %d: frame %d, control=%d, param=0x%x\n", block, i, myFrame.frame.control, myFrame.frame.publish.parameter);
            if ((res == 0) && (myFrame.frame.control == DDMP2_CONTROL_PUBLISH) &&
                (DDM2_PARAMETER_BASE_INSTANCE(myFrame.frame.publish.parameter) == US0DATA))
            {
                found_data_ack = true;
                us_data_read_ack_t *ack = (us_data_read_ack_t *)myFrame.frame.publish.value.raw;
                EXPECT_EQ(ack->result, US_ACK_RESULT_OK) << "Block " << block << " should ACK OK";
                break;
            }
        }
        EXPECT_TRUE(found_data_ack) << "Should have received US0DATA ACK for block " << block;

        clearSentDDMP2Frames();

        // Set US0BTR (block transfer result) with ACK OK
        int32_t btr_result = US_ACK_RESULT_OK;
        connector_send_frame_to_broker(DDMP2_CONTROL_SET,
                                       (US0BTR | DDM2_PARAMETER_INSTANCE(us_instance)),
                                       &btr_result, sizeof(btr_result),
                                       connector_unittest.connector_id,
                                       portMAX_DELAY);

        // Time for processing
        // vTaskDelay(pdMS_TO_TICKS(50));

        // Verify US0BTR publish (ACK OK response)
        bool found_btr_pub = false;
        num_frames = getNumSentDDMP2Frames();
        printf("Block %d: num_frames after US0BTR = %d\n", block, num_frames);
        for (int i = 0; i < num_frames; i++)
        {
            int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
            if ((res == 0) && (myFrame.frame.control == DDMP2_CONTROL_PUBLISH) &&
                (DDM2_PARAMETER_BASE_INSTANCE(myFrame.frame.publish.parameter) == US0BTR))
            {
                found_btr_pub = true;
                int32_t *result = (int32_t *)myFrame.frame.publish.value.raw;
                EXPECT_EQ(*result, US_ACK_RESULT_OK) << "Block " << block << " BTR should be OK";
                break;
            }
        }
        EXPECT_TRUE(found_btr_pub) << "Should have received US0BTR publish for block " << block;
    }

    // Step 3: Final BTR set to trigger completion
    int32_t final_btr = US_ACK_RESULT_OK;
    connector_send_frame_to_broker(DDMP2_CONTROL_SET,
                                   (US0BTR | DDM2_PARAMETER_INSTANCE(us_instance)),
                                   &final_btr, sizeof(final_btr),
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // Verify progress was published
    bool found_progress_100 = false;
    num_frames = getNumSentDDMP2Frames();
    for (int i = 0; i < num_frames; i++)
    {
        int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
        if ((res == 0) && (DDM2_PARAMETER_BASE_INSTANCE(myFrame.frame.publish.parameter) == US0PROG))
        {
            int32_t *progress = (int32_t *)myFrame.frame.publish.value.raw;
            if (*progress == 100)
            {
                found_progress_100 = true;
                break;
            }
        }
    }
    EXPECT_TRUE(found_progress_100) << "Should have published 100% progress";

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
}

/*
 * Test: USMSTAT error in begin state causes transition to idle
 */
TEST_F(ConnectorUsDicmTestFixture, USMSTATErrorInBeginState)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;
    int usm_instance = 0;

    // Test data for OTA transfer
    const size_t block_size = 64;
    const size_t total_size = block_size * 2;

    // Calculate expected CRC
    uint8_t test_data[total_size];
    for (size_t i = 0; i < total_size; i++)
    {
        test_data[i] = i & 0xFF;
    }
    crc32_t expected_crc = crc32_init();
    expected_crc = crc32_update(expected_crc, test_data, total_size);
    expected_crc = crc32_finalize(expected_crc);

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    // Subscribe to required parameters
    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                   (USM0DD | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   NULL, 0,
                                   getConnectorId(),
                                   portMAX_DELAY);

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                   (USM0TBS | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   NULL, 0,
                                   getConnectorId(),
                                   portMAX_DELAY);

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                   (USM0STAT | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   NULL, 0,
                                   getConnectorId(),
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(10));
    clearSentDDMP2Frames();

    // Step 1: Publish USM0DD to move to begin state
    us_dd_download_done_t dd;
    memset(&dd, 0, sizeof(dd));
    dd.size = total_size;
    dd.crc = expected_crc;
    dd.service = 0x01;

    const char *fwid = FIRMWARE_BUILD_ID;
    size_t fwid_len = strlen(fwid);
    size_t dd_size = offsetof(us_dd_download_done_t, fwid) + fwid_len;

    uint8_t dd_buffer[256];
    memcpy(dd_buffer, &dd, offsetof(us_dd_download_done_t, fwid));
    memcpy(dd_buffer + offsetof(us_dd_download_done_t, fwid), fwid, fwid_len);

    mock_esp_ota_set_next_partition_valid(true);
    mock_esp_ota_set_begin_fail(false);

    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                   (USM0DD | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   dd_buffer, dd_size,
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(50));

    // Verify we're in begin state by checking US0RRQ was published
    bool found_rrq = false;
    int num_frames = getNumSentDDMP2Frames();
    for (int i = 0; i < num_frames; i++)
    {
        int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
        if ((res == 0) && (DDM2_PARAMETER_BASE_INSTANCE(myFrame.frame.publish.parameter) == US0RRQ))
        {
            found_rrq = true;
            break;
        }
    }
    EXPECT_TRUE(found_rrq) << "Should have sent US0RRQ after USM0DD";

    clearSentDDMP2Frames();

    // Step 2: Send USMSTAT with error (non-zero value) while in begin state
    int32_t status_error = 1;  // Non-zero indicates error
    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                   (USM0STAT | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   &status_error, sizeof(status_error),
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(50));

    // Step 3: Verify state machine went back to idle by sending another USM0DD
    // If we're in idle, it should process this and send US0RRQ again
    clearSentDDMP2Frames();

    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                   (USM0DD | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   dd_buffer, dd_size,
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(50));

    // Should receive US0RRQ again if we're back in idle state
    found_rrq = false;
    num_frames = getNumSentDDMP2Frames();
    for (int i = 0; i < num_frames; i++)
    {
        int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
        if ((res == 0) && (DDM2_PARAMETER_BASE_INSTANCE(myFrame.frame.publish.parameter) == US0RRQ))
        {
            found_rrq = true;
            break;
        }
    }
    EXPECT_TRUE(found_rrq) << "Should have sent US0RRQ again after returning to idle state";

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
}

/*
 * Test: USMSTAT error in progress state causes OTA abort and transition to idle
 */
TEST_F(ConnectorUsDicmTestFixture, USMSTATErrorInProgressState)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;
    int usm_instance = 0;
    int us_instance = 0;

    // Test data for OTA transfer
    const size_t block_size = 64;
    const size_t total_size = block_size * 2;
    uint8_t test_data[total_size];
    for (size_t i = 0; i < total_size; i++)
    {
        test_data[i] = i & 0xFF;
    }

    crc32_t expected_crc = crc32_init();
    expected_crc = crc32_update(expected_crc, test_data, total_size);
    expected_crc = crc32_finalize(expected_crc);

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    // Subscribe to required parameters
    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                   (USM0DD | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   NULL, 0,
                                   getConnectorId(),
                                   portMAX_DELAY);

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                   (USM0TBS | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   NULL, 0,
                                   getConnectorId(),
                                   portMAX_DELAY);

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                   (USM0STAT | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   NULL, 0,
                                   getConnectorId(),
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(10));
    clearSentDDMP2Frames();

    // Step 1: Publish USM0DD to start OTA process
    us_dd_download_done_t dd;
    memset(&dd, 0, sizeof(dd));
    dd.size = total_size;
    dd.crc = expected_crc;
    dd.service = 0x01;

    const char *fwid = FIRMWARE_BUILD_ID;
    size_t fwid_len = strlen(fwid);
    size_t dd_size = offsetof(us_dd_download_done_t, fwid) + fwid_len;

    uint8_t dd_buffer[256];
    memcpy(dd_buffer, &dd, offsetof(us_dd_download_done_t, fwid));
    memcpy(dd_buffer + offsetof(us_dd_download_done_t, fwid), fwid, fwid_len);

    mock_esp_ota_set_next_partition_valid(true);
    mock_esp_ota_set_begin_fail(false);

    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                   (USM0DD | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   dd_buffer, dd_size,
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(50));
    clearSentDDMP2Frames();

    // Step 2: Publish USM0TBS to move to in_progress state
    int32_t transfer_block_size = block_size;
    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                   (USM0TBS | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   &transfer_block_size, sizeof(transfer_block_size),
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(50));
    clearSentDDMP2Frames();

    // Step 3: Send first data block to confirm we're in progress state
    uint8_t *data_to_send = &test_data[0];
    connector_send_frame_to_broker(DDMP2_CONTROL_SET,
                                   (US0DATA | DDM2_PARAMETER_INSTANCE(us_instance)),
                                   data_to_send, block_size,
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(100));

    // Verify US0DATA ACK was received
    bool found_data_ack = false;
    int num_frames = getNumSentDDMP2Frames();
    for (int i = 0; i < num_frames; i++)
    {
        int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
        if ((res == 0) && (myFrame.frame.control == DDMP2_CONTROL_PUBLISH) &&
            (DDM2_PARAMETER_BASE_INSTANCE(myFrame.frame.publish.parameter) == US0DATA))
        {
            found_data_ack = true;
            break;
        }
    }
    EXPECT_TRUE(found_data_ack) << "Should have received US0DATA ACK";

    clearSentDDMP2Frames();

    // Reset mock to track OTA abort
    mock_esp_ota_reset_abort_count();

    // Step 4: Send USMSTAT with error while in progress state
    int32_t status_error = 1;  // Non-zero indicates error
    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                   (USM0STAT | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   &status_error, sizeof(status_error),
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(50));

    // Verify OTA was aborted
    EXPECT_EQ(mock_esp_ota_get_abort_count(), 1) << "OTA should have been aborted";

    // Step 5: Verify state machine went back to idle by sending another USM0DD
    clearSentDDMP2Frames();

    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                   (USM0DD | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   dd_buffer, dd_size,
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(50));

    // Should receive US0RRQ again if we're back in idle state
    bool found_rrq = false;
    num_frames = getNumSentDDMP2Frames();
    for (int i = 0; i < num_frames; i++)
    {
        int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
        if ((res == 0) && (DDM2_PARAMETER_BASE_INSTANCE(myFrame.frame.publish.parameter) == US0RRQ))
        {
            found_rrq = true;
            break;
        }
    }
    EXPECT_TRUE(found_rrq) << "Should have sent US0RRQ again after returning to idle state";

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
}

/*
 * Test: USMSTAT error in wait_complete state causes OTA abort and transition to idle
 */
TEST_F(ConnectorUsDicmTestFixture, USMSTATErrorInWaitCompleteState)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;
    int usm_instance = 0;
    int us_instance = 0;

    // Test data for OTA transfer (single block to simplify)
    const size_t block_size = 64;
    const size_t total_size = block_size;  // Single block
    uint8_t test_data[total_size];
    for (size_t i = 0; i < total_size; i++)
    {
        test_data[i] = i & 0xFF;
    }

    crc32_t expected_crc = crc32_init();
    expected_crc = crc32_update(expected_crc, test_data, total_size);
    expected_crc = crc32_finalize(expected_crc);

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    // Subscribe to required parameters
    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                   (USM0DD | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   NULL, 0,
                                   getConnectorId(),
                                   portMAX_DELAY);

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                   (USM0TBS | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   NULL, 0,
                                   getConnectorId(),
                                   portMAX_DELAY);

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                   (USM0STAT | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   NULL, 0,
                                   getConnectorId(),
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(10));
    clearSentDDMP2Frames();

    // Step 1: Publish USM0DD to start OTA process
    us_dd_download_done_t dd;
    memset(&dd, 0, sizeof(dd));
    dd.size = total_size;
    dd.crc = expected_crc;
    dd.service = 0x01;

    const char *fwid = FIRMWARE_BUILD_ID;
    size_t fwid_len = strlen(fwid);
    size_t dd_size = offsetof(us_dd_download_done_t, fwid) + fwid_len;

    uint8_t dd_buffer[256];
    memcpy(dd_buffer, &dd, offsetof(us_dd_download_done_t, fwid));
    memcpy(dd_buffer + offsetof(us_dd_download_done_t, fwid), fwid, fwid_len);

    mock_esp_ota_set_next_partition_valid(true);
    mock_esp_ota_set_begin_fail(false);

    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                   (USM0DD | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   dd_buffer, dd_size,
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(50));
    clearSentDDMP2Frames();

    // Step 2: Publish USM0TBS to move to in_progress state
    int32_t transfer_block_size = block_size;
    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                   (USM0TBS | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   &transfer_block_size, sizeof(transfer_block_size),
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(50));
    clearSentDDMP2Frames();

    // Step 3: Send single data block (which completes the transfer and moves to wait_complete)
    connector_send_frame_to_broker(DDMP2_CONTROL_SET,
                                   (US0DATA | DDM2_PARAMETER_INSTANCE(us_instance)),
                                   test_data, block_size,
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(100));

    // Verify US0DATA ACK was received with OK (should transition to wait_complete)
    bool found_data_ack = false;
    int num_frames = getNumSentDDMP2Frames();
    for (int i = 0; i < num_frames; i++)
    {
        int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
        if ((res == 0) && (myFrame.frame.control == DDMP2_CONTROL_PUBLISH) &&
            (DDM2_PARAMETER_BASE_INSTANCE(myFrame.frame.publish.parameter) == US0DATA))
        {
            found_data_ack = true;
            us_data_read_ack_t *ack = (us_data_read_ack_t *)myFrame.frame.publish.value.raw;
            EXPECT_EQ(ack->result, US_ACK_RESULT_OK) << "Should ACK OK for complete transfer";
            break;
        }
    }
    EXPECT_TRUE(found_data_ack) << "Should have received US0DATA ACK";

    clearSentDDMP2Frames();

    // Reset mock to track OTA abort
    mock_esp_ota_reset_abort_count();

    // Step 4: Send USMSTAT with error while in wait_complete state
    int32_t status_error = 1;  // Non-zero indicates error
    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                   (USM0STAT | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   &status_error, sizeof(status_error),
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(50));

    // Verify OTA was aborted
    EXPECT_EQ(mock_esp_ota_get_abort_count(), 1) << "OTA should have been aborted";

    // Step 5: Verify state machine went back to idle by sending another USM0DD
    clearSentDDMP2Frames();

    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                   (USM0DD | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   dd_buffer, dd_size,
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(50));

    // Should receive US0RRQ again if we're back in idle state
    bool found_rrq = false;
    num_frames = getNumSentDDMP2Frames();
    for (int i = 0; i < num_frames; i++)
    {
        int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
        if ((res == 0) && (DDM2_PARAMETER_BASE_INSTANCE(myFrame.frame.publish.parameter) == US0RRQ))
        {
            found_rrq = true;
            break;
        }
    }
    EXPECT_TRUE(found_rrq) << "Should have sent US0RRQ again after returning to idle state";

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
}

/*
 * Test: USMSTAT with OK status (0) does not cause state change
 */
TEST_F(ConnectorUsDicmTestFixture, USMSTATOkStatusNoStateChange)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;
    int usm_instance = 0;
    int us_instance = 0;

    // Test data for OTA transfer
    const size_t block_size = 64;
    const size_t total_size = block_size * 2;
    uint8_t test_data[total_size];
    for (size_t i = 0; i < total_size; i++)
    {
        test_data[i] = i & 0xFF;
    }

    crc32_t expected_crc = crc32_init();
    expected_crc = crc32_update(expected_crc, test_data, total_size);
    expected_crc = crc32_finalize(expected_crc);

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    // Subscribe to required parameters
    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                   (USM0DD | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   NULL, 0,
                                   getConnectorId(),
                                   portMAX_DELAY);

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                   (USM0TBS | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   NULL, 0,
                                   getConnectorId(),
                                   portMAX_DELAY);

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                   (USM0STAT | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   NULL, 0,
                                   getConnectorId(),
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(10));
    clearSentDDMP2Frames();

    // Step 1: Publish USM0DD to start OTA process
    us_dd_download_done_t dd;
    memset(&dd, 0, sizeof(dd));
    dd.size = total_size;
    dd.crc = expected_crc;
    dd.service = 0x01;

    const char *fwid = FIRMWARE_BUILD_ID;
    size_t fwid_len = strlen(fwid);
    size_t dd_size = offsetof(us_dd_download_done_t, fwid) + fwid_len;

    uint8_t dd_buffer[256];
    memcpy(dd_buffer, &dd, offsetof(us_dd_download_done_t, fwid));
    memcpy(dd_buffer + offsetof(us_dd_download_done_t, fwid), fwid, fwid_len);

    mock_esp_ota_set_next_partition_valid(true);
    mock_esp_ota_set_begin_fail(false);

    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                   (USM0DD | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   dd_buffer, dd_size,
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(50));
    clearSentDDMP2Frames();

    // Step 2: Publish USM0TBS to move to in_progress state
    int32_t transfer_block_size = block_size;
    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                   (USM0TBS | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   &transfer_block_size, sizeof(transfer_block_size),
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(50));
    clearSentDDMP2Frames();

    // Reset mock to track OTA abort
    mock_esp_ota_reset_abort_count();

    // Step 3: Send USMSTAT with OK status (0) - should NOT cause state change
    int32_t status_ok = 0;  // Zero indicates OK
    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                   (USM0STAT | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   &status_ok, sizeof(status_ok),
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(50));

    // Verify OTA was NOT aborted
    EXPECT_EQ(mock_esp_ota_get_abort_count(), 0) << "OTA should NOT have been aborted for OK status";

    // Step 4: Verify we're still in progress state by sending data
    uint8_t *data_to_send = &test_data[0];
    connector_send_frame_to_broker(DDMP2_CONTROL_SET,
                                   (US0DATA | DDM2_PARAMETER_INSTANCE(us_instance)),
                                   data_to_send, block_size,
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(100));

    // Should receive US0DATA ACK if we're still in progress state
    bool found_data_ack = false;
    int num_frames = getNumSentDDMP2Frames();
    for (int i = 0; i < num_frames; i++)
    {
        int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
        if ((res == 0) && (myFrame.frame.control == DDMP2_CONTROL_PUBLISH) &&
            (DDM2_PARAMETER_BASE_INSTANCE(myFrame.frame.publish.parameter) == US0DATA))
        {
            found_data_ack = true;
            us_data_read_ack_t *ack = (us_data_read_ack_t *)myFrame.frame.publish.value.raw;
            EXPECT_EQ(ack->result, US_ACK_RESULT_OK) << "Should still be processing data in progress state";
            break;
        }
    }
    EXPECT_TRUE(found_data_ack) << "Should have received US0DATA ACK - still in progress state";

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
}

/*
 * Test: OTA write failure during data transfer causes abort and transition to idle
 */
TEST_F(ConnectorUsDicmTestFixture, OTAWriteFailureCausesAbort)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;
    int usm_instance = 0;
    int us_instance = 0;

    // Test data for OTA transfer
    const size_t block_size = 64;
    const size_t total_size = block_size * 2;
    uint8_t test_data[total_size];
    for (size_t i = 0; i < total_size; i++)
    {
        test_data[i] = i & 0xFF;
    }

    crc32_t expected_crc = crc32_init();
    expected_crc = crc32_update(expected_crc, test_data, total_size);
    expected_crc = crc32_finalize(expected_crc);

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    // Subscribe to required parameters
    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                   (USM0DD | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   NULL, 0,
                                   getConnectorId(),
                                   portMAX_DELAY);

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                   (USM0TBS | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   NULL, 0,
                                   getConnectorId(),
                                   portMAX_DELAY);

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                   (USM0STAT | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   NULL, 0,
                                   getConnectorId(),
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(10));
    clearSentDDMP2Frames();

    // Step 1: Publish USM0DD to start OTA process
    us_dd_download_done_t dd;
    memset(&dd, 0, sizeof(dd));
    dd.size = total_size;
    dd.crc = expected_crc;
    dd.service = 0x01;

    const char *fwid = FIRMWARE_BUILD_ID;
    size_t fwid_len = strlen(fwid);
    size_t dd_size = offsetof(us_dd_download_done_t, fwid) + fwid_len;

    uint8_t dd_buffer[256];
    memcpy(dd_buffer, &dd, offsetof(us_dd_download_done_t, fwid));
    memcpy(dd_buffer + offsetof(us_dd_download_done_t, fwid), fwid, fwid_len);

    mock_esp_ota_set_next_partition_valid(true);
    mock_esp_ota_set_begin_fail(false);

    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                   (USM0DD | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   dd_buffer, dd_size,
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(50));

    // Verify US0RRQ was published
    bool found_rrq = false;
    int num_frames = getNumSentDDMP2Frames();
    for (int i = 0; i < num_frames; i++)
    {
        int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
        if ((res == 0) && (DDM2_PARAMETER_BASE_INSTANCE(myFrame.frame.publish.parameter) == US0RRQ))
        {
            found_rrq = true;
            break;
        }
    }
    EXPECT_TRUE(found_rrq) << "Should have sent US0RRQ after USM0DD";

    clearSentDDMP2Frames();

    // Step 2: Publish USM0TBS to move to in_progress state
    int32_t transfer_block_size = block_size;
    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                   (USM0TBS | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   &transfer_block_size, sizeof(transfer_block_size),
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(50));
    clearSentDDMP2Frames();

    // Step 3: Configure mock to fail OTA write
    mock_esp_ota_set_write_fail(true);
    mock_esp_ota_reset_abort_count();

    // Step 4: Send first data block - this should trigger the write failure
    uint8_t *data_to_send = &test_data[0];
    connector_send_frame_to_broker(DDMP2_CONTROL_SET,
                                   (US0DATA | DDM2_PARAMETER_INSTANCE(us_instance)),
                                   data_to_send, block_size,
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(100));

    // Verify US0DATA ACK was received with ABORT result
    bool found_abort_ack = false;
    num_frames = getNumSentDDMP2Frames();
    for (int i = 0; i < num_frames; i++)
    {
        int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
        if ((res == 0) && (myFrame.frame.control == DDMP2_CONTROL_PUBLISH) &&
            (DDM2_PARAMETER_BASE_INSTANCE(myFrame.frame.publish.parameter) == US0DATA))
        {
            found_abort_ack = true;
            us_data_read_ack_t *ack = (us_data_read_ack_t *)myFrame.frame.publish.value.raw;
            EXPECT_EQ(ack->result, US_ACK_RESULT_ABORT) << "Should ACK with ABORT on write failure";
            EXPECT_EQ(ack->crc, 0x11223344u) << "CRC should be set to error value";
            break;
        }
    }
    EXPECT_TRUE(found_abort_ack) << "Should have received US0DATA ACK with ABORT";

    // Verify OTA was aborted
    EXPECT_EQ(mock_esp_ota_get_abort_count(), 1) << "OTA should have been aborted";

    clearSentDDMP2Frames();

    // Step 5: Verify state machine went back to idle by sending another USM0DD
    // Reset mock to allow successful OTA start
    mock_esp_ota_set_write_fail(false);

    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                   (USM0DD | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   dd_buffer, dd_size,
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(50));

    // Should receive US0RRQ again if we're back in idle state
    found_rrq = false;
    num_frames = getNumSentDDMP2Frames();
    for (int i = 0; i < num_frames; i++)
    {
        int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
        if ((res == 0) && (DDM2_PARAMETER_BASE_INSTANCE(myFrame.frame.publish.parameter) == US0RRQ))
        {
            found_rrq = true;
            break;
        }
    }
    EXPECT_TRUE(found_rrq) << "Should have sent US0RRQ again after returning to idle state";

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
}

/*
 * Test: OTA write failure on second block causes abort
 */
TEST_F(ConnectorUsDicmTestFixture, OTAWriteFailureOnSecondBlockCausesAbort)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;
    int usm_instance = 0;
    int us_instance = 0;

    // Test data for OTA transfer (3 blocks to test failure on second)
    const size_t block_size = 64;
    const size_t total_size = block_size * 3;
    uint8_t test_data[total_size];
    for (size_t i = 0; i < total_size; i++)
    {
        test_data[i] = i & 0xFF;
    }

    crc32_t expected_crc = crc32_init();
    expected_crc = crc32_update(expected_crc, test_data, total_size);
    expected_crc = crc32_finalize(expected_crc);

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    // Subscribe to required parameters
    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                   (USM0DD | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   NULL, 0,
                                   getConnectorId(),
                                   portMAX_DELAY);

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                   (USM0TBS | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   NULL, 0,
                                   getConnectorId(),
                                   portMAX_DELAY);

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                   (USM0STAT | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   NULL, 0,
                                   getConnectorId(),
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(10));
    clearSentDDMP2Frames();

    // Step 1: Publish USM0DD to start OTA process
    us_dd_download_done_t dd;
    memset(&dd, 0, sizeof(dd));
    dd.size = total_size;
    dd.crc = expected_crc;
    dd.service = 0x01;

    const char *fwid = FIRMWARE_BUILD_ID;
    size_t fwid_len = strlen(fwid);
    size_t dd_size = offsetof(us_dd_download_done_t, fwid) + fwid_len;

    uint8_t dd_buffer[256];
    memcpy(dd_buffer, &dd, offsetof(us_dd_download_done_t, fwid));
    memcpy(dd_buffer + offsetof(us_dd_download_done_t, fwid), fwid, fwid_len);

    mock_esp_ota_set_next_partition_valid(true);
    mock_esp_ota_set_begin_fail(false);
    mock_esp_ota_set_write_fail(false);  // First block should succeed

    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                   (USM0DD | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   dd_buffer, dd_size,
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(50));
    clearSentDDMP2Frames();

    // Step 2: Publish USM0TBS to move to in_progress state
    int32_t transfer_block_size = block_size;
    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                   (USM0TBS | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   &transfer_block_size, sizeof(transfer_block_size),
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(50));
    clearSentDDMP2Frames();

    // Step 3: Send first data block - this should succeed
    uint8_t *data_to_send = &test_data[0];
    connector_send_frame_to_broker(DDMP2_CONTROL_SET,
                                   (US0DATA | DDM2_PARAMETER_INSTANCE(us_instance)),
                                   data_to_send, block_size,
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(100));

    // Verify first block ACK OK
    bool found_ok_ack = false;
    int num_frames = getNumSentDDMP2Frames();
    for (int i = 0; i < num_frames; i++)
    {
        int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
        if ((res == 0) && (myFrame.frame.control == DDMP2_CONTROL_PUBLISH) &&
            (DDM2_PARAMETER_BASE_INSTANCE(myFrame.frame.publish.parameter) == US0DATA))
        {
            found_ok_ack = true;
            us_data_read_ack_t *ack = (us_data_read_ack_t *)myFrame.frame.publish.value.raw;
            EXPECT_EQ(ack->result, US_ACK_RESULT_OK) << "First block should ACK OK";
            break;
        }
    }
    EXPECT_TRUE(found_ok_ack) << "Should have received US0DATA ACK OK for first block";

    clearSentDDMP2Frames();

    // Send US0BTR for first block
    int32_t btr_result = US_ACK_RESULT_OK;
    connector_send_frame_to_broker(DDMP2_CONTROL_SET,
                                   (US0BTR | DDM2_PARAMETER_INSTANCE(us_instance)),
                                   &btr_result, sizeof(btr_result),
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(50));
    clearSentDDMP2Frames();

    // Step 4: Configure mock to fail OTA write for second block
    mock_esp_ota_set_write_fail(true);
    mock_esp_ota_reset_abort_count();

    // Step 5: Send second data block - this should fail
    data_to_send = &test_data[block_size];
    connector_send_frame_to_broker(DDMP2_CONTROL_SET,
                                   (US0DATA | DDM2_PARAMETER_INSTANCE(us_instance)),
                                   data_to_send, block_size,
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(100));

    // Verify US0DATA ACK was received with ABORT result
    bool found_abort_ack = false;
    num_frames = getNumSentDDMP2Frames();
    for (int i = 0; i < num_frames; i++)
    {
        int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
        if ((res == 0) && (myFrame.frame.control == DDMP2_CONTROL_PUBLISH) &&
            (DDM2_PARAMETER_BASE_INSTANCE(myFrame.frame.publish.parameter) == US0DATA))
        {
            found_abort_ack = true;
            us_data_read_ack_t *ack = (us_data_read_ack_t *)myFrame.frame.publish.value.raw;
            EXPECT_EQ(ack->result, US_ACK_RESULT_ABORT) << "Should ACK with ABORT on write failure";
            break;
        }
    }
    EXPECT_TRUE(found_abort_ack) << "Should have received US0DATA ACK with ABORT";

    // Verify OTA was aborted
    EXPECT_EQ(mock_esp_ota_get_abort_count(), 1) << "OTA should have been aborted";

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
}

/*
 * Test: US0BTR failure in in_progress state causes abort and BTR publish with abort result
 */
TEST_F(ConnectorUsDicmTestFixture, US0BTRFailureInProgressState)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;
    int usm_instance = 0;
    int us_instance = 0;

    // Test data for OTA transfer
    const size_t block_size = 64;
    const size_t total_size = block_size * 2;
    uint8_t test_data[total_size];
    for (size_t i = 0; i < total_size; i++)
    {
        test_data[i] = (uint8_t)(i & 0xFF);
    }

    // Calculate expected CRC
    crc32_t expected_crc = crc32_init();
    expected_crc = crc32_update(expected_crc, test_data, total_size);
    expected_crc = crc32_finalize(expected_crc);

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    // Subscribe to USM parameters
    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                   (USM0DD | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   NULL, 0,
                                   getConnectorId(),
                                   portMAX_DELAY);

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                   (USM0TBS | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   NULL, 0,
                                   getConnectorId(),
                                   portMAX_DELAY);

    clearSentDDMP2Frames();

    // Step 1: Publish USM0DD (download done)
    us_dd_download_done_t dd;
    memset(&dd, 0, sizeof(dd));
    dd.size = total_size;
    dd.crc = expected_crc;
    dd.service = 0x01;

    const char *fwid = FIRMWARE_BUILD_ID;
    size_t fwid_len = strlen(fwid);
    size_t dd_size = offsetof(us_dd_download_done_t, fwid) + fwid_len;

    uint8_t dd_buffer[256];
    memcpy(dd_buffer, &dd, offsetof(us_dd_download_done_t, fwid));
    memcpy(dd_buffer + offsetof(us_dd_download_done_t, fwid), fwid, fwid_len);

    mock_esp_ota_set_next_partition_valid(true);
    mock_esp_ota_set_begin_fail(false);

    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                   (USM0DD | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   dd_buffer, dd_size,
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(50));
    clearSentDDMP2Frames();

    // Step 2: Publish USM0TBS to transition to in_progress state
    int32_t transfer_block_size = block_size;
    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                   (USM0TBS | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   &transfer_block_size, sizeof(transfer_block_size),
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(50));
    clearSentDDMP2Frames();

    // Step 3: Send US0BTR with failure result (not US_ACK_RESULT_OK)
    int32_t btr_fail_result = US_ACK_RESULT_ABORT;
    connector_send_frame_to_broker(DDMP2_CONTROL_SET,
                                   (US0BTR | DDM2_PARAMETER_INSTANCE(us_instance)),
                                   &btr_fail_result, sizeof(btr_fail_result),
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(50));

    // Verify US0BTR was published with ABORT result
    bool found_btr_abort = false;
    int num_frames = getNumSentDDMP2Frames();
    for (int i = 0; i < num_frames; i++)
    {
        int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
        if ((res == 0) && (myFrame.frame.control == DDMP2_CONTROL_PUBLISH) &&
            (DDM2_PARAMETER_BASE_INSTANCE(myFrame.frame.publish.parameter) == US0BTR))
        {
            int32_t *result = (int32_t *)myFrame.frame.publish.value.raw;
            if (*result == US_ACK_RESULT_ABORT)
            {
                found_btr_abort = true;
                break;
            }
        }
    }
    EXPECT_TRUE(found_btr_abort) << "Should have published US0BTR with ABORT result on BTR failure";

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
}

/*
 * Test: US0BTR failure in wait_complete state causes abort and transition to idle
 */
TEST_F(ConnectorUsDicmTestFixture, US0BTRFailureInWaitCompleteState)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;
    int usm_instance = 0;
    int us_instance = 0;

    // Test data for OTA transfer (single block to complete transfer)
    const size_t block_size = 64;
    const size_t total_size = block_size;
    uint8_t test_data[total_size];
    for (size_t i = 0; i < total_size; i++)
    {
        test_data[i] = (uint8_t)(i & 0xFF);
    }

    // Calculate expected CRC
    crc32_t expected_crc = crc32_init();
    expected_crc = crc32_update(expected_crc, test_data, total_size);
    expected_crc = crc32_finalize(expected_crc);

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    // Subscribe to USM parameters
    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                   (USM0DD | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   NULL, 0,
                                   getConnectorId(),
                                   portMAX_DELAY);

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                   (USM0TBS | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   NULL, 0,
                                   getConnectorId(),
                                   portMAX_DELAY);

    clearSentDDMP2Frames();

    // Step 1: Publish USM0DD
    us_dd_download_done_t dd;
    memset(&dd, 0, sizeof(dd));
    dd.size = total_size;
    dd.crc = expected_crc;
    dd.service = 0x01;

    const char *fwid = FIRMWARE_BUILD_ID;
    size_t fwid_len = strlen(fwid);
    size_t dd_size = offsetof(us_dd_download_done_t, fwid) + fwid_len;

    uint8_t dd_buffer[256];
    memcpy(dd_buffer, &dd, offsetof(us_dd_download_done_t, fwid));
    memcpy(dd_buffer + offsetof(us_dd_download_done_t, fwid), fwid, fwid_len);

    mock_esp_ota_set_next_partition_valid(true);
    mock_esp_ota_set_begin_fail(false);

    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                   (USM0DD | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   dd_buffer, dd_size,
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(50));
    clearSentDDMP2Frames();

    // Step 2: Publish USM0TBS
    int32_t transfer_block_size = block_size;
    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                   (USM0TBS | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   &transfer_block_size, sizeof(transfer_block_size),
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(50));
    clearSentDDMP2Frames();

    // Step 3: Send complete data block to transition to wait_complete state
    us_data_frame_t *data_frame = (us_data_frame_t *)hal_mem_malloc_prefer(sizeof(us_data_frame_t) + total_size, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    memcpy(data_frame->data, test_data, total_size);

    connector_send_frame_to_broker(DDMP2_CONTROL_SET,
                                   (US0DATA | DDM2_PARAMETER_INSTANCE(us_instance)),
                                   data_frame->data, total_size,
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    hal_mem_free(data_frame);
    // vTaskDelay(pdMS_TO_TICKS(50));
    clearSentDDMP2Frames();

    // Step 4: Send US0BTR with failure result in wait_complete state
    int32_t btr_fail_result = US_ACK_RESULT_ABORT;
    connector_send_frame_to_broker(DDMP2_CONTROL_SET,
                                   (US0BTR | DDM2_PARAMETER_INSTANCE(us_instance)),
                                   &btr_fail_result, sizeof(btr_fail_result),
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(50));

    // Verify US0BTR was published with ABORT result
    bool found_btr_abort = false;
    int num_frames = getNumSentDDMP2Frames();
    for (int i = 0; i < num_frames; i++)
    {
        int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
        if ((res == 0) && (myFrame.frame.control == DDMP2_CONTROL_PUBLISH) &&
            (DDM2_PARAMETER_BASE_INSTANCE(myFrame.frame.publish.parameter) == US0BTR))
        {
            int32_t *result = (int32_t *)myFrame.frame.publish.value.raw;
            if (*result == US_ACK_RESULT_ABORT)
            {
                found_btr_abort = true;
                break;
            }
        }
    }
    EXPECT_TRUE(found_btr_abort) << "Should have published US0BTR with ABORT result on BTR failure in wait_complete";

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
}

/*
 * Test: OTA end failure (esp_ota_end returns != ESP_OK) in flash_complete state
 *       causes US0DATA publish with ABORT result
 */
TEST_F(ConnectorUsDicmTestFixture, OTAEndFailureInFlashCompleteState)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;
    int usm_instance = 0;
    int us_instance = 0;

    // Test data for OTA transfer (single block)
    const size_t block_size = 64;
    const size_t total_size = block_size;
    uint8_t test_data[total_size];
    for (size_t i = 0; i < total_size; i++)
    {
        test_data[i] = (uint8_t)(i & 0xFF);
    }

    // Calculate expected CRC
    crc32_t expected_crc = crc32_init();
    expected_crc = crc32_update(expected_crc, test_data, total_size);
    expected_crc = crc32_finalize(expected_crc);

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    // Subscribe to USM parameters
    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                   (USM0DD | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   NULL, 0,
                                   getConnectorId(),
                                   portMAX_DELAY);

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                   (USM0TBS | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   NULL, 0,
                                   getConnectorId(),
                                   portMAX_DELAY);

    clearSentDDMP2Frames();

    // Step 1: Publish USM0DD
    us_dd_download_done_t dd;
    memset(&dd, 0, sizeof(dd));
    dd.size = total_size;
    dd.crc = expected_crc;
    dd.service = 0x01;

    const char *fwid = FIRMWARE_BUILD_ID;
    size_t fwid_len = strlen(fwid);
    size_t dd_size = offsetof(us_dd_download_done_t, fwid) + fwid_len;

    uint8_t dd_buffer[256];
    memcpy(dd_buffer, &dd, offsetof(us_dd_download_done_t, fwid));
    memcpy(dd_buffer + offsetof(us_dd_download_done_t, fwid), fwid, fwid_len);

    mock_esp_ota_set_next_partition_valid(true);
    mock_esp_ota_set_begin_fail(false);

    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                   (USM0DD | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   dd_buffer, dd_size,
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(50));
    clearSentDDMP2Frames();

    // Step 2: Publish USM0TBS
    int32_t transfer_block_size = block_size;
    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                   (USM0TBS | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   &transfer_block_size, sizeof(transfer_block_size),
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(50));
    clearSentDDMP2Frames();

    // Step 3: Send complete data block
    connector_send_frame_to_broker(DDMP2_CONTROL_SET,
                                   (US0DATA | DDM2_PARAMETER_INSTANCE(us_instance)),
                                   test_data, total_size,
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(100));
    clearSentDDMP2Frames();

    // Set mock to fail esp_ota_end
    mock_esp_ota_set_end_fail(true);
    mock_esp_ota_reset_abort_count();

    // Step 4: Send US0BTR with OK result to trigger transition to flash_complete
    int32_t btr_ok_result = US_ACK_RESULT_OK;
    connector_send_frame_to_broker(DDMP2_CONTROL_SET,
                                   (US0BTR | DDM2_PARAMETER_INSTANCE(us_instance)),
                                   &btr_ok_result, sizeof(btr_ok_result),
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(100));

    // Verify US0DATA was published with ABORT result due to OTA end failure
    bool found_data_abort = false;
    int num_frames = getNumSentDDMP2Frames();
    for (int i = 0; i < num_frames; i++)
    {
        int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
        if ((res == 0) && (myFrame.frame.control == DDMP2_CONTROL_PUBLISH) &&
            (DDM2_PARAMETER_BASE_INSTANCE(myFrame.frame.publish.parameter) == US0DATA))
        {
            us_data_read_ack_t *ack = (us_data_read_ack_t *)myFrame.frame.publish.value.raw;
            if (ack->result == US_ACK_RESULT_ABORT)
            {
                found_data_abort = true;
                break;
            }
        }
    }
    EXPECT_TRUE(found_data_abort) << "Should have published US0DATA with ABORT result when esp_ota_end fails";

    // Verify OTA was aborted after esp_ota_end failure
    EXPECT_EQ(mock_esp_ota_get_abort_count(), 1) << "OTA should have been aborted after esp_ota_end failure";

    // Reset mock
    mock_esp_ota_set_end_fail(false);

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
}

/*
 * Test: OTA set boot partition failure (esp_ota_end returns ESP_OK but
 *       esp_ota_set_boot_partition returns != ESP_OK) in flash_complete state
 */
TEST_F(ConnectorUsDicmTestFixture, OTASetBootPartitionFailureInFlashCompleteState)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;
    int usm_instance = 0;
    int us_instance = 0;

    // Test data for OTA transfer (single block)
    const size_t block_size = 64;
    const size_t total_size = block_size;
    uint8_t test_data[total_size];
    for (size_t i = 0; i < total_size; i++)
    {
        test_data[i] = (uint8_t)(i & 0xFF);
    }

    // Calculate expected CRC
    crc32_t expected_crc = crc32_init();
    expected_crc = crc32_update(expected_crc, test_data, total_size);
    expected_crc = crc32_finalize(expected_crc);

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    // Subscribe to USM parameters
    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                   (USM0DD | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   NULL, 0,
                                   getConnectorId(),
                                   portMAX_DELAY);

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                   (USM0TBS | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   NULL, 0,
                                   getConnectorId(),
                                   portMAX_DELAY);

    clearSentDDMP2Frames();

    // Step 1: Publish USM0DD
    us_dd_download_done_t dd;
    memset(&dd, 0, sizeof(dd));
    dd.size = total_size;
    dd.crc = expected_crc;
    dd.service = 0x01;

    const char *fwid = FIRMWARE_BUILD_ID;
    size_t fwid_len = strlen(fwid);
    size_t dd_size = offsetof(us_dd_download_done_t, fwid) + fwid_len;

    uint8_t dd_buffer[256];
    memcpy(dd_buffer, &dd, offsetof(us_dd_download_done_t, fwid));
    memcpy(dd_buffer + offsetof(us_dd_download_done_t, fwid), fwid, fwid_len);

    mock_esp_ota_set_next_partition_valid(true);
    mock_esp_ota_set_begin_fail(false);

    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                   (USM0DD | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   dd_buffer, dd_size,
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(50));
    clearSentDDMP2Frames();

    // Step 2: Publish USM0TBS
    int32_t transfer_block_size = block_size;
    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                   (USM0TBS | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   &transfer_block_size, sizeof(transfer_block_size),
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(50));
    clearSentDDMP2Frames();

    // Step 3: Send complete data block
    connector_send_frame_to_broker(DDMP2_CONTROL_SET,
                                   (US0DATA | DDM2_PARAMETER_INSTANCE(us_instance)),
                                   test_data, total_size,
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(100));
    clearSentDDMP2Frames();

    // Set mock: esp_ota_end succeeds but esp_ota_set_boot_partition fails
    mock_esp_ota_set_end_fail(false);
    mock_esp_ota_set_boot_partition_fail(true);
    mock_esp_ota_reset_abort_count();

    // Step 4: Send US0BTR with OK result to trigger transition to flash_complete
    int32_t btr_ok_result = US_ACK_RESULT_OK;
    connector_send_frame_to_broker(DDMP2_CONTROL_SET,
                                   (US0BTR | DDM2_PARAMETER_INSTANCE(us_instance)),
                                   &btr_ok_result, sizeof(btr_ok_result),
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(100));

    bool found_data_response = false;
    int num_frames = getNumSentDDMP2Frames();
    for (int i = 0; i < num_frames; i++)
    {
        int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
        if ((res == 0) && (myFrame.frame.control == DDMP2_CONTROL_PUBLISH) &&
            (DDM2_PARAMETER_BASE_INSTANCE(myFrame.frame.publish.parameter) == US0DATA))
        {
            found_data_response = true;
            break;
        }
    }
    EXPECT_TRUE(found_data_response) << "Should have published US0DATA response";

    // Reset mock
    mock_esp_ota_set_boot_partition_fail(false);

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
}

/*
 * Test: Verify state returns to idle after OTA end failure
 */
TEST_F(ConnectorUsDicmTestFixture, OTAEndFailureReturnsToIdleState)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;
    int usm_instance = 0;
    int us_instance = 0;

    // Test data for OTA transfer (single block)
    const size_t block_size = 64;
    const size_t total_size = block_size;
    uint8_t test_data[total_size];
    for (size_t i = 0; i < total_size; i++)
    {
        test_data[i] = (uint8_t)(i & 0xFF);
    }

    // Calculate expected CRC
    crc32_t expected_crc = crc32_init();
    expected_crc = crc32_update(expected_crc, test_data, total_size);
    expected_crc = crc32_finalize(expected_crc);

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    // Subscribe to USM parameters
    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                   (USM0DD | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   NULL, 0,
                                   getConnectorId(),
                                   portMAX_DELAY);

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                   (USM0TBS | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   NULL, 0,
                                   getConnectorId(),
                                   portMAX_DELAY);

    clearSentDDMP2Frames();

    // Step 1: Publish USM0DD
    us_dd_download_done_t dd;
    memset(&dd, 0, sizeof(dd));
    dd.size = total_size;
    dd.crc = expected_crc;
    dd.service = 0x01;

    const char *fwid = FIRMWARE_BUILD_ID;
    size_t fwid_len = strlen(fwid);
    size_t dd_size = offsetof(us_dd_download_done_t, fwid) + fwid_len;

    uint8_t dd_buffer[256];
    memcpy(dd_buffer, &dd, offsetof(us_dd_download_done_t, fwid));
    memcpy(dd_buffer + offsetof(us_dd_download_done_t, fwid), fwid, fwid_len);

    mock_esp_ota_set_next_partition_valid(true);
    mock_esp_ota_set_begin_fail(false);

    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                   (USM0DD | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   dd_buffer, dd_size,
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(50));
    clearSentDDMP2Frames();

    // Step 2: Publish USM0TBS
    int32_t transfer_block_size = block_size;
    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                   (USM0TBS | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   &transfer_block_size, sizeof(transfer_block_size),
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(50));
    clearSentDDMP2Frames();

    // Step 3: Send complete data block
    connector_send_frame_to_broker(DDMP2_CONTROL_SET,
                                   (US0DATA | DDM2_PARAMETER_INSTANCE(us_instance)),
                                   test_data, total_size,
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(100));
    clearSentDDMP2Frames();

    // Set mock to fail esp_ota_end
    mock_esp_ota_set_end_fail(true);

    // Step 4: Send US0BTR with OK result to trigger transition to flash_complete
    int32_t btr_ok_result = US_ACK_RESULT_OK;
    connector_send_frame_to_broker(DDMP2_CONTROL_SET,
                                   (US0BTR | DDM2_PARAMETER_INSTANCE(us_instance)),
                                   &btr_ok_result, sizeof(btr_ok_result),
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(100));
    clearSentDDMP2Frames();

    // Reset mock for next OTA attempt
    mock_esp_ota_set_end_fail(false);

    // Step 5: Verify state machine returned to idle by starting new OTA
    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                   (USM0DD | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   dd_buffer, dd_size,
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(50));

    // Should receive US0RRQ if we're back in idle state
    bool found_rrq = false;
    int num_frames = getNumSentDDMP2Frames();
    for (int i = 0; i < num_frames; i++)
    {
        int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
        if ((res == 0) && (DDM2_PARAMETER_BASE_INSTANCE(myFrame.frame.publish.parameter) == US0RRQ))
        {
            found_rrq = true;
            break;
        }
    }
    EXPECT_TRUE(found_rrq) << "Should have sent US0RRQ - state machine should be back in idle";

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
}

/*
 * Test: US0LIST includes FS firmware ID when CONNECTOR_US_FS is enabled
 */
TEST_F(ConnectorUsDicmTestFixture, US0LISTIncludesFSFirmwareId)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;
    int us_instance = 0;

    vPortPauseScheduler();
    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                   (US0LIST | DDM2_PARAMETER_INSTANCE(us_instance)),
                                   NULL,
                                   0,
                                   connector_unittest.connector_id,
                                   (TickType_t)portMAX_DELAY);

    // Should receive a PUBLISH with the firmware ID list
    EXPECT_GE(getNumSentDDMP2Frames(), 1) << "Should have sent at least one DDMP2 frame";

    int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
    EXPECT_FALSE(res) << "Should have a frame available";
    EXPECT_EQ(myFrame.frame.control, DDMP2_CONTROL_PUBLISH) << "Should have sent a PUBLISH";
    EXPECT_EQ(DDM2_PARAMETER_BASE_INSTANCE(myFrame.frame.publish.parameter), US0LIST)
        << "Should be US0LIST parameter";

    // Check that the list contains the FS firmware ID
    char *list = (char *)myFrame.frame.publish.value.raw;
    char expected_fs_id[128];
    snprintf(expected_fs_id, sizeof(expected_fs_id), "%s-FS", FIRMWARE_BUILD_ID);

    EXPECT_NE(strstr(list, expected_fs_id), nullptr)
        << "US0LIST should contain FS firmware ID: " << expected_fs_id;

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
}

/*
 * Test: Successful OTA transfer for FS partition type
 */
TEST_F(ConnectorUsDicmTestFixture, SuccessfulOTATransferFSPartition)
{
    DDMP2_FRAME myFrame;
    size_t frame_size = 0;
    int usm_instance = 0;
    int us_instance = 0;

    // Test data for OTA transfer (2 blocks)
    const size_t block_size = 64;
    const size_t total_size = block_size * 2;
    uint8_t test_data[total_size];
    for (size_t i = 0; i < total_size; i++)
    {
        test_data[i] = (uint8_t)(i & 0xFF);
    }

    // Calculate expected CRC
    crc32_t expected_crc = crc32_init();
    expected_crc = crc32_update(expected_crc, test_data, total_size);
    expected_crc = crc32_finalize(expected_crc);

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();

    // Subscribe to USM parameters
    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                   (USM0DD | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   NULL, 0,
                                   getConnectorId(),
                                   portMAX_DELAY);

    connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE,
                                   (USM0TBS | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   NULL, 0,
                                   getConnectorId(),
                                   portMAX_DELAY);

    clearSentDDMP2Frames();

    // Step 1: Publish USM0DD (download done) with FS firmware ID
    us_dd_download_done_t dd;
    memset(&dd, 0, sizeof(dd));
    dd.size = total_size;
    dd.crc = expected_crc;
    dd.service = 0x01;

    // Use FS firmware ID: FIRMWARE_BUILD_ID + "-FS"
    char fs_fwid[128];
    snprintf(fs_fwid, sizeof(fs_fwid), "%s-FS", FIRMWARE_BUILD_ID);
    size_t fwid_len = strlen(fs_fwid);
    size_t dd_size = offsetof(us_dd_download_done_t, fwid) + fwid_len;

    uint8_t dd_buffer[256];
    memcpy(dd_buffer, &dd, offsetof(us_dd_download_done_t, fwid));
    memcpy(dd_buffer + offsetof(us_dd_download_done_t, fwid), fs_fwid, fwid_len);

    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                   (USM0DD | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   dd_buffer, dd_size,
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(50));

    // Verify US0RRQ was published
    bool found_rrq = false;
    int num_frames = getNumSentDDMP2Frames();
    for (int i = 0; i < num_frames; i++)
    {
        int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
        if ((res == 0) && (DDM2_PARAMETER_BASE_INSTANCE(myFrame.frame.publish.parameter) == US0RRQ))
        {
            found_rrq = true;
            break;
        }
    }
    EXPECT_TRUE(found_rrq) << "Should have sent US0RRQ after USM0DD for FS";

    clearSentDDMP2Frames();

    // Step 2: Publish USM0TBS (transfer block size)
    int32_t transfer_block_size = block_size;
    connector_send_frame_to_broker(DDMP2_CONTROL_PUBLISH,
                                   (USM0TBS | DDM2_PARAMETER_INSTANCE(usm_instance)),
                                   &transfer_block_size, sizeof(transfer_block_size),
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(50));
    clearSentDDMP2Frames();

    // Step 3: Send first data block
    connector_send_frame_to_broker(DDMP2_CONTROL_SET,
                                   (US0DATA | DDM2_PARAMETER_INSTANCE(us_instance)),
                                   &test_data[0], block_size,
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(100));

    // Verify US0DATA ACK for first block
    bool found_data_ack = false;
    num_frames = getNumSentDDMP2Frames();
    for (int i = 0; i < num_frames; i++)
    {
        int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
        if ((res == 0) && (myFrame.frame.control == DDMP2_CONTROL_PUBLISH) &&
            (DDM2_PARAMETER_BASE_INSTANCE(myFrame.frame.publish.parameter) == US0DATA))
        {
            found_data_ack = true;
            us_data_read_ack_t *ack = (us_data_read_ack_t *)myFrame.frame.publish.value.raw;
            EXPECT_EQ(ack->result, US_ACK_RESULT_OK) << "First block should ACK OK";
            break;
        }
    }
    EXPECT_TRUE(found_data_ack) << "Should have received US0DATA ACK for first block";

    clearSentDDMP2Frames();

    // Send US0BTR OK for first block
    int32_t btr_result = US_ACK_RESULT_OK;
    connector_send_frame_to_broker(DDMP2_CONTROL_SET,
                                   (US0BTR | DDM2_PARAMETER_INSTANCE(us_instance)),
                                   &btr_result, sizeof(btr_result),
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(50));
    clearSentDDMP2Frames();

    // Step 4: Send second (final) data block
    connector_send_frame_to_broker(DDMP2_CONTROL_SET,
                                   (US0DATA | DDM2_PARAMETER_INSTANCE(us_instance)),
                                   &test_data[block_size], block_size,
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(100));
    clearSentDDMP2Frames();

    // Step 5: Send final US0BTR OK to trigger completion
    connector_send_frame_to_broker(DDMP2_CONTROL_SET,
                                   (US0BTR | DDM2_PARAMETER_INSTANCE(us_instance)),
                                   &btr_result, sizeof(btr_result),
                                   connector_unittest.connector_id,
                                   portMAX_DELAY);

    // vTaskDelay(pdMS_TO_TICKS(100));

    // Verify final US0DATA publish with OK_RESTART
    bool found_final_data = false;
    num_frames = getNumSentDDMP2Frames();
    for (int i = 0; i < num_frames; i++)
    {
        int res = getNextSentDDMP2Frame(&myFrame, &frame_size);
        if ((res == 0) && (myFrame.frame.control == DDMP2_CONTROL_PUBLISH) &&
            (DDM2_PARAMETER_BASE_INSTANCE(myFrame.frame.publish.parameter) == US0DATA))
        {
            us_data_read_ack_t *ack = (us_data_read_ack_t *)myFrame.frame.publish.value.raw;
            if (ack->result == US_ACK_RESULT_OK_RESTART)
            {
                found_final_data = true;
                EXPECT_EQ(ack->crc, expected_crc) << "Final CRC should match expected";
                break;
            }
        }
    }
    EXPECT_TRUE(found_final_data) << "Should have received final US0DATA with OK_RESTART";

    clearSentDDMP2Frames();
    clearReceivedDDMP2Frames();
}
