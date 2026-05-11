/*! \file connector_system.c
	\brief Connector for time and other system devices
	\author	Sebastien Fortin
	\author	Jens Björnhager
*/

#include "configuration.h"

#include <string.h>
#include <time.h>
#include <sys/time.h>
#ifndef CONFIG_IDF_TARGET_LINUX
#include "esp_sntp.h"
#endif
#include "connector_system.h"
#include "ddm_wrapper.h"
#include "utils.h"
#include "hal_rtc.h"
#include "hal_gpio.h"

#ifdef CONFIG_HEAP_TASK_TRACKING
#include "esp_heap_task_info.h"
#endif

#ifndef CONNECTOR_SYSTEM_STAT_ENABLE
#define CONNECTOR_SYSTEM_STAT_ENABLE 0	// Set to 1 to get FreeRTOS task stat printout. Requires sdkconfig changes
#endif

#ifndef CONNECTOR_SYSTEM_NTP_SERVER
#define CONNECTOR_SYSTEM_NTP_SERVER ""
#endif

#if (CONNECTOR_SYSTEM_STAT_ENABLE != 0)
#if !defined(configGENERATE_RUN_TIME_STATS) || (defined(configGENERATE_RUN_TIME_STATS) && (configGENERATE_RUN_TIME_STATS == 0))
#error "Enable CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS in sdkconfig"
#endif
#if !defined(configUSE_STATS_FORMATTING_FUNCTIONS) || (defined(configUSE_STATS_FORMATTING_FUNCTIONS) && (configUSE_STATS_FORMATTING_FUNCTIONS == 0))
#error "Enable CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS in sdkconfig"
#endif
#if !defined(configUSE_TRACE_FACILITY) || (defined(configUSE_TRACE_FACILITY) && (configUSE_TRACE_FACILITY == 0))
#error "Enable CONFIG_FREERTOS_USE_TRACE_FACILITY in sdkconfig"
#endif
#if !defined(CONFIG_HEAP_TASK_TRACKING)
#error "Enable CONFIG_HEAP_TASK_TRACKING in sdkconfig"
#endif
#endif //(CONNECTOR_SYSTEM_STAT_ENABLE != 0)

#define TIMER_INTERVAL_SYSTEM_TIME_MS			1000
#define TIMER_INTERVAL_SYSTEM_HEALTH_MS			5000
#define TIMER_INTERVAL_SYSTEM_STAT_MS			10000

//! \~ DDM Wrapper data
typedef struct 
{	
	ddmw_t ddm;

	struct
	{
		ddmw_item_t syst;
		ddmw_item_t tmz;
		ddmw_item_t ntps;
		ddmw_item_t tsrc;
		ddmw_item_t gpior;
		ddmw_item_t gpiow;
		ddmw_item_t systupd;
	} svc;		 
} connector_system_t;

static EXT_RAM_ATTR TimerHandle_t timer_handle[3];

static EXT_RAM_ATTR connector_system_t connector_sys;				//!< \~ Export connector
static EXT_RAM_ATTR char ntp_server[DDMW_ITEM_DATA_CAPACITY + 1];	//!< \~ External storage for ntp server string + NULL

#if CONNECTOR_SYSTEM_STAT_ENABLE
#define MAX_TASK_NUM 20                         // Max number of per tasks info that it can store
#define MAX_BLOCK_NUM 10                        // Max number of per block info that it can store

static size_t s_prepopulated_num = 0;
static heap_task_totals_t s_totals_arr[MAX_TASK_NUM];
//static heap_task_block_t s_block_arr[MAX_BLOCK_NUM];

static void esp_dump_per_task_heap_info(int32_t caps, const char* label)
{
    static heap_task_info_params_t heap_info = {0};
    heap_info.caps[0] = MALLOC_CAP_8BIT|MALLOC_CAP_INTERNAL; // MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM;        			// Gets heap with CAP_8BIT capabilities
    heap_info.mask[0] = MALLOC_CAP_8BIT|MALLOC_CAP_INTERNAL;
    heap_info.caps[1] = MALLOC_CAP_32BIT|MALLOC_CAP_INTERNAL; // MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM;                    // Gets heap with CAP_8BIT capabilities
    heap_info.mask[1] = MALLOC_CAP_32BIT|MALLOC_CAP_INTERNAL;
    heap_info.caps[2] = MALLOC_CAP_SPIRAM; // MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM;                    // Gets heap with CAP_8BIT capabilities
    heap_info.mask[2] = MALLOC_CAP_SPIRAM;

    heap_info.tasks = NULL;                     // Passing NULL captures heap info for all tasks
    heap_info.num_tasks = 0;
    heap_info.totals = s_totals_arr;            // Gets task wise allocation details
    heap_info.num_totals = &s_prepopulated_num;
    heap_info.max_totals = MAX_TASK_NUM;        // Maximum length of "s_totals_arr"
    heap_info.blocks = NULL;//s_block_arr;             // Gets block wise allocation details. For each block, gets owner task, address and size
    heap_info.max_blocks = 0;//MAX_BLOCK_NUM;       // Maximum length of "s_block_arr"

    heap_caps_get_per_task_info(&heap_info);

    for (int i = 0 ; i < (int)*heap_info.num_totals; i++) {
		if (heap_info.totals[i].size[0] + heap_info.totals[i].size[1] > 0) {
			LOG(I, "Task: %24s -> Int CAP_8BIT: %8d Int CAP_32BIT: %8d CAP_SPIRAM: %8d",
					heap_info.totals[i].task ? pcTaskGetTaskName(heap_info.totals[i].task) : "Pre-Scheduler allocs" ,

//					label,
					heap_info.totals[i].size[0],
                    heap_info.totals[i].size[1],
                    heap_info.totals[i].size[2]
					);
		}
	}
	//printf("\n");
}
#endif /* CONNECTOR_SYSTEM_STAT_ENABLE */

static void connector_system_timer_callback(TimerHandle_t xTimer)
{
	uint32_t event_id = pdTICKS_TO_MS(xTimerGetPeriod(xTimer));
	ddmw_send_generic_event_data(&connector_sys.ddm, event_id, NULL, 0);
}

/*! \brief Update RTC with supplied posix time
	\param[in] sys_time posix time to set
 */
static void update_rtc(const time_t time_posix)
{
    struct tm system_time_buffer;
	struct tm *system_time = gmtime_r(&time_posix, &system_time_buffer);
	char time_string[32] = { 0 };

	if (system_time)
	{
		const hal_err_t Rtc_set_restult = hal_rtc_set_datetime(system_time);
		
	    if (Rtc_set_restult == HAL_E_OK)
	    {
	        LOG(I, "Updated RTC time to %s", time_string);
	    }
	    else if (Rtc_set_restult != HAL_E_DEVICE)
	    {
	        LOG(E, "Failed to set RTC time");
	    }
	}
	else
	{
		LOG(E, "gmtime_r failed (%ld)", time_posix);
	}
}

#ifndef CONFIG_IDF_TARGET_LINUX
/*! \brief SNTP sync time callback
	\param[in] tv Time received from SNTP
 */
static void sntp_sync_time_cb(struct timeval * const tv)
{
	char time_string[32] = { 0 };

	connector_system_get_local_time_string(time_string, sizeof(time_string), NULL);

	LOG(I, "SNTP set time to %s (%ld)", time_string, tv->tv_sec);

	update_rtc(tv->tv_sec);
	ddmw_set_i32(&connector_sys.svc.tsrc, SVC0TSRC_NTP);
	ddmw_set_i32(&connector_sys.svc.systupd, tv->tv_sec);
}
#endif

/*! \brief Set system time from RTC
 */
static hal_err_t read_rtc(void)
{
	HAL_RTC_DATE_TIME rtc_time;
	hal_err_t rtc_err = hal_rtc_read(&rtc_time);
	char time_string[32] = { 0 };

	switch (rtc_err)
	{
	case HAL_E_OK:
	{
		struct tm sys_time =
		{
			.tm_year = rtc_time.year - 1900,
			.tm_mon = rtc_time.month - 1,
			.tm_mday = rtc_time.day,
			.tm_wday = rtc_time.weekday - 1,
			.tm_hour = rtc_time.hour,
			.tm_min = rtc_time.minute,
			.tm_sec = rtc_time.second,
			.tm_yday = 0,
			.tm_isdst = 0,
		};

		time_t posix_time = mktime(&sys_time);
		struct timeval tv =	{ .tv_sec = posix_time,	};
	
		settimeofday(&tv, NULL);
		ddmw_set_i32(&connector_sys.svc.syst, posix_time);
		ddmw_set_i32(&connector_sys.svc.systupd, posix_time);
		ddmw_set_i32(&connector_sys.svc.tsrc, SVC0TSRC_RTC);

		connector_system_get_local_time_string(time_string, sizeof(time_string), NULL);
		
		LOG(I, "Updated time from RTC to %s (%ld)", time_string, posix_time);
	}
		break;
	case HAL_E_DEVICE:
		break;
	default:
		LOG(E, "RTC reading error");
		break;
	}

	return rtc_err;
}

/*! \brief Connector initialization
 */
static int connector_system_init(void)
{
	int instance;

	ZERO(ntp_server);
	memset(&connector_sys, 0, sizeof(connector_sys));
	ddmw_init(&connector_sys.ddm, &connector_system);

	instance = ddmw_register(&connector_sys.ddm, SVC0);

	ddmw_add(&connector_sys.ddm, &connector_sys.svc.syst, SVC0SYST, instance);
	ddmw_add(&connector_sys.ddm, &connector_sys.svc.tmz, SVC0TMZ, instance);
	ddmw_add(&connector_sys.ddm, &connector_sys.svc.ntps, SVC0NTPS, instance);
	ddmw_add(&connector_sys.ddm, &connector_sys.svc.tsrc, SVC0TSRC, instance);
	ddmw_add(&connector_sys.ddm, &connector_sys.svc.gpior, SVC0GPIOR, instance);
	ddmw_add(&connector_sys.ddm, &connector_sys.svc.gpiow, SVC0GPIOW, instance);
	ddmw_add(&connector_sys.ddm, &connector_sys.svc.systupd, SVC0SYSTUPD, instance);

	ddmw_set_i32(&connector_sys.svc.syst, 0);
	ddmw_load_i32(&connector_sys.svc.tmz, "svc", 0, "tmz", 0);
	ddmw_load_str(&connector_sys.svc.ntps, "svc", 0, "ntps", CONNECTOR_SYSTEM_NTP_SERVER);
	ddmw_get_data(&connector_sys.svc.ntps, ntp_server, sizeof(ntp_server));
	ddmw_set_i32(&connector_sys.svc.tsrc, SVC0TSRC_NONE);
	ddmw_set_has_no_value(&connector_sys.svc.gpior, true);
	ddmw_set_has_no_value(&connector_sys.svc.gpiow, true);
	ddmw_set_has_no_value(&connector_sys.svc.systupd, true);

	// Set time zone (UTC)
	setenv("TZ", "UTC0", 1);
	tzset();
	
	read_rtc();

	// Initiate connection to time server
#ifndef CONFIG_IDF_TARGET_LINUX
	sntp_set_time_sync_notification_cb(sntp_sync_time_cb);
	esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
	esp_sntp_setservername(0, ntp_server[0] ? ntp_server : NULL);
	esp_sntp_init();
#endif
	TRUE_CHECK((timer_handle[0] = xTimerCreate(NULL, pdMS_TO_TICKS(TIMER_INTERVAL_SYSTEM_TIME_MS),   pdTRUE, NULL, connector_system_timer_callback)) != NULL);
	TRUE_CHECK((timer_handle[1] = xTimerCreate(NULL, pdMS_TO_TICKS(TIMER_INTERVAL_SYSTEM_HEALTH_MS), pdTRUE, NULL, connector_system_timer_callback)) != NULL);
#if CONNECTOR_SYSTEM_STAT_ENABLE
	TRUE_CHECK((timer_handle[2] = xTimerCreate(NULL, pdMS_TO_TICKS(TIMER_INTERVAL_SYSTEM_STAT_MS),   pdTRUE, NULL, connector_system_timer_callback)) != NULL);
#endif // CONNECTOR_SYSTEM_STAT_ENABLE

	TRUE_CHECK(xTimerStart(timer_handle[0], portMAX_DELAY));
	TRUE_CHECK(xTimerStart(timer_handle[1], portMAX_DELAY));
#if CONNECTOR_SYSTEM_STAT_ENABLE
	TRUE_CHECK(xTimerStart(timer_handle[2], portMAX_DELAY));
#endif // CONNECTOR_SYSTEM_STAT_ENABLE

	return 1;
}

/*! \brief Check system health, connector subtask
	\param[in] time_interval time_interval provided by the timers
 */
static void connector_system_task_health(uint32_t time_interval)
{
	multi_heap_info_t heap_info;
	if (time_interval == TIMER_INTERVAL_SYSTEM_HEALTH_MS)
	{
		heap_caps_get_info(&heap_info, MALLOC_CAP_8BIT|MALLOC_CAP_INTERNAL);
		uint32_t totalFreeBytesIRAM = heap_info.total_free_bytes;
		uint32_t FreeInternalRAM = heap_info.minimum_free_bytes;
		uint32_t LargestFreeInternalRAMBlock = heap_info.largest_free_block;
#if CONFIG_ESP32_SPIRAM_SUPPORT
		heap_caps_get_info(&heap_info, MALLOC_CAP_SPIRAM);
		uint32_t FreePSRAM = heap_info.minimum_free_bytes;
#endif
		if (LargestFreeInternalRAMBlock < (4 * 1024) + 128)
		{
			LOG(E, "Largest free internal RAM block=%d", LargestFreeInternalRAMBlock);
		}
		
		if (totalFreeBytesIRAM < 4096)
		{
			LOG(E, "Free internal RAM=%d", totalFreeBytesIRAM);
		}

		if (FreeInternalRAM < 1024)
		{
			LOG(E, "Lifetime minimum free internal RAM=%d", FreeInternalRAM);
		}
#if CONFIG_ESP32_SPIRAM_SUPPORT
		if (FreePSRAM < 64 * 1024)
		{
			LOG(E, "Lifetime minimum free PSRAM=%d", FreePSRAM);
		}
#endif
	}

#if CONNECTOR_SYSTEM_STAT_ENABLE
	if (time_interval == TIMER_INTERVAL_SYSTEM_STAT_MS)		// Every 10 seconds
	{
		// Requires activation of FreeRTOS statistic (option)
#if (CONNECTOR_SYSTEM_STAT_ENABLE == 1) || (CONNECTOR_SYSTEM_STAT_ENABLE == 2)
		static EXT_RAM_ATTR char buffer[1536];
#endif
		const int min_free_8bit_cap = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
		const int min_free_32bit_cap = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_32BIT);
		const int min_free_spiram = heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM);

		esp_dump_per_task_heap_info(MALLOC_CAP_INTERNAL|MALLOC_CAP_32BIT, "IRAM");
		//esp_dump_per_task_heap_info(MALLOC_CAP_SPIRAM, "External");
		LOG(I, "System Heap Utilisation Stats:\n");
		LOG(I, "||   Miniumum Free DRAM\t|   Minimum Free IRAM\t|   Minimum Free SPIRAM\t|| \n");
		LOG(I, "||\t%-6d\t\t|\t%-6d\t\t|\t%-6d\t\t||\n", min_free_8bit_cap, (min_free_32bit_cap - min_free_8bit_cap), min_free_spiram);
		LOG(I, "Heap size left: %d\n", esp_get_free_heap_size());
#if (CONNECTOR_SYSTEM_STAT_ENABLE == 1)
		vTaskGetRunTimeStats(buffer);
		LOG(I, "\n%s", buffer);
#elif (CONNECTOR_SYSTEM_STAT_ENABLE == 2)
		vTaskList(buffer);
		LOG(I, "\n%s", buffer);
#endif
	}
#endif //CONNECTOR_SYSTEM_STAT_ENABLE
}

/*! \brief Get local time (System time + timezone offset)
	\param[in] elapsed Milliseconds elapsed since last call
 */
bool connector_system_get_local_time(struct tm * const date_time)
{
	// Get system time
    bool ret_value = false;
    int32_t tmz;
	struct timeval tv_now;

	if (!gettimeofday(&tv_now, NULL))
	{
		time_t time_posix = tv_now.tv_sec;

		// Convert to local
        tmz = ddmw_get_i32(&connector_sys.svc.tmz);
        tmz /= 1000;  /* tmz unit is milliseconds, unit must be seconds */
		time_posix += tmz;

		// Decode
		struct tm my_time_info;
		struct tm *time_info = gmtime_r(&time_posix, &my_time_info);

		if (time_info)
		{
		    *date_time = *time_info;
		    ret_value = true;
		}
	}

	return ret_value;
}

int connector_system_posix_time_string(const time_t posix_time, char * const time_string, const size_t time_string_size)
{
	struct tm local_time;
	struct tm *sys_time = gmtime_r(&posix_time, &local_time);

	if (sys_time)
	{
		strftime(time_string, time_string_size, "%Y-%m-%d %H:%M:%S", sys_time);
		return 1;
	}

	return 0;
}

/*! \brief Get local time string (System time + timezone offset)
	\param[out] time_string Buffer to store the time string
	\param[in] time_string_size Size of the buffer
	\param[out] local_time Pointer to a tm structure to store the local time (optional)
	\return Size of the resulting string
 */
size_t connector_system_get_local_time_string(char * const time_string, const size_t time_string_size, struct tm * const local_time)
{
	struct tm time;
	struct tm * const Ptime = (local_time != NULL) ? local_time : &time;

	if (connector_system_get_local_time(Ptime))
	{
		return strftime(time_string, time_string_size, "%Y-%m-%d %H:%M:%S", Ptime);
	}

	return 0;
}

/*! \brief Set system time from POSIX timestamp
	\param[in] posix_time POSIX time to set system time to
 */
int connector_system_set_system_time_posix(const time_t posix_time)
{
	struct timeval tv =	{ .tv_sec = posix_time,	};

	const int Settimeofday_result = settimeofday(&tv, NULL);
	
	if (!Settimeofday_result)
	{
		ddmw_set_i32(&connector_sys.svc.syst, posix_time);
		ddmw_set_i32(&connector_sys.svc.tsrc, SVC0TSRC_EXTERNAL);
		ddmw_set_i32(&connector_sys.svc.systupd, posix_time);
		update_rtc(posix_time);
		
		char time_string[32] = { 0 };

		connector_system_get_local_time_string(time_string, sizeof(time_string), NULL);

		LOG(I, "Updated time from external source: %s (%ld)", time_string, posix_time);
	} 

	return Settimeofday_result;
}

static SVC0GPIOR_ENUM attempt_gpio_read(const uint32_t gpio_number)
{
	HAL_GPIO_RESULT_ENUM err;
	int level = 0;
	err = hal_gpio_getlevel(HAL_GPIO_DEVICE_ESP32, 0, gpio_number, &level);

	if (HAL_GPIO_OK != err)
	{
	    return SVC0GPIOR_INPUT_PIN_OUT_OF_RANGE;
	}

	return level;
}

static SVC0GPIOR_ENUM attempt_gpio_write(const uint32_t gpio_number, const int32_t gpio_value)
{
//	if ((gpio_number >= GPIO_NUM_MAX) || !GPIO_IS_VALID_OUTPUT_GPIO(gpio_number))
//	{
//		return SVC0GPIOR_OUTPUT_PIN_OUT_OF_RANGE;
//	}

    HAL_GPIO_RESULT_ENUM set_result = hal_gpio_setlevel(HAL_GPIO_DEVICE_ESP32, 0, gpio_number, (uint32_t)gpio_value);

	if (set_result != HAL_GPIO_OK)
	{
		return SVC0GPIOR_OUTPUT_PIN_OUT_OF_RANGE;
	}
	else
	{
	    return SVC0GPIOR_LOW;
	}
}

/*! \brief Connector task
	\param[in] frame provided by connector task
 */
static void connector_system_task(const DDMP2_FRAME * frame)
{
	ddmw_process(&connector_sys.ddm, frame);
	ddmw_save_if_updated_i32(&connector_sys.svc.tmz, "svc", 0, "tmz");

	if (ddmw_is_generic_event_updated(&connector_sys.ddm))
	{
		uint32_t event_id = ddmw_get_generic_event_id(&connector_sys.ddm);
		switch (event_id)
		{
		case TIMER_INTERVAL_SYSTEM_TIME_MS:
		{
			struct timeval tv_now;
			gettimeofday(&tv_now, NULL);
			ddmw_set_i32(&connector_sys.svc.syst, tv_now.tv_sec);
			break;
		}
		case TIMER_INTERVAL_SYSTEM_HEALTH_MS:
		{
			connector_system_task_health(TIMER_INTERVAL_SYSTEM_HEALTH_MS);
			break;
		}
		case TIMER_INTERVAL_SYSTEM_STAT_MS:
		{
			connector_system_task_health(TIMER_INTERVAL_SYSTEM_STAT_MS);
			break;
		}
		default:
			LOG(E, "Invalid timer interval!");
			break;
		}
	}

	if (connector_sys.svc.ntps.updated)
	{
		ddmw_save_if_updated_str(&connector_sys.svc.ntps, "svc", 0, "ntps");
		ddmw_get_data(&connector_sys.svc.ntps, ntp_server, sizeof(ntp_server));
#ifndef CONFIG_IDF_TARGET_LINUX
		esp_sntp_setservername(0, ntp_server[0] ? ntp_server : NULL);
#endif
	}

	if (ddmw_is_updated(&connector_sys.svc.syst))
	{
		time_t posix_time = ddmw_get_i32(&connector_sys.svc.syst);
		struct timeval tv =	{ .tv_sec = posix_time,	};

		const int Settimeofday_result = settimeofday(&tv, NULL);

		if (!Settimeofday_result)
		{
			ddmw_set_i32(&connector_sys.svc.tsrc, SVC0TSRC_SET);
			ddmw_set_i32(&connector_sys.svc.systupd, tv.tv_sec);
			update_rtc(posix_time);
		}
	}

	if (ddmw_is_updated(&connector_sys.svc.gpior))
	{
		const int32_t gpio_number = ddmw_get_i32(&connector_sys.svc.gpior);
		const SVC0GPIOR_ENUM gpior = attempt_gpio_read(gpio_number);
		ddmw_set_i32(&connector_sys.svc.gpior, gpior);
	}

	if (ddmw_is_updated(&connector_sys.svc.gpiow))
	{
		const int32_t gpio_write = ddmw_get_i32(&connector_sys.svc.gpiow);
		const int32_t gpio_value = (gpio_write >= 0);
		const int32_t gpio_number = gpio_value ? gpio_write : ~gpio_write;
		const SVC0GPIOR_ENUM gpiow = attempt_gpio_write(gpio_number, gpio_value);
		ddmw_set_i32(&connector_sys.svc.gpiow, gpiow);
	}

	// process modified parameters
	ddmw_process_publish(&connector_sys.ddm);
}

//! \~ Connector data
CONNECTOR connector_system =
{
	.name = "System services connector",
	.initialize = connector_system_init,
	.process_event = connector_system_task,
};
