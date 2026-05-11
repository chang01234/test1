/**
 * \brief Product database library, stores, caches and handles DDMP2 frames
 *        related to PROD<X> classes.
 */

#ifndef PRODUCT_DATABASE_H_
#define PRODUCT_DATABASE_H_

#include <stdbool.h>
#include <stdint.h>

#include "configuration.h"
#include "uid_generator.h"

#define PROD_DB_ERR_MUTEX_NOT_CREATED         -1
#define PROD_DB_ERR_MEM_ALLOC_FAILED          -2
#define PROD_DB_ERR_UID_NOT_GENERATED         -3
#define PROD_DB_ERR_INVALID_DATA              -4
#define PROD_DB_ERR_INVALID_DDM_INST          -5
#define PROD_DB_ERR_CANNOT_WRITE_TO_NVS       -6
#define PROD_DB_ERR_PRODUCT_ALREADY_EXISTS    -7
#define PROD_DB_ERR_NO_VALID_FIELD            -8
#define PROD_DB_ERR_PROD_CLASS_ALREADY_EXISTS -9
#define PROD_DB_ERR_PROD_CLASS_DOES_NOT_EXIST -10
#define PROD_DB_ERR_INVALID_UID               -11

#define INVALID_DDM_INSTANCE -1
#define INVALID_CONNECTOR_ID 0xFF

#define PROD_DB_MAX_FIELD_SIZE (64 + 1)

typedef struct prodxprop_type_t
{
    union
    {
        struct
        {
            uint8_t cls : 4;   //< Bits 0-3
            uint8_t intf : 4;  //< Bits 4-7
        } type;
        uint8_t data;
    };
} prodxprop_type_t;

typedef struct ProdDBMetaData_t
{
    union
    {
        int8_t data_i8[4];
        uint8_t data_u8[4];
        int16_t data_i16[2];
        uint16_t data_u16[2];
        int32_t data_i32;
        uint32_t data_u32;
    };
} ProdDBMetaData_t;

#define PRODXPROP_TYPE_CLASS_GENERIC     0  //< Unknown or generic type
#define PRODXPROP_TYPE_CLASS_CLIMATE     1  //< Climate products (heaters/air-conditioners/fans)
#define PRODXPROP_TYPE_CLASS_POWER       2  //< Mobile power products (batteries etc)
#define PRODXPROP_TYPE_INTERFACE_UNKNOWN 0  //< Unknown interface
#define PRODXPROP_TYPE_INTERFACE_BLE     1  //< Product detected on BLE
#define PRODXPROP_TYPE_INTERFACE_WIFI    2  //< Product detected on WiFi
#define PRODXPROP_TYPE_INTERFACE_RVC     3  //< Product detected on RVC bus
#define PRODXPROP_TYPE_INTERFACE_CI_BUS  4  //< Product detected on CI-Bus
#define PRODXPROP_TYPE_INTERFACE_CMC     5  //< Product detected on CMC bus
#define PRODXPROP_TYPE_INTERFACE_UART    6  //< Product detected on UART (external)
#define PRODXPROP_TYPE_INTERFACE_RS485   7  //< Product detected on RS485 bus

typedef struct prod_database
{
    char mdl[PROD_DB_MAX_FIELD_SIZE];
    char sn[PROD_DB_MAX_FIELD_SIZE];
    char name[PROD_DB_MAX_FIELD_SIZE];
    char uid[DICM_UID_KEY_STR_LEN];
} prod_database_t;

typedef enum
{
    FIELD_NAME = 0,
    FIELD_SN,
    FIELD_SKU,
    FIELD_PNC,
    FIELD_FWVER,
    FIELD_HWVER,
    FIELD_MDL,
    FIELD_EAN,
    FIELD_DESC,
    FIELD_CLIST,
    FIELD_PROP,  // only when reading the cache, and trigger a PUB on update
    FIELD_PROP_SA,
    FIELD_PROP_TYPE,
    FIELD_PROP_INST,
    FIELD_PROP_CLASS,
    FIELD_MANUF,
    FIELD_UID,
    FIELD_CONN_ID,
    FIELD_DDM_INST,
    FIELD_NO_OF_LINKED_CLASSES,
    FIELD_NO_OF_LINKED_PROP_CLASSES,
    FIELD_RESET,
    FIELD_INDICATE,
    FIELD_FWID,
    FIELD_METADATA,
    FIELD_INVALID
} ProdClassDescField_t;

typedef enum
{
    // Power products (Bits 15-0)
    PRODDB_PRODUCTTYPE_SHUNT = (1 << 0),
    PRODDB_PRODUCTTYPE_BATTERY = (1 << 1),
    PRODDB_PRODUCTTYPE_CHARGER = (1 << 2),
    PRODDB_PRODUCTTYPE_SOLARCHARGER = (1 << 3),
    PRODDB_PRODUCTTYPE_INVERTER = (1 << 4),
    PRODDB_PRODUCTTYPE_INVERTERCHARGER = (1 << 5),
    PRODDB_PRODUCTTYPE_CONVERTER = (1 << 6),

    // Climate products (Bits 23-16)
    PRODDB_PRODUCTTYPE_THERMOSTAT = (1 << 16),
    PRODDB_PRODUCTTYPE_AIRCOND = (1 << 17),
    PRODDB_PRODUCTTYPE_VENTILATION = (1 << 18),
    PRODDB_PRODUCTTYPE_HEATING = (1 << 19),

    // Generic products (Bits 30-24)
    PRODDB_PRODUCTTYPE_GENERIC = (1 << 24),

    // Ending entry
    PRODDB_PRODUCTTYPE_LAST = (1 << 31),
} ProdDBProductType_t;

typedef void (*prod_reset_handler_t)(int32_t new_reset, int ddm_instance);
typedef void (*prod_indicate_handler_t)(bool indicate, int ddm_instance);

int ProdDBProdClassNodeCreate(const void *data, size_t data_size, uint8_t connector_id);
void MAYBE_UNUSED ProdDBProdClassNodeDelete(int ddm_instance);
int ProdDBInit(void);
void ProdDBDeInit(void);
bool ProdDBUpdateCache(const void *prod_field, size_t prod_field_size, ProdClassDescField_t field, int ddm_instance);
void ProdDBReadCache(ProdClassDescField_t field, int ddm_instance, void *data, size_t *data_size);
int ProdDBFrameHandler(const DDMP2_FRAME *const p_frame);
int ProdDBVirtualProdClassNodeCreate(const char *uid, int ddm_instance);
void ProdDBVirtualUpdateCache(const void *prod_field, size_t prod_field_size, ProdClassDescField_t field, int ddm_instance);
bool ProdDBProdClassNodeExists(int ddm_instance);
void ProdDBProdClassNodeAddResetHandler(int ddm_instance, prod_reset_handler_t handler);
void ProdDBProdClassNodeAddIndicateHandler(int ddm_instance, prod_indicate_handler_t handler);
int ProdDBSearchCache(const void *prod_field, ProdClassDescField_t field);
int32_t ProdDBSetProductType(int ddm_instance, int32_t input);
int32_t ProdDBGetProductType(int ddm_instance);
bool ProdDBIsValidSemverVersion(const char *version);
#endif /* PRODUCT_DATABASE_H_ */
