/*! \file
    \brief RTC watchdog

    This file contains interface of RTC watchdog. The responsibility of RTC watchdog is to
    measure RTC XTAL time constants and then deduce if components are valid or not. If the RTC
    XTAL components are valid, then the RTC XTAL oscillator will be enabled and used by the RTC
    peripheral.

    Use RTC_WATCHDOG_LOG_MEASURES macro to enable more verbose logging.

    \author Nenad Radulovic (nenad.radulovic@seavus.com)
*/

#ifndef RTC_WATCHDOG_H_
#define RTC_WATCHDOG_H_

#if defined(RTC_WATCHDOG_LOG_MEASURES)
#define RTC_WATCHDOG_LOG_MEASURES_ENABLED       1
#else
#define RTC_WATCHDOG_LOG_MEASURES_ENABLED       0
#endif

/**
 * @brief       Initialize RTC watchdog module
 *
 * This function will measure RTC XTAL time constants and then deduce if components are valid or
 * not. If the RTC  XTAL components are valid, then the RTC XTAL oscillator will be enabled and used
 * by the RTC peripheral.
 *
 * The execution time of this function is around 1s.
 *
 * @note        This function must be called before any application initialization takes place. This
 *              is because the function will chose which oscillator to use for RTC. Then the RTC
 *              is used by peripherals such as BLE and WiFi. Since RTC must be operable before the
 *              BLE and WiFi are operational this function must be called before they are
 *              initialized.
 * @note        Either disable FreeRTOS scheduler (or interrupts) or call this function before the
 *              application initializes any peripheral. This is needed to avoid task context
 *              switches during the measurement period.
 */
void rtc_watchdog_init(void);

#endif /* RTC_WATCHDOG_H_ */