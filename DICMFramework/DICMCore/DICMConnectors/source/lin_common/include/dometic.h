/*
 * dometic.h
 *
 *  Created on: 17 Feb 2020
 *      Author: Stefan.Henningsohn
 */

#ifndef DOMETIC_H_
#define DOMETIC_H_

#include <stdint.h>
#include <stdbool.h>
#include "ddm2.h"

#define DOMETIC_AC_DEFAULT_CTRL_ID (0x08)
#define DOMETIC_AC_DEFAULT_INFO_ID (0x17)

#define DOMETIC_FRESHWELL_CTRL_FRAME_ID DOMETIC_AC_DEFAULT_CTRL_ID
#define DOMETIC_FRESHWELL_INFO_FRAME_ID DOMETIC_AC_DEFAULT_INFO_ID


/** LIN frame defines for Sharc project */
#define DOMETIC_SHARC_WTR_CTRL_ID (0x37)
#define DOMETIC_SHARC_WTR_INFO_ID (0x38)
#define DOMETIC_SHARC_AIR_CTRL_ID (0x39)
#define DOMETIC_SHARC_AIR_INFO_ID (0x3A)

/** LIN frame defines for Shape project */
#define DOMETIC_SHAPE_AC_CTRL_FRAME_ID  (0x08)  /** LIN AC control frame  */
#define DOMETIC_SHAPE_AC_INFO_FRAME_ID  (0x17)  /** LIN AC information frame  */
#define DOMETIC_SHAPE_AC2_CTRL_FRAME_ID (0x2B)  /** LIN AC2 control frame  */
#define DOMETIC_SHAPE_AC2_INFO_FRAME_ID (0x2C)  /** LIN AC2 information frame  */
#define DOMETIC_REQ_FRAME_ID  (0x3C)  /** LIN read by identifier - (master) request frame  */
#define DOMETIC_RES_FRAME_ID  (0x3D)  /** LIN read by identifier - (slave) response frame  */

#define DOMETIC_SHAPE_DATA_LENGTH   8
#define DOMETIC_SHARC_NONE_DATA_LENGTH  8

/** LIN frame defines for INVENTILATE project */
#define DOMETIC_INVENT_V1_CTRL_FRAME_ID (0x2B)  /** LIN control frame  */
#define DOMETIC_INVENT_V1_INFO_FRAME_ID (0x2C)  /** LIN information frame  */
#define DOMETIC_INVENT_V1_REQ_FRAME_ID  (0x3C)  /** LIN read by identifier - (master) request frame  */
#define DOMETIC_INVENT_V1_RES_FRAME_ID  (0x3D)  /** LIN read by identifier - (slave) response frame  */

#define DOMETIC_INVENT_V1_DATA_LENGTH   8

/** LIN dev frame defines for Bridge NRX project */
#define DOMETIC_NRX_V1_CTRL_FRAME_ID (0x0B)  /** LIN control frame  */
#define DOMETIC_NRX_V1_INFO_FRAME_ID (0x0C)  /** LIN information frame  */
#define DOMETIC_NRX_V1_REQ_FRAME_ID  (0x3C)  /** LIN read by identifier - (master) request frame  */
#define DOMETIC_NRX_V1_RES_FRAME_ID  (0x3D)  /** LIN read by identifier - (slave) response frame  */

#define DOMETIC_NRX_V1_DATA_LENGTH   8

typedef enum _LIGHT_INTERNAL
{
    LIGHT_OFF = 0,
    LIGHT_50  = 1,
    LIGHT_100 = 2,
    LIGHT_RSV = 3,
} LIGHT_INTERNAL_T;

/* AC_MODE_T enum to map AC mode on DDM side */
typedef enum _AC_MODE
{
    MODE_COOL        = 0,
    MODE_HEAT        = 1,
    MODE_VENTILATION = 2,
    MODE_AUTO        = 3,
    MODE_DRY         = 4,

} AC_MODE_T;

/* LIN_AC_MODE_T enum to map AC mode on LIN side */
typedef enum _LIN_AC_MODE
{
    LIN_MODE_AUTO        = 0,
    LIN_MODE_COOL        = 1,
    LIN_MODE_DRY         = 2,
    LIN_MODE_HEAT        = 3,
    LIN_MODE_VENTILATION = 4,
} LIN_AC_MODE_T;

/* LIN_WTR_MODE_T enum to map AC mode on DDM side */
typedef enum _LIN_WTR_MODE
{
    LIN_WTRTEMP_ECO   = 0,
    LIN_WTRTEMP_HOT   = 1,
    LIN_WTRTEMP_BOOST = 2,
    LIN_WTRTEMP_OFF   = 3,
}LIN_WTR_MODE_T;

/* WTR_MODE_T enum to map AC mode on DDM side */
typedef enum _WTR_MODE
{
    WTRTEMP_ECO   = 1,
    WTRTEMP_HOT   = 2,
    WTRTEMP_BOOST = 3,
    WTRTEMP_OFF   = 0,
}WTR_MODE_T;


/* invent_v1_ledbright_ddm enum to map LED brightness on DDM side */
typedef enum _INVENT_V1_LEDBRIGHT_DDM    // _INV_LEDBRIGHT
{
    INV_LEDBRIGHT_0PER   = 0,
    INV_LEDBRIGHT_5PER   = 5,
    INV_LEDBRIGHT_40PER  = 40,
    INV_LEDBRIGHT_100PER = 100,
} INVENT_V1_LEDBRIGHT_DDM;

/* invent_v1_ledbright_lin enum to map LED brightness on LIN side */
typedef enum _INVENT_V1_LEDBRIGHT_LIN    //_LIN_INV_LEDBRIGHT_T
{
    INV_LIN_LEDBRIGHT_0     = 0,   //0x0000,
    INV_LIN_LEDBRIGHT_1     = 1,   //0x0001,
    INV_LIN_LEDBRIGHT_4     = 4,   //0x0100,
    INV_LIN_LEDBRIGHT_10    = 10,  //0x1010,
    INV_LIN_LEDBRIGHT_MAX   = 4
} INVENT_V1_LEDBRIGHT_LIN;

typedef struct
{
	uint8_t mode_a;
	uint8_t fan_mode;
	uint8_t light_status;
	uint8_t power;
	uint8_t mode_b;
    uint8_t fan_speed;
    uint8_t target_temp;
	uint8_t dim_lvl;
    uint8_t sync_frame;
} dometic_ac_ctrl_t;

typedef struct
{
	uint8_t mode_a;
	uint8_t fan_mode;
	uint8_t light_status;
	uint8_t power;
	uint8_t mode_b;
    uint8_t fan_speed;
    uint8_t target_temp;
	uint8_t dim_lvl;
    uint8_t local_change;
    uint8_t ci_error;
} dometic_ac_info_t;

typedef struct
{
    uint8_t wtr_temp;
    uint8_t wtr_but2;
    uint8_t wtr_on;
    uint8_t wtr_wakeup;
    uint8_t air_wakeup;
    uint8_t wakeup;
    uint8_t esel;
    uint8_t wtr_esel;
    uint8_t air_esel;
    uint8_t wtr_sync_frame;
    uint8_t wtr_page;
    int32_t air_temp;
    uint8_t air_on;
    uint8_t air_mode;
    uint8_t air_smaxfan;
    uint8_t air_vminfan;
    uint8_t air_sync_frame;
    uint8_t time_seconds;
    uint8_t time_minutes;
    uint8_t time_hours;
    uint8_t date_day;
    uint8_t date_month;
    uint8_t date_year;
} dometic_sharc_ctrl_t;

typedef struct
{
    uint8_t wtr_temp;
    uint8_t wtr_on;
    uint8_t online;
    uint8_t esel;
    uint16_t wtr_errcd_1;
    uint16_t wtr_errcd_2;
    uint16_t wtr_errcd_3;
    uint16_t wtr_errcd_4;
    uint8_t err_status;
    uint8_t wtr_warning;
    uint8_t wtr_air_fault;
    uint8_t wtr_local_change;
    uint8_t wtr_ci_error;
    uint8_t wtr_page;
    int32_t air_temp;
    uint8_t air_on;
    uint8_t air_mode;
    uint8_t air_smaxfan;
    uint8_t air_vminfan;
    uint8_t air_ac_status;
    uint8_t air_time_off;
    uint8_t air_time_on;
    uint8_t air_wtr_time_on;
    uint8_t air_local_change;
    uint8_t air_ci_error;
    uint8_t air_page;
    uint8_t time_seconds;
    uint8_t time_minutes;
    uint8_t time_hours;
    uint8_t date_day;
    uint8_t date_month;
    uint8_t date_year;
    uint8_t wtr_temp_status;
    uint8_t wtr_ac_heater_status;
    uint8_t wtr_gas_heater_status;
    int32_t air_room_temp;
} dometic_sharc_info_t;

typedef struct
{
    uint8_t nad; // node address for diagnostic
    uint8_t pci; // protocol control information
    uint8_t rsid; // Response service Identifier
    uint16_t supplier_id;// Supplier ID
    uint16_t func_id; // Function ID
    uint8_t var_id; // variant ID
}dometic_gen_pid_diag_info_t;

/*typedef struct
{
    uint8_t nad; // node address for diagnostic
    uint8_t pci; // protocol control information
    uint8_t rsid; // Response service Identifier
    uint16_t supplier_id;// Supplier ID
    uint16_t func_id; // Function ID
    uint8_t var_id; // variant ID
}dometic_freshwell_pid_diag_info_t;*/

typedef struct
{
    uint8_t nad;
    uint8_t pci;
    uint8_t rsid;
    uint16_t supplier_id;
    uint16_t func_id;
    uint8_t var_id;
}dometic_sharc_pid_diag_info_t; /* Diagnostic Dometic SHARC Product Identifier Information Structure */

typedef struct
{
    uint8_t nad;
    uint8_t pci;
    uint8_t sid;
    uint8_t data1;
    uint16_t supplier_id;
    uint16_t func_id;
}dometic_sharc_serial_diag_ctrl_t; /* Diagnostic Dometic SHARC Serial Control Structure */

typedef struct
{
    uint8_t nad;
    uint8_t pci;
    uint8_t rsid;
    uint32_t serial_number;
    uint8_t data8;
}dometic_sharc_serial_diag_info_t; /* Diagnostic Dometic SHARC Serial Information Structure */

typedef struct
{
    uint8_t nad;
    uint8_t pci;
    uint8_t sid;
    uint16_t supplier_id;
    uint16_t func_id;
    uint8_t new_nad;
}dometic_sharc_nad_diag_ctrl_t; /* Diagnostic Dometic SHARC Assign NAD Control Structure */

typedef struct
{
    uint8_t nad;
    uint8_t pci;
    uint8_t rsid;
    uint8_t data1;
    uint8_t data2;
    uint8_t data3;
    uint8_t data4;
    uint8_t data5;
}dometic_sharc_nad_diag_info_t; /* Diagnostic Dometic SHARC Assign NAD Information Structure */

typedef struct
{
    uint8_t nad;
    uint8_t pci;
    uint8_t sid;
    uint8_t start_index;
    uint32_t product_id;
}dometic_sharc_frame_diag_ctrl_t; /* Diagnostic Dometic SHARC Assign Frame Control Structure */

typedef struct
{
    uint8_t nad;
    uint8_t pci;
    uint8_t rsid;
    uint8_t data1;
    uint8_t data2;
    uint8_t data3;
    uint8_t data4;
    uint8_t data5;
}dometic_sharc_frame_diag_info_t; /* Diagnostic Dometic SHARC Assign Frame Information Structure */


typedef struct
{
    uint8_t nad;
    uint8_t pci;
    uint8_t rsid;
    uint8_t req_sid;
    uint8_t data2;
    uint8_t data3;
    uint8_t data4;
    uint8_t data5;
}dometic_gen_neg_diag_info_t;

/**
 * @brief       Remote data structure for Shape product, Fix variant
 *
 * Remote data structure is common to request/response frames.
 *
 * The remote data structure is part of synchronization procedure of controllers. The received
 * remote data structure and stored remote data are compared to evaluate if master has successfully
 * synchronized to slave.
 *
 * Remote data structure is mapped directly to DDM interface. Each member of the structure
 * corresponds to exactly one DDM parameter.
 *
 * Each member of the structure should be named as DDM parameter to make the mapping to DDM tables
 * easier to read.
 *
 * The size of each member is of minimal size needed to keep the data range (to reduce memory usage).
 * These fields are then expanded to int32_t/other types and stored in DDM during the runtime. All
 * intergral structure member types are of signed type to ease the data conversion to DDM.
 *
 * In the comment section of the structure member it is noted:
 * - [byte][bitfield] as the source of the value from byte array. If multiple sources are used to
 *   generate one value for DDM, then all bytes/bitfields are noted here.
 * - Description from CI/LIN bus description
 * - Name of DDM paramater which is mapped to this field.
 */
typedef struct dometic_shape_ac_remote_data
{
    /* Remote Data 0 */
    uint8_t md;                         /* [0][0:2]: AC Function Mode, maps to AC0MD */
    uint8_t sleep;                      /* [0][3:3]: Sleep Function, maps to AC0SLEEP */
    uint8_t toffa;                      /* [0][4:4]: Turning off timer flag, maps to AC0TOFFA */
    uint8_t tona;                       /* [0][5:5]: Turning on timer flag, maps to AC0TONA */
                                        /* [0][6:6]: RFU */
    uint8_t on;                         /* [0][7:7]: Air Conditioner status, maps to AC0ON */
    /* Remote Data 1 */
    uint8_t elgt;                       /* [1][0:0]: Turn ON/Off External Light, maps to AC0ELGT */
    uint8_t fspd;                       /* [1][1:3]: Fan Speed Mode, maps to AC0FSPD */
    uint8_t ttemp;                      /* [1][4:7]: Target temperature, maps to AC0TTEMP */
    /* Remote Data 2 */
    uint16_t tonm;                      /* 1-to-1 split-field:
                                         * [2][0:7]: Turning on minutes left
                                         * [4][0:3]: Residual timer time within the 10min set
                                         * Turning on minutes left, maps to AC0TONM */
    /* Remote Data 3 */
    uint16_t toffm;                     /* 1-to-1 split-field:
                                         * [3][0:7]: Turning off minutes left
                                         * [4][4:7]: Residual timer time within the 10min set
                                         * Turning off minutes left, maps to AC0TOFFM */
    /* Remote Data 4 */
    /* Contains bit-fields processed by other members */
    /* Remote Data 5 */
    uint8_t lgt_dmr;                    /* 1-to-many
                                         * [5][0:2]: Internal Lighting system status,
                                         * Maps to AC0LGT, AC0DMR */
    uint8_t hfavl;                      /* [5][3:3]: Bit indicates if the Heater is configured/available, maps to AC0HFAVL (Read only) */
    uint8_t lfavl;                      /* [5][4:4]: Bit indicates if the ADB Light is configured/available, maps to AC0LFAVL (Read only) */
    uint8_t remctrl;                    /* [5][5:5]: Current setting of RemoveCtrlDis, maps to AC0REMCTRL (Read only) */
                                        /* [5][6:7]: RFU */
    /* Remote data 6 */
    /* RFU */
} dometic_shape_ac_remote_data_t;

typedef struct dometic_shape_ac_ctrl_status
{
    uint8_t remctrl;                    /* [7][0:0]: Remote control disable, maps to AC0REMCTRL */
                                        /* [7][1:1]: RFU */
                                        /* [7][2:2]: SyncFrame, handled by protocol */
                                        /* [7][3:3]: TimerUpdate, not handled in current implementation. */
    uint8_t actext;                     /* many-to-1:
                                         * [7][4:4]: Disable the Output for external inverter: 1 - Force OFF
                                         * [7][5:5]: Enable the output for external inverter: 1 - Force ON
                                         * Status of connected actuators, maps to AC0ACTEXT */
                                        /* [7][6:7] RFU */
} dometic_shape_ac_ctrl_status_t;

typedef struct dometic_shape_ac_ctrl_protocol
{
    bool is_sync_frame;                 /* [7][2:2] Master LIN synchronized to AC: 1-Master Synchro */
} dometic_shape_ac_ctrl_protocol_t;

typedef struct dometic_shape_ac_ctrl_bundle
{
    dometic_shape_ac_remote_data_t rd;
    dometic_shape_ac_ctrl_status_t st;
    dometic_shape_ac_ctrl_protocol_t protocol;
} dometic_shape_ac_ctrl_bundle_t;

typedef struct dometic_shape_ac_info_status
{
    /* Remote data 6 */
    uint8_t actext;                     /* many-to-one:
                                         * [6][0:0]: Heater is running: 1 - Heater ON
                                         * [6][1:1]: Compressor Status: 1 - Compressor ON
                                         * maps to AC0ACTEXT
                                         */
    uint8_t itemp;                      /* [6][2:7]: Internal temperature Leve: Internal temperature value (0 – 63), maps to AC0ITEMP */

                                        /* [7][0:0]: Implemented by protocol, LocalChange */
    uint8_t status;                     /* many-to-one:
                                         * [7][1:1]: Mains Failure: 1 - Mains failed
                                         * [7][2:2]: Temp. Probe error: 1 - Error
                                         * Status of AC, maps to AC0STATUS */
                                        /* [7][3:3]: NOT IMPLEMENTED */
                                        /* [7][4:4]: NOT IMPLEMENTED */
    uint8_t remctrl;                    /* [7][5:5]: Remote control disable, maps to AC0REMCTRL */
    uint8_t not_init;                   /* Local-variable mapping:
                                         * [7][6:6]: Card not initialized: 1 - NotInit, maps to local variable */
    uint8_t ci_error;                   /* Local-variable mapping:
                                         * [7][7:7]: Error on CI bus: 1 - Error, maps to local variable */
} dometic_shape_ac_info_status_t;

typedef struct dometic_shape_ac_info_protocol
{
    bool is_local_change_frame;
} dometic_shape_ac_info_protocol_t;

typedef struct dometic_shape_ac_info_bundle
{
    dometic_shape_ac_remote_data_t rd;
    dometic_shape_ac_info_status_t st;
    dometic_shape_ac_info_protocol_t protocol;
} dometic_shape_ac_info_bundle_t;

/**
 * @brief       Extract the contents of LIN frame contained in @a data argument to Dometic structure
 *              pointed by @a bundle_buffer argument
 *
 * @param       bundle_buffer Pointer to bundle buffer which contains Dometic structure
 * @param       data Pointer to LIN frame buffer
 */
void dometic_shape_ac_ctrl_extract(dometic_shape_ac_ctrl_bundle_t * bundle_buffer, const uint8_t data[DOMETIC_SHAPE_DATA_LENGTH]);

/**
 * @brief       Stuff the contents of Dometic structure pointed by @a bundle_buffer argument to LIN
 *              frame buffer pointed by @a data argument.
 *
 * @param       bundle_buffer Pointer to bundle buffer which contains Dometic structure
 * @param       data Pointer to LIN frame buffer
 */
void dometic_shape_ac_info_stuff(const dometic_shape_ac_info_bundle_t * bundle_buffer, uint8_t data[DOMETIC_SHAPE_DATA_LENGTH]);

/**
 * @brief       Convect Dometic md member value to AC0MD DDM value
 *
 * @param       conv_data Pointer to Dometic md member
 * @param       conv_data_size Sizeof Dometic md member in bytes
 * @param       ddm_data Pointer to allocated memory where the result should be placed
 * @param       ddm_data_size Size of the allocated memory pointed by `ddm_data`
 * @return      How many bytes are written to `ddm_data`
 */
size_t dometic_shape_ac_conv_md_to_ac0md(const void * conv_data, size_t conv_data_size, void * ddm_data, size_t ddm_data_size);

/**
 * @brief       Convert AC0MD value to remote data member in Dometic structure
 *
 * @param       ddm_data Pointer to AC0MD data
 * @param       ddm_data_size Size of AC0MD data in bytes
 * @param       conv_data_size Size of info status error member in bytes
 * @param       conv_data Pointer to info status error member
 */
void dometic_shape_ac_conv_ac0md_to_md(const void * const * ddm_data, const size_t * ddm_data_size, size_t conv_data_size, void * conv_data);

#define dometic_shape_ac_conv_sleep_to_ac0sleep    dometic_conv_any_int_to_any_ddm
#define dometic_shape_ac_conv_ac0sleep_to_sleep    dometic_conv_any_ddm_to_any_int

#define dometic_shape_ac_conv_toffa_to_ac0toffa    dometic_conv_any_int_to_any_ddm
#define dometic_shape_ac_conv_ac0toffa_to_toffa    dometic_conv_any_ddm_to_any_int

#define dometic_shape_ac_conv_tona_to_ac0tona      dometic_conv_any_int_to_any_ddm
#define dometic_shape_ac_conv_ac0tona_to_tona      dometic_conv_any_ddm_to_any_int

#define dometic_shape_ac_conv_on_to_ac0on          dometic_conv_any_int_to_any_ddm
#define dometic_shape_ac_conv_ac0on_to_on          dometic_conv_any_ddm_to_any_int

#define dometic_shape_ac_conv_elgt_to_ac0elgt      dometic_conv_any_int_to_any_ddm
#define dometic_shape_ac_conv_ac0elgt_to_elgt      dometic_conv_any_ddm_to_any_int

size_t dometic_shape_ac_conv_fspd_to_ac0fspd(const void * conv_data, size_t conv_data_size, void * ddm_data, size_t ddm_data_size);

void dometic_shape_ac_conv_ac0fspd_to_fspd(const void * const * ddm_data, const size_t * ddm_data_size, size_t conv_data_size, void * conv_data);

size_t dometic_shape_ac_conv_ttemp_to_ac0ttemp(const void * conv_data, size_t conv_data_size, void * ddm_data, size_t ddm_data_size);

void dometic_shape_ac_conv_ac0ttemp_to_ttemp(const void * const * ddm_data, const size_t * ddm_data_size, size_t conv_data_size, void * conv_data);

/**
 * @brief       Convect Dometic tonm member value to AC0TONM DDM value
 *
 * @param       conv_data Pointer to Dometic md member
 * @param       conv_data_size Sizeof Dometic md member in bytes
 * @param       ddm_data Pointer to allocated memory where the result should be placed
 * @param       ddm_data_size Size of the allocated memory pointed by `ddm_data`
 * @return      How many bytes are written to `ddm_data`
 */
size_t dometic_shape_ac_conv_tonm_to_ac0tonm(const void * conv_data, size_t conv_data_size, void * ddm_data, size_t ddm_data_size);

/**
 * @brief       Convert AC0TONM value to remote data member in Dometic structure
 *
 * @param       ddm_data Pointer to AC0STATUS data
 * @param       ddm_data_size Size of AC0STATUS data in bytes
 * @param       conv_data_size Size of info status error member in bytes
 * @param       conv_data Pointer to info status error member
 */
void dometic_shape_ac_conv_ac0tonm_to_tonm(const void * const * ddm_data, const size_t * ddm_data_size, size_t conv_data_size, void * conv_data);

size_t dometic_shape_ac_conv_toffm_to_ac0toffm(const void * conv_data, size_t conv_data_size, void * ddm_data, size_t ddm_data_size);

void dometic_shape_ac_conv_ac0toffm_to_toffm(const void * const * ddm_data, const size_t * ddm_data_size, size_t conv_data_size, void * conv_data);

size_t dometic_shape_ac_conv_lgt_dmr_to_ac0lgt(const void * conv_data, size_t conv_data_size, void * ddm_data, size_t ddm_data_size);
size_t dometic_shape_ac_conv_lgt_dmr_to_ac0dmr(const void * conv_data, size_t conv_data_size, void * ddm_data, size_t ddm_data_size);

void dometic_shape_ac_conv_ac0lgt_ac0dmr_to_lgt_dmr(const void * const * ddm_data, const size_t * ddm_data_size, size_t conv_data_size, void * conv_data);

#define dometic_shape_ac_conv_hfavl_to_ac0hfavl    dometic_conv_any_int_to_any_ddm

#define dometic_shape_ac_conv_ac0lfavl_to_lfavl    dometic_conv_any_ddm_to_any_int

#define dometic_shape_ac_conv_lfavl_to_ac0lfavl    dometic_conv_any_int_to_any_ddm

#define dometic_shape_ac_conv_ac0hfavl_to_hfavl    dometic_conv_any_ddm_to_any_int

#define dometic_shape_ac_conv_rd_remctrl_to_ac0remctrl dometic_conv_any_int_to_any_ddm
#define dometic_shape_ac_conv_ac0remctrl_to_rd_remctrl dometic_conv_any_ddm_to_any_int

size_t dometic_shape_ac_conv_st_actext_to_ac0actext(const void * conv_data, size_t conv_data_size, void * ddm_data, size_t ddm_data_size);

void dometic_shape_ac_conv_ac0actext_to_st_actext(const void * const * ddm_data, const size_t * ddm_data_size, size_t conv_data_size, void * conv_data);

void dometic_shape_ac_conv_ac0itemp_to_itemp(const void * const * ddm_data, const size_t * ddm_data_size, size_t conv_data_size, void * conv_data);

void dometic_shape_ac_conv_ac0status_to_status(const void * const * ddm_data, const size_t * ddm_data_size, size_t conv_data_size, void * conv_data);

#define dometic_shape_ac_conv_st_remctrl_to_ac0remctrl dometic_conv_any_int_to_any_ddm
#define dometic_shape_ac_conv_ac0remctrl_to_st_remctrl dometic_conv_any_ddm_to_any_int

typedef struct dometic_shape_ac2_input_current_limit
{
    /* Input Current limit */
    uint8_t input_current_limit;        /* [0][0:3]: AC input current limit settings */
                                        /* [0][4:7]: RFU */
} dometic_shape_ac2_input_current_limit_t;
typedef struct dometic_shape_ac2_info_bundle
{
    dometic_shape_ac2_input_current_limit_t icl;
} dometic_shape_ac2_info_bundle_t;

typedef struct dometic_shape_ac2_ctrl_bundle
{
    dometic_shape_ac2_input_current_limit_t icl;
} dometic_shape_ac2_ctrl_bundle_t;

/**
 * @brief       Extract the contents of LIN frame contained in @a data argument to Dometic structure
 *              pointed by @a bundle_buffer argument
 *
 * @param       bundle_buffer Pointer to bundle buffer which contains Dometic structure
 * @param       data Pointer to LIN frame buffer
 */
void dometic_shape_ac2_ctrl_extract(dometic_shape_ac2_ctrl_bundle_t * bundle_buffer, const uint8_t data[DOMETIC_SHAPE_DATA_LENGTH]);

/**
 * @brief       Stuff the contents of Dometic structure pointed by @a bundle_buffer argument to LIN
 *              frame buffer pointed by @a data argument.
 *
 * @param       bundle_buffer Pointer to bundle buffer which contains Dometic structure
 * @param       data Pointer to LIN frame buffer
 */
void dometic_shape_ac2_info_stuff(const dometic_shape_ac2_info_bundle_t * bundle_buffer, uint8_t data[DOMETIC_SHAPE_DATA_LENGTH]);

/**
 * @brief       Convert AC0CURRLIM value to current limit member in Dometic structure
 *
 * @param       ddm_data Pointer to AC0CURRLIM data
 * @param       ddm_data_size Size of AC0CURRLIM data in bytes
 * @param       conv_data_size Size of current limit member in bytes
 * @param       conv_data Pointer to current limit member
 */
void dometic_shape_ac2_conv_ac0currlim_to_currlim(const void * const * ddm_data, const size_t * ddm_data_size, size_t conv_data_size, void * conv_data);

/**
 * @brief       Convect Dometic currlim member value to AC0CURRLIM DDM value
 *
 * @param       conv_data Pointer to Dometic current limit member
 * @param       conv_data_size Sizeof Dometic current limit member in bytes
 * @param       ddm_data Pointer to allocated memory where the result should be placed
 * @param       ddm_data_size Size of the allocated memory pointed by `ddm_data`
 * @return      How many bytes are written to `ddm_data`
 */
size_t dometic_shape_ac2_conv_currlim_to_ac0currlim(const void * conv_data, size_t conv_data_size, void * ddm_data, size_t ddm_data_size);


size_t dometic_conv_any_int_to_any_ddm(const void * conv_data, size_t conv_data_size, void * ddm_data, size_t ddm_data_size);

/**
 * @brief       Convert any DDM to any integer
 *
 * This function is used to convert the value of any DDM to any type of integer in Dometic structure.
 *
 * The function expects a DDM data and DDM data size in first members of @a ddm_data and
 * @a ddm_data_size arrays. Only the first DDM entry value is converted to @a conv_data.
 *
 * @code
 * ddm_data ----------> +---+
 *                      | 0 |-----------> +---+
 *                      | 1 |--------+    |   | DDM 0 data ---+
 *                      |...|        |    +---+               |
 *                      | n |----+   +--> +---+               |
 *                      +---+    |        |   | DDM 1 data ---|--+
 *                               |        +---+               |  |
 *                               +------> +---+               |  |
 *                                        |   | DDM n data ---|--|--+
 *                                        +---+               |  |  |
 *                                                            |  |  |
 * ddm_data_size -----> +---+                                 |  |  |
 *                      | 0 | = sizeof(DDM 0 data) <----------+  |  |
 *                      | 1 | = sizeof(DDM 1 data) <-------------+  |
 *                      |...|                                       |
 *                      | n | = sizeof(DDM n data) <----------------+
 *                      +---+
 * @endcode
 * @param       ddm_data Pointer to array of pointers pointing to DDM data.
 * @param       ddm_data_size Pointer to array of unsigned integers noting the size of each DDM data
 *              in bytes. Each field in this array correspond to DDM data field with same index in
 *              @a ddm_data array.
 * @param       conv_data_size Dometic structure member field size
 * @param       conv_data Pointer to Dometic structure member
 */
void dometic_conv_any_ddm_to_any_int(const void * const * ddm_data, const size_t * ddm_data_size, size_t conv_data_size, void * conv_data);

/* ############### SHARC product ############### */

/* SHARC WTR common */
typedef struct dometic_sharc_wtr_page0_remote_data
{
    /* Remote Data 0 */
    uint8_t wtrtemp;                    /* [0][0:1]: Water heater mode: [0..1], maps to HTR0WTRTEMP */
    /* Remote Data 1 */
    uint8_t wtron;                      /* [1][0:0]: Water heater command, Sleep/on: [0..1], maps to HTR0WTRON*/
    /* Remote Data 2 */
    uint8_t esel;                       /* [2][0:3]: Energy source: [0..4]: maps to HTR0ESEL */
    /* Remote Data 3 */
    /* Remote Data 4 */
    /* Remote Data 5 */
    /* Remote Data 6 */
    /* Remote Data 7 */
} dometic_sharc_wtr_page0_remote_data_t;

typedef struct dometic_sharc_wtr_page1_remote_data
{
    /* Remote Data 0 */
    uint8_t times;                      /* [0][0:5]: Set time seconds: [0..63], maps to HTR0TIMES */
    /* Remote Data 1 */
    uint8_t timem;                      /* [1][0:5]: Set time minutes: [0..63], maps to HTR0TIMEM */
    /* Remote Data 2 */
    uint8_t timeh;                      /* [2][0:4]: Set time hours: [0..31], maps to HTR0TIMEH */
    /* Remote Data 3 */
    uint8_t dated;                      /* [3][0:4]: Set time hours: [0..31], maps to HTR0DATED */
    /* Remote Data 4 */
    uint8_t datem;                      /* [4][0:3]: Set time hours: [0..15], maps to HTR0DATEM */
    /* Remote Data 5 */
    uint8_t datey;                      /* [5][0:7]: Set time hours: [0..255], offset 2000, maps to HTR0DATEY */
    /* Remote Data 6 */
    /* Remote Data 7 */
} dometic_sharc_wtr_page1_remote_data_t;

typedef struct dometic_sharc_wtr_remote_data
{
    dometic_sharc_wtr_page0_remote_data_t page0;
    dometic_sharc_wtr_page1_remote_data_t page1;
} dometic_sharc_wtr_remote_data_t;

/* SHARC WTR CTRL */
typedef struct dometic_sharc_wtr_ctrl_protocol
{
    bool is_sync_frame;                 /* [7][2:2] Master LIN synchronized to AC: 1-Master Synchro */
    uint8_t ctrl_page;                  /* [7][0:1] Select CTRL page to apply */
    uint8_t info_page;                  /* [7][3:4] Select INFO page to read next */
} dometic_sharc_wtr_ctrl_protocol_t;

typedef struct dometic_sharc_wtr_ctrl_bundle
{
    dometic_sharc_wtr_remote_data_t rd;
    dometic_sharc_wtr_ctrl_protocol_t protocol;
} dometic_sharc_wtr_ctrl_bundle_t;

/* SHARC WTR INFO */

typedef struct dometic_sharc_wtr_info_protocol
{
    bool is_local_change_frame;         /* Part of synchronization protocol */
    uint8_t info_page;                  /* Designates INFO page which is returned */
    bool response_error;                /* Notifies Master if there was response_error */
} dometic_sharc_wtr_info_protocol_t;

typedef struct dometic_sharc_wtr_info_page0_status
{
    uint8_t aon;                        /* [1][1:1]: Water heater command status, Online: [0..1]: maps to HTR0AON */
    uint8_t wtrts;                      /* [5][0:1] Water temperature status, maps to HTR0WTRTS */
    uint8_t acwtrhst;                   /* [5][2:2] AC water heater status, maps to HTR0ACWTRHST */
    uint8_t gaswtrhst;                  /* [5][3:3] Gas water heater status, maps to HTR0GASWTRHST */
} dometic_sharc_wtr_info_page0_status_t;

typedef struct dometic_sharc_wtr_info_page2_status
{
    uint16_t errcd1;                    /* [bits 0:9]: Active error code 1, maps to HTR0ERRCD1 */
    uint16_t errcd2;                    /* [bits 10:19]: Active error code 2, maps to HTR0ERRCD2 */
    uint16_t errcd3;                    /* [bits 20:29]: Active error code 3, maps to HTR0ERRCD3 */
    uint16_t errcd4;                    /* [bits 30:39]: Active error code 4, maps to HTR0ERRCD4 */
    uint8_t errst;                      /* [bits 40:41]: Error status, maps to HTR0ERRST */
} dometic_sharc_wtr_info_page2_status_t;

typedef struct dometic_sharc_wtr_info_status
{
    dometic_sharc_wtr_info_page0_status_t page0;
    dometic_sharc_wtr_info_page2_status_t page2;
    /* Data fields common to all pages */
    uint8_t active_error_flag;          /* [7][3:3]: Active error flag */
} dometic_sharc_wtr_info_status_t;

typedef struct dometic_sharc_wtr_info_bundle
{
    dometic_sharc_wtr_remote_data_t rd;
    dometic_sharc_wtr_info_protocol_t protocol;
    dometic_sharc_wtr_info_status_t st;
} dometic_sharc_wtr_info_bundle_t;

/**
 * @brief       Convect Dometic wtrtemp member value to HTR0WTRTEMP DDM value
 *
 * @param       conv_data Pointer to Dometic md member
 * @param       conv_data_size Sizeof Dometic md member in bytes
 * @param       ddm_data Pointer to allocated memory where the result should be placed
 * @param       ddm_data_size Size of the allocated memory pointed by `ddm_data`
 * @return      How many bytes are written to `ddm_data`
 */
size_t dometic_sharc_wtr_conv_wtrtemp_to_htr0wtrtemp(const void * conv_data, size_t conv_data_size, void * ddm_data, size_t ddm_data_size);

/**
 * @brief       Convert HTR0WTRTEMP value to data member in Dometic structure
 *
 * @param       ddm_data Pointer to HTR0WTRTEMP data
 * @param       ddm_data_size Size of HTR0WTRTEMP data in bytes
 * @param       conv_data_size Size of info status error member in bytes
 * @param       conv_data Pointer to info status error member
 */
void dometic_sharc_wtr_conv_htr0wtrtemp_to_wtrtemp(const void * const * ddm_data, const size_t * ddm_data_size, size_t conv_data_size, void * conv_data);

/**
 * @brief       Convect Dometic wtron member value to HTR0WTRON DDM value
 *
 * Use generic conversion function for this field.
 */
#define dometic_sharc_wtr_conv_wtron_to_htr0wtron       dometic_conv_any_int_to_any_ddm

/**
 * @brief       Convect HTR0WTRON DDM value to Dometic wtron member value
 *
 * Use generic conversion function for this field.
 */
#define dometic_sharc_wtr_conv_htr0wtron_to_wtron       dometic_conv_any_ddm_to_any_int

/**
 * @brief       Convect HTR0ON DDM value to Dometic aon member value
 *
 * Use generic conversion function for this field.
 */
#define dometic_sharc_wtr_conv_htr0aon_to_aon           dometic_conv_any_ddm_to_any_int

/**
 * @brief       Convect Dometic esel member value to HTR0ESEL DDM value
 *
 * Use generic conversion function for this field.
 */
#define dometic_sharc_wtr_conv_esel_to_htr0esel         dometic_conv_any_int_to_any_ddm

/**
 * @brief       Convect HTRESEL DDM value to Dometic esel member value
 *
 * Use generic conversion function for this field.
 */
#define dometic_sharc_wtr_conv_htr0esel_to_esel         dometic_conv_any_ddm_to_any_int

/**
 * @brief       Convect HTRWTRTS DDM value to Dometic wtrts member value
 *
 * Use generic conversion function for this field.
 */
#define dometic_sharc_wtr_conv_htr0wtrts_to_wtrts       dometic_conv_any_ddm_to_any_int

/**
 * @brief       Convect HTRACWTRHST DDM value to Dometic acwtrhst member value
 *
 * Use generic conversion function for this field.
 */
#define dometic_sharc_wtr_conv_htr0acwtrhst_to_acwtrhst                         \
    dometic_conv_any_ddm_to_any_int

/**
 * @brief       Convect HTRGASWTRHST DDM value to Dometic gaswtrhst member value
 *
 * Use generic conversion function for this field.
 */
#define dometic_sharc_wtr_conv_htr0gaswtrhst_to_gaswtrhst                       \
    dometic_conv_any_ddm_to_any_int

/**
 * @brief       Convect Dometic times member value to HTR0TIMES DDM value
 *
 * Use generic conversion function for this field.
 */
#define dometic_sharc_wtr_conv_times_to_htr0times       dometic_conv_any_int_to_any_ddm

/**
 * @brief       Convect HTR0TIMES DDM value to Dometic times member value
 *
 * Use generic conversion function for this field.
 */
#define dometic_sharc_wtr_conv_htr0times_to_times       dometic_conv_any_ddm_to_any_int

/**
 * @brief       Convect Dometic timem member value to HTR0TIMEM DDM value
 *
 * Use generic conversion function for this field.
 */
#define dometic_sharc_wtr_conv_timem_to_htr0timem       dometic_conv_any_int_to_any_ddm

/**
 * @brief       Convect HTR0TIMEM DDM value to Dometic timem member value
 *
 * Use generic conversion function for this field.
 */
#define dometic_sharc_wtr_conv_htr0timem_to_timem       dometic_conv_any_ddm_to_any_int

/**
 * @brief       Convect Dometic timeh member value to HTR0TIMEH DDM value
 *
 * Use generic conversion function for this field.
 */
#define dometic_sharc_wtr_conv_timeh_to_htr0timeh       dometic_conv_any_int_to_any_ddm

/**
 * @brief       Convect HTR0TIMEH DDM value to Dometic timeh member value
 *
 * Use generic conversion function for this field.
 */
#define dometic_sharc_wtr_conv_htr0timeh_to_timeh       dometic_conv_any_ddm_to_any_int

/**
 * @brief       Convect Dometic dated member value to HTR0DATED DDM value
 *
 * Use generic conversion function for this field.
 */
#define dometic_sharc_wtr_conv_dated_to_htr0dated       dometic_conv_any_int_to_any_ddm

/**
 * @brief       Convect HTR0DATED DDM value to Dometic dated member value
 *
 * Use generic conversion function for this field.
 */
#define dometic_sharc_wtr_conv_htr0dated_to_dated       dometic_conv_any_ddm_to_any_int

/**
 * @brief       Convect Dometic datem member value to HTR0DATEM DDM value
 *
 * Use generic conversion function for this field.
 */
#define dometic_sharc_wtr_conv_datem_to_htr0datem       dometic_conv_any_int_to_any_ddm

/**
 * @brief       Convect HTR0DATEM DDM value to Dometic datem member value
 *
 * Use generic conversion function for this field.
 */
#define dometic_sharc_wtr_conv_htr0datem_to_datem       dometic_conv_any_ddm_to_any_int

/**
 * @brief       Convect Dometic datey member value to HTR0DATEY DDM value
 *
 * Use generic conversion function for this field.
 */
#define dometic_sharc_wtr_conv_datey_to_htr0datey       dometic_conv_any_int_to_any_ddm

/**
 * @brief       Convect HTR0DATEY DDM value to Dometic datey member value
 *
 * Use generic conversion function for this field.
 */
#define dometic_sharc_wtr_conv_htr0datey_to_datey       dometic_conv_any_ddm_to_any_int

/**
 * @brief       Convect HTR0ERRCD1 DDM value to Dometic errcd1 member value
 *
 * Use generic conversion function for this field.
 */
#define dometic_sharc_wtr_conv_htr0errcd1_to_errcd1     dometic_conv_any_ddm_to_any_int

/**
 * @brief       Convect HTR0ERRCD2 DDM value to Dometic errcd2 member value
 *
 * Use generic conversion function for this field.
 */
#define dometic_sharc_wtr_conv_htr0errcd2_to_errcd2     dometic_conv_any_ddm_to_any_int

/**
 * @brief       Convect HTR0ERRCD3 DDM value to Dometic errcd3 member value
 *
 * Use generic conversion function for this field.
 */
#define dometic_sharc_wtr_conv_htr0errcd3_to_errcd3     dometic_conv_any_ddm_to_any_int

/**
 * @brief       Convect HTR0ERRCD4 DDM value to Dometic errcd4 member value
 *
 * Use generic conversion function for this field.
 */
#define dometic_sharc_wtr_conv_htr0errcd4_to_errcd4     dometic_conv_any_ddm_to_any_int

/**
 * @brief       Convect HTR0ERRST DDM value to Dometic errst member value
 *
 * Use generic conversion function for this field.
 */
#define dometic_sharc_wtr_conv_htr0errst_to_errst       dometic_conv_any_ddm_to_any_int

/**
 * @brief       Extract the contents of LIN frame contained in @a data argument to Dometic structure
 *              pointed by @a bundle_buffer argument
 *
 * @param       bundle_buffer Pointer to bundle buffer which contains Dometic structure
 * @param       data Pointer to LIN frame buffer
 */
void dometic_sharc_wtr_ctrl_extract(dometic_sharc_wtr_ctrl_bundle_t * bundle_buffer, const uint8_t data[DOMETIC_SHARC_NONE_DATA_LENGTH]);

/**
 * @brief       Stuff the contents of Dometic structure pointed by @a bundle_buffer argument to LIN
 *              frame buffer pointed by @a data argument.
 *
 * @param       bundle_buffer Pointer to bundle buffer which contains Dometic structure
 * @param       data Pointer to LIN frame buffer
 */
void dometic_sharc_wtr_info_stuff(const dometic_sharc_wtr_info_bundle_t * bundle_buffer, uint8_t data[DOMETIC_SHARC_NONE_DATA_LENGTH]);

/* SHARC AIR common */
typedef struct dometic_sharc_air_page0_remote_data
{
    /* Remote Data 0 */
    uint8_t atemp;                      /* [0][0:7]: Target room temperature: -64..62.5, maps to HTR0ATEMP */
    /* Remote Data 1 */
    /* Remote Data 2 */
    uint8_t esel;                       /* [2][0:3]: Energy source: [0..4]: maps to HTR0ESEL */
    /* Remote Data 3 */
    uint8_t amd;                        /* [3][0:2]: Air heater mode: [0..2]: maps to HTR0AMD */
    /* Remote Data 4 */
    uint8_t smaxfan;                    /* [4][0:2]: Fan speed, Silent mode max fan speed: [0..7]: maps to HTR0SMAXFAN */
    uint8_t vminfan;                    /* [4][3:5]: Fan speed, Ventilation mode min fan speed: [0..7]: maps to HTR0VMINFAN */
    /* Remote Data 5 */
    /* Remote data 6 */
} dometic_sharc_air_page0_remote_data_t;

typedef struct dometic_sharc_air_remote_data
{
    dometic_sharc_air_page0_remote_data_t page0;
} dometic_sharc_air_remote_data_t;

/* SHARC AIR CTRL */

typedef struct dometic_sharc_air_ctrl_protocol
{
    bool is_sync_frame;                 /* [7][2:2] Master LIN synchronized to AC: 1-Master Synchro */
    uint8_t ctrl_page;                  /* [7][0:1] Select CTRL page to apply */
    uint8_t info_page;                  /* [7][3:4] Select INFO page to read next */
} dometic_sharc_air_ctrl_protocol_t;

typedef struct dometic_sharc_air_ctrl_bundle
{
    dometic_sharc_air_remote_data_t rd;
    dometic_sharc_air_ctrl_protocol_t protocol;
} dometic_sharc_air_ctrl_bundle_t;

/* SHARC AIR INFO */

typedef struct dometic_sharc_air_info_protocol
{
    bool is_local_change_frame;         /* Part of synchronization protocol */
    uint8_t info_page;                  /* Designates INFO page which is returned */
    bool response_error;                /* Notifies Master if there was response_error */
} dometic_sharc_air_info_protocol_t;

typedef struct dometic_sharc_air_info_page0_status
{
    uint8_t aon;                        /* [1][1:1]: Air heater command status, Online: [0..1]: maps to HTR0AON */
    uint8_t rts;                        /* [5][0:7]: Room temperature: [0..200]: maps to HTR0RTS */
    uint8_t acst;                       /* [6][0:0]: AC status: [0..1]: maps to HTR0ACST */
    uint8_t ahtoffst;                   /* [6][1:1]: Air heater timer off status: [0..1]: maps to HTR0AHTOFFST */
    uint8_t ahtonst;                    /* [6][2:2]: Air heater timer on status: [0..1]: maps to HTR0AHTONST */
    uint8_t wtrtst;                     /* [6][3:3]: Water heater timer status: [0..1]: maps to HTR0WTRTST */
} dometic_sharc_air_info_page0_status_t;

typedef struct dometic_sharc_air_info_status
{
    dometic_sharc_air_info_page0_status_t page0;
    /* Data fields common to all pages */
    uint8_t active_error_flag;          /* [7][3:3]: Active error flag */
} dometic_sharc_air_info_status_t;

typedef struct dometic_sharc_air_info_bundle
{
    dometic_sharc_air_remote_data_t rd;
    dometic_sharc_air_info_protocol_t protocol;
    dometic_sharc_air_info_status_t st;
} dometic_sharc_air_info_bundle_t;

/**
 * @brief       Convect Dometic atemp member value to HTR0ATEMP DDM value
 *
 * @param       conv_data Pointer to Dometic md member
 * @param       conv_data_size Sizeof Dometic md member in bytes
 * @param       ddm_data Pointer to allocated memory where the result should be placed
 * @param       ddm_data_size Size of the allocated memory pointed by `ddm_data`
 * @return      How many bytes are written to `ddm_data`
 */
size_t dometic_sharc_air_conv_atemp_to_htr0atemp(
    const void * conv_data,
    size_t conv_data_size,
    void * ddm_data,
    size_t ddm_data_size);

/**
 * @brief       Convert HTR0ATEMP value to Dometic atemp member value
 *
 * @param       ddm_data Pointer to HTR0ATEMP data
 * @param       ddm_data_size Size of HTR0ATEMP data in bytes
 * @param       conv_data_size Size of info status error member in bytes
 * @param       conv_data Pointer to info status error member
 */
void dometic_sharc_air_conv_htr0atemp_to_atemp(
    const void * const * ddm_data,
    const size_t * ddm_data_size,
    size_t conv_data_size,
    void * conv_data);

/**
 * @brief       Convect HTR0AON DDM value to Dometic aon member value
 *
 * Use generic conversion function for this field.
 */
#define dometic_sharc_air_conv_htr0aon_to_aon           dometic_conv_any_ddm_to_any_int

/**
 * @brief       Convect Dometic esel member value to HTR0ESEL DDM value
 *
 * Use generic conversion function for this field.
 */
#define dometic_sharc_air_conv_esel_to_htr0esel         dometic_conv_any_int_to_any_ddm

/**
 * @brief       Convect HTRESEL DDM value to Dometic esel member value
 *
 * Use generic conversion function for this field.
 */
#define dometic_sharc_air_conv_htr0esel_to_esel         dometic_conv_any_ddm_to_any_int

/**
 * @brief       Convect Dometic amd member value to HTR0AMD DDM value
 *
 * Use generic conversion function for this field.
 */
#define dometic_sharc_air_conv_amd_to_htr0amd           dometic_conv_any_int_to_any_ddm

/**
 * @brief       Convect HTR0AMD DDM value to Dometic amd member value
 *
 * Use generic conversion function for this field.
 */
#define dometic_sharc_air_conv_htr0amd_to_amd           dometic_conv_any_ddm_to_any_int

/**
 * @brief       Convect Dometic smaxfan member value to HTR0SMAXFAN DDM value
 *
 * Use generic conversion function for this field.
 */
#define dometic_sharc_air_conv_smaxfan_to_htr0smaxfan   dometic_conv_any_int_to_any_ddm

/**
 * @brief       Convect HTR0SMAXFAN DDM value to Dometic smaxfan member value
 *
 * Use generic conversion function for this field.
 */
#define dometic_sharc_air_conv_htr0smaxfan_to_smaxfan   dometic_conv_any_ddm_to_any_int

/**
 * @brief       Convect Dometic vminfan member value to HTR0VMINFAN DDM value
 *
 * Use generic conversion function for this field.
 */
#define dometic_sharc_air_conv_vminfan_to_htr0vminfan   dometic_conv_any_int_to_any_ddm

/**
 * @brief       Convect HTR0VMINFAN DDM value to Dometic vminfan member value
 *
 * Use generic conversion function for this field.
 */
#define dometic_sharc_air_conv_htr0vminfan_to_vminfan   dometic_conv_any_ddm_to_any_int

/**
 * @brief       Convert HTR0RTS value to status data member in Dometic structure
 *
 * @param       ddm_data Pointer to HTR0RTS data
 * @param       ddm_data_size Size of HTR0RTS data in bytes
 * @param       conv_data_size Size of info status error member in bytes
 * @param       conv_data Pointer to info status error member
 */
void dometic_sharc_air_conv_htr0rts_to_st_rts(
    const void * const * ddm_data,
    const size_t * ddm_data_size,
    size_t conv_data_size,
    void * conv_data);

/**
 * @brief       Convect HTR0ACST DDM value to Dometic acst member value
 *
 * Use generic conversion function for this field.
 */
#define dometic_sharc_air_conv_htr0acst_to_acst         dometic_conv_any_ddm_to_any_int

/**
 * @brief       Convect HTR0AHTOFFST DDM value to Dometic ahtoffst member value
 *
 * Use generic conversion function for this field.
 */
#define dometic_sharc_air_conv_htr0ahtoffst_to_ahtoffst dometic_conv_any_ddm_to_any_int

/**
 * @brief       Convect HTR0AHTONST DDM value to Dometic ahtonst member value
 *
 * Use generic conversion function for this field.
 */
#define dometic_sharc_air_conv_htr0ahtonst_to_ahtonst   dometic_conv_any_ddm_to_any_int

/**
 * @brief       Convect HTR0WTRTST DDM value to Dometic wtrtst member value
 *
 * Use generic conversion function for this field.
 */
#define dometic_sharc_air_conv_htr0wtrtst_to_wtrtst     dometic_conv_any_ddm_to_any_int

/**
 * @brief       Extract the contents of LIN frame contained in @a data argument to Dometic structure
 *              pointed by @a bundle_buffer argument
 *
 * @param       bundle_buffer Pointer to bundle buffer which contains Dometic structure
 * @param       data Pointer to LIN frame buffer
 */
void dometic_sharc_air_ctrl_extract(dometic_sharc_air_ctrl_bundle_t * bundle_buffer, const uint8_t data[DOMETIC_SHARC_NONE_DATA_LENGTH]);

/**
 * @brief       Stuff the contents of Dometic structure pointed by @a bundle_buffer argument to LIN
 *              frame buffer pointed by @a data argument.
 *
 * @param       bundle_buffer Pointer to bundle buffer which contains Dometic structure
 * @param       data Pointer to LIN frame buffer
 */
void dometic_sharc_air_info_stuff(const dometic_sharc_air_info_bundle_t * bundle_buffer, uint8_t data[DOMETIC_SHARC_NONE_DATA_LENGTH]);

/**
 * Structure to store control frame data.
 *
 * This structure is based on "dometic-Shape-cibus-frame-description__PROPOSAL_R4" document.
 */
typedef struct
{
    /* Remote Data 0 */
    uint8_t mode;                       /* [0][0:2]: AC Function Mode, maps to AC0MD */
    uint8_t sleep_mode;                 /* [0][3:3]: Sleep Function, maps to AC0SLEEP */
    uint8_t timer_off_mode;             /* [0][4:4]: Turning off timer flag, maps to AC0TOFFA */
    uint8_t timer_on_mode;              /* [0][5:5]: Turning on timer flag, maps to AC0TONA */
                                        /* [0][6:6]: RFU */
    uint8_t power;                      /* [0][7:7]: Air Conditioner status: 1- A/C on, maps to AC0ON */
    /* Remote Data 1 */
    uint8_t light_external;             /* [1][0:0]: Turn ON/Off External Light, maps to AC0ELGT */
    uint8_t fan_speed_mode;             /* [1][1:3]: Fan Speed Mode, maps to AC0FSPD */
    uint16_t temp;                      /* [1][4:7]: Target temperature, maps to AC0TTEMP */
    /* Remote Data 2 */
    uint16_t on_timer_min;              /* [2][0:7] [4][0:3]: Turning on minutes left, maps to AC0TONM */
    /* Remote Data 3 */
    uint16_t off_timer_min;             /* [3][0:7] [4][4:7]: Turning off minutes left, maps to AC0TOFFM */
    /* Remote Data 4 */
                                        /* [4][0:3]: Residual timer time within the 10min set, maps to AC0TONM */
                                        /* [4][4:7]: Residual timer time within the 10min set, maps to AC0TOFFM */
    /* Remote Data 5 */
    uint8_t light_internal;             /* [5][0:2]: Internal Lighting system status, maps to AC0LGT */
    uint8_t light_dmr;                  /* [5][0:2]: Internal Lighting system status, maps to AC0DMR */

    uint8_t ac0_heat_enable;            /* [5][3:3]: Bit indicates if the Heater is configured/available, maps to AC0HFAVL (Read only) */
    uint8_t ac0_light_enable;           /* [5][4:4]: Bit indicates if the ADB Light is configured/available, maps to AC0LFAVL (Read only) */
    uint8_t remote_ctrl_status;         /* [5][5:5]: Current setting of RemoveCtrlDis, maps to AC0REMCTRL (Read only) */
                                        /* [5][6:7]: RFU */

    uint8_t fan_spd;            /* Fan setting */
    uint8_t fan_md;             /* Fan setting */
    uint8_t remote_ctrl_dis;    /* Remote control disable: 1-Disabled */
    uint8_t sync_frame;         /* Master LIN synchronized to AC: 1-Master Synchro */
    uint8_t timer_update;       /* Update Timer: 1-Update Timer */
    uint8_t remote_on_dis;      /* Remote Input disable */
    uint8_t set_inverter_off;   /* Disable the Output for external invert */
    uint8_t set_inverter_on;    /* Enable the output for external invert */
    uint32_t actuators;         /* Status of connected actuators */
} dometic_shape_ctrl_t;

/**
 * Structure to store information frame data.
*/
typedef struct
{
    uint8_t mode;               /* AC Function Mode */
    uint8_t fan_speed_mode;            /* Fan setting */
    uint8_t fan_spd;            /* Fan setting */
    uint8_t fan_md;             /* Fan setting */
    uint8_t light_internal;     /* Lighting system status: 1- Lighting system on */
    uint8_t light_dmr;          /* Lighting system dimming: 50% or 100% */
    uint8_t power;              /* Air Conditioner status: 1- A/C on */
    uint16_t temp;              /* Target temperature: Temperature = Temp + 16 */
    uint8_t sleep_mode;         /* Sleep Function: 1- Sleep mode enabled */
    uint8_t timer_off_mode;     /* Turning off timer flag: 1-Timer Off enabled */
    uint8_t timer_on_mode;      /* Turining on timer flag: 1- Timer On enabled */
    uint16_t on_timer_bits;     /* Turning on minutes left: TimeON = OnTimerMSBits * 10 */
    uint16_t off_timer_min;    /* Turning off minutes left: TimeOFF = OffTimerMSBits * 10 */
    uint8_t ac0_heat_enable;   /* Bit indicates if the Heater is configured/available: 1 = available, 0 = not available */
    uint8_t ac0_light_enable;    /* Bit indicates if the ADB Light is configured/available: 1 = available, 0 = not available */
    uint8_t light_external; /* Roof top unit lighting status: 1= External lighting on */
    int32_t int_temp;           /* Internal temperature Level: Internal temperature value (0 – 63) */
    uint8_t local_change;       /* Status Modified from external control: 1-Sync Request */
    uint8_t heater_run;         /* Heater is running 1 – Heater ON */
    uint8_t comp_run;           /* 1-Compressor ON */
    uint8_t no_main;            /* Mains Failure: 1-Mains Failed */
    uint8_t error;              /* Temp. Probe error: 1- error */
    uint8_t remote_ctrl_status; /* Remote control disable status: 1-Disabled */
    uint8_t timer_on_req;       /* AC On due to a timer on timeout: 1- Timer On Completed */
    uint8_t timer_off_req;      /* AC Off due to a timer off timeout: 1 - Timer Off Completed */
    uint8_t remote_on;          /* AC On due to a Remote input request: 1 - Remote On Enabled */
    uint8_t no_init;            /* Card not initialized: 1 - NotInit */
    uint8_t lin_error;          /* Error on LIN bus: 1 -Error */
    uint8_t ci_error;           /* Error on CI bu */
    uint32_t actuators;         /* Status of connected actuators */
    uint8_t test_status;        /* Check if LIN test mode is requested */
    uint16_t status[DDMP2_MAX_VALUE_SIZE]; /* Active error list */
    uint16_t status_size;                  /* Active error list size */
} dometic_shape_info_t;

typedef struct
{
    /* TODO: Fill in this structure once we now the mappings */
    int dummy;
} dometic_shape_var_info_t;

typedef struct
{
    /* TODO: Fill in this structure once we now the mappings */
    int dummy;
} dometic_shape_var_ctrl_t;

#define DOMETIC_AC_CTRL_SIZE 8
#define DOMETIC_AC_INFO_SIZE 8
void dometic_ac_C_Extract (dometic_ac_ctrl_t* pCtrl, uint8_t* pu8Data);
void dometic_ac_I_Extract(dometic_ac_info_t*  pInfo, uint8_t* pu8Data);
void dometic_ac_C_Stuff(uint8_t* pu8Data, dometic_ac_ctrl_t*  pCtrl);

extern int32_t dometic_ac_mode_get(void);

#define DOMETIC_SHARC_CTRL_SIZE 8
#define DOMETIC_SHARC_INFO_SIZE 8
void dometic_sharc_wtr_C_Extract (dometic_sharc_ctrl_t * pCtrl, const uint8_t * const pu8Data);
void dometic_sharc_wtr_I_Extract(dometic_sharc_info_t *  pInfo, uint8_t * pu8Data);
void dometic_sharc_wtr_C_Stuff(uint8_t * pu8Data, dometic_sharc_ctrl_t *  pCtrl);
void dometic_sharc_wtr_I_Stuff(uint8_t * pu8Data, dometic_sharc_info_t * pInfo);
void dometic_sharc_air_C_Extract (dometic_sharc_ctrl_t * pCtrl, const uint8_t * const pu8Data);
void dometic_sharc_air_I_Extract(dometic_sharc_info_t *  pInfo, uint8_t * pu8Data);
void dometic_sharc_air_C_Stuff(uint8_t * pu8Data, dometic_sharc_ctrl_t *  pCtrl);
void dometic_sharc_air_I_Stuff(uint8_t * pu8Data, dometic_sharc_info_t * pInfo);

#define DOMETIC_REQ_FRAME_SIZE_DIAG 8

void dometic_gen_pid_diag_I_Extract(dometic_gen_pid_diag_info_t* pInfo, uint8_t* pu8Data);
//extern void dometic_freshwell_pid_diag_I_Extract(dometic_freshwell_pid_diag_info_t* pInfo, uint8_t* pu8Data);

void dometic_sharc_pid_diag_I_Extract(dometic_sharc_pid_diag_info_t* pInfo, uint8_t* pu8Data);

void dometic_sharc_serial_diag_C_Stuff(uint8_t* pu8Data, dometic_sharc_serial_diag_ctrl_t* pCtrl);
void dometic_sharc_serial_diag_I_Extract(dometic_sharc_serial_diag_info_t* pInfo, uint8_t* pu8Data);
void dometic_sharc_assignnad_diag_C_Stuff(uint8_t* pu8Data, dometic_sharc_nad_diag_ctrl_t* pCtrl);
void dometic_sharc_assignnad_diag_I_Extract(dometic_sharc_nad_diag_info_t* pInfo, uint8_t* pu8Data);
void dometic_sharc_assignframe_diag_C_Stuff(uint8_t* pu8Data, dometic_sharc_frame_diag_ctrl_t* pCtrl);
void dometic_sharc_assignframe_diag_I_Extract(dometic_sharc_frame_diag_info_t* pInfo, uint8_t* pu8Data);

void dometic_generic_negative_diag_I_Extract(dometic_gen_neg_diag_info_t* pInfo, uint8_t* pu8Data);
#define DOMETIC_SHAPE_AC_INFO_SIZE 8  /** LIN info frame size */
#define DOMETIC_SHAPE_AC_CTRL_SIZE 8  /** LIN control frame size */
void dometic_shape_ac_I_Extract(dometic_shape_info_t* pInfo, const uint8_t* pu8Data);
void dometic_shape_ac_C_Extract(dometic_shape_ctrl_t* pCtrl, const uint8_t* pu8Data);
void dometic_shape_ac_I_Stuff(uint8_t* pu8Data, const dometic_shape_info_t * pInfo);
void dometic_shape_ac_C_Stuff(uint8_t* pu8Data, dometic_shape_ctrl_t * pCtrl);
void dometic_shape_var_I_Extract(dometic_shape_var_info_t* pInfo, const uint8_t* pu8Data);
void dometic_shape_var_C_Extract(dometic_shape_var_ctrl_t* pCtrl, const uint8_t* pu8Data);
void dometic_shape_var_I_Stuff(uint8_t* pu8Data, const dometic_shape_var_info_t* pInfo);
void dometic_shape_var_C_Stuff(uint8_t* pu8Data, const dometic_shape_var_ctrl_t* pCtrl);

uint8_t  dometic_get_info_page(uint8_t id);

/* For Inventilate Project */
/* ----------------------- */

/**
 * @brief       Remote data structure for Inventilate product, variant v1
 *
 * Remote data structure is common to request/response frames.
 *
 * The remote data structure is part of synchronization procedure of controllers. The received
 * remote data structure and stored remote data are compared to evaluate if master has successfully
 * synchronized to slave.
 *
 * Remote data structure is mapped directly to DDM interface. Each member of the structure
 * corresponds to exactly one DDM parameter.
 *
 * Each member of the structure should be named as DDM parameter to make the mapping to DDM tables
 * easier to read.
 *
 * The size of each member is of minimal size needed to keep the data range (to reduce memory usage).
 * These fields are then expanded to int32_t/other types and stored in DDM during the runtime. All
 * intergral structure member types are of signed type to ease the data conversion to DDM.
 *
 * In the comment section of the structure member it is noted:
 * - [byte][bitfield] as the source of the value from byte array. If multiple sources are used to
 *   generate one value for DDM, then all bytes/bitfields are noted here.
 * - Description from CI/LIN bus description
 * - Name of DDM paramater which is mapped to this field.
 */
typedef struct dometic_invent_v1_remote_data
{
    /* Remote Data 0 */
    uint8_t inv_pwr;                    /* [0][0:0]: Power Status, maps to AC0MD */
    uint8_t inv_mode;                   /* [0][1:3]: Mode, maps to AC0MD */
    uint8_t inv_ledstrip_bright;        /* [0][4:6]: LED Strip brightness, maps to AC0MD */

    /* Remote Data 1 */
    uint8_t inv_filtreset;              /* [1][0:1]: Filter Reset, maps to AC0MD */
    uint8_t inv_stormodeavl;            /* [1][2:2]: Storage mode available, maps to AC0MD */

    /* Remote Data 2 */
	/* Read only data */

    /* Remote Data 3 to 6 */
	/* Read only data */

} dometic_invent_v1_remote_data_t;

typedef struct dometic_invent_v1_ctrl_status
{
    uint8_t ctrl_page;                  /* [7][0:1]: Bit 0-1 */
    uint8_t info_pageid;                /* [7][3:4]: Info page, not handled in current implementation. */
                                        /* [7][5:7] RFU */
} dometic_invent_v1_ctrl_status_t;

typedef struct dometic_invent_v1_ctrl_protocol
{
    bool is_sync_frame;                 /* [7][2:2] Master LIN synchronized to Invent: 1-Master Synchro */
} dometic_invent_v1_ctrl_protocol_t;

typedef struct dometic_invent_v1_ctrl_bundle
{
    dometic_invent_v1_remote_data_t rd;
    dometic_invent_v1_ctrl_status_t st;
    dometic_invent_v1_ctrl_protocol_t protocol;
} dometic_invent_v1_ctrl_bundle_t;

typedef struct dometic_invent_v1_info_status
{
    /* Remote Data 1 */
    uint8_t inv_airqua;                 /* [1][5:7]: Air quality level, maps to IV0AQST (Read Only) */

    /* Remote Data 2 */
    uint8_t inv_pwrsrcstat;             /* [2][0:1]: Power source Status, maps to IV0PWRSRC (Read Only) */
    uint8_t inv_wifistat;               /* [2][2:4]: WIFI Status, maps to WIFI0STS (Read Only) */
    uint8_t inv_ionizstat;              /* [2][5:5]: Ionizer Status, maps to IV0IONST (Read Only) */
                                        /* [2][6:7]: Reserved */

    /* Remote Data 3 to 6 */
    uint32_t inv_errcode;               /* [3][0:7]: Error Code Byte 0 – LSB, maps to IV0ERRST */
                                        /* [4][0:7]: Error Code Byte 1, maps to IV0ERRST */
                                        /* [5][0:7]: Error Code Byte 2, maps to IV0ERRST */
                                        /* [6][0:7]: Error Code Byte 3 – MSB, maps to IV0ERRST */
                                        /* Read Only */
} dometic_invent_v1_info_status_t;

typedef struct dometic_invent_v1_info_protocol
{
    bool is_local_change_frame;
    uint8_t info_page;                  //!< \~ [7][1:2]
    uint8_t active_error_flag;          //!< \~ [7][3] 
    uint8_t response_error;             //!< \~ [7][7]
} dometic_invent_v1_info_protocol_t;

typedef struct dometic_invent_v1_info_bundle
{
    dometic_invent_v1_remote_data_t rd;
    dometic_invent_v1_info_status_t st;
    dometic_invent_v1_info_protocol_t protocol;
} dometic_invent_v1_info_bundle_t;

/**
 * @brief       Extract the contents of LIN frame contained in @a data argument to Dometic structure
 *              pointed by @a bundle_buffer argument
 *
 * @param       bundle_buffer Pointer to bundle buffer which contains Dometic structure
 * @param       data Pointer to LIN frame buffer
 */
void dometic_invent_v1_ctrl_extract(dometic_invent_v1_ctrl_bundle_t * bundle_buffer, const uint8_t data[DOMETIC_INVENT_V1_DATA_LENGTH]);

/**
 * @brief       Stuff the contents of Dometic structure pointed by @a bundle_buffer argument to LIN
 *              frame buffer pointed by @a data argument.
 *
 * @param       bundle_buffer Pointer to bundle buffer which contains Dometic structure
 * @param       data Pointer to LIN frame buffer
 */
void dometic_invent_v1_info_stuff(const dometic_invent_v1_info_bundle_t * bundle_buffer, uint8_t data[DOMETIC_INVENT_V1_DATA_LENGTH]);

/**
 * @brief       Convert Dometic ledbright member value to DIM0LVL DDM value
 *
 * @param       conv_data Pointer to Dometic md member
 * @param       conv_data_size Sizeof Dometic md member in bytes
 * @param       ddm_data Pointer to allocated memory where the result should be placed
 * @param       ddm_data_size Size of the allocated memory pointed by `ddm_data`
 * @return      How many bytes are written to `ddm_data`
 */
size_t dometic_invent_v1_conv_ledbright_to_dim0lvl(const void * conv_data, size_t conv_data_size, void * ddm_data, size_t ddm_data_size);

/**
 * @brief       Convert DIM0LVL value to remote data member ledbright member in Dometic structure
 *
 * @param       ddm_data Pointer to DIM0LVL data
 * @param       ddm_data_size Size of DIM0LVL data in bytes
 * @param       conv_data_size Size of info status error member in bytes
 * @param       conv_data Pointer to info status error member
 */
void dometic_invent_v1_conv_dim0lvl_to_ledbright(const void * const * ddm_data, const size_t * ddm_data_size, size_t conv_data_size, void * conv_data);

#define dometic_invent_v1_conv_pwr_to_iv0pwron              dometic_conv_any_int_to_any_ddm
#define dometic_invent_v1_conv_iv0pwrn_to_pwr               dometic_conv_any_ddm_to_any_int

#define dometic_invent_v1_conv_mode_to_iv0mode              dometic_conv_any_int_to_any_ddm
#define dometic_invent_v1_conv_iv0mode_to_mode              dometic_conv_any_ddm_to_any_int

#define dometic_invent_v1_conv_filtreset_to_iv0filst        dometic_conv_any_int_to_any_ddm
#define dometic_invent_v1_conv_iv0filst_to_filtreset        dometic_conv_any_ddm_to_any_int

#define dometic_invent_v1_conv_storage_to_iv0storage        dometic_conv_any_int_to_any_ddm
#define dometic_invent_v1_conv_iv0storage_to_storage        dometic_conv_any_ddm_to_any_int

#define dometic_invent_v1_conv_airqua_to_iv0aqst            dometic_conv_any_int_to_any_ddm
#define dometic_invent_v1_conv_iv0aqst_to_airqua            dometic_conv_any_ddm_to_any_int

#define dometic_invent_v1_conv_pwrsrcstat_to_iv0pwrsrc      dometic_conv_any_int_to_any_ddm
#define dometic_invent_v1_conv_iv0pwrsrc_to_pwrsrcstat      dometic_conv_any_ddm_to_any_int

#define dometic_invent_v1_conv_wifistat_to_wifi0sts         dometic_conv_any_int_to_any_ddm
#define dometic_invent_v1_conv_wifi0sts_to_wifistat         dometic_conv_any_ddm_to_any_int

#define dometic_invent_v1_conv_ionizstat_to_iv0ionst        dometic_conv_any_int_to_any_ddm
#define dometic_invent_v1_conv_iv0ionst_to_ionizstat        dometic_conv_any_ddm_to_any_int

#define dometic_invent_v1_conv_errcode_to_iv0errst          dometic_conv_any_int_to_any_ddm
#define dometic_invent_v1_conv_iv0errst_to_errcode          dometic_conv_any_ddm_to_any_int

/*Lin dev Start For Bridge NRX Project */
typedef struct dometic_nrx_v1_remote_data //common params
{
    /* Byte 0 */
    uint8_t nrx_userMode;               /* [0][3:4]: 0: Performance Mode 1: Quiet Mode 2: Eco Mode 3: Freezer Mode */
    /* Byte 1 */
    uint8_t nrx_setTemp;                /* [1][0:3]: 0: step 1 1: step 2 2: step 3 3: step 4 4: step 5 */
    /* Byte 7 */
    uint8_t nrx_power;                  /* [7][7:7]: 0: off  1: on */
} dometic_nrx_v1_remote_data_t;

typedef struct dometic_nrx_v1_ctrl_status
{
    uint8_t ctrl_page;                  /* [7][0:1]: Bit 0-3 */
    // uint8_t info_pageid;                /* [7][3:4]: Info page, not handled in current implementation. */
                                        /* [7][5:7] RFU */
} dometic_nrx_v1_ctrl_status_t;

typedef struct dometic_nrx_v1_ctrl_protocol
{
    bool is_sync_frame;                 /* [7][3:3] Master LIN synchronized to NRX: 1-Master Synchro */
} dometic_nrx_v1_ctrl_protocol_t;

typedef struct dometic_nrx_v1_ctrl_bundle
{
    dometic_nrx_v1_remote_data_t rd;
    dometic_nrx_v1_ctrl_status_t st;
    dometic_nrx_v1_ctrl_protocol_t protocol;
} dometic_nrx_v1_ctrl_bundle_t;

typedef struct dometic_nrx_v1_info_status
{
    /* Byte 0 */
    uint8_t nrx_CI_BUS;                 /* [0][5:5]: 0: Turn Off  1: Turn On (Read Only) */
    uint8_t nrx_comprStatus;       /* [0][6:6]: 0: Off  1: On (Read Only) */
    /* Byte 3 */
    uint8_t nrx_C_Type;                 /* [3][0:0]: 0:ABSORPTION, 1:Compressor (Read Only) */

    /* Byte 4-Byte 5 */
    int32_t nrx_Fresh_Temp;             /* [3-4][0:7]:  (Read Only) */

    /* Byte 6 */
    uint8_t nrx_Error;                  /* [6][0:6]:  (Read Only) */
} dometic_nrx_v1_info_status_t;

typedef struct dometic_nrx_v1_info_protocol
{
    bool is_local_change_frame;
} dometic_nrx_v1_info_protocol_t;

typedef struct dometic_nrx_v1_info_bundle
{
    dometic_nrx_v1_remote_data_t rd;
    dometic_nrx_v1_info_status_t st;
    dometic_nrx_v1_info_protocol_t protocol;
} dometic_nrx_v1_info_bundle_t;

/**
 * @brief       Extract the contents of LIN frame contained in @a data argument to Dometic structure
 *              pointed by @a bundle_buffer argument
 *
 * @param       bundle_buffer Pointer to bundle buffer which contains Dometic structure
 * @param       data Pointer to LIN frame buffer
 */
void dometic_nrx_v1_ctrl_extract(dometic_nrx_v1_ctrl_bundle_t * bundle_buffer, const uint8_t data[DOMETIC_NRX_V1_DATA_LENGTH]);

/**
 * @brief       Stuff the contents of Dometic structure pointed by @a bundle_buffer argument to LIN
 *              frame buffer pointed by @a data argument.
 *
 * @param       bundle_buffer Pointer to bundle buffer which contains Dometic structure
 * @param       data Pointer to LIN frame buffer
 */
void dometic_nrx_v1_info_stuff(const dometic_nrx_v1_info_bundle_t * bundle_buffer, uint8_t data[DOMETIC_NRX_V1_DATA_LENGTH]);

/**
 * @brief       Convert Dometic ledbright member value to DIM0LVL DDM value
 *
 * @param       conv_data Pointer to Dometic md member
 * @param       conv_data_size Sizeof Dometic md member in bytes
 * @param       ddm_data Pointer to allocated memory where the result should be placed
 * @param       ddm_data_size Size of the allocated memory pointed by `ddm_data`
 * @return      How many bytes are written to `ddm_data`
 */
size_t dometic_nrx_v1_conv_ledbright_to_dim0lvl(const void * conv_data, size_t conv_data_size, void * ddm_data, size_t ddm_data_size);

/**
 * @brief       Convert DIM0LVL value to remote data member ledbright member in Dometic structure
 *
 * @param       ddm_data Pointer to DIM0LVL data
 * @param       ddm_data_size Size of DIM0LVL data in bytes
 * @param       conv_data_size Size of info status error member in bytes
 * @param       conv_data Pointer to info status error member
 */
void dometic_nrx_v1_conv_dim0lvl_to_ledbright(const void * const * ddm_data, const size_t * ddm_data_size, size_t conv_data_size, void * conv_data);
        
#define dometic_nrx_v1_conv_nrx_userMode_to_nrx0mode           dometic_conv_any_int_to_any_ddm
#define dometic_nrx_v1_conv_nrx0mode_to_nrx_userMode           dometic_conv_any_ddm_to_any_int

#define dometic_nrx_v1_conv_nrx_setTemp_to_nrx0lvl             dometic_conv_any_int_to_any_ddm
#define dometic_nrx_v1_conv_nrx0lvl_to_nrx_setTemp             dometic_conv_any_ddm_to_any_int

#define dometic_nrx_v1_conv_nrx_power_to_nrx0pwron             dometic_conv_any_int_to_any_ddm
#define dometic_nrx_v1_conv_nrx0pwron_to_nrx_power             dometic_conv_any_ddm_to_any_int

#define dometic_nrx_v1_conv_nrx0compstat_to_nrx_comprStatus    dometic_conv_any_ddm_to_any_int
#define dometic_nrx_v1_conv_nrx0temp_to_nrx_Fresh_Temp         dometic_conv_any_ddm_to_any_int
#define dometic_nrx_v1_conv_nrx0errst_to_nrx_Error             dometic_conv_any_ddm_to_any_int

/* End For Inventilate Project */

#endif //DOMETIC_H_

