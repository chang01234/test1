/*
 * connector_lindev.h
 */

#ifndef CONNECTOR_LINDEV_H_
#define CONNECTOR_LINDEV_H_

#include "configuration.h"

#if defined(CONNECTOR_LINDEV_SHAPE)
#define connector_lindev_shape connector_lindev
#include "connector_lindev_shape.h"
#elif defined(CONNECTOR_LINDEV_SHARC)
#define connector_lindev_sharc connector_lindev
#include "connector_lindev_sharc.h"
#elif defined(CONNECTOR_LINDEV_INVENT)
#define connector_lindev_invent connector_lindev
#include "connector_lindev_invent.h"
#elif defined(CONNECTOR_LINDEV_NRX)
#define connector_lindev_nrx connector_lindev
#include "connector_lindev_nrx.h"
#else
#error "Define CONNECTOR_LINDEV_SHAPE or CONNECTOR_LINDEV_SHARC or CONNECTOR_LINDEV_INVENT macro to enable the product LINDEV implementation"
#endif

/**
 * @brief   Enable LINDEV operation
 *
 * By default LINDEV operation is enabled. At start-up it is not needed to call this function in
 * order to have LINDEV in operation.
 */
void connector_lindev_enable(void);

/**
 * @brief   Disable LINDEV operation
 *
 * Use this function to disable LINDEV operation during sensitive firmware modes, like while doing
 * OTA.
 */
void connector_lindev_disable(void);

#endif /* CONNECTOR_LINDEV_H_ */
