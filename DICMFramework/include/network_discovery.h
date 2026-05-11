/**
 * @file
 * @brief Network discovery interface
 *
 * Network discovery over Wi-Fi or LAN connections
 *
 * @defgroup    network_discovery Network discovery
 * @brief       Network discovery interface
 * @{
 */

#ifndef NETWORK_DISCOVERY_H_
#define NETWORK_DISCOVERY_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "esp_netif.h"
#include "esp_err.h"

#define NETWORK_DISCOVERY__ERROR_OK				0
#define NETWORK_DISCOVERY__ERROR_INVALID_SIZE	1
#define NETWORK_DISCOVERY__ERROR_DATA_MISMATCH	2
#define NETWORK_DISCOVERY__ERROR_NO_MEMORY		3

/**
 * @brief       Specifies the maximum size of request in bytes
 */
#define NETWORK_DISCOVERY__MAX_REQUEST_SIZE		4

/**
 * @brief       Defines which port should be used for UDP discovery service
 */
#define NETWORK_DISCOVERY__LISTEN_PORT			13143

typedef struct _network_discovery_item_result_t
{
    char *name;
    char *hostname;
    char *board;
    char *sku;
    char *fwversion;
    esp_ip_addr_t ip_addr;
    uint16_t port;
} network_discovery_item_result_t;

typedef struct _network_discovery_result_t
{
    int num_items;
    network_discovery_item_result_t *items;
} network_discovery_result_t;

typedef void (*network_discovery_query_callback_t)(void);

/**
 * @brief       Evaluate request data if it is a valid request for discovery from application
 *
 * @param       request_data Pointer to buffer containing incoming request data.
 * @param       request_data_size Size of data in buffer in bytes.
 * @return      Validation process status
 * @retval      NETWORK_DISCOVERY__ERROR_OK Request has been successfully validated.
 * @retval      NETWORK_DISCOVERY__ERROR_INVALID_SIZE When the length of passed request_data
 *              buffer is not of correct size.
 * @retval      NETWORK_DISCOVERY__ERROR_DATA_MISMATCH When the request_data has data that is
 *              different than expected.
 */
int network_discovery__request_validate(const void * const request_data, const size_t request_data_size);

/**
 * @brief       Generate response to the application
 *
 * This function will generate a response of unknown contents to the caller. What caller needs to
 * know is that there is some data to be sent, as a response, and how big the response is in bytes.
 *
 * @param       response_data Pointer to variable of `const void *` type that will point to the
 *              generated response.
 * @param       response_data_size Pointer to variable of `size_t` type that will contain the
 *              information of number of bytes pointed by @a packet_data pointer.
 * @return      Operation status
 * @return      NETWORK_DISCOVERY__ERROR_OK Response has been successfully generated.
 * @return      NETWORK_DISCOVERY__ERROR_NO_MEMORY Response can't be generated because of memory
 *              allocation failure.
 * @note        Each generated response needs to be recycled by call to
 *              @ref network_discovery__response_free in order to release allocated memory.
 * @code
 * const void * response_data;
 * size_t response_data_size;
 *
 * int discovery_error = network_discovery__response_generate(&response_data, &response_data_size);
 * if (discovery_error == NETWORK_DISCOVERY__ERROR_OK)
 * {
 *     // Send the response_data
 *     network_discovery__response_free(response_data);
 * }
 * else
 * {
 *     // Handle error
 * }
 * @endcode
 */
int network_discovery__response_generate(void ** response_data, size_t * response_data_size);

int network_discovery__mdns_initialize(void);
esp_err_t network_discovery__mdns_update(const char * const key, const char * const value);

/**
 * @brief       Recycle the previosly generated response.
 *
 * @param       response_data_size Previously generated response that needs to be recycled.
 */
void network_discovery__response_free(void * response_data_size);

void network_discovery__query_service(network_discovery_result_t *p_result);
bool network_discovery__query_async_service(network_discovery_query_callback_t notify_callback);
bool network_discovery__query_fetch_results(network_discovery_result_t *p_result);
void network_discovery__query_free_results(network_discovery_result_t *p_result);

#endif /* NETWORK_DISCOVERY_H_ */
/** @} */
