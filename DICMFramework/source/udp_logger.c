/*! \file udp_logger.c
	\brief Remote logger

	Logger that sends data to a remote host.
 */

#include "configuration.h"

//#ifdef UDP_LOGGER
#ifndef CONFIG_IDF_TARGET_LINUX

#include "freertos/FreeRTOS.h"
#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"

#include "udp_logger.h"
#include "hal_mem.h"

static volatile int socket_fd = -1;
static int socket_logger;
static struct sockaddr_in addr_logger;
static SemaphoreHandle_t udp_logger_mutex;
int udp_logger_registered = 0;

void initialize_udp_logger(void)
{
	udp_logger_mutex=xSemaphoreCreateMutex();
}

int udp_printf(const char *fmt, ...)
{
	va_list args;
	int printed = 0;
	char *sentry;

	TRUE_CHECK(xSemaphoreTake(udp_logger_mutex,portMAX_DELAY));
	{
		sentry = hal_mem_malloc_prefer(512, HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
		assert(sentry != NULL);
		va_start(args, fmt);
		printed = vsprintf(&sentry[1], fmt, args);
		va_end(args);

		sentry[0] = 0x20;	
		(void)sendto(socket_logger, sentry, printed+1, 0, (struct sockaddr *)&addr_logger, sizeof(addr_logger));
		free(sentry);

		TRUE_CHECK(xSemaphoreGive(udp_logger_mutex));
	}
	return printed;
}

void register_addr_udp_logger(int socket, struct sockaddr_in *addr)
{
	udp_logger_registered = 1;
	socket_logger = socket;
	memcpy(&addr_logger, addr, sizeof(addr_logger));
}

//#endif
#endif

