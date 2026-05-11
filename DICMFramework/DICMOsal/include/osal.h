/*! \file osal.h
	\brief Operating System Abstraction Layer
*/

#ifndef OSAL_H_
#define OSAL_H_


#include <stdint.h>

#define FREE_RTOS

#ifdef FREE_RTOS
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#include "freertos/FreeRTOS.h"
#pragma GCC diagnostic pop

#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"

typedef SemaphoreHandle_t osal_mutex_handle_t;

typedef SemaphoreHandle_t osal_sem_handle_t;

typedef BaseType_t osal_base_type_t;

typedef UBaseType_t osal_ubase_type_t;

typedef QueueHandle_t osal_queue_handle_t;

typedef TickType_t osal_tick_type_t;

typedef StaticQueue_t osal_static_queue_t;

typedef TaskFunction_t osal_task_func_t;

typedef TaskHandle_t osal_task_handle_t;

enum
{
  OSAL_TIMEOUT_NOTIMEOUT    = 0,      // return immediately
  OSAL_TIMEOUT_NORMAL       = 10,     // default timeout
  OSAL_TIMEOUT_WAIT_FOREVER = 0xFFFFFFFFUL
};

#define osal_success pdTRUE

#define osal_fail pdFALSE

#define osal_queue_empty errQUEUE_EMPTY	

#define osal_queue_full errQUEUE_FULL	

#endif 

osal_base_type_t osal_semaphore_wait (osal_sem_handle_t sem_handle, uint32_t wait_time_msec);

osal_base_type_t osal_semaphore_post (osal_sem_handle_t sem_handle);

osal_queue_handle_t osal_queue_create_static(osal_base_type_t queue_len, osal_base_type_t item_size, uint8_t *queue_storage_buffer, osal_static_queue_t *static_queue);

osal_base_type_t osal_queue_receive (osal_queue_handle_t queue_handle, void *buffer, uint32_t wait_time_msec);

osal_base_type_t osal_queue_send (osal_queue_handle_t queue_handle, const void *item_to_queue, uint32_t wait_time_msec);

void osal_task_delay(uint32_t msec);

osal_mutex_handle_t osal_create_mutex(void);
 
osal_base_type_t osal_task_create(osal_task_func_t task_func, const char * const task_name, unsigned short stack_depth, void* parameters, osal_ubase_type_t task_priority, osal_task_handle_t *task_handle_t);

osal_queue_handle_t osal_queue_create(osal_base_type_t queue_len, osal_base_type_t item_size);

osal_base_type_t osal_queue_send_from_isr (osal_queue_handle_t queue_handle, const void * const item_to_queue, osal_base_type_t* const higher_prio_task_woken);


#endif /* OSAL_H_ */
