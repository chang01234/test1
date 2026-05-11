/**
 * @file lin_server_definition.h
 * @author Borjan Bozhinovski (borjan.bozhinovski@qinshift.com)
 * @brief LIN Server core definition types.
 * @date 2023-12-28
 */

#ifndef LIN_SERVER_DEFINITION__
#define LIN_SERVER_DEFINITION__

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "lin_common.h"
#include "ddm_store.h"

#define DISCRIMINATOR_DDM_PARAMETER_MODEL_NONE                     (UINT32_MAX)

#define LIN_SERVER_BUS_DETECTION_COUNTER_INIT_VALUE                (3)

#define LIN_SERVER_CTRL_FRAME_VERIFICATION_PERIOD_MS               (2000)
#define LIN_SERVER_CTRL_FRAME_VERIFICATION_COUNTER                 (LIN_SERVER_CTRL_FRAME_VERIFICATION_PERIOD_MS / LIN_SERVER_TICK_TIMER_PERIOD_MS)
#define LIN_SERVER_CTRL_FRAME_LOCAL_CHANGE_ACK_PERIOD_MS           (2000)
#define LIN_SERVER_CTRL_FRAME_LOCAL_CHANGE_ACK_COUNTER             (LIN_SERVER_CTRL_FRAME_LOCAL_CHANGE_ACK_PERIOD_MS / LIN_SERVER_TICK_TIMER_PERIOD_MS)
#define LIN_SERVER_CTRL_FRAME_LOCAL_CHANGE_ACK_RESET_COUNTER       (3) //!< Reset LocalChange ACK process counter

/**
 * @brief       Define how many DDM parameters we can map to a single link map member
 */
#define LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY                      (3)
/**
 * @brief       Define how many DDM parameter data bytes we can map to a single link map member
 */
#define LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY           (20)

/**
 * @brief       Describe a LIN server DDM2 to Lin Logic map entry
 *
 * LIN server DDM2 to Lin Logic entry is a map with a primary function to map DDM2 parameters to the
 * particular logic handle structure @ref lin_server_ddm2_to_lin_ctrl_logic_t.
 *
 * LIN server DDM2 to Lin Logic entry is a structure that:
 * - describes which logical functions to be used for which DDM2 parameters
 * 
 * @param       ddm2_parameters are stored in an array which is of fixed length and the
 *              length is determined at compile time by the @ref ELEMENTS macro. These 
 *              parameters are used as triggers of the @ref pre_send_cb and @ref post_send_cb
 *              functions.
 * @param       pre_send_cb is a callback function that evaluates proposed parameter changes
 *              and decides whether to store, schedule, or reject them based on its internal
 *              logic. It's invoked before conversion functions are executed.
 * @param       post_send_cb is a callback function triggered after successful ctrl frame transmission,
 *              responsible for managing signal states.
 */
#define DDM2_TO_LIN_CTRL_LOGIC_ENTRY(ddm2_parameters, pre_send_cb, post_send_cb)   \
    {                                                                              \
        .ddm2_parameter_list = (ddm2_parameters),                                  \
        .ddm2_parameter_list_count = ELEMENTS(ddm2_parameters),                    \
        .ddm2_to_lin_handle_logic_pre_send = (pre_send_cb),                        \
        .ddm2_to_lin_handle_logic_post_send = (post_send_cb)                       \
    }

/**
 * @brief       Describe a LIN server LIN to DDM2 Info Logic structure
 *
 * The LIN server LIN to Info frame logic entry is a map with a primary function to map info frames
 * to the particular logic handle structure @ref lin_server_lin_to_ddm2_info_logic_t.
 *
 * LIN server LIN to info frame Logic entry is a structure that:
 * - describes which logical function to be used for which info frame
 */
#define LIN_INFO_TO_DDM2_LOGIC_ENTRY(info_frame, post_receive_cb)         \
    {                                                                     \
        .info_frame_def = (info_frame),                                   \
        .lin_to_ddm2_handle_logic_post_receive = (post_receive_cb)        \
    }

/* Forward declaration */
typedef struct lin_server_device_frame lin_server_device_frame_t;
typedef struct lin_server_device_frame_signals lin_server_device_frame_signals_t;
typedef uint32_t lin_scheduler_frame_type_requests_t;

typedef enum lin_server_sync_protocol_type
{
	LIN_SERVER_SYNC_PROTOCOL_NONE,
	LIN_SERVER_SYNC_PROTOCOL_LOCAL_CHANGE,
} lin_server_sync_protocol_type_t;

typedef struct lin_server_slave_device_sync_protocol_local_change
{
	union
	{
		struct info_frame_local_change
		{
			uint8_t local_change_byte_pos;
			uint8_t local_change_bit_pos;
		} local_change;
		struct ctrl_frame_sync_frame
		{
			uint8_t sync_frame_byte_pos;
			uint8_t sync_frame_bit_pos;
		} sync_frame;
	};
} lin_server_slave_device_sync_protocol_local_change_t;

/**
 * @brief		LIN Server synchronization protocol
 *
 * @note		All members of this structure are private to LIN server slave device.
 * @note		Type definition is for internal LIN server module usage.
 */
typedef struct lin_server_slave_device_sync_protocol
{
	union
	{
		const lin_server_slave_device_sync_protocol_local_change_t * local_change;
	};
} lin_server_slave_device_sync_protocol_t;

/**
 * @brief		LIN Server frame mutable data
 *
 * @note		All members of this structure are private to LIN server slave device.
 * @note		Type definition is for internal LIN server module usage.
 */
typedef struct lin_server_device_frame_data_t
{
	bool has_ctrl_frame_been_sent;											/*!< Initiates the procedure to verify acceptance of the control frame from the slave device */
	int ctrl_frame_type;													/*!< Specifies the type of frame to be verified */
	int ctrl_frame_verification_counter;									/*!< Tracks the count for the frame currently under verification */
	int ctrl_frame_verification_local_change_ack_counter;					/*!< Tracks the count of read frames before we start the ACK */
} lin_server_device_frame_data_t;

/**
 * \brief      LIN frame definition
 *
 * This structure describes each LIN frame.
 */
typedef struct lin_server_device_frame_def
{
	uint8_t frame_id;												//!< Frame ID
	uint8_t frame_len;												//!< Frame len
	lin_frame_type_t frame_type;									//!< Control or info frame
	lin_server_device_frame_t * frame;								//!< definition of lin slave frame
	lin_server_slave_device_sync_protocol_t protocol_definition;	//!< Definition of LIN slave frame supported synchornization protocol.
} lin_server_device_frame_def_t;

typedef struct lin_server_device_frame_bundle_def
{
	const lin_server_device_frame_def_t * ctrl_frame_def;
	const lin_server_device_frame_def_t * info_frame_def;
} lin_server_device_frames_bundle_def_t;

/* Conversion table for DDM to LIN */
typedef struct lin_server_map_ddm_to_lin
{
	const uint32_t ddm[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY];
	int (* convert_ddm_to_lin)(const uint8_t * ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], const size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY], lin_server_device_frame_signals_t * slave_device_frame);
	const lin_server_device_frame_def_t * ctrl_frame_def;
} lin_server_map_ddm_to_lin_t;

/* Conversion table for LIN to DDM */
typedef struct lin_server_map_lin_to_ddm
{
	const uint32_t ddm[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY];
	int (* convert_lin_to_ddm)(const lin_server_device_frame_signals_t * const slave_device_frame, uint8_t ddm2_parameters_value[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY][LIN_SERVER_MAX_DDM_DATA_BYTES_PER_LINK_MAP_ENTRY], size_t ddm2_parameters_value_size[LIN_SERVER_MAX_DDM_PER_LINK_MAP_ENTRY]);
	const lin_server_device_frame_def_t * info_frame_def;
} lin_server_map_lin_to_ddm_t;

typedef struct lin_server_slave_info_response
{
	uint8_t error;
	uint8_t ci_error;
	uint8_t no_main;
	uint8_t no_init;
} lin_server_slave_info_response_t;

/* Defines the arrangement for the discriminator model */
typedef struct lin_server_generic_profile_entry
{
	uint8_t variant_id;
	uint16_t function_id;
	uint32_t ddm_model;
} lin_server_generic_profile_entry_t;
typedef struct lin_server_generic_profile
{
	uint32_t discriminator_ddm;
	const lin_server_generic_profile_entry_t * profile_entry;
	size_t profile_entry_size;
} lin_server_generic_profile_t;

/**
 * @brief Function specific confguration data
 * 
 * @note A single device can have multiple function IDs, indicating that the same product is available in
 * multiple variants that differ only in function. Each function ID variant may differ in how it handles
 * the same DDM2 parameters or may not handle them at all. This structure can be used to list specific
 * conversion functions in both, LIN to DDM and DDM to LIN, directions using @a map_function_specific_ddm_to_lin
 * and @a map_function_specific_lin_to_ddm.
 */
typedef struct lin_server_function_specific_config_data
{
	uint16_t function_id;													//!< Function id

	const uint8_t * variant_ids;											//!< Array of variant IDs that can describe a single device's function ID
	size_t variant_ids_size;												//!< Size of variant_ids

	const lin_server_map_ddm_to_lin_t * map_function_specific_ddm_to_lin;	/*!< Array of DDM to LIN function specific conversion functions.
																			 * Can be NULL if no specific conversion functions are needed.
																			 */
	size_t map_function_specific_ddm_to_lin_size;							//!< Size of map_function_specific_ddm_to_lin
	const lin_server_map_lin_to_ddm_t * map_function_specific_lin_to_ddm;	/*!< Array of LIN to DDM function specific conversion functions.
																			 * Can be NULL if no specific conversion functions are needed.
																			 */
	size_t map_function_specific_lin_to_ddm_size;							//!< Size of map_function_specific_lin_to_ddm
} lin_server_function_specific_config_data_t;

/**
 * @brief		LIN server slave device mutable data
 * 
 * This structre is used to store data in RAM that should be modified in runtime
 * 
 * @note		All members of this structure are private to LIN server slave device.
 * @note		Type definition is for internal LIN server module usage.
 */
typedef struct lin_server_slave_device_mutable_data
{
	int class_instance;		 				//!< DDM2 class instance
	int prod_instance;						//!< Production class instance

	bool is_initialized;														//!< Store whether the device is initialized
	bool has_been_updated;														//!< Keeps track whenever new update is receveid from slave

	struct
	{
		const lin_server_function_specific_config_data_t * active_config_data;		/*!< Keeps reference of the active slave device configuration set during the bus probing process.
																					* It depends on the function_id received in the diagnostic response frame i.e. how the slave has
																					* presented itself to the master.
																					*/
		uint8_t active_variant_id;
		uint32_t active_device_model;
	};

	struct
	{
		bool is_on_bus_detected;												//!< Store whether the device is detected on bus
		int8_t bus_detection_counter;											//!< Counter that indicates reconection status request
	};

	struct
	{
		ddm_store_t * heap_store;													/*!< Copy of the owned parameters storage used for handling the ddm2 to lin logical
																				 * function without affecting the original storage.
																				 */
		int8_t table_index;														/*!< @lin_server_ddm2_to_lin_ctrl_logic_t table index set by the DDM2 parameter trigger.
																				 * It is used for executing the @ddm2_to_lin_handle_logic_post_send event.
																				 */
	};

	struct
	{
		void * data;
		size_t data_size;
	};

	SemaphoreHandle_t lock;														//!< Synchronization
} lin_server_slave_device_mutable_data_t;

typedef struct lin_server_slave_device lin_server_slave_device_t;
typedef enum lin_server_logic_func_response
{
	LIN_SERVER_LOGIC_FUNC_RESPONSE_NONE,
	LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_AND_PUBLISH,
	LIN_SERVER_LOGIC_FUNC_RESPONSE_STORE_PUBLISH_AND_SCHEDULE,
	LIN_SERVER_LOGIC_FUNC_INVALID_CONVERSION,
} lin_server_logic_func_response_t;

/**
 * @brief       Describe a LIN server DDM2 to Lin CTRL Logic structure
 *
 * The LIN server DDM2 to Lin CTRL Logic is a data structure designed to establish
 * connection between parameters and their respective logic handling functions.
 * The members of this structure are defined using @ref DDM2_TO_LIN_CTRL_LOGIC_ENTRY.
 */
typedef struct lin_server_ddm2_to_lin_ctrl_logic
{
	/**
	 * @brief size of @ref ddm2_parameter_list, normally calculated with
	 * the help of the @ref ELEMENTS macro.
	 */
	size_t ddm2_parameter_list_count;
	/**
	 * @brief Array of fixed length that stores DDM2 parameters. These parameters
	 * are used as triggers of the @ref pre_send_cb and @ref post_send_cb functions.
	 */
	const uint32_t * ddm2_parameter_list;
	/**
	 * @brief callback function that evaluates proposed parameter changes
	 * and decides whether to store, schedule, or reject them based on its
	 * internal logic. It's invoked before conversion functions are executed.
	 * Dedicated DDM2 parameter's CTRL 'slave_device_ctrl_frame' is propagated as
	 * input argument. */
	lin_server_logic_func_response_t (* ddm2_to_lin_handle_logic_pre_send)(const lin_server_slave_device_t * const device, const ddm_entry_t * const updated_parameter, ddm_store_t * ddm_store, const lin_server_device_frame_def_t * ctrl_frame_def);
	/**
	 * @brief callback function triggered after successful ctrl frame transmission,
	 * responsible for managing signal states. Dedicated DDM2 parameter's CTRL 'slave_device_ctrl_frame'
	 * is propagated as input argument
	 */
	lin_server_logic_func_response_t (* ddm2_to_lin_handle_logic_post_send)(const lin_server_slave_device_t * const device, const lin_server_device_frame_def_t * ctrl_frame_def);
} lin_server_ddm2_to_lin_ctrl_logic_t;

/**
 * @brief       Describe a LIN server LIN to DDM2 Info Logic structure
 *
 * The LIN server LIN INFO to DDM2 is a data structure designed to establish
 * connection between the info frame and their respective logic handling functions.
 * A logic functions is triggered only if the received info frame is listed in @param info_frame_def.
 * This cb functions can be used to read/write signals that are not directly connected to any DDM2
 * parameters, as well as it can be used for DDM2 parameters related logic updates.
 */
typedef struct lin_server_lin_to_ddm2_info_logic
{
	/**
	 * @brief Info frame_def trigger. @ref lin_to_ddm2_handle_logic_post_receive
	 * callback functions will be trigger for the listed info_frame_def
	 */
	const lin_server_device_frame_def_t * info_frame_def;
	/**
	 * @brief callback function triggered after successful reception of the INFO frame frame.
	 * As input arguments, ctrl/info bundle pair is provided, that can be used to read/write
	 * any signals from/to the frames.
	 */
	lin_server_logic_func_response_t (* lin_to_ddm2_handle_logic_post_receive)(const lin_server_slave_device_t * const device, const lin_server_device_frame_def_t * info_frame_def, ddm_store_t * ddm_store, const lin_server_device_frame_def_t * ctrl_frame_def);
} lin_server_lin_to_ddm2_info_logic_t;

/**
 * @brief		LIN server slave device
 *
 * This is used to store slave devices data. The data can be accesed by LIN server
 * descriptor using @ref lin_server_descriptor
 *
 * @note		All members of this structure are private to LIN server slave device.
 * @note		Type definition is for internal LIN server module usage.
 */
typedef struct lin_server_slave_device
{
	uint32_t device_class;												//!< Device class
	const char * name;													//!< Name of the slave device
	uint32_t device_type;												//!< Device type(lin_server_device_type_t)
	ddm_store_t * ddm_owned_store;										//!< Owned DDM2 parameters storage of the slave's class
	const ddm_store_ddm_t * ddm_owned_initial_values;					//!< Initial values for the defined DDM2 parameters
	size_t ddm_owned_initial_values_size;								//!< Size of ddm_owned_initial_values
	ddm_store_t * ddm_production_store;									//!< DDM2 production parameters storage of the slave's class
	const ddm_store_ddm_t * ddm_production_initial_values;				//!< Initial values for the defined DDM2 production parameters
	size_t ddm_production_initial_values_size;							//!< Size of ddm_production_initial_values
	const lin_server_device_frames_bundle_def_t * frames_bundle_defs;	//!< Definition of LIN frame per slave device
	size_t frames_bundle_defs_size;										//!< Size of lin_server_frame_defs
	const lin_server_map_ddm_to_lin_t * map_ddm_to_lin; 
	size_t map_ddm_to_lin_size;
	const lin_server_map_lin_to_ddm_t * map_lin_to_ddm;
	size_t map_lin_to_ddm_size;
	const lin_server_function_specific_config_data_t * function_specific_config_data;
	size_t function_specific_config_data_size;
	const lin_device_config_data_t * device_config;
	int (* stuff_function)(const lin_server_slave_device_t * slave_device, const lin_server_device_frames_bundle_def_t * frames_bundle_defs, uint8_t * data, size_t * data_size, lin_scheduler_frame_type_requests_t scheduled_frame_type_requests);
	int (* extract_function)(const lin_server_slave_device_t * slave_device, const lin_server_device_frames_bundle_def_t * frames_bundle_defs, const uint8_t * data, size_t data_size, lin_server_slave_info_response_t * info_status);
	int (* init_function)(const lin_server_slave_device_t * slave_device);
	const lin_server_ddm2_to_lin_ctrl_logic_t * ddm2_to_lin_ctrl_logic;
	size_t ddm2_to_lin_ctrl_logic_size;
	const lin_server_lin_to_ddm2_info_logic_t * lin_to_ddm2_info_logic;
	size_t lin_to_ddm2_info_logic_size;
	lin_server_slave_device_mutable_data_t * data;
	lin_server_sync_protocol_type_t protocol;
	const lin_server_generic_profile_t * generic_profile;
} lin_server_slave_device_t;

/**
 * @brief		LIN Server slave devices
 *
 * This is used as storage of all supported devices by LIN Master
 *
 * @note		All members of this structure are private to LIN server devices.
 * @note		Type definition is for internal LIN server module usage.
 */
typedef struct lin_server_slave_devices
{
	const lin_server_slave_device_t * slave_device;
} lin_server_slave_devices_t;

#endif //LIN_SERVER_DEFINITION__