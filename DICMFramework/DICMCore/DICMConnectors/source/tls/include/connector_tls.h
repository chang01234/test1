#ifndef CONNECTOR_TLS_H_
#define CONNECTOR_TLS_H_

#include <stdint.h>
#include "connector.h"

#define TLS_CONTEXT_KEY_SIZE     2048

/**
 * @brief Get the TLS key.
 *
 * This function retrieves the TLS key from the TLS context and stores
 * it in the provided buffer. The buffer should be large enough to hold the key.
 *
 * @param key Pointer to the buffer where the key will be stored.
 */
void tls_context_get_key(char *key);

/**
 * @brief Get the TLS certificate.
 *
 * This function retrieves the TLS certificate from the TLS context and stores
 * it in the provided buffer. The buffer should be large enough to hold the certificate.
 *
 * @param cert Pointer to the buffer where the certificate will be stored.
 */
void tls_context_get_certificate(char *cert);

extern CONNECTOR connector_tls;

#endif
