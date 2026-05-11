/*****************************************************************************
 * \file       connector_tls.c
 * \brief      TLS connector
 * \copyright  Dometic Group
 *             This source file and the information contained in it are
 *             confidential and proprietary to Dometic Group
 *             The reproduction or disclosure, in whole or in part,
 *             to anyone outside of Dometic Group without the written
 *             approval of a Dometic Group officer under a Non-Disclosure
 *             Agreement is expressly prohibited.
 *
 *             All rights reserved
 *****************************************************************************/

#include <string.h>
#include <time.h>

#include "broker.h"
#include "connector_tls.h"
#include "esp_mac.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/error.h"
#include "mbedtls/pk.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/x509_csr.h"

#define TLS_CONFIG_KEY_SIZE TLS_CONTEXT_KEY_SIZE

#define TLS_KEY_RSA_BITS     2048
#define TLS_KEY_RSA_EXPONENT 65537

#define PERSONALIZATION_LEN 32

#define CERT_EXPIRED (0)

#define EVENT_CERT_VALIDATE (0x01)

typedef struct tls_context
{
    uint32_t device_class;
    CONNECTOR *connector;

    char pers_string[PERSONALIZATION_LEN];

    SemaphoreHandle_t key_mutex;
    SemaphoreHandle_t cert_mutex;

    uint8_t key[TLS_CONFIG_KEY_SIZE];
    uint8_t csr[TLS_CONFIG_KEY_SIZE];
    uint8_t cert[TLS_CONFIG_KEY_SIZE];
    uint8_t cacert[TLS_CONFIG_KEY_SIZE];

    uint8_t fragment_buffer[TLS_CONFIG_KEY_SIZE];
    uint32_t fragment_parameter;

    bool time_synced;
    TimerHandle_t cert_validation_timer;
} tls_context_t;

static tls_context_t *tls_instance = NULL;

static int task_delay_rng(void *p_rng, unsigned char *output, size_t output_len);
static time_t x509_time_to_epoch(const mbedtls_x509_time *t);
static bool tls_key_generate(tls_context_t *tls);
static void tls_csr_generate(tls_context_t *tls);
static bool tls_cert_verify_with_ca(const tls_context_t *tls, const char *cert);
static time_t tls_x509_cert_expiry(const char *cert);
static bool tls_x509_cert_is_valid(const tls_context_t *tls, const char *cert);
static void tls_validate_certificates(tls_context_t *tls);
static void connector_tls_cert_validation_timer(TimerHandle_t xTimer);
static void connector_tls_send_fragment_frames(const uint32_t parameter,
                                               const uint8_t *fragment, const size_t fragment_len, const uint8_t tls_connector_id);
static void connector_tls_publish_data(const tls_context_t *tls, uint32_t parameter, void *value, uint8_t len);
static void connector_tls_request_validation(tls_context_t *tls);
static void connector_tls_handle_subscribe(tls_context_t *tls, const DDMP2_FRAME *pframe);
static void connector_tls_handle_fragment(tls_context_t *tls, const DDMP2_FRAME *pframe);
static void connector_tls_handle_set(tls_context_t *tls, const DDMP2_FRAME *pframe);
static void connector_tls_handle_publish(tls_context_t *tls, const DDMP2_FRAME *pframe);
static void connector_tls_handle_generic(tls_context_t *tls, const DDMP2_FRAME *pframe);
static void connector_tls_task(void *task_param);
static int connector_tls_initialize(void);

void tls_context_get_key(char *key)
{
    tls_context_t *tls = tls_instance;

    if (!key)
    {
        LOG(E, "Invalid key buffer or length.");
        return;
    }

    if (!tls)
    {
        snprintf(key, TLS_CONTEXT_KEY_SIZE, "invalid");
        return;
    }

    TRUE_CHECK(xSemaphoreTake(tls->key_mutex, portMAX_DELAY));
    snprintf(key, TLS_CONTEXT_KEY_SIZE, "%s", tls->key);
    xSemaphoreGive(tls->key_mutex);
}

void tls_context_get_certificate(char *cert)
{
    tls_context_t *tls = tls_instance;

    if (!cert)
    {
        LOG(E, "Invalid certificate buffer or length.");
        return;
    }

    if (!tls)
    {
        snprintf(cert, TLS_CONTEXT_KEY_SIZE, "invalid");
        return;
    }

    TRUE_CHECK(xSemaphoreTake(tls->cert_mutex, portMAX_DELAY));
    snprintf(cert, TLS_CONTEXT_KEY_SIZE, "%s", tls->cert);
    xSemaphoreGive(tls->cert_mutex);
}

static uint32_t max_timer_period_ms(void)
{
    return (UINT32_MAX / configTICK_RATE_HZ);
}

static int task_delay_rng(void *p_rng, unsigned char *output, size_t output_len)
{
    // Yield so other tasks can run
    vTaskDelay(pdMS_TO_TICKS(10));

    // Call the default CTR-DRBG random generator
    return mbedtls_ctr_drbg_random(p_rng, output, output_len);
}

static time_t x509_time_to_epoch(const mbedtls_x509_time *t)
{
    struct tm tm_time;
    tm_time.tm_year = t->year - 1900;
    tm_time.tm_mon = t->mon - 1;
    tm_time.tm_mday = t->day;
    tm_time.tm_hour = t->hour;
    tm_time.tm_min = t->min;
    tm_time.tm_sec = t->sec;
    tm_time.tm_isdst = -1;
    return mktime(&tm_time);
}

static bool tls_key_generate(tls_context_t *tls)
{
    int ret;
    esp_err_t err;
    mbedtls_pk_context pk;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    const char *pers = tls->pers_string;

    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    // Seed the random number generator
    ret = mbedtls_ctr_drbg_seed(
        &ctr_drbg,
        mbedtls_entropy_func,
        &entropy,
        (const unsigned char *)pers,
        strlen(pers));
    if (ret != 0)
    {
        mbedtls_entropy_free(&entropy);
        mbedtls_ctr_drbg_free(&ctr_drbg);
        LOG(E, "Failed to seed random number generator: -0x%x", -ret);
        return false;
    }

    mbedtls_pk_init(&pk);

    // Generate the server private key
    ret = mbedtls_pk_setup(&pk, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA));
    if (ret != 0)
    {
        mbedtls_pk_free(&pk);
        mbedtls_entropy_free(&entropy);
        mbedtls_ctr_drbg_free(&ctr_drbg);
        LOG(E, "Failed to set up RSA key: -0x%x", -ret);
        return false;
    }

    LOG(I, "Generating TLS private key...");

    // Generate 2048-bit RSA key with exponent = 65537,
    // using our custom RNG callback that feeds WDT
    ret = mbedtls_rsa_gen_key(mbedtls_pk_rsa(pk), task_delay_rng, &ctr_drbg,
                              TLS_KEY_RSA_BITS, TLS_KEY_RSA_EXPONENT);
    if (ret != 0)
    {
        mbedtls_pk_free(&pk);
        mbedtls_entropy_free(&entropy);
        mbedtls_ctr_drbg_free(&ctr_drbg);
        LOG(E, "Failed to generate RSA key: -0x%x", -ret);
        return false;
    }

    // Write the key to a buffer
    memset(tls->key, 0, sizeof(tls->key));
    ret = mbedtls_pk_write_key_pem(&pk, tls->key, sizeof(tls->key));
    if (ret != 0)
    {
        mbedtls_pk_free(&pk);
        mbedtls_entropy_free(&entropy);
        mbedtls_ctr_drbg_free(&ctr_drbg);
        LOG(E, "mbedtls_pk_write_key_pem failed: -0x%x", -ret);
        return false;
    }

    err = config_set_str("tls0key", (const char *)tls->key);
    if (err != ESP_OK)
    {
        mbedtls_pk_free(&pk);
        mbedtls_entropy_free(&entropy);
        mbedtls_ctr_drbg_free(&ctr_drbg);
        LOG(E, "Failed to save TLS key to flash: tls0key 0x%04x", err);
        return false;
    }

    LOG(I, "TLS private key generated and saved to flash.");

    mbedtls_pk_free(&pk);
    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_free(&ctr_drbg);

    return true;
}

static void tls_csr_generate(tls_context_t *tls)
{
    int ret;
    mbedtls_pk_context pk;
    mbedtls_x509write_csr csr;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    const char *pers = tls->pers_string;
    const unsigned char *tls_key = tls->key;

    LOG(I, "Generating TLS Certificate Signing Request...");

    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    // Seed the random number generator
    ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                (const unsigned char *)pers, strlen(pers));
    if (ret != 0)
    {
        mbedtls_entropy_free(&entropy);
        mbedtls_ctr_drbg_free(&ctr_drbg);
        LOG(E, "Failed to seed random number generator: -0x%x", -ret);
        return;
    }

    mbedtls_pk_init(&pk);

    // Parse the server key
    ret = mbedtls_pk_parse_key(&pk, tls_key, strlen((const char *)tls_key) + 1,
                               NULL, 0, task_delay_rng, &ctr_drbg);
    if (ret != 0)
    {
        mbedtls_pk_free(&pk);
        mbedtls_entropy_free(&entropy);
        mbedtls_ctr_drbg_free(&ctr_drbg);
        LOG(E, "Failed to parse server key: -0x%x", -ret);
        return;
    }

    mbedtls_x509write_csr_init(&csr);

    // Set up the CSR
    mbedtls_x509write_csr_set_subject_name(&csr,
                                           "C=SE,ST=Stockholm,L=Solna,O=Dometic Holding AB,"
                                           "OU=Connectivity,CN=Firmware");
    mbedtls_x509write_csr_set_key(&csr, &pk);
    mbedtls_x509write_csr_set_md_alg(&csr, MBEDTLS_MD_SHA256);

    // Generate the CSR
    ret = mbedtls_x509write_csr_pem(&csr, tls->csr, sizeof(tls->csr),
                                    task_delay_rng, &ctr_drbg);
    if (ret != 0)
    {
        mbedtls_pk_free(&pk);
        mbedtls_x509write_csr_free(&csr);
        mbedtls_entropy_free(&entropy);
        mbedtls_ctr_drbg_free(&ctr_drbg);
        LOG(E, "Failed to write CSR: -0x%x", -ret);
        return;
    }

    LOG(I, "TLS Certificate Signing Request was successfully generated.");

    connector_tls_send_fragment_frames(
        TLS0CSR,
        tls->csr,
        strlen((const char *)tls->csr),
        tls->connector->connector_id);

    mbedtls_pk_free(&pk);
    mbedtls_x509write_csr_free(&csr);
    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_free(&ctr_drbg);
}

static bool tls_cert_verify_with_ca(const tls_context_t *tls, const char *cert)
{
    int ret;
    bool is_valid = false;
    char error_buf[100];
    uint32_t flags;
    mbedtls_x509_crt ca_cert;
    mbedtls_x509_crt server_cert;

    mbedtls_x509_crt_init(&ca_cert);
    mbedtls_x509_crt_init(&server_cert);

    ret = mbedtls_x509_crt_parse(&ca_cert, (const uint8_t *)tls->cacert,
                                 strlen((const char *)tls->cacert) + 1);
    if (ret != 0)
    {
        mbedtls_strerror(ret, error_buf, 100);
        LOG(W, "Failed to parse CA certificate: %s", error_buf);
        return false;
    }

    ret = mbedtls_x509_crt_parse(&server_cert, (const uint8_t *)cert,
                                 strlen(cert) + 1);
    if (ret != 0)
    {
        mbedtls_strerror(ret, error_buf, 100);
        LOG(W, "Failed to parse server certificate: %s", error_buf);
        return false;
    }

    ret = mbedtls_x509_crt_verify(&server_cert, &ca_cert,
                                  NULL, NULL, &flags, NULL, NULL);
    if (ret != 0)
    {
        mbedtls_strerror(ret, error_buf, 100);
        LOG(W, "Certificate verification failed: %s", error_buf);
    }
    else
    {
        is_valid = true;
    }

    mbedtls_x509_crt_free(&ca_cert);
    mbedtls_x509_crt_free(&server_cert);

    return is_valid;
}

static time_t tls_x509_cert_expiry(const char *cert)
{
    time_t cert_valid_to = 0;
    mbedtls_x509_crt x509_cert;

    if (!cert || (strlen(cert) == 0))
    {
        LOG(W, "Invalid certificate buffer or length.");
        return cert_valid_to;
    }

    mbedtls_x509_crt_init(&x509_cert);
    int ret = mbedtls_x509_crt_parse(
        &x509_cert,
        (const uint8_t *)cert,
        strlen(cert) + 1);
    if (ret == 0)
    {
        cert_valid_to = x509_time_to_epoch(&x509_cert.valid_to);
    }
    mbedtls_x509_crt_free(&x509_cert);

    return cert_valid_to;
}

static bool tls_x509_cert_is_valid(const tls_context_t *tls, const char *cert)
{
    bool is_valid = true;
    mbedtls_x509_crt x509_cert;
    char error_buf[100];

    mbedtls_x509_crt_init(&x509_cert);
    int ret = mbedtls_x509_crt_parse(
        &x509_cert,
        (const uint8_t *)cert,
        strlen(cert) + 1);
    if (ret != 0)
    {
        is_valid = false;
        mbedtls_strerror(ret, error_buf, 100);
        LOG(W, "Failed to parse certificate: %s", error_buf);
    }
    else if (tls->time_synced)
    {
        time_t now = time(NULL);
        time_t valid_from = x509_time_to_epoch(&x509_cert.valid_from);
        time_t valid_to = tls_x509_cert_expiry(cert);
        if ((now < valid_from) || (now > valid_to))
        {
            const uint32_t expired = CERT_EXPIRED;
            is_valid = false;
            connector_tls_publish_data(tls, TLS0EXPIRY, (void *)&expired, sizeof(uint32_t));

            LOG(W, "Certificate is not valid. Valid from: %d-%d-%d, Valid to: %d-%d-%d",
                x509_cert.valid_from.year, x509_cert.valid_from.mon, x509_cert.valid_from.day,
                x509_cert.valid_to.year, x509_cert.valid_to.mon, x509_cert.valid_to.day);
        }
    }

    mbedtls_x509_crt_free(&x509_cert);

    return is_valid;
}

static void tls_validate_certificates(tls_context_t *tls)
{
    bool cert_valid = strlen((const char *)tls->cert) != 0,
         ca_cert_valid = strlen((const char *)tls->cacert) != 0;

    if (ca_cert_valid && !tls_x509_cert_is_valid(tls, (const char *)tls->cacert))
    {
        ca_cert_valid = false;
        config_erase("tls0cacert");
        memset(tls->cacert, 0, sizeof(tls->cacert));
        LOG(W, "Deleted invalid CA certificate.");
    }

    if (cert_valid && (!ca_cert_valid ||
                       !tls_x509_cert_is_valid(tls, (const char *)tls->cert)))
    {
        cert_valid = false;
        config_erase("tls0cert");
        memset(tls->cert, 0, sizeof(tls->cert));
        LOG(W, "Deleted invalid server certificate.");
    }

    if (cert_valid && ca_cert_valid &&
        !tls_cert_verify_with_ca(tls, (const char *)tls->cert))
    {
        cert_valid = false;
        config_erase("tls0cert");
        memset(tls->cert, 0, sizeof(tls->cert));
        LOG(W, "Deleted invalid server certificate.");
    }

    connector_tls_publish_data(tls, TLS0CERT, (void *)(cert_valid ? &One : &Zero), sizeof(int32_t));
    connector_tls_publish_data(tls, TLS0CACERT, (void *)(ca_cert_valid ? &One : &Zero), sizeof(int32_t));
}

static void connector_tls_cert_validation_timer(TimerHandle_t timer)
{
    tls_context_t *tls = (tls_context_t *)pvTimerGetTimerID(timer);

    connector_tls_request_validation(tls);
}

static void connector_tls_send_fragment_frames(const uint32_t parameter,
                                               const uint8_t *fragment,
                                               const size_t fragment_len,
                                               const uint8_t tls_connector_id)
{
    size_t len;
    DDMP2_FRAME frame;

    // Transmit the Fragment by breaking it into smaller chunks
    // Chunk 0: 4 bytes (parameter) + 148 bytes (CSR)
    // Chunk 1 to n: 152 bytes (CSR)
    // Chunk n+1: 0xffff
    for (size_t i = 0; i < fragment_len;)
    {
        if (i == 0)
        {
            char chunk[152];

            memcpy(chunk, &parameter, 4);
            memcpy(chunk + 4, fragment, MIN(fragment_len - i, 148));

            ddmp2_create_fragment(
                &frame,
                i,
                chunk,
                4 + MIN(fragment_len, 148),
                0);

            i += MIN(fragment_len, 148);
        }
        else
        {
            len = MIN(fragment_len - i, 152);

            ddmp2_create_fragment(
                &frame,
                i,
                fragment + i,
                len,
                0);
            i += len;
        }

        frame.source_connector = tls_connector_id;
        connector_forward_frame_to_broker(&frame);
    }

    memset(&frame, 0, sizeof(frame));
    frame.frame.control = DDMP2_CONTROL_FRAGMENT;
    frame.frame.fragment.offset = 0xffff;
    frame.frame_size = 3;
    frame.source_connector = tls_connector_id;
    frame.destination_connector = 0;

    connector_forward_frame_to_broker(&frame);
}

static void connector_tls_publish_data(const tls_context_t *tls, uint32_t parameter, void *data, uint8_t size)
{
    TRUE_CHECK(connector_send_frame_to_broker(
        DDMP2_CONTROL_PUBLISH,
        parameter,
        data,
        size,
        tls->connector->connector_id,
        portMAX_DELAY));
}

static void connector_tls_request_validation(tls_context_t *tls)
{
    // Cancel any existing validation timer
    if (xTimerStop(tls->cert_validation_timer, 0) != pdPASS)
    {
        LOG(E, "Failed to stop certificate validation timer.");
    }

    // Request validation of TLS certificates
    connector_send_frame_to_connector(
        DDMP2_CONTROL_GENERIC,
        EVENT_CERT_VALIDATE,
        &One,
        sizeof(One),
        tls->connector->connector_id,
        portMAX_DELAY);
}

static void connector_tls_handle_subscribe(tls_context_t *tls, const DDMP2_FRAME *pframe)
{
    int32_t response = 0;

    switch (pframe->frame.subscribe.parameter)
    {
    case TLS0CSR:
        if (strlen((const char *)tls->csr) != 0)
        {
            connector_tls_send_fragment_frames(
                TLS0CSR,
                tls->csr,
                strlen((const char *)tls->csr),
                tls->connector->connector_id);
        }
        break;
    case TLS0CERT:
        TRUE_CHECK(xSemaphoreTake(tls->cert_mutex, portMAX_DELAY));
        response = tls_cert_verify_with_ca(tls, (const char *)tls->cert) ? 1 : 0;
        xSemaphoreGive(tls->cert_mutex);
        connector_tls_publish_data(tls, TLS0CERT, &response, sizeof(int32_t));
        break;
    case TLS0CACERT:
        response = tls_x509_cert_is_valid(tls, (const char *)tls->cacert) ? 1 : 0;
        connector_tls_publish_data(tls, TLS0CACERT, &response, sizeof(int32_t));
        break;
    case TLS0EXPIRY:
        connector_tls_request_validation(tls);
        break;
    }
}

static void connector_tls_handle_fragment(tls_context_t *tls, const DDMP2_FRAME *pframe)
{
    int err = 0;
    int32_t response = 0;
    uint8_t *fragment_buffer = (uint8_t *)tls->fragment_buffer;
    size_t fragment_size = sizeof(tls->fragment_buffer);
    size_t fragment_len = pframe->frame_size - 3;
    static uint16_t offset = 0;

    if (pframe->frame.fragment.offset == 0xffff)
    {
        if (tls->fragment_parameter == TLS0CERT)
        {
            if (!tls_cert_verify_with_ca(tls, (const char *)fragment_buffer) ||
                (err = config_set_str("tls0cert", (const char *)fragment_buffer)) != ESP_OK)
            {
                LOG(E, "Failed to save TLS certificate to flash: tls0cert 0x%04x", err);
            }
            else
            {
                response = 1;
                TRUE_CHECK(xSemaphoreTake(tls->cert_mutex, portMAX_DELAY));
                memcpy(tls->cert, fragment_buffer, strlen((const char *)fragment_buffer));
                xSemaphoreGive(tls->cert_mutex);
                LOG(D, "Signed TLS certificate saved to flash.");
                connector_tls_request_validation(tls);
            }
            connector_tls_publish_data(tls, TLS0CERT, (void *)&response, sizeof(int32_t));
        }
        else if (tls->fragment_parameter == TLS0CACERT)
        {
            if (!tls_x509_cert_is_valid(tls, (const char *)fragment_buffer) ||
                (err = config_set_str("tls0cacert", (const char *)fragment_buffer)) != ESP_OK)
            {
                LOG(E, "Failed to save TLS CA cert to flash: tls0cacert 0x%04x", err);
            }
            else
            {
                response = 1;
                LOG(D, "CA certificate saved to flash.");
                memcpy(tls->cacert, fragment_buffer, strlen((const char *)fragment_buffer));
            }
            connector_tls_publish_data(tls, TLS0CACERT, (void *)&response, sizeof(int32_t));
        }
    }
    else if (pframe->frame.fragment.offset == 0)
    {
        memcpy(&tls->fragment_parameter, pframe->frame.fragment.value, 4);
        memset(fragment_buffer, 0, fragment_size);
        memcpy(fragment_buffer, pframe->frame.fragment.value + 4, fragment_len - 4);
        offset = fragment_len - 4;
    }
    else
    {
        memcpy(fragment_buffer + offset, pframe->frame.fragment.value, fragment_len);
        offset += fragment_len;
    }
}

static void connector_tls_handle_set(tls_context_t *tls, const DDMP2_FRAME *pframe)
{
    switch (pframe->frame.set.parameter)
    {
    case TLS0DEL:
        config_erase("tls0cert");
        TRUE_CHECK(xSemaphoreTake(tls->cert_mutex, portMAX_DELAY));
        memset(tls->cert, 0, sizeof(tls->cert));
        xSemaphoreGive(tls->cert_mutex);
        connector_tls_publish_data(tls, TLS0CERT, (void *)&Zero, sizeof(int32_t));
        LOG(W, "TLS certificate deleted from flash.");
        break;
    case TLS0CSR:
        memset(tls->csr, 0, sizeof(tls->csr));
        tls_csr_generate(tls);
        break;
    }
}

static void connector_tls_handle_publish(tls_context_t *tls, const DDMP2_FRAME *pframe)
{
    switch (pframe->frame.publish.parameter)
    {
    case SVC0TSRC:
        if ((SVC0TSRC_ENUM)pframe->frame.publish.value.uint32 == SVC0TSRC_NTP)
        {
            LOG(D, "Time synced with NTP server.");
            TRUE_CHECK(xSemaphoreTake(tls->cert_mutex, portMAX_DELAY));
            tls->time_synced = true;
            xSemaphoreGive(tls->cert_mutex);
            connector_tls_request_validation(tls);
        }
        break;
    }
}

static void connector_tls_handle_generic(tls_context_t *tls, const DDMP2_FRAME *pframe)
{
    if (pframe->frame.generic.id == EVENT_CERT_VALIDATE)
    {
        time_t cert_valid_to = 0, ca_cert_valid_to = 0;

        tls_validate_certificates(tls);
        cert_valid_to = tls_x509_cert_expiry((const char *)tls->cert);
        ca_cert_valid_to = tls_x509_cert_expiry((const char *)tls->cacert);

        if (tls->time_synced && (cert_valid_to > 0) && (ca_cert_valid_to > 0))
        {
            time_t expiry = MIN(cert_valid_to, ca_cert_valid_to);
            const time_t now = time(NULL);
            const uint32_t period_ms = MIN(max_timer_period_ms(), MAX(expiry - now, 1) * 1000);

            connector_tls_publish_data(tls, TLS0EXPIRY, (void *)&expiry, sizeof(uint32_t));

            LOG(I, "Validate TLS certificate in: %d ms", period_ms);
            if (xTimerChangePeriod(tls->cert_validation_timer, pdMS_TO_TICKS(period_ms), 0) != pdPASS)
            {
                LOG(E, "Failed to change TLS certificate validation timer period.");
            }
        }
        else if ((cert_valid_to == 0) || (ca_cert_valid_to == 0))
        {
            const uint32_t expiry = CERT_EXPIRED;
            connector_tls_publish_data(tls, TLS0EXPIRY, (void *)&expiry, sizeof(uint32_t));
        }
    }
}

static void connector_tls_task(void *task_param)
{
    size_t size;
    esp_err_t err;
    DDMP2_FRAME *pframe;
    DDMP2_FRAME l_frame;
    tls_context_t *tls = (tls_context_t *)task_param;
    CONNECTOR *connector = tls->connector;

    TRUE_CHECK(xSemaphoreTake(tls->key_mutex, portMAX_DELAY));
    size = sizeof(tls->key);
    if ((err = config_get_str("tls0key", (char *)tls->key, &size)) != ESP_OK)
    {
        LOG(W, "Failed to load TLS key, generating new one. Error: 0x%04x", err);
        if (!tls_key_generate(tls))
        {
            xSemaphoreGive(tls->key_mutex);
            vTaskDelete(NULL);
            return;
        }
    }
    xSemaphoreGive(tls->key_mutex);

    TRUE_CHECK(xSemaphoreTake(tls->cert_mutex, portMAX_DELAY));
    size = sizeof(tls->cert);
    if ((err = config_get_str("tls0cert", (char *)tls->cert, &size)) != ESP_OK)
    {
        LOG(W, "Failed to load TLS certificate. Error: 0x%04x", err);
    }
    xSemaphoreGive(tls->cert_mutex);

    size = sizeof(tls->cacert);
    if ((err = config_get_str("tls0cacert", (char *)tls->cacert, &size)) != ESP_OK)
    {
        LOG(W, "Failed to load CA certificate. Error: 0x%04x", err);
    }

    if (broker_register_instance(&tls->device_class, tls->connector->connector_id) == -1)
    {
        LOG(E, "Failed to register TLS0 instance. Disabling TLS connector!");
        vTaskDelete(NULL);
        return;
    }

    xTimerStart(tls->cert_validation_timer, 0);

    TRUE_CHECK(connector_send_frame_to_broker(
        DDMP2_CONTROL_SUBSCRIBE,
        SVC0TSRC,
        NULL,
        0,
        tls->connector->connector_id,
        portMAX_DELAY));

    while (1)
    {
        pframe = xRingbufferReceive(connector->to_connector, &size, portMAX_DELAY);
        if (NULL == pframe)
        {
            continue;
        }
        memcpy(&l_frame, pframe, size);
        vRingbufferReturnItem(connector->to_connector, pframe);
        pframe = &l_frame;

        switch (pframe->frame.control)
        {
        case DDMP2_CONTROL_SUBSCRIBE:
            connector_tls_handle_subscribe(tls, pframe);
            break;
        case DDMP2_CONTROL_FRAGMENT:
            connector_tls_handle_fragment(tls, pframe);
            break;
        case DDMP2_CONTROL_SET:
            connector_tls_handle_set(tls, pframe);
            break;
        case DDMP2_CONTROL_PUBLISH:
            connector_tls_handle_publish(tls, pframe);
            break;
        case DDMP2_CONTROL_GENERIC:
            connector_tls_handle_generic(tls, pframe);
            break;
        }
    }

    vTaskDelete(NULL);
}

static int connector_tls_initialize(void)
{
    uint8_t mac[6];
    tls_context_t *tls;

    TRUE_CHECK_RETURN0(
        (tls = hal_mem_malloc_prefer(sizeof(*tls),
                                     HAL_MEM_SPIRAM,
                                     HAL_MEM_INTERNAL_RAM)) != NULL);

    memset(tls, 0, sizeof(*tls));

    tls->cert_validation_timer = xTimerCreate(
        "connector_tls_cert_validation_timer",
        pdMS_TO_TICKS(1000),
        pdFALSE,
        (void *)tls,
        connector_tls_cert_validation_timer);

    if (tls->cert_validation_timer == NULL)
    {
        LOG(E, "Failed to create TLS certificate validation timer.");
        hal_mem_free(tls);
        return 0;
    }

    tls->key_mutex = xSemaphoreCreateMutex();
    tls->cert_mutex = xSemaphoreCreateMutex();

    if ((tls->key_mutex == NULL) || (tls->cert_mutex == NULL))
    {
        LOG(E, "Failed to create mutexes for TLS connector.");
        if (tls->key_mutex != NULL)
        {
            vSemaphoreDelete(tls->key_mutex);
        }
        if (tls->cert_mutex != NULL)
        {
            vSemaphoreDelete(tls->cert_mutex);
        }
        if (tls->cert_validation_timer != NULL)
        {
            xTimerDelete(tls->cert_validation_timer, 0);
        }
        hal_mem_free(tls);
        return 0;
    }

    tls->device_class = TLS0;
    tls->connector = &connector_tls;
    tls->fragment_parameter = 0;
    tls->time_synced = false;

    memset(tls->key, 0, sizeof(tls->key));
    memset(tls->csr, 0, sizeof(tls->csr));
    memset(tls->cert, 0, sizeof(tls->cert));

    esp_efuse_mac_get_default(mac);
    snprintf(tls->pers_string, sizeof(tls->pers_string),
             "dicm_ddm2_tls_%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    tls_instance = tls;

    TRUE_CHECK(xTaskCreate(
        connector_tls_task,
        "connector_tls",
        4096,
        tls,
        xTASK_PRIORITY_BELOW_NORMAL,
        NULL));

    return 1;
}

CONNECTOR connector_tls = {
    .name = "TLS connector",
    .initialize = connector_tls_initialize,
};
