
#include "iGeneralDefinitions.h"
#include "eds_port_definition.h"
#include "eds_port.h"
#include "eds.h"

#include <assert.h>

#include "freertos/timers.h"

#define ALIGN_UP(num, align)                                       \
        (((num) + (align) - 1u) & ~((align) - 1u))

#define MS_PER_TICK                      100

#if (CRITICAL_METHOD == CRITICAL_METHOD_MUTEX)
static SemaphoreHandle_t critical_mutex = NULL;
static StaticSemaphore_t critical_mutex_buffer;
#endif

static void eds__tick_handler(TimerHandle_t handle)
{
	(void)handle;
	eds__tick_process_all();
}

void eds_port__sleep_init(struct eds_port__sleep *sleep)
{
    sleep->handle = xSemaphoreCreateBinaryStatic(&sleep->sem);
    ASSERT(sleep->handle != NULL);
}

void eds_port__sleep_wait(struct eds_port__sleep *sleep)
{
    BaseType_t retval;

    retval = xSemaphoreTake(sleep->handle, portMAX_DELAY);
    ASSERT(retval == pdTRUE);
}

void eds_port__sleep_signal(struct eds_port__sleep *sleep)
{
    /* Ignore return value, we are not interested in queue full errors :-? */
    (void)xSemaphoreGive(sleep->handle);
}

void eds_port__critical_lock(struct eds_port__critical *critical)
{
#if (CRITICAL_METHOD == CRITICAL_METHOD_ISR)
    vPortCPUInitializeMutex(&critical->mux);
    taskENTER_CRITICAL(&critical->mux);
#elif (CRITICAL_METHOD == CRITICAL_METHOD_MUTEX)
    static bool is_initialized = false;
    BaseType_t retval;

    if (!is_initialized)
    {
        critical_mutex = xSemaphoreCreateMutexStatic(&critical_mutex_buffer);
        assert(critical_mutex != NULL);
        is_initialized = true;
    }
    retval = xSemaphoreTake(critical_mutex, portMAX_DELAY);
    ASSERT(retval == pdTRUE);
#endif
}

void eds_port__critical_unlock(struct eds_port__critical *critical)
{
#if (CRITICAL_METHOD == CRITICAL_METHOD_ISR)
    taskEXIT_CRITICAL(&critical->mux);
#elif (CRITICAL_METHOD == CRITICAL_METHOD_MUTEX)
    xSemaphoreGive(critical_mutex);
#endif
}

uint_fast8_t eds_port__ffs(uint32_t value)
{
    return 31u - __builtin_clz(value);
}

size_t eds_port__align_up(size_t non_aligned_value)
{
    return ALIGN_UP(non_aligned_value, sizeof(void *));
}

uint32_t eds_port__tick_duration_ms(void)
{
    return MS_PER_TICK;
}

uint32_t eds_port__tick_from_ms(uint32_t ms)
{
    return ms / MS_PER_TICK;
}

void eds_port__init(void)
{
    TimerHandle_t sm_timer;

    sm_timer = xTimerCreate(NULL, pdMS_TO_TICKS(MS_PER_TICK), pdTRUE, NULL, eds__tick_handler);
    TRUE_CHECK_RETURN(sm_timer != NULL);
	TRUE_CHECK(xTimerStart(sm_timer, portMAX_DELAY));
}

