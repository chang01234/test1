/*! \file broker.h
    \brief Broker header

    Broker and DDMP handling.
 */

#ifndef BROKER_H_
#define BROKER_H_

#include "freertos/FreeRTOS.h"
#include <stdint.h>

#include "configuration.h"
#include "ddm2.h"

// See FrameworkDefaults.cmake for other settings for other targets with limited memory
#ifndef SUBSCRIPTION_DEPTH
#define SUBSCRIPTION_DEPTH 4096  //!< \~ Subscription table size in entries
#endif
#ifndef INVENTORY_DEPTH
#define INVENTORY_DEPTH 1024  //!< \~ Inventory table size in entries
#endif
#ifndef ROUTE_DEPTH
#define ROUTE_DEPTH 4096  //!< \~ Route table size in entries
#endif
#ifndef INSTANCE_DEPTH
#define INSTANCE_DEPTH 4096  //!< \~ Instance table size in entries
#endif

#define BROKER_EXTENDED_LOG 0  // keep 0 when committing

#if BROKER_EXTENDED_LOG
#define BROKER_LOG(level, format, ...) LOG(level, format, __VA_ARGS__)
#else  // BROKER_EXTENDED_LOG
#define BROKER_LOG(level, format, ...) ((void)0)
#endif  // BROKER_EXTENDED_LOG

#if (MULTIBROKER == 1)
#define MULTIBROKER_INSTANCE_BITS  (8 - MULTIBROKER_BROKER_BITS)
#define MULTIBROKER_MAX_INSTANCE   ((1 << MULTIBROKER_INSTANCE_BITS) - 1)
#define MULTIBROKER_MAX_BROKERS    ((1 << MULTIBROKER_BROKER_BITS) - 1)
#define MULTIBROKER_MAX_CLIENTS    (MULTIBROKER_MAX_BROKERS - 1)
#define MULTIBROKER_SERVER_ID      MULTIBROKER_MAX_BROKERS
#define MULTIBROKER_INSTANCE_MASK  MULTIBROKER_MAX_INSTANCE
#define MULTIBROKER_BROKER_ID_MASK (MULTIBROKER_MAX_BROKERS << MULTIBROKER_INSTANCE_BITS)
#endif

void broker_serve_inventory(const DDMP2_FRAME *const pframe);
int broker_request_instance_n(const uint32_t device_class);
int broker_request_instance(uint32_t *const device_class);
int broker_register_instance(uint32_t *const device_class, const int connector_id);
int broker_forward_publish(const DDMP2_FRAME *const pframe);
int broker_forward_fragment(const DDMP2_FRAME *const pframe, const uint32_t parameter);
void initialize_broker(void);

#endif /* BROKER_H_ */
