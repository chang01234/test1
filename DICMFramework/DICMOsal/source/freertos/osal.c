/*! \file osal.c
	\brief Operating System Abstraction Layer
*/

#include "osal.h"


/*===========================================================================*/
/*       osal_semaphore_wait                                                 */
/*===========================================================================*/
osal_base_type_t osal_semaphore_wait (osal_sem_handle_t sem_handle, uint32_t wait_time_msec)
{
	osal_base_type_t ret = 0;
	
	uint32_t const ticks = (wait_time_msec == OSAL_TIMEOUT_WAIT_FOREVER) ? portMAX_DELAY : pdMS_TO_TICKS(wait_time_msec);
	
	ret =  xSemaphoreTake(sem_handle, ticks);
	
	return ret;
}

/*===========================================================================*/
/*       osal_semaphore_post                                                  */
/*===========================================================================*/
osal_base_type_t osal_semaphore_post (osal_sem_handle_t sem_handle)
{
	osal_base_type_t ret = 0;
	
	ret = xSemaphoreGive(sem_handle);

	return ret;
}

/*===========================================================================*/
/*       osal_queue_receive                                                  */
/*===========================================================================*/
osal_base_type_t osal_queue_receive (osal_queue_handle_t queue_handle, void *buffer, uint32_t  wait_time_msec)
{
	osal_base_type_t ret = 0;

	uint32_t const ticks = (wait_time_msec == OSAL_TIMEOUT_WAIT_FOREVER) ? portMAX_DELAY : pdMS_TO_TICKS(wait_time_msec);

	ret = xQueueReceive(queue_handle, buffer, ticks);

	return ret;
}

/*===========================================================================*/
/*       osal_queue_send                                                     */
/*===========================================================================*/
osal_base_type_t osal_queue_send (osal_queue_handle_t queue_handle, const void *item_to_queue, uint32_t wait_time_msec)
{
	osal_base_type_t ret = 0;

	uint32_t const ticks = (wait_time_msec == OSAL_TIMEOUT_WAIT_FOREVER) ? portMAX_DELAY : pdMS_TO_TICKS(wait_time_msec);

	ret = xQueueSendToBack(queue_handle, item_to_queue, ticks);
	
	return ret;
}

/*===========================================================================*/
/*       osal_queue_send_from_isr                                            */
/*===========================================================================*/
osal_base_type_t osal_queue_send_from_isr (osal_queue_handle_t queue_handle, const void * const item_to_queue, osal_base_type_t* const higher_prio_task_woken)
{
	BaseType_t  ret = 0;

  	ret = xQueueSendToBackFromISR(queue_handle, item_to_queue, higher_prio_task_woken );  

	return ret;
}

/*===========================================================================*/
/*       osal_task_delay                                                     */
/*===========================================================================*/
void osal_task_delay(uint32_t msec)
{
	vTaskDelay( pdMS_TO_TICKS(msec) );
}

/*===========================================================================*/
/*       osal_queue_create_static                                            */
/*===========================================================================*/
osal_queue_handle_t osal_queue_create_static(osal_base_type_t queue_len, osal_base_type_t item_size, uint8_t *queue_storage_buffer, osal_static_queue_t *static_queue)
{
	osal_queue_handle_t que_handle_t;

    que_handle_t = xQueueCreateStatic(queue_len, item_size, queue_storage_buffer, static_queue);

	return que_handle_t;
}

/*===========================================================================*/
/*       osal_queue_create                                                   */
/*===========================================================================*/
osal_queue_handle_t osal_queue_create(osal_base_type_t queue_len, osal_base_type_t item_size)
{
	osal_queue_handle_t que_handle_t;

    que_handle_t = xQueueCreate(queue_len, item_size);

	return que_handle_t;
}

/*===========================================================================*/
/*       osal_create_mutex                                                   */
/*===========================================================================*/
osal_mutex_handle_t osal_create_mutex()
{
  osal_mutex_handle_t mutex_handle_t = xSemaphoreCreateMutex();

  return mutex_handle_t;
}

/*===========================================================================*/
/*       osal_task_create                                                    */
/*===========================================================================*/
osal_base_type_t osal_task_create(osal_task_func_t task_func, const char * const task_name, unsigned short stack_depth, void* parameters, osal_ubase_type_t task_priority, osal_task_handle_t *task_handle_t)
{
	osal_base_type_t ret = 0;
	
    ret = xTaskCreate( task_func, task_name, stack_depth, parameters, task_priority, task_handle_t);

	return ret;
}