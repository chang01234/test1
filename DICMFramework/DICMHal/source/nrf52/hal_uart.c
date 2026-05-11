/*! \file hal_uart.c
	\brief UART Hardware Abstraction Layer NRF implementation
 */

#include "hal_uart.h"
#include "sys_defs.h"
#include "nrf.h"
#include "bsp.h"
#include "nrf_drv_uart.h"
#include "nrf_uart.h"
#include "nrf_drv_timer.h"
#include "nrf_drv_ppi.h"

extern TaskHandle_t TaskHandle_uart;

#define RX_BUF_SIZE  200
#define MDM_UART_TIMEOUT (500)


static uint8_t *uart_rx_buffer;
static uint32_t amount;

nrf_drv_timer_t mdm_uart_timer = NRF_DRV_TIMER_INSTANCE(0);

nrf_ppi_channel_t ppi_channel;

nrf_drv_uart_t app_uart_inst = NRF_DRV_UART_INSTANCE(APP_UART_DRIVER_INSTANCE);


void mdm_uart_timer_event_handler(nrf_timer_event_t event_type, void* p_context)
{
	nrfx_uarte_rx_abort(&app_uart_inst.uarte);
	//(void)nrf_drv_uart_rx(&app_uart_inst, uart_rx_buffer, RX_BUF_SIZE);
}

static void uart_event_handler(nrf_drv_uart_event_t * p_event, void* p_context)
{

	uint32_t err_code;
	int i;
	switch (p_event->type)
	{
		case NRF_DRV_UART_EVT_RX_DONE:

			amount = nrf_uarte_rx_amount_get(app_uart_inst.uarte.p_reg);
#if 0
			for(i=0;i<amount;i++)
				printf("buffer[%d]=%x\n",i,uart_rx_buffer[i]);
			printf("READ:%s\n",uart_rx_buffer);
#endif
			if(p_event->data.rxtx.bytes == 0)
			{
				// printf("Task Resumed Zero bytes\n");
				// vTaskResume(TaskHandle_uart);
				// A new start RX is needed to continue to receive data
				memset(uart_rx_buffer, '\0', sizeof(uart_rx_buffer));
				(void)nrf_drv_uart_rx(&app_uart_inst, uart_rx_buffer, RX_BUF_SIZE);
				break;
			}
			
			vTaskResume(TaskHandle_uart);
			break;

		case NRF_DRV_UART_EVT_ERROR:
			printf("UART ERROR EVT:%x\n",p_event->data.error.error_mask);
			(void)nrf_drv_uart_rx(&app_uart_inst, uart_rx_buffer, RX_BUF_SIZE);
			break;

		case NRF_DRV_UART_EVT_TX_DONE:
			printf("Tx Done\n");
			break;

		default:
			break;
	}
}

int32_t hal_uart_read (uint8_t *data, size_t length)
{

	uart_rx_buffer = data;
	nrf_drv_uart_rx(&app_uart_inst, uart_rx_buffer, RX_BUF_SIZE);
	printf("Task suspended amount:%d\n",amount);

	vTaskSuspend(NULL);
	return amount;
}

int32_t hal_uart_write ( const char *data, size_t length)
{
	return nrf_drv_uart_tx(&app_uart_inst,data,length);
}


static void mdm_timer_init()
{
	nrf_drv_timer_config_t timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
	timer_cfg.frequency = NRF_TIMER_FREQ_16MHz;

	nrf_drv_timer_init(&mdm_uart_timer, &timer_cfg, mdm_uart_timer_event_handler);

	nrf_drv_timer_compare(&mdm_uart_timer,NRF_TIMER_CC_CHANNEL0,   
			nrf_drv_timer_ms_to_ticks(&mdm_uart_timer,MDM_UART_TIMEOUT),
			true );
	nrf_drv_timer_enable(&mdm_uart_timer);
}

static void mdm_ppi_init()
{

	nrf_drv_ppi_init();

	nrf_drv_ppi_channel_alloc(&ppi_channel);

	nrf_drv_ppi_channel_assign(ppi_channel, 
			nrf_uarte_event_address_get(app_uart_inst.uarte.p_reg, NRF_UARTE_EVENT_RXDRDY), 
			nrf_timer_task_address_get(mdm_uart_timer.p_reg,NRF_TIMER_TASK_CLEAR));

	nrf_drv_ppi_channel_enable(ppi_channel);

}

static void mdm_config_uart()
{
	nrf_drv_uart_config_t config = NRF_DRV_UART_DEFAULT_CONFIG; 

	config.baudrate = NRF_UART_BAUDRATE_115200;
	//config.baudrate = NRF_UART_BAUDRATE_38400;
	config.hwfc = NRF_UART_HWFC_DISABLED;
	//config.interrupt_priority =APP_IRQ_PRIORITY_LOWEST;
	config.interrupt_priority =APP_IRQ_PRIORITY_HIGHEST;
	//config.interrupt_priority =APP_IRQ_PRIORITY_HIGH;
	config.parity = NRF_UART_PARITY_EXCLUDED;
	config.pselcts = NRF_UART_PSEL_DISCONNECTED;
	config.pselrts = NRF_UART_PSEL_DISCONNECTED;
	config.pselrxd = RX_PIN_NUMBER;
	config.pseltxd = TX_PIN_NUMBER;

	nrf_drv_uart_init(&app_uart_inst, &config, uart_event_handler);
}

int32_t hal_uart_init (uint32_t baudrate,void (*rx_listener)(void *))
{
	mdm_ppi_init();

	mdm_config_uart();

	mdm_timer_init();	

	xTaskCreate(rx_listener, "uart_rx_data", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES, &TaskHandle_uart);

	return 0;
}
