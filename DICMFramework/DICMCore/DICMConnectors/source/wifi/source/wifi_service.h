#ifndef WIFI_SERVICE_H_
#define WIFI_SERVICE_H_

#include <stdint.h>

#include "connector.h"
#include "fsm.h"

// FSM definitions, also bit numbers
#define WIFI_SERVICE_TLS_INVALID_EVENT      (FSM_USER_EVENT)
#define WIFI_SERVICE_TLS_VALID_EVENT        (FSM_USER_EVENT + 1)
#define WIFI_SERVICE_STA_CONNECTED_EVENT    (FSM_USER_EVENT + 2)
#define WIFI_SERVICE_STA_DISCONNECTED_EVENT (FSM_USER_EVENT + 3)
#define WIFI_SERVICE_AP_STARTED_EVENT       (FSM_USER_EVENT + 4)
#define WIFI_SERVICE_AP_STOPPED_EVENT       (FSM_USER_EVENT + 5)

typedef struct
{
    fsm_t sm;
    uint32_t sm_bit;
} ws_fsm_t;

#ifdef CONFIG_IDF_TARGET_LINUX
ws_fsm_t *get_ws_fsm(void);
void sm_wifi_service_startup(fsm_t *const fsm, fsm_event_t const *const event);
void sm_wifi_service_waiting(fsm_t *const fsm, fsm_event_t const *const event);
void sm_wifi_service_active(fsm_t *const fsm, fsm_event_t const *const event);
#endif

/**
 * @brief Wi-Fi service initialization function
 *
 * This function initializes the Wi-Fi service and sets up the
 * necessary parameters. It creates a mutex for socket management
 * and allocates memory for TLS key and certificate.
 *
 * @param connector Pointer to the CONNECTOR structure
 */
void wifi_services_init(CONNECTOR *connector);

#ifdef CONNECTOR_TLS
/**
 * @brief Wi-Fi service TLS validity setting function
 *
 * This function sets the TLS validity status for the Wi-Fi service.
 * It updates the TLS context with the provided key and certificate.
 *
 * @param tls_valid Boolean value indicating the TLS validity status
 */
void wifi_services_tls_valid(bool tls_valid);
#endif  // CONNECTOR_TLS

/**
 * @brief Wi-Fi send frame function
 *
 * This function sends a frame to the Wi-Fi service. It is used to
 * send control messages or publish data to the broker.
 *
 * @param pframe Pointer to the DDMP2_FRAME structure containing the
 *               frame data
 * @return int Returns 0 on success, or a negative error code on failure
 */
int wifi_service_send_frame(const DDMP2_FRAME *pframe);

/**
 * @brief Wi-Fi service frame function for generic events
 *
 * Used to trigger fsm
 *
 * @param pframe Pointer to the DDMP2_FRAME structure containing the
 *               frame data
 * @return nothing
 */
void wifi_service_handle_generic_parameter(const DDMP2_FRAME *pframe);

#endif
