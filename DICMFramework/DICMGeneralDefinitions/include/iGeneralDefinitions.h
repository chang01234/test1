/*! \file iGeneralDefinitions.h
    \brief Common definitions header

    Logging definitions, assert definitions, task definitions, other system definitions
*/

#ifndef IGENERALDEFINITIONS_H_
#define IGENERALDEFINITIONS_H_

#include "esp_log.h"
#include <inttypes.h>
#include <stdint.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#include "freertos/FreeRTOS.h"
#pragma GCC diagnostic pop

#ifndef ELEMENTS
#define ELEMENTS(x) (sizeof(x) / sizeof(x[0]))  //!< \~ Element count of array
#endif
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))  //!< \~ Minimum value macro
#endif
#ifndef MAX
#define MAX(a, b) ((a) < (b) ? (b) : (a))  //!< \~ Maximum value macro
#endif

#define DEFAULT_DEVICE_PASSWORD  "00000000"  //!< \~ Default device password
#define DEFAULT_STATION_SSID     ""          //!< \~ Factory reset station SSID
#define DEFAULT_STATION_PASSWORD ""          //!< \~ Factory reset station password
#define DEFAULT_PRODUCT_ID       0x04        //!< \~ Default box type type identification (4=Gateway)

#define WIFI_IP_BIT       0x00000001  //!< \~ WiFi interface currently has an IP address
#define MODEM_IP_BIT      0x00000002  //!< \~ PPP interface currently has an IP address
#define ETHERNET_IP_BIT   0x00000004  //!< \~ Ethernet interface currently has an IP address
#define INTERNET_BIT      0x00000008  //!< \~ Connected to internet
#define LOCAL_NETWORK_BIT 0x00000010  //!< \~ System is connected/connecting to local network with no internet access

#define SYSTEM_START_BIT 0x00000001  //!< \~ System started, used for delay of functionality if needed

#if configMAX_PRIORITIES == 25
#define xTASK_PRIORITY_IDLE         1
#define xTASK_PRIORITY_BELOW_NORMAL 2
#define xTASK_PRIORITY_NORMAL       3
#define xTASK_PRIORITY_ABOVE_NORMAL 4
#define xTASK_PRIORITY_HIGH         5
#define xTASK_PRIORITY_REALTIME     20
#define xTASK_STACK_DEFAULT         4096
#elif configMAX_PRIORITIES == 7
#define xTASK_PRIORITY_IDLE         1
#define xTASK_PRIORITY_BELOW_NORMAL 2
#define xTASK_PRIORITY_NORMAL       3
#define xTASK_PRIORITY_ABOVE_NORMAL 4
#define xTASK_PRIORITY_HIGH         5
#define xTASK_PRIORITY_REALTIME     6
#define xTASK_STACK_DEFAULT         4096
#else
#error "Please define task priorities"
#endif

#define UINT32(x) *((uint32_t *)(x))
#define UINT16(x) *((uint16_t *)(x))
#define UINT8(x)  *((uint8_t *)(x))

#define INT32(x) *((int32_t *)(x))
#define INT16(x) *((int16_t *)(x))
#define INT8(x)  *((int8_t *)(x))

#define XSTR(X) #X       //!< \~ Make a string
#define STR(X)  XSTR(X)  //!< \~ Evaluate and make a string

#define MACFORMAT   "%02x%02x%02x%02x%02x%02x"          //!< \~ Format string for printing a complete MAC address
#define MACFORMAT2  "%02x:%02x:%02x:%02x:%02x:%02x"     //!< \~ Format string for printing a complete MAC address
#define MACVALUE(a) a[0], a[1], a[2], a[3], a[4], a[5]  //!< \~ Macro to extract the six bytes of a uint8_t[6] MAC address

#define MAC3FORMAT   "%02x%02x%02x"    //!< \~ Format string for printing the three last bytes of a MAC address
#define MAC3VALUE(a) a[3], a[4], a[5]  //!< \~ Macro to extract the three last bytes from a uint8_t[6] MAC address

#define MAYBE_UNUSED __attribute__((unused))
#define WEAK         __attribute__((weak))

//! \~ Expands to the current file's name
#include <assert.h>

// #define FILENAME 					(__builtin_strrchr(__FILE__, '/') + 1)
#define FILENAME (__builtin_strrchr("/" __BASE_FILE__, '/') + 1)

// token pasting madness
#define X_COMPILE_TIME_ASSERT4(COND, MSG) typedef char static_assertion_##MSG[(!!(COND)) * 2 - 1]
#define X_COMPILE_TIME_ASSERT3(X, L)      X_COMPILE_TIME_ASSERT4(X, static_assertion_at_line_##L)
#define X_COMPILE_TIME_ASSERT2(X, L)      X_COMPILE_TIME_ASSERT3(X, L)

/**
 * @brief 		Statically (compile-time) ensure the assertion is true
 */
#define COMPILE_TIME_ASSERT(X) X_COMPILE_TIME_ASSERT2(X, __LINE__)

#ifdef NDEBUG
#define TRUE_CHECK(x) (x)
#define TRUE_CHECK_RETURN(x) \
    {                        \
        if (!(int)(x))       \
            return;          \
    }
#define TRUE_CHECK_RETURNX(x, result) \
    do                                \
    {                                 \
        if (!(int)(result))           \
            return (x);               \
    } while (0)
#define TRUE_CHECK_RETURN0(x) TRUE_CHECK_RETURNX(0, x)
#define TRUE_CHECK_RETURN1(x) TRUE_CHECK_RETURNX(1, x)
#define ZERO_CHECK(x)         ((void)(x))
#define ZERO_CHECK_RETURN(x) \
    {                        \
        if ((int)(x))        \
            return;          \
    }
#define ZERO_CHECK_RETURN_VAL(x) \
    {                            \
        if ((int)(x))            \
            return x;            \
    }
#define ZERO_CHECK_RETURNX(x, result) \
    do                                \
    {                                 \
        if ((int)(result))            \
            return (x);               \
    } while (0)
#define ZERO_CHECK_RETURN0(x)   ZERO_CHECK_RETURNX(0, x)
#define ZERO_CHECK_RETURN1(x)   ZERO_CHECK_RETURNX(1, x)
#define NONNEG_CHECK(x)         (x)
#define ASSERT(x)               ((void)0)
#define BOUNDED(x, h)           ((void)0)
#define RANGE(x, l, h)          ((void)0)
#define LOG(level, format, ...) ((void)0)
#define LOGC(format, ...)       ((void)0)
#define LOGB(format, ...)       ((void)0)
#define LOGP(format, ...)       ((void)0)
#define LOGE(format, ...)       ((void)0)
#define LOGW(format, ...)       ((void)0)
#define LOGI(format, ...)       ((void)0)
#define LOGD(format, ...)       ((void)0)
#define LOGV(format, ...)       ((void)0)
#else
#define TRUE_CHECK(x)                                    \
    {                                                    \
        int errchk_result = (int)(x);                    \
        if (!errchk_result)                              \
            LOG(E, "%s returned %d", #x, errchk_result); \
    }  // verify that result is nonzero

#define TRUE_CHECK_RETURN(x)                             \
    {                                                    \
        int errchk_result = (int)(x);                    \
        if (!errchk_result)                              \
        {                                                \
            LOG(E, "%s returned %d", #x, errchk_result); \
            return;                                      \
        }                                                \
    }  // verify that result is nonzero, exit function if not

#define TRUE_CHECK_RETURNX(x, result)                         \
    do                                                        \
    {                                                         \
        int errchk_result = (int)(result);                    \
        if (!errchk_result)                                   \
        {                                                     \
            LOG(E, "%s returned %d", #result, errchk_result); \
            return (x);                                       \
        }                                                     \
    } while (0);  // verify that result is true, exit function with result `x` if not

// verify that result is nonzero, exit function with result 0 if not
#define TRUE_CHECK_RETURN0(x) TRUE_CHECK_RETURNX(0, x)

// verify that result is nonzero, exit function with result 1 if not
#define TRUE_CHECK_RETURN1(x) TRUE_CHECK_RETURNX(1, x)

#define ZERO_CHECK(x)                                    \
    {                                                    \
        int errchk_result = (int)(x);                    \
        if (errchk_result)                               \
            LOG(E, "%s returned %d", #x, errchk_result); \
    }  // verify that result is zero

#define ZERO_CHECK_RETURNX(x, result)                         \
    do                                                        \
    {                                                         \
        int errchk_result = (int)(result);                    \
        if (errchk_result)                                    \
        {                                                     \
            LOG(E, "%s returned %d", #result, errchk_result); \
            return (x);                                       \
        }                                                     \
    } while (0);  // verify that result is zero, exit function with result `val` if not

#define ZERO_CHECK_RETURN(x)                             \
    {                                                    \
        int errchk_result = (int)(x);                    \
        if (errchk_result)                               \
        {                                                \
            LOG(E, "%s returned %d", #x, errchk_result); \
            return;                                      \
        }                                                \
    }  // verify that result is zero, exit function if not

#define ZERO_CHECK_RETURN_VAL(x)                         \
    do                                                   \
    {                                                    \
        int errchk_result = (int)(x);                    \
        if (errchk_result)                               \
        {                                                \
            LOG(E, "%s returned %d", #x, errchk_result); \
            return (x);                                  \
        }                                                \
    } while (0);  // verify that result is zero, exit function with result if not

// verify that result is zero, exit function with result 0 if not
#define ZERO_CHECK_RETURN0(x) ZERO_CHECK_RETURNX(0, (x))

// verify that result is zero, exit function with result 1 if not
#define ZERO_CHECK_RETURN1(x) ZERO_CHECK_RETURNX(1, (x))

#define NONNEG_CHECK(x)                                  \
    {                                                    \
        int errchk_result = (int)(x);                    \
        if (errchk_result < 0)                           \
            LOG(E, "%s returned %d", #x, errchk_result); \
    }  // verify that result is nonnegative

#define ASSERT(x)                                 \
    {                                             \
        int errchk_result = (int)(x);             \
        if (!errchk_result)                       \
            LOG(E, "%s assertion was false", #x); \
    }  // verify that expression is true

#define BOUNDED(x, h)                                                                \
    {                                                                                \
        int errchk_result = (int)(x);                                                \
        if (!((errchk_result >= 0) && (errchk_result < (h))))                        \
            LOG(E, "0<=(%s)<%s assertion false (%s=%d)", #x, #h, #x, errchk_result); \
    }  // verify that x is within [0,h)

#define RANGE(x, l, h)                                                                      \
    {                                                                                       \
        int errchk_result = (int)(x);                                                       \
        if (!((errchk_result >= (l)) && (errchk_result < (h))))                             \
            LOG(E, "%s<=(%s)<%s assertion false ((%s)=%d)", #l, #x, #h, #x, errchk_result); \
    }  // verify that x is within [l,h)

//! \~ Log color definitions
#if CONFIG_LOG_COLORS
#define LOG_COLOR_C LOG_COLOR(LOG_COLOR_CYAN)
#define LOG_COLOR_B LOG_COLOR(LOG_COLOR_BROWN)
#define LOG_COLOR_P LOG_COLOR(LOG_COLOR_PURPLE)
#else
#define LOG_COLOR_C
#define LOG_COLOR_B
#define LOG_COLOR_P
#endif

#define LOGC(format, ...)       esp_log_write(ESP_LOG_INFO, FILENAME, LOG_FORMAT(C, "%s(%" PRIu32 "): " format), esp_log_timestamp(), FILENAME, __func__, __LINE__, ##__VA_ARGS__)
#define LOGB(format, ...)       esp_log_write(ESP_LOG_INFO, FILENAME, LOG_FORMAT(B, "%s(%" PRIu32 "): " format), esp_log_timestamp(), FILENAME, __func__, __LINE__, ##__VA_ARGS__)
#define LOGP(format, ...)       esp_log_write(ESP_LOG_INFO, FILENAME, LOG_FORMAT(P, "%s(%" PRIu32 "): " format), esp_log_timestamp(), FILENAME, __func__, __LINE__, ##__VA_ARGS__)

//! \~ Extended LOG macros for different log levels as defined in esp_log.h
#define LOGE(format, ...)       ESP_LOGE(FILENAME, "%s(%" PRId16 "): " format, __func__, __LINE__, ##__VA_ARGS__)
#define LOGW(format, ...)       ESP_LOGW(FILENAME, "%s(%" PRId16 "): " format, __func__, __LINE__, ##__VA_ARGS__)
#define LOGI(format, ...)       ESP_LOGI(FILENAME, "%s(%" PRId16 "): " format, __func__, __LINE__, ##__VA_ARGS__)
#define LOGD(format, ...)       ESP_LOGD(FILENAME, "%s(%" PRId16 "): " format, __func__, __LINE__, ##__VA_ARGS__)
#define LOGV(format, ...)       ESP_LOGV(FILENAME, "%s(%" PRId16 "): " format, __func__, __LINE__, ##__VA_ARGS__)

//! \~ Log macro, prepends file and function where it is invoked
#define LOG(level, format, ...) LOG##level(format, ##__VA_ARGS__)

#endif  // NDEBUG

#endif  // IGENERALDEFINITIONS_H_
