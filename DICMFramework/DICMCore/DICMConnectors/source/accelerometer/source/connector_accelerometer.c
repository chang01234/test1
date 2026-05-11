/*****************************************************************************
 * \file       connector_accelerometer.c
 * \brief      Connector for Accelerometer LIS2DH12
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
#include "configuration.h"
#include "connector_accelerometer.h"
#include "ddm_wrapper.h"
#include "utils.h"
#include "hal_rtc.h"
#include "drv_lis2dh12.h"


/**********************************************************
 * Private defines
 *********************************************************/
#define LIS2DH12_TIMER_POLL_EVENT			(0x01)
#define LIS2DH12_TIMER_PERIOD_MS			(1000)

/**********************************************************
 * Private types
 *********************************************************/
typedef struct _lis2dh12_param_t
{
	// DDM
	ddmw_item_t avl;   		// Available
	ddmw_item_t id;    		// Acceleromter ID
	ddmw_item_t temp;     	// Temperature
	ddmw_item_t coordx; 	// Accleration x data in mg
	ddmw_item_t coordy;   	// Accleration y data in mg
	ddmw_item_t coordz;   	// Accleration z data in mg
	ddmw_item_t event; 		// Enable Event anf get Event
} accelerometer_param_t;

typedef struct _connector_accelerometer_t
{
	// DDM wrapper
	ddmw_t ddm;
	accelerometer_param_t items;
} connector_accelerometer_t;

static EXT_RAM_ATTR connector_accelerometer_t accelerometer_event;

/**********************************************************
 * Private functions
 *********************************************************/
static int		connector_accelerometer_init(void);
static void     connector_accelerometer_task(const DDMP2_FRAME * frame);
static void     connector_accelerometer_register(accelerometer_param_t* acc_param);
static void 	connector_accelerometer_pval(void);
static void 	connector_accelerometer_cb(uint16_t seq_id, uint16_t parameter);


/**********************************************************
 * Private variables
 *********************************************************/
uint16_t lis2dh12_event = 0;
extern acc_event_t event;
static EXT_RAM_ATTR TimerHandle_t timer_handle;
static EXT_RAM_ATTR StaticTimer_t timer_buffer;

/**********************************************************
 * Public variables
 *********************************************************/
CONNECTOR connector_accelerometer =
{
	.name            = "Accelerometer connector",
	.initialize      = connector_accelerometer_init,
	.process_event   = connector_accelerometer_task
};

/**********************************************************
 * Implementation
 *********************************************************/
//! \brief Unblock connector_accelerometer_task each second
static void connector_accelerometer_timer_callback(TimerHandle_t xTimer)
{
	ddmw_send_generic_event_data(&accelerometer_event.ddm, LIS2DH12_TIMER_POLL_EVENT, NULL, 0);
}

/**********************************************************
 * Function:    connector_accelerometer_init
 * Description: Initialize the connector
 *********************************************************/
static int connector_accelerometer_init(void)
{
	// Memory
	memset(&accelerometer_event, 0, sizeof(accelerometer_event));

	// DDM
	ddmw_init(&accelerometer_event.ddm, &connector_accelerometer);

	// Register to ddm
	connector_accelerometer_register(&accelerometer_event.items);

	//Initialize Accelerometer parameters
	ddmw_set_i32(&accelerometer_event.items.avl, 1);

	lis2dh12_seq_init(NULL, &connector_accelerometer_cb);

	TRUE_CHECK_RETURN0(timer_handle = xTimerCreateStatic(
		NULL,
		pdMS_TO_TICKS(LIS2DH12_TIMER_PERIOD_MS),
		pdTRUE,
		NULL,
		connector_lis2dh12_timer_callback,
		&timer_buffer));

	TRUE_CHECK(xTimerStart(timer_handle, portMAX_DELAY));

	return 1;
}

/**********************************************************
 * Function:    connector_accelerometer_task
 * Description: Run connector task
 *********************************************************/
static void connector_accelerometer_task(const DDMP2_FRAME * frame)
{
	ddmw_process(&accelerometer_event.ddm, frame);
	connector_accelerometer_pval();
	ddmw_process_publish(&accelerometer_event.ddm);
}

/**********************************************************
 * Function:    connector_accelerometer_register
 * Description: Register accelerometer instance
 *********************************************************/
static void connector_accelerometer_register(accelerometer_param_t* acc_param)
{
	// Register new instance to DDM
	int instance = ddmw_register(&accelerometer_event.ddm, SACCM0AVL);
	
	ddmw_add(&accelerometer_event.ddm, &acc_param->avl,			SACCM0AVL,		instance);//saccm0avl
	ddmw_add(&accelerometer_event.ddm, &acc_param->id,			SACCM0WAI, 		instance);//saccm0wai
	ddmw_add(&accelerometer_event.ddm, &acc_param->temp,		SACCM0TEMP,		instance);//saccm0temp
	ddmw_add(&accelerometer_event.ddm, &acc_param->coordx,		SACCM0ACCX,		instance);//saccm0sendaccx
	ddmw_add(&accelerometer_event.ddm, &acc_param->coordy,		SACCM0ACCY,		instance);//saccm0sendaccy
	ddmw_add(&accelerometer_event.ddm, &acc_param->coordz,		SACCM0ACCZ,		instance);//saccm0sendaccz
	ddmw_add(&accelerometer_event.ddm, &acc_param->event, 		SACCM0EVENT,	instance);//saccm0event
}

/**********************************************************
 * Function:	connector_accelerometer_pval
 * Description: Check Parameter values
 *********************************************************/
static void connector_accelerometer_pval(void)
{	
	if (ddmw_is_generic_event_updated(&accelerometer_event.ddm))
	{
		// Publish parameters each second
		if (ddmw_get_generic_event_id(&accelerometer_event.ddm) == LIS2DH12_TIMER_POLL_EVENT)
		{
			if (ddmw_is_subscribed(&accelerometer_event.items.id))
			{
				uint8_t id;
				if (!lis2dh12_seq_read(LIS2DH12_MEAS_TYPE_WHOAMI, &id))
				{
					ddmw_set_i32(&accelerometer_event.items.id, id);
				}
			}

			if (ddmw_is_subscribed(&accelerometer_event.items.temp))
			{
				uint32_t temperature;
				if (!lis2dh12_seq_read(LIS2DH12_MEAS_TYPE_TEMP, &temperature))
				{
					ddmw_set_i32(&accelerometer_event.items.temp, temperature * 1000);
				}
			}

			if (ddmw_is_subscribed(&accelerometer_event.items.coordx) || ddmw_is_subscribed(&accelerometer_event.items.coordy) || ddmw_is_subscribed(&accelerometer_event.items.coordz))
			{
				int16_t coord[3];
				if (!lis2dh12_seq_read(LIS2DH12_MEAS_TYPE_ACCEL, coord))
				{
					if (ddmw_is_subscribed(&accelerometer_event.items.coordx))
					{
						ddmw_set_i32(&accelerometer_event.items.coordx, coord[0]);
					}

					if (ddmw_is_subscribed(&accelerometer_event.items.coordy))
					{
						ddmw_set_i32(&accelerometer_event.items.coordy, coord[1]);
					}

					if (ddmw_is_subscribed(&accelerometer_event.items.coordz))
					{
						ddmw_set_i32(&accelerometer_event.items.coordz, coord[2]);
					}
				}
			}
		}
	}
	else
	{
		if (ddmw_is_updated(&accelerometer_event.items.event))
		{
			//LOG(I,"Event is updated")
			event = ddmw_get_i32(&accelerometer_event.items.event);

			switch (event)
			{
			case Tampering:
			case Single_Tapping:
			case Double_Tapping:
				enable_tap_event();
				break;
			case Fall_Detection:
				enable_freefall_event();
				break;
			case Moving:
				enable_wakeup_event();
				break;
			case Tilting:
				enable_tilt_event();
				break;
			case motion_and_tap:
				enable_motion_and_tap_event();
				break;
			case None:
				disable_all_event();
				break;
			default:
				LOG(E, "Acceleration event(%d) does not exist", event);
				return;
			}

			//Setting event parameter to None after enabling event
			event = None;
			ddmw_set_i32(&accelerometer_event.items.event, event);
		}
	}
}

static void connector_accelerometer_cb (uint16_t seq_id, uint16_t parameter)
{
	//LOG(I,"losdh12 callback executed")
	//printf("%d\n", parameter);
	lis2dh12_event = parameter;
	ddmw_set_i32(&accelerometer_event.items.event, lis2dh12_event);
}
