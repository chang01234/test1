/*! \file connector_light.c
	\brief Connector for Light
	\Author Sundaramoorthy-LTTS
 */

/** Includes ******************************************************************/
#include "configuration.h"

#include "connector_light.h"
#include "linear_interpolation.h"
#include "connector.h"
#include "broker.h"
#include "osal.h"
#include "hal_ledc.h"
#include "hal_i2c_master.h"
#ifdef DEVICE_TCA9554A
#include "drv_tca9554a.h"
#endif
#include "sorted_list.h"
#include "math.h"

#define CONNECTOR_LIGHT_SUB_DEPTH	((uint8_t)   20u)
#define DDMP_UNAVAILABLE            ((uint8_t) 0xFFu)
#define DIMMER_MAX_LEVELS           7
#define LIGHT_STRIP_DUTY_CYCLE_MIN  0
#define LIGHT_STRIP_DUTY_CYCLE_MAX  100
#define NO_ERROR                    0
#define ERROR                       1
#define CONN_LIGHT_DEBUG            1

/* Static Function declarations */
static int initialize_connector_light(void);
static void conn_light_process_task(void *pvParameter);
static int install_parameters(void);
static void start_subscribe(void);
static int add_subscription(DDMP2_FRAME *pframe);
static void handle_dim0cmd_cb(uint32_t ddm_param, int32_t i32Value);
static void handle_dim0lvl_cb(uint32_t ddm_param, int32_t i32Value);
//static void handle_pwrmode_cb(uint32_t ddm_param, int32_t i32Value);
static void process_set_and_publish_request(uint32_t ddm_param, int32_t i32value, DDMP2_CONTROL_ENUM req_type);
static void process_subscribe_request(uint32_t ddm_param);
static uint8_t get_ddm_index_from_db(uint32_t ddm_param);
static void start_publish(void);
static void sync_dim_cmd_to_dim_lvl(uint32_t dim_cmd_param, int32_t dim_cmd_value);
static void sync_dim_lvl_to_dim_cmd(uint32_t dim_lvl_param, int32_t dim_lvl_value);

/* Structure for Connector Light */
CONNECTOR connector_light =
{
	.name       = "Connector Light",
	.initialize = initialize_connector_light,
};

/* DDM Parameter table for connector light */
static conn_light_param_t conn_light_param_db[] =
{
   {.ddm_parameter = DIM0CMD,       .type = DDM2_TYPE_INT32_T, .pub = 1, .sub = 0, .i32Value = 0, .cb_func = handle_dim0cmd_cb},
   {.ddm_parameter = DIM0DD,        .type = DDM2_TYPE_INT32_T, .pub = 1, .sub = 0, .i32Value = 0, .cb_func = NULL},
   {.ddm_parameter = DIM0LVL,       .type = DDM2_TYPE_INT32_T, .pub = 1, .sub = 0, .i32Value = 0, .cb_func = handle_dim0lvl_cb},
   {.ddm_parameter = DIM0OC,        .type = DDM2_TYPE_INT32_T, .pub = 1, .sub = 0, .i32Value = 0, .cb_func = NULL},
   {.ddm_parameter = DIM0LSTAT,     .type = DDM2_TYPE_INT32_T, .pub = 1, .sub = 0, .i32Value = 0, .cb_func = NULL},
};

/* Calculate the connector light database table num elements */
static const uint32_t conn_light_db_elements = ELEMENTS(conn_light_param_db);

DECLARE_SORTED_LIST_EXTRAM(conn_light_sub_table, CONNECTOR_LIGHT_SUB_DEPTH);       //!< \~ Subscription table storage

static int conn_light_instance = -1;

/**
  * @brief  Initialize the Connector for Light
  * @param  none.
  * @retval none.
  */
static int initialize_connector_light(void)
{
#if CONN_LIGHT_DEBUG
    LOG(I, "initialize_connector_light");
#endif

    /* Create Task for connector OnboardHMI to handle/process all the ddmp related activities */
	TRUE_CHECK(osal_task_create(conn_light_process_task, CONNECTOR_LIGHT_PROCESS_TASK_NAME, CONNECTOR_LIGHT_PROCESS_TASK_STACK_DEPTH, NULL, CONNECTOR_LIGHT_PROCESS_TASK_PRIORITY, NULL));
    
    /* Publish the DIM0AVL class availbility to the broker */
    conn_light_instance = install_parameters();
    /* Subsribe the all the needed DDMP parameters to the broker */
    if (conn_light_instance != -1)
    {
        start_subscribe();
        start_publish();
    }

    return conn_light_instance == -1 ? CONNECTOR_INIT_FAILURE : CONNECTOR_INIT_SUCCESS;
}

/**
  * @brief  Function to publish the available classes in connedtor onboard HMI to the broker
  * @param  none.
  * @retval none.
  */
static int install_parameters(void)
{
    int instance = -1;
    uint32_t device_class = DIM0;

    NONNEG_CHECK(instance = broker_register_instance(&device_class, connector_light.connector_id));

    return instance;
}

/**
  * @brief  Function to publish the DDMP parameters
  * @param  none.
  * @retval none.
  */
static void start_publish(void)
{
    conn_light_param_t *ptr_param_db;
    uint16_t db_idx;
    uint8_t num_elements = ELEMENTS(conn_light_param_db);
    
    for ( db_idx = 0; db_idx < num_elements; db_idx++ )
    {
        ptr_param_db = &conn_light_param_db[db_idx];

        /* Check the DDM parameter need to publish */
        if ( ptr_param_db->pub )
        {
            TRUE_CHECK(connector_send_frame_to_broker(
                DDMP2_CONTROL_PUBLISH,
                ptr_param_db->ddm_parameter | DDM2_PARAMETER_INSTANCE(conn_light_instance),
                &ptr_param_db->i32Value,
                sizeof(ptr_param_db->i32Value),
                connector_light.connector_id,
                portMAX_DELAY));
        }
    }
}

/**
  * @brief  Function to subscribe the DDMP parameters needed for Connector Light
  * @param  none.
  * @retval none.
  */
static void start_subscribe(void)
{
    conn_light_param_t *ptr_param_db;
	uint16_t db_idx;
    
    for ( db_idx = 0; db_idx < conn_light_db_elements; db_idx++ )
	{
        ptr_param_db = &conn_light_param_db[db_idx];
        /* Check the DDM parameter need subscribtion */
	    if ( ptr_param_db->sub )
	    {
            connector_send_frame_to_broker(DDMP2_CONTROL_SUBSCRIBE, ptr_param_db->ddm_parameter, NULL, 0, connector_light.connector_id, portMAX_DELAY);
		}
    }
}

/**
  * @brief  Function to process the set and publish parameter from broker
  * @param  DDM parameter.
  * @retval none.
  */
static void process_set_and_publish_request(uint32_t ddm_param, int32_t i32value, DDMP2_CONTROL_ENUM req_type)
{
    int32_t i32Index;
    int32_t i32Factor;
    int32_t pub_value = i32value;
	uint16_t db_idx;
	conn_light_param_t* param_db;

#if CONN_LIGHT_DEBUG
    LOG(I, "Received ddm_param = 0x%x i32value = %d", ddm_param, i32value);
#endif

	/* Validate the DDM parameter received */
	db_idx = get_ddm_index_from_db(ddm_param);
 
	if ( DDMP_UNAVAILABLE != db_idx )
	{
        if ( DDMP2_CONTROL_SET == req_type )
        {
            /* Frame and send the publish request */
            TRUE_CHECK(connector_send_frame_to_broker(
                DDMP2_CONTROL_PUBLISH,
                ddm_param | DDM2_PARAMETER_INSTANCE(conn_light_instance),
                &pub_value,
                sizeof(pub_value),
                connector_light.connector_id,
                portMAX_DELAY));
        }

		param_db = &conn_light_param_db[db_idx];

#if CONN_LIGHT_DEBUG		
		LOG(I, "Valid DDMP parameter");
#endif
        i32Index = ddm2_parameter_list_lookup(DDM2_PARAMETER_BASE_INSTANCE(ddm_param));

        if ( -1 != i32Index )
        {
            if (  i32Index < DDM2_PARAMETER_COUNT )
                i32Factor = Ddm2_unit_factor_list[Ddm2_parameter_list_data[i32Index].in_unit];
            else
                i32Factor = 1;

            if ( i32Factor > 0 )
            {
                i32value = i32value / i32Factor;
            }
                
#if CONN_LIGHT_DEBUG
            LOG(I, "After factored i32value = %d", i32value);
#endif
            if ( i32value != param_db->i32Value )
            {
                /* Update the received value in the database table*/
	  	        param_db->i32Value = i32value;
		        /* Check callback function registered for this DDM parameter */
		        if ( NULL != param_db->cb_func )
		        {
                    /* Execute the callback function */
                    param_db->cb_func(ddm_param, i32value);
		        }
            }
        }
	}
}

/**
  * @brief  Task for Connector Light to hanle the DDMP request
  * @param  pvParameter.
  * @retval none.
  */
static void conn_light_process_task(void *pvParameter)
{
	DDMP2_FRAME *pframe;
	size_t frame_size;

	while (1)
	{
	    TRUE_CHECK (pframe = xRingbufferReceive(connector_light.to_connector, &frame_size, portMAX_DELAY));
        
        switch (pframe->frame.control)
	    {
            case DDMP2_CONTROL_PUBLISH: /* Data received from broker as subscribed by connector light */
#if CONN_LIGHT_DEBUG
                LOG(I, "DDMP2_CONTROL_PUBLISH");
#endif 
                process_set_and_publish_request(pframe->frame.publish.parameter, pframe->frame.publish.value.int32, pframe->frame.control);
                break;
            
            case DDMP2_CONTROL_SET:
#if CONN_LIGHT_DEBUG
                LOG(I, "DDMP2_CONTROL_SET");
#endif 
                process_set_and_publish_request(pframe->frame.publish.parameter, pframe->frame.publish.value.int32, pframe->frame.control);
                break;

            case DDMP2_CONTROL_SUBSCRIBE:   /* Connector light replies to data requested by other connectors through broker*/
#if CONN_LIGHT_DEBUG
                LOG(I, "DDMP2_CONTROL_SUBSCRIBE");
#endif
		        add_subscription(pframe);
                process_subscribe_request(pframe->frame.subscribe.parameter);
                break;
                
            default:
			    LOG(E, "Connector Light received UNHANDLED frame %02x from broker!",pframe->frame.control);
			    break;
        }
        
        vRingbufferReturnItem(connector_light.to_connector, pframe);
    }
}

/**
  * @brief  Add device to inventory if it does not already exists
  * @param  DDMP Frame.
  * @retval result 0 - Succesfully added to list / 1 - Fail.
  */
static int add_subscription(DDMP2_FRAME *pframe)
{
    SORTED_LIST_KEY_TYPE     key = pframe->frame.subscribe.parameter;
    SORTED_LIST_VALUE_TYPE value = 1;

    return sorted_list_single_add(&conn_light_sub_table, key, value);
}

/**
  * @brief  Function to process the subscribe request received from the broker
            Every subscribtion will be followed by publish of current value of the subscribed parameter
  * @param  DDM parameter.
  * @retval none.
  */
static void process_subscribe_request(uint32_t ddm_param)
{
	uint16_t db_idx;
    int index;
    int32_t value = 0;
    int factor = 0;
	conn_light_param_t* param_db;
    uint32_t list_value = 0;
    SORTED_LIST_RETURN_VALUE ret = sorted_list_unique_get(&list_value, &conn_light_sub_table, ddm_param, 0);

#if CONN_LIGHT_DEBUG
    LOG(I, "Received ddm_param = 0x%x ret = %d", ddm_param, ret);
#endif

    if ( SORTED_LIST_FAIL != ret )
	{
		/* Validate the DDM parameter received */
		db_idx = get_ddm_index_from_db(ddm_param);

		if ( DDMP_UNAVAILABLE != db_idx )
	  	{
			param_db = &conn_light_param_db[db_idx];

            index = ddm2_parameter_list_lookup(DDM2_PARAMETER_BASE_INSTANCE(ddm_param));

#if CONN_LIGHT_DEBUG
            LOG(I, "ddm2_parameter_list_lookup index = %d", index);
#endif
            if ( -1 != index )
			{
                factor = Ddm2_unit_factor_list[Ddm2_parameter_list_data[index].out_unit];

                if ( factor == 0 )
                {
                    factor = 1;
                }
                
                /* Multiply with the factor */
                value = param_db->i32Value * factor;
#if CONN_LIGHT_DEBUG
                LOG(I, "After factored i32value = %d", value);
#endif
                /* Frame and send the publish request */
                TRUE_CHECK(connector_send_frame_to_broker(
                    DDMP2_CONTROL_PUBLISH,
                    ddm_param | DDM2_PARAMETER_INSTANCE(conn_light_instance),
                    &value,
                    sizeof(value),
                    connector_light.connector_id,
                    portMAX_DELAY));
            }
            else
            {
                LOG(E, "DDMP 0x%x not found in ddm2_parameter_list_lookup", ddm_param);
            }
		}
        else
        {
            LOG(E, "Invalid DDMP Request ddm_param 0x%x", ddm_param);
        }
	}
	else
	{
		LOG(E, "SORTLIST_INVALID_VALUE ddm_param 0x%x", ddm_param);
	}
}

/**
  * @brief  Function to get ddm index from database table
  * @param  DDMP Parameter.
  * @retval none.
  */
static uint8_t get_ddm_index_from_db(uint32_t ddm_param)
{
	conn_light_param_t* param_db;
	uint8_t db_idx = DDMP_UNAVAILABLE; 
	uint8_t index;
	bool avail = false;

	for ( index = 0u; ( ( index < conn_light_db_elements ) && ( avail == false ) ); index++ )
 	{
		param_db = &conn_light_param_db[index];
      	
		/* Validate the DDM parameter received */
		if ( param_db->ddm_parameter == ddm_param )
	  	{
			db_idx = index;
			avail = true;
		}
	}
	
	return db_idx;
}

/**
  * @brief  Callback function to handle the DDMP DIM0CMD
  * @param  Parameter Value.
  * @retval none.
  */
static void handle_dim0cmd_cb(uint32_t ddm_param, int32_t i32Value)
{ 
    DIM0CMD_ENUM dim_cmd = (DIM0CMD_ENUM)i32Value;

    switch (dim_cmd)
    {
    case DIM0CMD_ON:
        LOG(D, "LIGHT ON");
        light_dimmer_set_duty(LED_DIMMER_ON_DUTY_CYCLE);
        break;

    case DIM0CMD_OFF:
    case DIM0CMD_STOP:
        LOG(D, "LIGHT OFF");
        light_dimmer_set_duty(LED_DIMMER_OFF_DUTY_CYCLE);
        break;

    default:
        LOG(W, "Dimmer Command Unhandled = %d", dim_cmd);
        break;
    }

    sync_dim_cmd_to_dim_lvl(ddm_param, i32Value);
}

/**
  * @brief  Callback function to handle the DDMP DIM0CMD
  * @param  Parameter Value.
  * @retval none.
  */
static void handle_dim0lvl_cb(uint32_t ddm_param, int32_t i32Value)
{ 
    uint32_t calc_resol;

    LOG(I,"ddm_param = %x i32Value = %d", ddm_param, i32Value);

    if ( i32Value <= LIGHT_STRIP_DUTY_CYCLE_MAX )
    {
        calc_resol = calc_linear_interpolation(LIGHT_STRIP_DUTY_CYCLE_MIN, LIGHT_STRIP_DUTY_CYCLE_MAX, LEDC_PWM_MIN_DUTY_CYCLE, LEDC_PWM_MAX_DUTY_CYCLE, i32Value);
#if CONN_LIGHT_DEBUG
        LOG(I,"Light Strip Duty Cycle = %d calc_resol = %d", i32Value, calc_resol);
#endif
        light_dimmer_set_duty(calc_resol);

        sync_dim_lvl_to_dim_cmd(ddm_param, i32Value);
    }
    else
    {
        LOG(E,"Invalid Dimmer Level Request = %d", i32Value);
    }
}

static void sync_dim_cmd_to_dim_lvl(uint32_t dim_cmd_param, int32_t dim_cmd_value)
{
    uint32_t dim_lvl_param = DIM0LVL | DDM2_PARAMETER_INSTANCE_PART(dim_cmd_param);
    int32_t dim_lvl_value = LIGHT_STRIP_DUTY_CYCLE_MIN;

    switch ((DIM0CMD_ENUM)dim_cmd_value)
    {
    case DIM0CMD_ON:
        {
            dim_lvl_value = LIGHT_STRIP_DUTY_CYCLE_MAX;
        }
        break;

    case DIM0CMD_OFF:
    case DIM0CMD_STOP:
        {
            dim_lvl_value = LIGHT_STRIP_DUTY_CYCLE_MIN;
        }
        break;

    default:
        break;
    }

    uint16_t db_idx = get_ddm_index_from_db(dim_lvl_param);
	if (DDMP_UNAVAILABLE != db_idx)
    {
        conn_light_param_t* param_db = &conn_light_param_db[db_idx];
        param_db->i32Value = dim_lvl_value;
    }

    TRUE_CHECK(connector_send_frame_to_broker(
                DDMP2_CONTROL_PUBLISH,
                dim_lvl_param,
                &dim_lvl_value,
                sizeof(dim_lvl_value),
                connector_light.connector_id,
                portMAX_DELAY));
}

static void sync_dim_lvl_to_dim_cmd(uint32_t dim_lvl_param, int32_t dim_lvl_value)
{
    uint32_t dim_cmd_param = DIM0CMD | DDM2_PARAMETER_INSTANCE_PART(dim_lvl_param);
    int32_t dim_cmd_value = ((dim_lvl_value > LIGHT_STRIP_DUTY_CYCLE_MIN) \
        && (dim_lvl_value <= LIGHT_STRIP_DUTY_CYCLE_MAX)) ? DIM0CMD_ON : DIM0CMD_OFF;

    uint16_t db_idx = get_ddm_index_from_db(dim_cmd_param);
	if (DDMP_UNAVAILABLE != db_idx)
    {
        conn_light_param_t* param_db = &conn_light_param_db[db_idx];
        param_db->i32Value = dim_cmd_value;
    }

    TRUE_CHECK(connector_send_frame_to_broker(
                DDMP2_CONTROL_PUBLISH,
                dim_cmd_param,
                &dim_cmd_value,
                sizeof(dim_cmd_value),
                connector_light.connector_id,
                portMAX_DELAY));
}
