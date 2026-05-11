/*! \file connector_unittest.h */

#ifndef CONNECTOR_UNITTEST_H_
#define CONNECTOR_UNITTEST_H_

#include <stdint.h>
#include "connector.h"
#include "ddm2.h"

extern CONNECTOR connector_unittest;
typedef void (*connector_unittest_task_handler_t)(DDMP2_FRAME *pframe);
typedef void (*connector_unittest_init_task_cb_t)(void);
void connector_unittest_enable(connector_unittest_init_task_cb_t init_cb, connector_unittest_task_handler_t task_handler);
void connector_unittest_enable_indexed_connector(connector_unittest_init_task_cb_t init_cb, connector_unittest_task_handler_t task_handler, uint8_t subconnector);
#endif /* CONNECTOR_UNITTEST_H_*/