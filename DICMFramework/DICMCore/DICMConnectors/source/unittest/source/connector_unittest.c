/*! \file connector_unittest.c
 *
 * The UnitTest connector is a connector wrapper that can be used for testing execution of functions within a connector
 *
 */

#include "connector_unittest.h"
#include "broker.h"
#include "configuration.h"
#include "connector.h"

/**********************************************************
 * Private declarations
 *********************************************************/
#ifndef CONNECTOR_UNITTEST_MAX_CONNECTIONS
#define CONNECTOR_UNITTEST_MAX_CONNECTIONS 4
#endif

static int connector_unittest_init(void);
static void connector_unittest_process_task(void *param);
static connector_unittest_task_handler_t l_task_handler[CONNECTOR_UNITTEST_MAX_CONNECTIONS + 1] = {NULL};
static connector_unittest_init_task_cb_t l_init_cb[CONNECTOR_UNITTEST_MAX_CONNECTIONS + 1] = {NULL};

/**********************************************************
 * Implementation
 *********************************************************/
// Connector interface
CONNECTOR connector_unittest =
    {
        .name = "UnitTest connector",
        .initialize = connector_unittest_init,
        .data_lines = CONNECTOR_UNITTEST_MAX_CONNECTIONS,
};

void connector_unittest_enable(connector_unittest_init_task_cb_t init_cb, connector_unittest_task_handler_t task_handler)
{
    if (NULL != task_handler)
    {
        l_task_handler[0] = task_handler;
    }
    else
    {
        l_task_handler[0] = NULL;
    }
    if (NULL != init_cb)
    {
        l_init_cb[0] = init_cb;
    }
    else
    {
        l_init_cb[0] = NULL;
    }
}

void connector_unittest_enable_indexed_connector(connector_unittest_init_task_cb_t init_cb, connector_unittest_task_handler_t task_handler, uint8_t subconnector)
{
    assert(subconnector <= CONNECTOR_UNITTEST_MAX_CONNECTIONS);
    if (NULL != task_handler)
    {
        l_task_handler[subconnector] = task_handler;
    }
    else
    {
        l_task_handler[subconnector] = NULL;
    }
    if (NULL != init_cb)
    {
        l_init_cb[subconnector] = init_cb;
    }
    else
    {
        l_init_cb[subconnector] = NULL;
    }
}
// Initialize the connector
static int connector_unittest_init(void)
{
    bool is_task_found = false;
    // Should we start up the task
    for (int i = 0; i < CONNECTOR_UNITTEST_MAX_CONNECTIONS; ++i)
    {
        if (NULL != l_init_cb[i])
        {
            l_init_cb[i]();
        }
        if (NULL != l_task_handler[i])
        {
            is_task_found = true;
        }
    }
    if (is_task_found)
    {
        // create connector unittest processing task
        TRUE_CHECK(xTaskCreate(connector_unittest_process_task, connector_unittest.name, 4096, NULL, xTASK_PRIORITY_NORMAL, NULL));

        return 1;
    }
    else
    {
        return 0;
    }
}

// Task that processes frames received from broker
static void connector_unittest_process_task(void *param)
{
    DDMP2_FRAME *pframe;
    size_t frame_size;

    while (1)
    {
        // retrieve frame from broker
        pframe = xRingbufferReceive(connector_unittest.to_connector, &frame_size, portMAX_DELAY);
        TRUE_CHECK(pframe != NULL);

        if (pframe == NULL)
        {
            continue;
        }
        int sub_connector = connectors[pframe->destination_connector]->sub_connector_id;
        if (NULL != l_task_handler[sub_connector])
        {
            l_task_handler[sub_connector](pframe);
        }
        vRingbufferReturnItem(connector_unittest.to_connector, pframe);  // return frame to ring buffer
    }
}
