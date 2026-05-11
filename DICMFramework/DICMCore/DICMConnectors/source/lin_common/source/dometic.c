
#include "configuration.h"

// #if CONNECTOR_LIN

/** Includes ******************************************************************/
#include "dometic.h"
#include "frame_util.h"
#include "pstore.h"
#include <string.h>

// #define DEBUG_LOG_APPLIN

#define EXTRACT_BITS(value, start, length) \
    (((value) >> (start)) & ((0x1u << (length)) - 1u))

#define STUFF_BITS(value, start, length) \
    (((uint8_t)(value) & ((0x1u << (length)) - 1u)) << (start))

static int32_t dometic_sharc_air_convert_temp_tosys(int32_t i32Value);
static int32_t dometic_sharc_air_convert_temp_tolin(int32_t i32Value);
static int32_t dometic_sharc_air_convert_itemp_tolin(int32_t i32Value);
static int32_t dometic_sharc_air_convert_itemp_tosys(int32_t i32Value);
static int32_t dometic_sharc_air_convert_wtrtemp_tosys(int32_t i32Value);
static int32_t dometic_sharc_air_convert_wtrtemp_tolin(int32_t i32Value);
static int32_t dometic_nrx_bridge_convert_temp_tolin(int32_t i32Value);

/* Global variable to store requested Info page ID */
uint8_t wtr_info_page;
uint8_t air_info_page;

/**
 *  \brief Conversion function for Shape temperature value.
 *
 *  This function will convert temperature for DDM parameter value.
 *
 *
 *  \param i32Value    LIN received raw value
 *
 */
static int32_t dometic_shape_convert_temp_tosys(int32_t i32Value)
{
    int32_t res = 0;
    /* Check if value is in range, range is defined based on size of the signal and conversion formula */
    if ((i32Value >= 0) && (i32Value <= 15))
    {
        res = (i32Value + 16) * 1000;
    }
    else
    {
        /* Define out of range value behavior */
        LOG(W, "LIN conversion error, or it is not initialised yet: %d", i32Value);
    }

    return res;
}

/**
 *  \brief Conversion function for Shape temperature value.
 *
 *  This function will convert temperature for LIN frame value.
 *
 *
 *  \param i32Value    DDM parameter physical value
 *
 */
static int32_t dometic_shape_convert_temp_tolin(int32_t i32Value)
{
    int32_t res = 0;
    /* Check if value is in range, range is defined based on size of the signal and conversion formula */
    if (i32Value >= 16)
    {
        res = (i32Value / 1000) - 16;
    }
    else
    {
        /* Define out of range value behavior */
        LOG(W, "LIN conversion error, or it is not initialised yet: %d", i32Value);
    }

    return res;
}

/**
 *  \brief Conversion function for Shape internal temperature value.
 *
 *  This function will convert internal temperature for DDM parameter value.
 *
 *
 *  \param i32Value    LIN received raw value
 *
 */
static int32_t dometic_shape_convert_itemp_tosys(int32_t i32Value)
{
    int32_t res = 0;
    /* Check if value is in range, range is defined based on size of the signal and conversion formula */
    if (i32Value <= 63)
    {
        res = i32Value * 1000;
    }
    else
    {
        /* Define out of range value behavior */
        LOG(W, "LIN conversion error, or it is not initialised yet: %d", i32Value);
    }

    return res;
}

/**
 *  \brief Conversion function for Shape internal temperature value.
 *
 *  This function will convert internal temperature for LIN frame value.
 *
 *
 *  \param i32Value    DDM parameter physical value
 *
 */
static int32_t dometic_shape_convert_itemp_tolin(int32_t i32Value)
{
    int32_t res = 0;
    /* Check if value is in range, range is defined based on size of the signal and conversion formula */
    if (i32Value <= 63000)
    {
        res = i32Value / 1000;
    }
    else
    {
        /* Define out of range value behavior */
        LOG(W, "LIN conversion error, or it is not initialised yet: %d", i32Value);
    }

    return res;
}

typedef enum shape_lin_ac_mode
{
    SHAPE_LIN_AC_MODE_AUTOMATIC,
    SHAPE_LIN_AC_MODE_COOL,
    SHAPE_LIN_AC_MODE_DEHUMIDIFICATION,
    SHAPE_LIN_AC_MODE_HEAT,
    SHAPE_LIN_AC_MODE_VENTILATION,
} shape_lin_ac_mode_t;

typedef enum shape_lin_fspd_mode
{
    SHAPE_LIN_FSPD_MODE_LOW,
    SHAPE_LIN_FSPD_MODE_MEDIUM,
    SHAPE_LIN_FSPD_MODE_HIGH,
    SHAPE_LIN_FSPD_MODE_TURBO,
    SHAPE_LIN_FSPD_MODE_AUTO,
} shape_lin_fspd_mode_t;

typedef enum shape_lin_light_internal
{
    SHAPE_LIN_LIGHT_INTERNAL_OFF,
    SHAPE_LIN_LIGHT_INTERNAL_50,
    SHAPE_LIN_LIGHT_INTERNAL_100,
    SHAPE_LIN_LIGHT_INTERNAL_RESERVED,
} shape_lin_light_internal_t;

typedef enum sharc_wtr_lin_water_heater_mode
{
    SHARC_WTR_LIN_WATER_HEATER_MODE_ECO,
    SHARC_WTR_LIN_WATER_HEATER_MODE_HOT,
    SHARC_WTR_LIN_WATER_HEATER_MODE_BOOST
} sharc_wtr_lin_water_heater_mode_t;

static shape_lin_ac_mode_t shape_ddm_to_lin_md(AC0MD_ENUM ddm_md)
{
    switch (ddm_md)
    {
    case AC0MD_AUTO:
        return SHAPE_LIN_AC_MODE_AUTOMATIC;
    case AC0MD_COOL:
        return SHAPE_LIN_AC_MODE_COOL;
    case AC0MD_DRY:
        return SHAPE_LIN_AC_MODE_DEHUMIDIFICATION;
    case AC0MD_HEAT:
        return SHAPE_LIN_AC_MODE_HEAT;
    case AC0MD_FAN:
        return SHAPE_LIN_AC_MODE_VENTILATION;
    default:
        return SHAPE_LIN_AC_MODE_AUTOMATIC;
    }
}

static AC0FSPD_ENUM shape_lin_to_ddm_fspd(shape_lin_fspd_mode_t lin_fspd_mode)
{
    switch (lin_fspd_mode)
    {
    case SHAPE_LIN_FSPD_MODE_LOW:
        return AC0FSPD_LOW;
    case SHAPE_LIN_FSPD_MODE_MEDIUM:
        return AC0FSPD_MED;
    case SHAPE_LIN_FSPD_MODE_HIGH:
        return AC0FSPD_HIGH;
    case SHAPE_LIN_FSPD_MODE_TURBO:
        return AC0FSPD_MAX;
    case SHAPE_LIN_FSPD_MODE_AUTO:
        return AC0FSPD_AUTO;
    default:
        return AC0FSPD_AUTO;
    }
}

static shape_lin_fspd_mode_t shape_ddm_to_lin_fspd(AC0FSPD_ENUM ddm_fspd_mode)
{
    switch (ddm_fspd_mode)
    {
    case AC0FSPD_LOW:
        return SHAPE_LIN_FSPD_MODE_LOW;
    case AC0FSPD_MED:
        return SHAPE_LIN_FSPD_MODE_MEDIUM;
    case AC0FSPD_HIGH:
        return SHAPE_LIN_FSPD_MODE_HIGH;
    case AC0FSPD_MAX:
        return SHAPE_LIN_FSPD_MODE_TURBO;
    case AC0FSPD_AUTO:
        return SHAPE_LIN_FSPD_MODE_AUTO;
    default:
        return SHAPE_LIN_FSPD_MODE_AUTO;
    }
}

size_t dometic_shape_ac_conv_md_to_ac0md(const void *conv_data, size_t conv_data_size, void *ddm_data, size_t ddm_data_size)
{
    /* md member is uint8_t type */
    const uint8_t *dometic_data = conv_data;
    /* The AC0MD is int32_t type */
    int32_t *ac0md_data = ddm_data;

    /* Ensure data sizes are correct */
    TRUE_CHECK_RETURN0(sizeof(*dometic_data) == conv_data_size);
    TRUE_CHECK_RETURN0(sizeof(*ac0md_data) <= ddm_data_size);
    /* Convert according to spec */
    switch (*dometic_data)
    {
    case SHAPE_LIN_AC_MODE_AUTOMATIC:
        *ac0md_data = AC0MD_AUTO;
        return sizeof(*ac0md_data);
    case SHAPE_LIN_AC_MODE_COOL:
        *ac0md_data = AC0MD_COOL;
        return sizeof(*ac0md_data);
    case SHAPE_LIN_AC_MODE_DEHUMIDIFICATION:
        *ac0md_data = AC0MD_DRY;
        return sizeof(*ac0md_data);
    case SHAPE_LIN_AC_MODE_HEAT:
        *ac0md_data = AC0MD_HEAT;
        return sizeof(*ac0md_data);
    case SHAPE_LIN_AC_MODE_VENTILATION:
        *ac0md_data = AC0MD_FAN;
        return sizeof(*ac0md_data);
    default:
        /* Tell to LIN generic code that we do not want to change DDM parameter value */
        return 0;
    }
}

void dometic_shape_ac_conv_ac0md_to_md(const void *const *ddm_data, const size_t *ddm_data_size, size_t conv_data_size, void *conv_data)
{
    /* Access only DDM 0 */
    /* md member is uint8_t type */
    uint8_t *dometic_data = conv_data;
    /* The AC0MD is int32_t type */
    const int32_t *ac0md_data = ddm_data[0];

    /* Ensure data sizes are correct */
    TRUE_CHECK_RETURN(sizeof(*dometic_data) == conv_data_size);
    TRUE_CHECK_RETURN(sizeof(*ac0md_data) == ddm_data_size[0]);
    /* Convert according to spec */
    *dometic_data = shape_ddm_to_lin_md(*ac0md_data);
}

size_t dometic_shape_ac_conv_fspd_to_ac0fspd(const void *conv_data, size_t conv_data_size, void *ddm_data, size_t ddm_data_size)
{
    /* fspd member is uint8_t type in the Dometic CTRL structure */
    const uint8_t *dometic_data = conv_data;
    /* The AC0FSPD is int32_t type */
    int32_t *ac0fspd_data = ddm_data;

    /* Ensure data sizes are correct */
    TRUE_CHECK_RETURN0(sizeof(*dometic_data) == conv_data_size);
    TRUE_CHECK_RETURN0(sizeof(*ac0fspd_data) <= ddm_data_size);
    /* Convert according to spec */
    *ac0fspd_data = shape_lin_to_ddm_fspd(*dometic_data);
    return sizeof(*ac0fspd_data);
}

void dometic_shape_ac_conv_ac0fspd_to_fspd(const void *const *ddm_data, const size_t *ddm_data_size, size_t conv_data_size, void *conv_data)
{
    /* Access only DDM 0 */
    /* fspd member is uint8_t type in the Dometic INFO structure */
    uint8_t *dometic_data = conv_data;
    /* The AC0FSPD is int32_t type */
    const int32_t *ac0fspd_data = ddm_data[0];

    /* Ensure data sizes are correct */
    TRUE_CHECK_RETURN(sizeof(*dometic_data) == conv_data_size);
    TRUE_CHECK_RETURN(sizeof(*ac0fspd_data) == ddm_data_size[0]);
    /* Convert according to spec */
    *dometic_data = shape_ddm_to_lin_fspd(*ac0fspd_data);
}

size_t dometic_shape_ac_conv_ttemp_to_ac0ttemp(const void *conv_data, size_t conv_data_size, void *ddm_data, size_t ddm_data_size)
{
    /* ttemp member is uint8_t in the Dometic structure */
    const uint8_t *dometic_data = conv_data;
    /* The AC0TTEMP is int32_t type */
    int32_t *ac0ttemp_data = ddm_data;

    /* Ensure data sizes are correct */
    TRUE_CHECK_RETURN0(sizeof(*dometic_data) == conv_data_size);
    TRUE_CHECK_RETURN0(sizeof(*ac0ttemp_data) <= ddm_data_size);

    *ac0ttemp_data = (int32_t)(*dometic_data + 16u) * 1000;
    return sizeof(int32_t);
}

void dometic_shape_ac_conv_ac0ttemp_to_ttemp(const void *const *ddm_data, const size_t *ddm_data_size, size_t conv_data_size, void *conv_data)
{
    /* Access only DDM 0 */
    /* ttemp member is uint8_t type in the Dometic INFO structure */
    uint8_t *dometic_data = conv_data;
    /* The AC0TTEMP is int32_t type */
    const int32_t *ac0ttemp_data = ddm_data[0];

    /* Ensure data sizes are correct */
    TRUE_CHECK_RETURN(sizeof(*dometic_data) == conv_data_size);
    TRUE_CHECK_RETURN(sizeof(*ac0ttemp_data) == ddm_data_size[0]);
    /* Convert according to spec */
    *dometic_data = (uint8_t)(*ac0ttemp_data / 1000) - 16u;
}

size_t dometic_shape_ac_conv_txm_to_ac0txm(const void *conv_data, size_t conv_data_size, void *ddm_data, size_t ddm_data_size)
{
    /* tonm member is uint16_t in the Dometic structure */
    const uint16_t *dometic_data = conv_data;
    /* The AC0TONM/AC0TOFFM is int32_t type */
    int32_t *ac0tonm_data = ddm_data;

    /* Ensure data sizes are correct */
    TRUE_CHECK_RETURN0(sizeof(*dometic_data) == conv_data_size);
    TRUE_CHECK_RETURN0(sizeof(*ac0tonm_data) <= ddm_data_size);
    /* Convert the two bytes according to spec: MSBbyte * 10 + LSBbyte */
    *ac0tonm_data = ((*dometic_data >> 8u) * 10) + (*dometic_data & 0xffu);
    /* Return that we have written some bytes to ddm_data */
    return sizeof(*ac0tonm_data);
}

size_t dometic_shape_ac_conv_tonm_to_ac0tonm(const void *conv_data, size_t conv_data_size, void *ddm_data, size_t ddm_data_size)
{
    /* tonm member is uint16_t in the Dometic structure */
    const uint16_t *dometic_data = conv_data;
    /* The AC0TONM is int32_t type */
    int32_t *ac0tonm_data = ddm_data;

    /* Ensure data sizes are correct */
    TRUE_CHECK_RETURN0(sizeof(*dometic_data) == conv_data_size);
    TRUE_CHECK_RETURN0(sizeof(*ac0tonm_data) <= ddm_data_size);
    /* Convert the two bytes according to spec: MSBbyte * 10 + LSBbyte */
    *ac0tonm_data = ((*dometic_data >> 8u) * 10) + (*dometic_data & 0xffu);
    /* Return that we have written some bytes to ddm_data */
    return sizeof(*ac0tonm_data);
}

void dometic_shape_ac_conv_ac0tonm_to_tonm(const void *const *ddm_data, const size_t *ddm_data_size, size_t conv_data_size, void *conv_data)
{
    /* Access only DDM 0 */
    /* tonm member is uint16_t in the Dometic structure */
    uint16_t *dometic_data = conv_data;
    /* The AC0TONM is int32_t type */
    const int32_t *ac0tonm_data = ddm_data[0];

    /* Ensure data sizes are correct */
    TRUE_CHECK_RETURN(sizeof(*dometic_data) == conv_data_size);
    TRUE_CHECK_RETURN(sizeof(*ac0tonm_data) == ddm_data_size[0]);
    /* Convert the DDM value to two bytes */
    *dometic_data = ((*ac0tonm_data / 10) << 8u) | (*ac0tonm_data % 10);
}

size_t dometic_shape_ac_conv_toffm_to_ac0toffm(const void *conv_data, size_t conv_data_size, void *ddm_data, size_t ddm_data_size)
{
    /* tonm member is uint16_t in the Dometic structure */
    const uint16_t *dometic_data = conv_data;
    /* The AC0TOFFM is int32_t type */
    int32_t *ac0toffm_data = ddm_data;

    /* Ensure data sizes are correct */
    TRUE_CHECK_RETURN0(sizeof(*dometic_data) == conv_data_size);
    TRUE_CHECK_RETURN0(sizeof(*ac0toffm_data) <= ddm_data_size);
    /* Convert the two bytes according to spec: MSBbyte * 10 + LSBbyte */
    *ac0toffm_data = ((*dometic_data >> 8u) * 10) + (*dometic_data & 0xffu);
    /* Return that we have written some bytes to ddm_data */
    return sizeof(*ac0toffm_data);
}

void dometic_shape_ac_conv_ac0toffm_to_toffm(const void *const *ddm_data, const size_t *ddm_data_size, size_t conv_data_size, void *conv_data)
{
    /* Access only DDM 0 */
    /* toffm member is uint16_t in the Dometic structure */
    uint16_t *dometic_data = conv_data;
    /* The AC0TOFFM is int32_t type */
    const int32_t *ac0toffm_data = ddm_data[0];

    /* Ensure data sizes are correct */
    TRUE_CHECK_RETURN(sizeof(*dometic_data) == conv_data_size);
    TRUE_CHECK_RETURN(sizeof(*ac0toffm_data) <= ddm_data_size[0]);
    /* Convert the DDM value to two bytes */
    *dometic_data = ((*ac0toffm_data / 10) << 8u) | (*ac0toffm_data % 10);
}

size_t dometic_shape_ac_conv_lgt_dmr_to_ac0lgt(const void *conv_data, size_t conv_data_size, void *ddm_data, size_t ddm_data_size)
{
    /* lgt_dmr member is uint8_t in the Dometic structure */
    const uint8_t *dometic_data = conv_data;
    /* The AC0LGT is int32_t type */
    int32_t *ac0lgt_data = ddm_data;

    /* Ensure data sizes are correct */
    TRUE_CHECK_RETURN0(sizeof(*dometic_data) == conv_data_size);
    TRUE_CHECK_RETURN0(sizeof(*ac0lgt_data) <= ddm_data_size);

    switch (*dometic_data)
    {
    case SHAPE_LIN_LIGHT_INTERNAL_OFF:
        *ac0lgt_data = 0;
        return sizeof(int32_t);
    case SHAPE_LIN_LIGHT_INTERNAL_50:
    case SHAPE_LIN_LIGHT_INTERNAL_100:
        *ac0lgt_data = 1;
        return sizeof(int32_t);
    default:
        // Reserved value provided by LIN, turn off the light
        *ac0lgt_data = 0;
        return sizeof(int32_t);
    }
}

size_t dometic_shape_ac_conv_lgt_dmr_to_ac0dmr(const void *conv_data, size_t conv_data_size, void *ddm_data, size_t ddm_data_size)
{
    /* lgt_dmr member is uint8_t in the Dometic structure */
    const uint8_t *dometic_data = conv_data;
    /* The AC0DMR is int32_t type */
    int32_t *ac0dmr_data = ddm_data;

    /* Ensure data sizes are correct */
    TRUE_CHECK_RETURN0(sizeof(*dometic_data) == conv_data_size);
    TRUE_CHECK_RETURN0(sizeof(*ac0dmr_data) <= ddm_data_size);

    switch (*dometic_data)
    {
    case SHAPE_LIN_LIGHT_INTERNAL_OFF:
        return 0;  // We didn't change the DDM
    case SHAPE_LIN_LIGHT_INTERNAL_50:
        *ac0dmr_data = 50;
        return sizeof(int32_t);
    case SHAPE_LIN_LIGHT_INTERNAL_100:
        *ac0dmr_data = 100;
        return sizeof(int32_t);
    default:
        return 0;  // We didn't change the DDM
    }
}

void dometic_shape_ac_conv_ac0lgt_ac0dmr_to_lgt_dmr(const void *const *ddm_data, const size_t *ddm_data_size, size_t conv_data_size, void *conv_data)
{
    /* This function expects two DDMs, at position 0 AC0LGT, at position 1 AC0DMR */
    /* lgt_dmr member is uint8_t in the Dometic structure */
    uint8_t *dometic_data = conv_data;
    /* This function receives 2 DDMs */
    const int32_t *ac0lgt_data = ddm_data[0];
    const int32_t *ac0dmr_data = ddm_data[1];

    /* Ensure data sizes are correct */
    TRUE_CHECK_RETURN(sizeof(*dometic_data) == conv_data_size);
    TRUE_CHECK_RETURN(sizeof(*ac0lgt_data) == ddm_data_size[0]);
    TRUE_CHECK_RETURN(sizeof(*ac0dmr_data) == ddm_data_size[1]);

    if (*ac0lgt_data == 0)
    {
        *dometic_data = SHAPE_LIN_LIGHT_INTERNAL_OFF;
    }
    else
    {
        switch (*ac0dmr_data)
        {
        case 50:
            *dometic_data = SHAPE_LIN_LIGHT_INTERNAL_50;
            break;
        case 100:
            *dometic_data = SHAPE_LIN_LIGHT_INTERNAL_100;
            break;
        default:
            // Reserved value was provided by DDM, return to LIN that light is off
            *dometic_data = SHAPE_LIN_LIGHT_INTERNAL_OFF;
            break;
        }
    }
}

size_t dometic_shape_ac_conv_st_actext_to_ac0actext(const void *conv_data, size_t conv_data_size, void *ddm_data, size_t ddm_data_size)
{
    /* Two bits:
     * bit 0: SetInverterOff, 1 - Force off
     * bit 1: SetInverterOn, 1 - Force on
     */
    /* actext member is uint8_t in the Dometic structure */
    const uint8_t *dometic_data = conv_data;
    /* The AC0ACTEXT is uint32_t type */
    uint32_t *ac0actext_data = ddm_data;

    /* Ensure data sizes are correct */
    TRUE_CHECK_RETURN0(sizeof(*dometic_data) == conv_data_size);
    TRUE_CHECK_RETURN0(sizeof(*ac0actext_data) <= ddm_data_size);

    switch (*dometic_data)
    {
    case (0x1u << 0u):
        *ac0actext_data &= ~(AC0ACTEXT_IN_INVERTERON_BITMASK);
        return sizeof(uint32_t);
    case (0x1u << 1u):
        *ac0actext_data |= AC0ACTEXT_IN_INVERTERON_BITMASK;
        return sizeof(uint32_t);
    default:
        return 0;
    }
}

void dometic_shape_ac_conv_ac0actext_to_st_actext(const void *const *ddm_data, const size_t *ddm_data_size, size_t conv_data_size, void *conv_data)
{
    /* Access only DDM 0 */
    /* Two bits:
     * bit 0: Inverter OFF
     * bit 1: Inverter ON
     */
    /* actext member is uint8_t in the Dometic structure */
    uint8_t *dometic_data = conv_data;
    /* The AC0ACTEXT is uint32_t type */
    const uint32_t *ac0actext_data = ddm_data[0];

    /* Ensure data sizes are correct */
    TRUE_CHECK_RETURN(sizeof(*dometic_data) == conv_data_size);
    TRUE_CHECK_RETURN(sizeof(*ac0actext_data) == ddm_data_size[0]);

    *dometic_data = 0u;

    if (AC0ACTEXT_OUT_HEATER_GET(*ac0actext_data))
    {
        *dometic_data |= 0x1u << 0u;
    }
    if (AC0ACTEXT_OUT_COMPRESSOR_GET(*ac0actext_data))
    {
        *dometic_data |= 0x1u << 1u;
    }
}

void dometic_shape_ac_conv_ac0itemp_to_itemp(const void *const *ddm_data, const size_t *ddm_data_size, size_t conv_data_size, void *conv_data)
{
    /* Access only DDM 0 */
    /* itemp member is uint8_t in the Dometic structure */
    uint8_t *dometic_data = conv_data;
    /* The AC0ITEMP is int32_t type */
    const int32_t *ac0itemp_data = ddm_data[0];

    /* Ensure data sizes are correct */
    TRUE_CHECK_RETURN(sizeof(*dometic_data) == conv_data_size);
    TRUE_CHECK_RETURN(sizeof(*ac0itemp_data) == ddm_data_size[0]);

    if ((*ac0itemp_data >= 0) && (*ac0itemp_data <= 63000))
    {
        *dometic_data = *ac0itemp_data / 1000;
    }
    else if (*ac0itemp_data < 0)
    {
        *dometic_data = 0;
    }
    else
    {
        *dometic_data = UINT8_MAX;
    }
}

void dometic_shape_ac_conv_ac0status_to_status(const void *const *ddm_data, const size_t *ddm_data_size, size_t conv_data_size, void *conv_data)
{
    /* Access only DDM 0 */
    /* AC0STATUS is an array of unsigned 16-bit integers */
    const uint16_t *ac0status_data = ddm_data[0];
    /* Array size is given in bytes, convert to number of elements */
    size_t ac0status_length = ddm_data_size[0] / sizeof(uint16_t);
    /* st_error is actually uint8_t in Dometic */
    uint8_t *dometic_st_status = conv_data;
    /* Check that we are using a type of correct size */
    TRUE_CHECK_RETURN(sizeof(*dometic_st_status) == conv_data_size);

    *dometic_st_status = 0u;
    for (size_t i = 0u; i < ac0status_length; i++)
    {
        switch (ac0status_data[i])
        {
        case GENERIC_COMMUNICATION_ERROR:
        case GENERIC_RVC_BUS_COMMUNICATION_ERROR:
        case GENERIC_CURRENT_SENSOR_ERROR:
        case GENERIC_EEPROM_ERROR:
        case AIRC_ROOM_TEMP_SENSOR_ERROR:
        case AIRC_OUTDOOR_TEMP_SENSOR_ERROR:
        case AIRC_EVAPORATOR_TEMP_SENSOR_ERROR:
        case AIRC_CONDENSOR_TEMP_SENSOR_ERROR:
        case AIRC_COMPRESSOR_DISCHARGE_TEMP_SENSOR_ERROR:
        case AIRC_COMPRESSOR_DRV_IPM_ERROR:
        case AIRC_COMPRESSOR_DISCHARGE_TEMP_SENSOR_PROTECTION_ERROR:
        case AIRC_EVAPORATOR_TEMP_SENSOR_COOL_PROTECTION_ERROR:
        case AIRC_CONDENSOR_TEMP_SENSOR_PROTECTION_ERROR:
        case AIRC_EVAPORATOR_TEMP_SENSOR_HEAT_PROTECTION_ERROR:
        case AIRC_COMPRESSOR_DRV_NO_START_ERROR:
        case AIRC_COMPRESSOR_DRV_IPM_PROTECTION_ERROR:
            *dometic_st_status |= 0x1u << 0;
            break;
        case GENERIC_AC_UNDER_VOLTAGE_ERROR:
        case GENERIC_AC_OVER_VOLTAGE_ERROR:
        case GENERIC_AC_OVER_CURRENT_ERROR:
            *dometic_st_status |= 0x1u << 1;
            break;
        default:
            break;
        }
    }
}

void dometic_shape_ac2_conv_ac0currlim_to_currlim(const void *const *ddm_data, const size_t *ddm_data_size, size_t conv_data_size, void *conv_data)
{
    /* Access only DDM 0 */
    /* input_current_limit member is uint8_t in the Dometic structure */
    uint8_t *dometic_data = conv_data;
    /* The AC0CURRLIM is int32_t type */
    const int32_t *ac0currlim_data = ddm_data[0];

    /* Ensure data sizes are correct */
    TRUE_CHECK_RETURN(sizeof(*dometic_data) == conv_data_size);
    TRUE_CHECK_RETURN(sizeof(*ac0currlim_data) == ddm_data_size[0]);

    switch (*ac0currlim_data)
    {
    case AC0CURRLIM_4A:
        *dometic_data = 4;
        break;
    case AC0CURRLIM_5A:
        *dometic_data = 5;
        break;
    case AC0CURRLIM_6A:
        *dometic_data = 6;
        break;
    case AC0CURRLIM_7A:
        *dometic_data = 7;
        break;
    case AC0CURRLIM_UNLIMITED:
        *dometic_data = 15;
        break;
    default:
        *dometic_data = 0;
        break;
    }
}

size_t dometic_shape_ac2_conv_currlim_to_ac0currlim(const void *conv_data, size_t conv_data_size, void *ddm_data, size_t ddm_data_size)
{
    /* input_current_limit member is uint8_t in the Dometic structure */
    const uint8_t *dometic_data = conv_data;
    /* The AC0CURRLIM is int32_t type */
    int32_t *ac0currlim_data = ddm_data;
    size_t ac0currlim_data_size;

    /* Ensure data sizes are correct */
    TRUE_CHECK_RETURN0(sizeof(*dometic_data) == conv_data_size);
    TRUE_CHECK_RETURN0(sizeof(*ac0currlim_data) <= ddm_data_size);

    switch (*dometic_data)
    {
    case 4:
        *ac0currlim_data = AC0CURRLIM_4A;
        ac0currlim_data_size = sizeof(*ac0currlim_data);
        break;
    case 5:
        *ac0currlim_data = AC0CURRLIM_5A;
        ac0currlim_data_size = sizeof(*ac0currlim_data);
        break;
    case 6:
        *ac0currlim_data = AC0CURRLIM_6A;
        ac0currlim_data_size = sizeof(*ac0currlim_data);
        break;
    case 7:  /* fall-through */
    case 8:  /* fall-through */
    case 9:  /* fall-through */
    case 10: /* fall-through */
    case 11: /* fall-through */
    case 12: /* fall-through */
    case 13: /* fall-through */
    case 14: /* fall-through */
        *ac0currlim_data = AC0CURRLIM_7A;
        ac0currlim_data_size = sizeof(*ac0currlim_data);
        break;
    case 15:
        *ac0currlim_data = AC0CURRLIM_UNLIMITED;
        ac0currlim_data_size = sizeof(*ac0currlim_data);
        break;
    case 0: /* fall-through */
    case 1: /* fall-through */
    case 2: /* fall-through */
    case 3: /* fall-through */
    default:
        ac0currlim_data_size = 0;
        break;
    }

    return ac0currlim_data_size;
}

size_t dometic_sharc_wtr_conv_wtrtemp_to_htr0wtrtemp(const void *conv_data, size_t conv_data_size, void *ddm_data, size_t ddm_data_size)
{
    /* wtrtemp member is uint8_t in the Dometic structure */
    const uint8_t *dometic_data = conv_data;
    /* The HTR0WTRTEMP is int32_t type */
    int32_t *htr0wtrtemp_data = ddm_data;

    /* Ensure data sizes are correct */
    TRUE_CHECK_RETURN0(sizeof(*dometic_data) == conv_data_size);
    TRUE_CHECK_RETURN0(sizeof(*htr0wtrtemp_data) <= ddm_data_size);

    switch (*dometic_data)
    {
    case SHARC_WTR_LIN_WATER_HEATER_MODE_ECO:
        *htr0wtrtemp_data = HTR0WTRTEMP_ECO;
        break;
    case SHARC_WTR_LIN_WATER_HEATER_MODE_HOT:
        *htr0wtrtemp_data = HTR0WTRTEMP_HOT;
        break;
    case SHARC_WTR_LIN_WATER_HEATER_MODE_BOOST:
        *htr0wtrtemp_data = HTR0WTRTEMP_BOOST;
        break;
    default:
        return 0;
    }
    return sizeof(*htr0wtrtemp_data);
}

void dometic_sharc_wtr_conv_htr0wtrtemp_to_wtrtemp(const void *const *ddm_data, const size_t *ddm_data_size, size_t conv_data_size, void *conv_data)
{
    /* Access only DDM 0 */
    /* wtrtemp member is uint8_t in the Dometic structure */
    uint8_t *dometic_data = conv_data;
    /* The HTR0WTRTEMP is int32_t type */
    const int32_t *htr0wtrtemp = ddm_data[0];
    /* Ensure data sizes are correct */
    TRUE_CHECK_RETURN(sizeof(*dometic_data) == conv_data_size);
    TRUE_CHECK_RETURN(sizeof(*htr0wtrtemp) == ddm_data_size[0]);
    switch (*htr0wtrtemp)
    {
    case HTR0WTRTEMP_ECO:
        *dometic_data = SHARC_WTR_LIN_WATER_HEATER_MODE_ECO;
        break;
    case HTR0WTRTEMP_HOT:
        *dometic_data = SHARC_WTR_LIN_WATER_HEATER_MODE_HOT;
        break;
    case HTR0WTRTEMP_BOOST:
        *dometic_data = SHARC_WTR_LIN_WATER_HEATER_MODE_BOOST;
        break;
    default:
        *dometic_data = SHARC_WTR_LIN_WATER_HEATER_MODE_ECO;
        break;
    }
}

void dometic_sharc_wtr_ctrl_extract(dometic_sharc_wtr_ctrl_bundle_t *bundle, const uint8_t data[DOMETIC_SHARC_NONE_DATA_LENGTH])
{
    /* Determine the current CTRL and next INFO page */
    uint_fast8_t ctrl_page = EXTRACT_BITS(data[7], 0, 2);
    uint_fast8_t info_page = EXTRACT_BITS(data[7], 3, 2);

    switch (ctrl_page)
    {
    case 0:
        bundle->rd.page0.wtrtemp = EXTRACT_BITS(data[0], 0, 2);
        bundle->rd.page0.wtron = EXTRACT_BITS(data[1], 0, 1);
        bundle->rd.page0.esel = EXTRACT_BITS(data[2], 0, 3);
        bundle->protocol.ctrl_page = ctrl_page;
        break;
    case 1:
        bundle->rd.page1.times = EXTRACT_BITS(data[0], 0, 6);
        bundle->rd.page1.timem = EXTRACT_BITS(data[1], 0, 6);
        bundle->rd.page1.timeh = EXTRACT_BITS(data[2], 0, 5);
        bundle->rd.page1.dated = EXTRACT_BITS(data[3], 0, 5);
        bundle->rd.page1.datem = EXTRACT_BITS(data[4], 0, 4);
        bundle->rd.page1.datey = EXTRACT_BITS(data[5], 0, 8);
        bundle->protocol.ctrl_page = ctrl_page;
        break;
    case 2:
        /* No data in WTR CTRL page 2 */
        bundle->protocol.ctrl_page = ctrl_page;
        break;
    default:
        break;
    }
    switch (info_page)
    {
    case 0:
    case 1:
    case 2:
        bundle->protocol.info_page = info_page;
        break;
    default:
        break;
    }
    bundle->protocol.is_sync_frame = EXTRACT_BITS(data[7], 2, 1);
}

void dometic_sharc_wtr_info_stuff(const dometic_sharc_wtr_info_bundle_t *bundle, uint8_t data[DOMETIC_SHARC_NONE_DATA_LENGTH])
{
    uint32_t info_page = bundle->protocol.info_page;

    switch (info_page)
    {
    case 0:
        data[0] = STUFF_BITS(bundle->rd.page0.wtrtemp, 0, 2);
        data[1] = STUFF_BITS(bundle->rd.page0.wtron, 0, 1) |
                  STUFF_BITS(bundle->st.page0.aon, 1, 1);
        data[2] = STUFF_BITS(bundle->rd.page0.esel, 0, 3);
        data[3] = 0u;
        data[4] = 0u;
        data[5] = STUFF_BITS(bundle->st.page0.wtrts, 0, 2) |
                  STUFF_BITS(bundle->st.page0.acwtrhst, 2, 1) |
                  STUFF_BITS(bundle->st.page0.gaswtrhst, 3, 1);
        data[6] = 0u;
        data[7] = STUFF_BITS(bundle->protocol.is_local_change_frame, 0, 1) |
                  STUFF_BITS(bundle->protocol.info_page, 1, 2) |
                  STUFF_BITS(bundle->st.active_error_flag, 3, 1) |
                  STUFF_BITS(bundle->protocol.response_error, 7, 1);
        break;
    case 1:
        data[0] = STUFF_BITS(bundle->rd.page1.times, 0, 6);
        data[1] = STUFF_BITS(bundle->rd.page1.timem, 0, 6);
        data[2] = STUFF_BITS(bundle->rd.page1.timeh, 0, 5);
        data[3] = STUFF_BITS(bundle->rd.page1.dated, 0, 5);
        data[4] = STUFF_BITS(bundle->rd.page1.datem, 0, 4);
        data[5] = STUFF_BITS(bundle->rd.page1.datey, 0, 8);
        data[6] = 0u;
        data[7] = STUFF_BITS(bundle->protocol.is_local_change_frame, 0, 1) |
                  STUFF_BITS(bundle->protocol.info_page, 1, 2) |
                  STUFF_BITS(bundle->st.active_error_flag, 3, 1) |
                  STUFF_BITS(bundle->protocol.response_error, 7, 1);
        break;
    case 2:
        data[0] = STUFF_BITS(bundle->st.page2.errcd1, 0, 8);
        data[1] = STUFF_BITS(bundle->st.page2.errcd1 >> 8u, 0, 2) |
                  STUFF_BITS(bundle->st.page2.errcd2, 2, 6);
        data[2] = STUFF_BITS(bundle->st.page2.errcd2 >> 6u, 0, 4) |
                  STUFF_BITS(bundle->st.page2.errcd3, 4, 4);
        data[3] = STUFF_BITS(bundle->st.page2.errcd3 >> 4u, 0, 6) |
                  STUFF_BITS(bundle->st.page2.errcd4, 6, 2);
        data[4] = STUFF_BITS(bundle->st.page2.errcd4 >> 2u, 0, 8);
        data[5] = STUFF_BITS(bundle->st.page2.errst, 0, 2);
        data[6] = 0u;
        data[7] = STUFF_BITS(bundle->protocol.is_local_change_frame, 0, 1) |
                  STUFF_BITS(bundle->protocol.info_page, 1, 2) |
                  STUFF_BITS(bundle->st.active_error_flag, 3, 1) |
                  STUFF_BITS(bundle->protocol.response_error, 7, 1);
        break;
    default:
        break;
    }
}

size_t dometic_sharc_air_conv_atemp_to_htr0atemp(
    const void *conv_data,
    size_t conv_data_size,
    void *ddm_data,
    size_t ddm_data_size)
{
    /* atemp member is uint8_t in the Dometic structure */
    const uint8_t *dometic_data = conv_data;
    /* The HTR0ATEMP is int32_t type */
    int32_t *htr0atemp_data = ddm_data;

    /* Ensure data sizes are correct */
    TRUE_CHECK_RETURN0(sizeof(*dometic_data) == conv_data_size);
    TRUE_CHECK_RETURN0(sizeof(*htr0atemp_data) <= ddm_data_size);

    if ((*dometic_data <= 253))
    {
        int32_t atemp;

        atemp = *dometic_data;
        if (atemp <= (int32_t)0x1c)  // If less than -50
        {
            atemp = (int32_t)0x1c;
        }
        *htr0atemp_data = ((atemp * 10) / 2 - 640) * 100;
        return sizeof(*htr0atemp_data);
    }
    else
    {
        return 0;  // Nothing was converted due to an error
    }
}

void dometic_sharc_air_conv_htr0atemp_to_atemp(
    const void *const *ddm_data,
    const size_t *ddm_data_size,
    size_t conv_data_size,
    void *conv_data)
{
    /* Access only DDM 0 */
    /* atemp member is uint8_t in the Dometic structure */
    uint8_t *dometic_data = conv_data;
    /* The HTR0ATEMP is int32_t type */
    const int32_t *htr0atemp_data = ddm_data[0];
    /* Ensure data sizes are correct */
    TRUE_CHECK_RETURN(sizeof(*dometic_data) == conv_data_size);
    TRUE_CHECK_RETURN(sizeof(*htr0atemp_data) == ddm_data_size[0]);
    if ((*htr0atemp_data >= -64000) && (*htr0atemp_data <= 62500))
    {
        *dometic_data = *htr0atemp_data / 500 + 128;
    }
    else
    {
        *dometic_data = 0;
    }
}

void dometic_sharc_air_conv_htr0rts_to_st_rts(
    const void *const *ddm_data,
    const size_t *ddm_data_size,
    size_t conv_data_size,
    void *conv_data)
{
    /* Access only DDM 0 */
    /* rts member is uint8_t in the Dometic structure */
    uint8_t *dometic_data = conv_data;
    int factor;
    int low_temperature_limit;
    int high_temperature_limit;
    /* The HTR0RTS is int32_t type */
    const int32_t *htr0rts_data = ddm_data[0];

    /* Get factor and calculate limits */
    factor = Ddm2_unit_factor_list[DDM2_UNIT_DEGC];
    low_temperature_limit = -50 * factor;  /* Low limit  -50C */
    high_temperature_limit = 150 * factor; /* High limit 150C */

    /* Ensure data sizes are correct */
    TRUE_CHECK_RETURN(sizeof(*dometic_data) == conv_data_size);
    TRUE_CHECK_RETURN(sizeof(*htr0rts_data) == ddm_data_size[0]);
    if ((*htr0rts_data >= low_temperature_limit) && (*htr0rts_data <= high_temperature_limit))
    {
        *dometic_data = (*htr0rts_data / factor) + 50;
    }
}

void dometic_sharc_air_ctrl_extract(dometic_sharc_air_ctrl_bundle_t *bundle, const uint8_t data[DOMETIC_SHARC_NONE_DATA_LENGTH])
{
    /* Determine the current CTRL and next INFO page */
    uint_fast8_t ctrl_page = EXTRACT_BITS(data[7], 0, 2);
    uint_fast8_t info_page = EXTRACT_BITS(data[7], 3, 2);

    switch (ctrl_page)
    {
    case 0:

        bundle->rd.page0.atemp = EXTRACT_BITS(data[0], 0, 8);
        bundle->rd.page0.esel = EXTRACT_BITS(data[2], 0, 3);
        bundle->rd.page0.amd = EXTRACT_BITS(data[3], 0, 3);
        bundle->rd.page0.smaxfan = EXTRACT_BITS(data[4], 0, 3);
        bundle->rd.page0.vminfan = EXTRACT_BITS(data[4], 3, 3);
        bundle->protocol.ctrl_page = ctrl_page;
        break;
    default:
        /* This CTRL frame supports only page 0. If it is not set to zero then
         * the contents of the frame are ignored.
         */
        break;
    }
    switch (info_page)
    {
    case 0:
        bundle->protocol.info_page = info_page;
        break;
    default:
        /* The matching INFO frame supports only page 0. If it is not set to
         * zero then the contents of the frame page are ignored.
         */
        break;
    }
    bundle->protocol.is_sync_frame = EXTRACT_BITS(data[7], 2, 1);
}

void dometic_sharc_air_info_stuff(const dometic_sharc_air_info_bundle_t *bundle, uint8_t data[DOMETIC_SHARC_NONE_DATA_LENGTH])
{
    /* We only support page 0 */
    data[0] = STUFF_BITS(bundle->rd.page0.atemp, 0, 8);
    data[1] = STUFF_BITS(bundle->st.page0.aon, 1, 1);
    data[2] = STUFF_BITS(bundle->rd.page0.esel, 0, 3);
    data[3] = STUFF_BITS(bundle->rd.page0.amd, 0, 3);
    data[4] = STUFF_BITS(bundle->rd.page0.smaxfan, 0, 3) |
              STUFF_BITS(bundle->rd.page0.vminfan, 3, 3);
    data[5] = STUFF_BITS(bundle->st.page0.rts, 0, 8);
    data[6] = STUFF_BITS(bundle->st.page0.acst, 0, 1) |
              STUFF_BITS(bundle->st.page0.ahtoffst, 1, 1) |
              STUFF_BITS(bundle->st.page0.ahtonst, 2, 1) |
              STUFF_BITS(bundle->st.page0.wtrtst, 3, 1);
    data[7] = STUFF_BITS(bundle->protocol.is_local_change_frame, 0, 1) |
              STUFF_BITS(bundle->protocol.info_page, 1, 2) |
              STUFF_BITS(bundle->st.active_error_flag, 3, 1) |
              STUFF_BITS(bundle->protocol.response_error, 7, 1);
}

/**
 *  \brief Conversion function for Shape AC mode value.
 *
 *  This function will convert AC mode for DDM parameter value.
 *
 *
 *  \param i32Value    LIN received raw value
 *
 */
static int32_t dometic_shape_convert_mode_tosys(int32_t i32Value)
{
    int32_t res = 0;

    if (i32Value <= 7)
    {
        /* Convert LIN value to DDMP value */
        switch (i32Value)
        {
        case LIN_MODE_AUTO:
            res = MODE_AUTO;
            break;
        case LIN_MODE_COOL:
            res = MODE_COOL;
            break;
        case LIN_MODE_DRY:
            res = MODE_DRY;
            break;
        case LIN_MODE_HEAT:
            res = MODE_HEAT;
            break;
        case LIN_MODE_VENTILATION:
            res = MODE_VENTILATION;
            break;
        default:
            res = i32Value;
            break;
        }
    }
    else
    {
        LOG(W, "LIN conversion error, value out of range: %d", i32Value);
    }
    return res;
}

/**
 *  \brief Conversion function for Shape AC mode value.
 *
 *  This function will convert AC mode for LIN frame value.
 *
 *
 *  \param i32Value    DDM parameter value
 *
 */
static int32_t dometic_shape_convert_mode_tolin(int32_t i32Value)
{
    int32_t res = 0;

    if (i32Value <= 7)
    {
        /* Convert DDMP value to LIN value */
        switch (i32Value)
        {
        case MODE_AUTO:
            res = LIN_MODE_AUTO;
            break;
        case MODE_COOL:
            res = LIN_MODE_COOL;
            break;
        case MODE_DRY:
            res = LIN_MODE_DRY;
            break;
        case MODE_HEAT:
            res = LIN_MODE_HEAT;
            break;
        case MODE_VENTILATION:
            res = LIN_MODE_VENTILATION;
            break;
        default:
            res = i32Value;
            break;
        }
    }
    else
    {
        LOG(W, "LIN conversion error, value out of range: %d", i32Value);
    }
    return res;
}

/**
 *  \brief Conversion function for Shape timer time value.
 *
 *  This function will convert timer time for DDM parameter value.
 *
 *
 *  \param off_ms   off timer most significant byte
 *  \param off_ls   off timer least significant byte
 *  \param on_ms    on timer most significant byte
 *  \param on_ls    on timer least significant byte
 *  \param off_time total off timer value
 *  \param on_time  total on timer value
 */
static void dometic_shape_convert_time_tosys(const int32_t off_ms,
                                             const int32_t off_ls,
                                             const int32_t on_ms,
                                             const int32_t on_ls,
                                             uint16_t *off_time,
                                             uint16_t *on_time)
{
    /* extract off time value from lin frame */
    *off_time = off_ms * 10 + off_ls;
    /* extract on time value from lin frame */
    *on_time = on_ms * 10 + on_ls;
}

/**
 *  \brief Conversion function for Shape timer time value.
 *
 *  This function will convert timer time for lin frame value.
 *
 *
 *  \param off_ms   off timer most significant byte
 *  \param off_ls   off timer least significant byte
 *  \param on_ms    on timer most significant byte
 *  \param on_ls    on timer least significant byte
 *  \param off_time total off timer value
 *  \param on_time  total on timer value
 *
 */
static void dometic_shape_convert_time_tolin(int32_t *off_ms,
                                             int32_t *off_ls,
                                             int32_t *on_ms,
                                             int32_t *on_ls,
                                             const uint16_t off_time,
                                             const uint16_t on_time)
{
    /* stuff off time to lin bytes */
    *off_ms = off_time / 10;
    *off_ls = off_time % 10;
    /* stuff on time to lin bytes */
    *on_ms = on_time / 10;
    *on_ls = on_time % 10;
}

/**
 *  \brief Conversion function for Shape actuators bit field.
 *
 *  This function will convert actuators values from lin frame values.
 *
 *
 *  \param heater      heater status
 *  \param compressor  compressor status
 *  \param inverter_on inverter_on status
 *  \param actuators   bit field satuses value
 *
 */
static void dometic_shape_convert_actuators_tosys(const uint8_t heater,
                                                  const uint8_t compressor,
                                                  const uint8_t inverter_on,
                                                  uint32_t *actuators)
{

    /* fill bit fields of actuators with received data */
    *actuators = ((heater & 0x01) << 0) |
                 ((compressor & 0x01) << 1) |
                 ((inverter_on & 0x01) << 4);
}

/**
 *  \brief Conversion function for Shape actuators bit field.
 *
 *  This function will convert actuators values for lin frame values.
 *
 *
 *  \param heater       heater status
 *  \param compressor   compressor status
 *  \param inverter_on  inverter_on status
 *  \param inverter_off inverter_on status
 *  \param actuators    bit field satuses value
 *
 */
static void dometic_shape_convert_actuators_tolin(uint8_t *heater,
                                                  uint8_t *compressor,
                                                  uint8_t *inverter_on,
                                                  uint8_t *inverter_off,
                                                  const uint32_t actuators)
{
    /* get bit field values to separate variables */
    if (heater != NULL)
    {
        *heater = (actuators >> 0) & 0x01;
    }
    if (compressor != NULL)
    {
        *compressor = (actuators >> 1) & 0x01;
    }
    if (inverter_on != NULL)
    {
        *inverter_on = (actuators >> 4) & 0x01;
    }
    if ((inverter_off != NULL) && (inverter_on != NULL))
    {
        *inverter_off = !(*inverter_on);
    }
}

/**
 *  \brief Conversion function for Shape light status and brightness.
 *
 *  This function will convert light values for lin frame values.
 *
 *
 *  \param light light status
 *  \param dmr   dimmer value
 *  \param input input value
 *
 */
static void dometic_shape_convert_light_tosys(uint8_t *light, uint8_t *dmr, const uint8_t input)
{
    switch (input)
    {
    case LIGHT_OFF:
        *light = 0u; /* turn off internal light */
        break;
    case LIGHT_50:
        *light = 1u; /* turn on internal light */
        *dmr = 50u;  /* dimmer value 50% */
        break;
    case LIGHT_100:
        *light = 1u; /* turn on internal light */
        *dmr = 100u; /* dimmer value 100% */
        break;
    default:
        break;
    }
}

/**
 *  \brief Conversion function for Shape light status and brightness.
 *
 *  This function will convert light values for lin frame values.
 *
 *
 *  \param light light status
 *  \param dmr   dimmer value
 *
 */
static uint8_t dometic_shape_convert_light_tolin(const uint8_t light, const uint8_t dmr)
{
    uint8_t output;
    if (light == LIGHT_OFF) /* turn off internal light */
    {
        output = LIGHT_OFF;
    }
    else if (dmr == 50u) /* dimmer value 50% */
    {
        output = LIGHT_50;
    }
    else if (dmr == 100u) /* dimmer value 100% */
    {
        output = LIGHT_100;
    }
    else
    {
        /* undefined dimmer value - turn light off */
        output = LIGHT_OFF;
    }

    return output;
}

/**
 *  \brief Conversion function for Shape errors and alarms.
 *
 *  This function will convert DDM parameter to lin error and alarms.
 *
 *
 *
 */
static void dometic_shape_convert_err_tolin(const uint16_t *input, uint8_t *temp_err, uint8_t *main_err)
{
    *temp_err = 0u; /* set no error value */
    *main_err = 0u; /* set no error value */

    for (uint16_t id = 0; id < DDMP2_MAX_VALUE_SIZE; id++)
    {
        /* check for temperature errors */
        if ((input[id] == GENERIC_COMMUNICATION_ERROR) ||
            (input[id] == GENERIC_RVC_BUS_COMMUNICATION_ERROR) ||
            (input[id] == GENERIC_CURRENT_SENSOR_ERROR) ||
            (input[id] == GENERIC_EEPROM_ERROR) ||
            (input[id] == AIRC_ROOM_TEMP_SENSOR_ERROR) ||
            (input[id] == AIRC_OUTDOOR_TEMP_SENSOR_ERROR) ||
            (input[id] == AIRC_EVAPORATOR_TEMP_SENSOR_ERROR) ||
            (input[id] == AIRC_CONDENSOR_TEMP_SENSOR_ERROR) ||
            (input[id] == AIRC_COMPRESSOR_DISCHARGE_TEMP_SENSOR_ERROR) ||
            (input[id] == AIRC_COMPRESSOR_DRV_IPM_ERROR) ||
            (input[id] == AIRC_COMPRESSOR_DISCHARGE_TEMP_SENSOR_PROTECTION_ERROR) ||
            (input[id] == AIRC_EVAPORATOR_TEMP_SENSOR_COOL_PROTECTION_ERROR) ||
            (input[id] == AIRC_CONDENSOR_TEMP_SENSOR_PROTECTION_ERROR) ||
            (input[id] == AIRC_EVAPORATOR_TEMP_SENSOR_HEAT_PROTECTION_ERROR) ||
            (input[id] == AIRC_COMPRESSOR_DRV_NO_START_ERROR) ||
            (input[id] == AIRC_COMPRESSOR_DRV_IPM_PROTECTION_ERROR))
        {
            *temp_err = 1u;
        }
        else
        {
            /* no temperature errors detected */
        }

        /* check for main errors */

        if ((input[id] == GENERIC_AC_UNDER_VOLTAGE_ERROR) ||
            (input[id] == GENERIC_AC_OVER_VOLTAGE_ERROR) ||
            (input[id] == GENERIC_AC_OVER_CURRENT_ERROR))
        {
            *main_err = 1u;
        }
        else
        {
            /* no main errors detected */
        }
    }
}

void dometic_conv_any_ddm_to_any_int(const void *const *ddm_data, const size_t *ddm_data_size, size_t conv_data_size, void *conv_data)
{
    TRUE_CHECK_RETURN(ddm_data_size[0] == sizeof(int32_t));
    switch (conv_data_size)
    {
    case sizeof(int8_t):
        *(int8_t *)conv_data = (int8_t) * (const int32_t *)ddm_data[0];
        break;
    case sizeof(int16_t):
        *(int16_t *)conv_data = (int16_t) * (const int32_t *)ddm_data[0];
        break;
    case sizeof(int32_t):
        *(int32_t *)conv_data = *(const int32_t *)ddm_data[0];
        break;
    default:
        LOG(W, "Unsupported conversion data size: %u", conv_data_size);
        break;
    }
}

size_t dometic_conv_any_int_to_any_ddm(const void *conv_data, size_t conv_data_size, void *ddm_data, size_t ddm_data_size)
{
    TRUE_CHECK_RETURN0(ddm_data_size >= sizeof(int32_t));
    switch (conv_data_size)
    {
    case sizeof(uint8_t):
        *(int32_t *)ddm_data = *(uint8_t *)conv_data;
        return sizeof(int32_t);
    case sizeof(uint16_t):
        *(int32_t *)ddm_data = *(uint16_t *)conv_data;
        return sizeof(int32_t);
    case sizeof(uint32_t):
        *(int32_t *)ddm_data = *(uint32_t *)conv_data;  // This may overflow, but fortunatelly we don't use 32-bit LIN fields
        return sizeof(int32_t);
    default:
        LOG(W, "Unsupported conversion data size: %u", conv_data_size);
        return 0;
    }
}

/**
 *  \brief Extract parameters from AC Ctrl frame buffer.
 *
 *
 *  \param pCtrl     out : Data structure.
 *  \param pu8Data   in  : DGN frame buffer.
 *
 */
void dometic_ac_C_Extract(dometic_ac_ctrl_t *pCtrl, uint8_t *pu8Data)
{
    /*           dest                      src */
    DGN_TO_BITS(pCtrl->mode_a, pu8Data[0], 1, 1);
    DGN_TO_BITS(pCtrl->fan_mode, pu8Data[0], 2, 1);
    DGN_TO_BITS(pCtrl->light_status, pu8Data[0], 6, 1);
    DGN_TO_BITS(pCtrl->power, pu8Data[0], 7, 1);
    DGN_TO_BITS(pCtrl->mode_b, pu8Data[1], 0, 2);
    DGN_TO_BITS(pCtrl->fan_speed, pu8Data[1], 2, 2);
    DGN_TO_BITS(pCtrl->target_temp, pu8Data[1], 4, 4);
    DGN_TO_BITS(pCtrl->dim_lvl, pu8Data[5], 3, 5);
    DGN_TO_BITS(pCtrl->sync_frame, pu8Data[7], 4, 4);
}

/**
 *  \brief Extract parameters from AC Info frame buffer.
 *
 *
 *  \param pInfo     out : Data structure.
 *  \param pu8Data   in  : DGN frame buffer.
 *
 */
void dometic_ac_I_Extract(dometic_ac_info_t *pInfo, uint8_t *pu8Data)
{
    /*           dest                      src */
    DGN_TO_BITS(pInfo->mode_a, pu8Data[0], 1, 1);
    DGN_TO_BITS(pInfo->fan_mode, pu8Data[0], 2, 1);
    DGN_TO_BITS(pInfo->light_status, pu8Data[0], 6, 1);
    DGN_TO_BITS(pInfo->power, pu8Data[0], 7, 1);
    DGN_TO_BITS(pInfo->mode_b, pu8Data[1], 0, 2);
    DGN_TO_BITS(pInfo->fan_speed, pu8Data[1], 2, 2);
    DGN_TO_BITS(pInfo->target_temp, pu8Data[1], 4, 4);
    DGN_TO_BITS(pInfo->dim_lvl, pu8Data[5], 3, 5);
    DGN_TO_BITS(pInfo->local_change, pu8Data[7], 0, 1);
    DGN_TO_BITS(pInfo->ci_error, pu8Data[7], 7, 1);
}

/**
 *  \brief Stuff parameters for AC into a frame buffer.
 *
 *
 *  \param pu8Data   out : DGN frame buffer.
 *  \param pCtrl     in  : Data structure.
 *
 */
void dometic_ac_C_Stuff(uint8_t *pu8Data, dometic_ac_ctrl_t *pCtrl)
{
    /* Clear */
    (void)memset(pu8Data, 0, DOMETIC_AC_CTRL_SIZE);

    /* Fill */
    /*           dest         source */
    BITS_TO_DGN(pu8Data[0], pCtrl->mode_a, 1, 1);
    BITS_TO_DGN(pu8Data[0], pCtrl->fan_mode, 2, 1);
    BITS_TO_DGN(pu8Data[0], pCtrl->light_status, 6, 1);
    BITS_TO_DGN(pu8Data[0], pCtrl->power, 7, 1);
    BITS_TO_DGN(pu8Data[1], pCtrl->mode_b, 0, 2);
    BITS_TO_DGN(pu8Data[1], pCtrl->fan_speed, 2, 2);
    BITS_TO_DGN(pu8Data[1], pCtrl->target_temp, 4, 4);
    BITS_TO_DGN(pu8Data[5], pCtrl->dim_lvl, 3, 5);
    BITS_TO_DGN(pu8Data[7], pCtrl->sync_frame, 2, 1);
}

/**
 *  \brief Extract parameters from SHARC water heater Ctrl frame buffer.
 *
 *
 *  \param pCtrl     out : Data structure.
 *  \param pu8Data   in  : DGN frame buffer.
 *
 */
void dometic_sharc_wtr_C_Extract_page0(dometic_sharc_ctrl_t *pCtrl, const uint8_t *const pu8Data)
{
    int32_t wtr_temp;
    uint8_t esel_old;

    esel_old = pCtrl->wtr_esel;
    /*           dest                      src */
    DGN_TO_BITS(wtr_temp, pu8Data[0], 0, 2);
    /* Data_0 reserved bits: 2-7 */

    DGN_TO_BITS(pCtrl->wtr_on, pu8Data[1], 0, 1);
    DGN_TO_BITS(pCtrl->wtr_wakeup, pu8Data[1], 1, 1);
    /* Data_1 reserved bits: 2-7 */

    DGN_TO_BITS(pCtrl->wtr_esel, pu8Data[2], 0, 8);
    /* Data_2 reserved bits: 3-7 */

    /* Data_3 reserved bits: 0-7 */

    /* Data_4 reserved bits: 0-7 */

    /* Data_5 reserved bits: 0-7 */

    /* Data_6 reserved bits: 0-7 */

    /* Data_7 reserved bits: 0-1 */
    DGN_TO_BITS(pCtrl->wtr_sync_frame, pu8Data[7], 2, 1);
    /* Data_7 reserved bits: 3-7 */

    pCtrl->wtr_temp = dometic_sharc_air_convert_wtrtemp_tosys(wtr_temp);

    if (pCtrl->wtr_esel != esel_old)
    {
        pCtrl->esel = pCtrl->wtr_esel;
    }

    pCtrl->wakeup = pCtrl->wtr_wakeup | pCtrl->air_wakeup;
}

/**
 *  \brief Extract parameters from SHARC water heater Ctrl frame buffer.
 *
 *
 *  \param pCtrl     out : Data structure.
 *  \param pu8Data   in  : DGN frame buffer.
 *
 */
void dometic_sharc_wtr_C_Extract_page1(dometic_sharc_ctrl_t *pCtrl, const uint8_t *const pu8Data)
{
    /*           dest                      src */
    DGN_TO_BITS(pCtrl->time_seconds, pu8Data[0], 0, 6);
    /* Data_0 reserved bits: 6-7 */

    DGN_TO_BITS(pCtrl->time_minutes, pu8Data[1], 0, 6);
    /* Data_1 reserved bits: 6-7 */

    DGN_TO_BITS(pCtrl->time_hours, pu8Data[2], 0, 5);
    /* Data_2 reserved bits: 5-7 */

    DGN_TO_BITS(pCtrl->date_day, pu8Data[3], 0, 5);
    /* Data_3 reserved bits: 5-7 */

    DGN_TO_BITS(pCtrl->date_month, pu8Data[4], 0, 4);
    /* Data_4 reserved bits: 4-7 */

    DGN_TO_BITS(pCtrl->date_year, pu8Data[5], 0, 8);
    /* Data_5 reserved bits: 4-7 */

    DGN_TO_BITS(pCtrl->wtr_but2, pu8Data[6], 0, 1);
    /* Data_6 reserved bits: 1-7 */

    /* Data_7 reserved bits: 0-1 */
    DGN_TO_BITS(pCtrl->wtr_sync_frame, pu8Data[7], 2, 1);
    /* Data_7 reserved bits: 3-7 */
}

/**
 *  \brief Return page ID received from previous control frame to
 *         current info frame.
 *
 *  \param id in : Info frame ID.
 *
 */
uint8_t dometic_get_info_page(uint8_t id)
{
    uint8_t ret = 0;
    switch (id)
    {
    case DOMETIC_SHARC_WTR_INFO_ID:
        ret = wtr_info_page;
        break;
    case DOMETIC_SHARC_AIR_INFO_ID:
        ret = air_info_page;
        break;
    case DOMETIC_SHAPE_AC_INFO_FRAME_ID:
        ret = 0;
        break;
    default:
        ret = 0;
        LOG(E, "Get page error - not suported frame id : 0x%x", id);
        break;
    }

    return ret;
}

/**
 *  \brief Extract parameters from SHARC water heater Ctrl frame buffer.
 *
 *
 *  \param pCtrl     out : Data structure.
 *  \param pu8Data   in  : DGN frame buffer.
 *
 */
void dometic_sharc_wtr_C_Extract(dometic_sharc_ctrl_t *pCtrl, const uint8_t *const pu8Data)
{
    uint8_t page;
    /* Get received control frame page ID */
    DGN_TO_BITS(page, pu8Data[7], 0, 2);
    /* Get requested info frame page ID */
    DGN_TO_BITS(wtr_info_page, pu8Data[7], 3, 2);
    switch (page)
    {
    case 0:
        dometic_sharc_wtr_C_Extract_page0(pCtrl, pu8Data);
        break;
    case 1:
        dometic_sharc_wtr_C_Extract_page1(pCtrl, pu8Data);
        break;
    case 2:
    case 3:
    default:
        LOG(W, "LIN control frame page not supported - %d", page);
        break;
    }
}

/**
 *  \brief Stuff parameters for SHARC water heater into a frame buffer.
 *
 *
 *  \param pu8Data   out : DGN frame buffer.
 *  \param pCtrl     in  : Data structure.
 *
 */
void dometic_sharc_wtr_C_Stuff(uint8_t *pu8Data, dometic_sharc_ctrl_t *pCtrl)
{
    int32_t wtr_temp;

    wtr_temp = dometic_sharc_air_convert_wtrtemp_tolin(pCtrl->wtr_temp);
    /* Clear */
    (void)memset(pu8Data, 0, DOMETIC_AC_CTRL_SIZE);

    /*           dest        src */
    BITS_TO_DGN(pu8Data[0], wtr_temp, 0, 2);
    /* Data_0 reserved bits: 2-7 */

    BITS_TO_DGN(pu8Data[1], pCtrl->wtr_on, 0, 1);
    BITS_TO_DGN(pu8Data[1], pCtrl->wakeup, 1, 1);
    /* Data_1 reserved bits: 2-7 */

    BITS_TO_DGN(pu8Data[2], pCtrl->wtr_esel, 0, 8);
    /* Data_2 reserved bits: 3-7 */

    /* Data_3 reserved bits: 0-7 */

    /* Data_4 reserved bits: 0-7 */

    /* Data_5 reserved bits: 0-7 */

    /* Data_6 reserved bits: 0-7 */

    /* Data_7 reserved bits: 0-1 */
    BITS_TO_DGN(pu8Data[7], pCtrl->wtr_sync_frame, 2, 1);
    /* Data_7 reserved bits: 3-7 */
}

/**
 *  \brief Stuff parameters for SHARC water heater into a frame buffer page 0.
 *
 *
 *  \param pInfo     in  : Data structure.
 *  \param pu8Data   out : DGN frame buffer.
 *
 */
void dometic_sharc_wtr_I_Stuff_page0(uint8_t *pu8Data, dometic_sharc_info_t *pInfo)
{
    int32_t wtr_temp;
    uint8_t active_error_flag;

    active_error_flag = pInfo->wtr_errcd_1 || pInfo->wtr_errcd_2 || pInfo->wtr_errcd_3 || pInfo->wtr_errcd_4;
    wtr_temp = dometic_sharc_air_convert_wtrtemp_tolin(pInfo->wtr_temp);
    /* Clear */
    (void)memset(pu8Data, 0, DOMETIC_AC_CTRL_SIZE);

    /*           dest        src */
    BITS_TO_DGN(pu8Data[0], wtr_temp, 0, 2);
    /* Data_0 reserved bits: 2-7 */

    BITS_TO_DGN(pu8Data[1], pInfo->wtr_on, 0, 1);
    BITS_TO_DGN(pu8Data[1], pInfo->online, 1, 1);
    /* Data_1 reserved bits: 2-7 */

    BITS_TO_DGN(pu8Data[2], pInfo->esel, 0, 8);
    /* Data_2 reserved bits: 3-7 */

    /* Data_3 reserved bits: 0-7 */

    /* Data_4 reserved bits: 0-7 */

    /* Data_5 reserved bits: 4-7 */
    BITS_TO_DGN(pu8Data[5], pInfo->wtr_temp_status, 0, 2);
    BITS_TO_DGN(pu8Data[5], pInfo->wtr_ac_heater_status, 2, 1);
    BITS_TO_DGN(pu8Data[5], pInfo->wtr_gas_heater_status, 3, 1);

    /* Data_6 reserved bits: 0-7 */

    BITS_TO_DGN(pu8Data[7], pInfo->wtr_local_change, 0, 1);
    BITS_TO_DGN(pu8Data[7], 0x00, 1, 2);
    BITS_TO_DGN(pu8Data[7], active_error_flag, 3, 1);
    /* Data_7 reserved bits: 3-6 */
    BITS_TO_DGN(pu8Data[7], pInfo->wtr_ci_error, 7, 1);
}

/**
 *  \brief Stuff parameters for SHARC water heater into a frame buffer page 1.
 *
 *
 *  \param pInfo     in  : Data structure.
 *  \param pu8Data   out : DGN frame buffer.
 *
 */
void dometic_sharc_wtr_I_Stuff_page1(uint8_t *pu8Data, dometic_sharc_info_t *pInfo)
{
    uint8_t active_error_flag;

    active_error_flag = pInfo->wtr_errcd_1 || pInfo->wtr_errcd_2 || pInfo->wtr_errcd_3 || pInfo->wtr_errcd_4;

    /* Clear */
    (void)memset(pu8Data, 0, DOMETIC_AC_CTRL_SIZE);

    BITS_TO_DGN(pu8Data[0], pInfo->time_seconds, 0, 6);
    /* Data_0 reserved bits: 6-7 */

    BITS_TO_DGN(pu8Data[1], pInfo->time_minutes, 0, 6);
    /* Data_1 reserved bits: 6-7 */

    BITS_TO_DGN(pu8Data[2], pInfo->time_hours, 0, 5);
    /* Data_2 reserved bits: 5-7 */

    BITS_TO_DGN(pu8Data[3], pInfo->date_day, 0, 5);
    /* Data_3 reserved bits: 5-7 */

    BITS_TO_DGN(pu8Data[4], pInfo->date_month, 0, 4);
    /* Data_4 reserved bits: 4-7 */

    BITS_TO_DGN(pu8Data[5], pInfo->date_year, 0, 8);
    /* Data_4 reserved bits: 4-7 */

    /* Data_6 reserved bits: 0-7 */

    BITS_TO_DGN(pu8Data[7], pInfo->wtr_local_change, 0, 1);
    BITS_TO_DGN(pu8Data[7], 0x01, 1, 2);
    BITS_TO_DGN(pu8Data[7], active_error_flag, 3, 1);
    /* Data_7 reserved bits: 3-6 */
    BITS_TO_DGN(pu8Data[7], pInfo->wtr_ci_error, 7, 1);
}

/**
 *  \brief Stuff parameters for SHARC water heater into a frame buffer page 2.
 *
 *
 *  \param pInfo     in  : Data structure.
 *  \param pu8Data   out : DGN frame buffer.
 *
 */
void dometic_sharc_wtr_I_Stuff_page2(uint8_t *pu8Data, dometic_sharc_info_t *pInfo)
{
    uint16_t error_code_msb;
    uint16_t error_code_lsb;
    uint8_t active_error_flag;

    active_error_flag = pInfo->wtr_errcd_1 || pInfo->wtr_errcd_2 || pInfo->wtr_errcd_3 || pInfo->wtr_errcd_4;

    /* Clear */
    (void)memset(pu8Data, 0, DOMETIC_AC_CTRL_SIZE);

    /* Extrat warning and fault from status variable*/
    pInfo->wtr_warning = pInfo->err_status & 0x01u;
    pInfo->wtr_air_fault = (pInfo->err_status >> 1) & 0x01u;

    /* padded data in frame each error code is 10bits */
    /*Error code 1 Bit: 0-9*/
    error_code_lsb = pInfo->wtr_errcd_1 & 0b0011111111;
    error_code_msb = (pInfo->wtr_errcd_1 & 0b1100000000) >> 0x08;
    BITS_TO_DGN(pu8Data[0], error_code_lsb, 0, 8);
    BITS_TO_DGN(pu8Data[1], error_code_msb, 0, 2);
    /*Error code 2 Bit: 10-19*/
    error_code_lsb = pInfo->wtr_errcd_2 & 0b0000111111;
    error_code_msb = (pInfo->wtr_errcd_2 & 0b1111000000) >> 0x06;
    BITS_TO_DGN(pu8Data[1], error_code_lsb, 2, 6);
    BITS_TO_DGN(pu8Data[2], error_code_msb, 0, 4);
    /*Error code 3 Bit: 20-29*/
    error_code_lsb = pInfo->wtr_errcd_3 & 0b0000001111;
    error_code_msb = (pInfo->wtr_errcd_3 & 0b1111110000) >> 0x04;
    BITS_TO_DGN(pu8Data[2], error_code_lsb, 4, 4);
    BITS_TO_DGN(pu8Data[3], error_code_msb, 0, 6);
    /*Error code 4 Bit: 30-39*/
    error_code_lsb = pInfo->wtr_errcd_4 & 0b0000000011;
    error_code_msb = (pInfo->wtr_errcd_4 & 0b1111111100) >> 0x02;
    BITS_TO_DGN(pu8Data[3], error_code_lsb, 6, 2);
    BITS_TO_DGN(pu8Data[4], error_code_msb, 0, 8);

    BITS_TO_DGN(pu8Data[5], pInfo->wtr_warning, 0, 1);
    BITS_TO_DGN(pu8Data[5], pInfo->wtr_air_fault, 1, 1);
    /* Data_5 reserved bits: 2-7 */

    BITS_TO_DGN(pu8Data[7], pInfo->wtr_local_change, 0, 1);
    BITS_TO_DGN(pu8Data[7], 0x02, 1, 2);
    BITS_TO_DGN(pu8Data[7], active_error_flag, 3, 1);
    /* Data_7 reserved bits: 3-6 */
    BITS_TO_DGN(pu8Data[7], pInfo->wtr_ci_error, 7, 1);
}

/**
 *  \brief  Select stuffing function according to requested page
 *
 *
 *  \param pu8Data   in  : DGN frame buffer.
 *  \param pInfo     out : Data structure.
 *
 */
void dometic_sharc_wtr_I_Stuff(uint8_t *pu8Data, dometic_sharc_info_t *pInfo)
{
    uint8_t page;
    /* Get requested info page ID from master */
    page = dometic_get_info_page(DOMETIC_SHARC_WTR_INFO_ID);
    switch (page)
    {
    case 0:
        dometic_sharc_wtr_I_Stuff_page0(pu8Data, pInfo);
        break;
    case 1:
        dometic_sharc_wtr_I_Stuff_page1(pu8Data, pInfo);
        break;
    case 2:
        dometic_sharc_wtr_I_Stuff_page2(pu8Data, pInfo);
        break;
    case 3:
    default:
        LOG(W, "LIN information frame page not supported - %d", page);
        break;
    }
}

/**
 *  \brief Extract parameters from SHARC air heater Ctrl frame buffer page 0.
 *
 *
 *  \param pCtrl     out : Data structure.
 *  \param pu8Data   in  : DGN frame buffer.
 *
 */
void dometic_sharc_air_C_Extract_page0(dometic_sharc_ctrl_t *pCtrl, const uint8_t *const pu8Data)
{
    int32_t air_temp;
    uint8_t esel_old;

    esel_old = pCtrl->air_esel;
    /*           dest                      src */
    DGN_TO_BITS(air_temp, pu8Data[0], 0, 8);

    DGN_TO_BITS(pCtrl->air_on, pu8Data[1], 0, 1);
    DGN_TO_BITS(pCtrl->air_wakeup, pu8Data[1], 1, 1);
    /* Data_1 reserved bits: 2-7 */

    DGN_TO_BITS(pCtrl->air_esel, pu8Data[2], 0, 8);
    /* Data_2 reserved bits: 4-7 */

    DGN_TO_BITS(pCtrl->air_mode, pu8Data[3], 0, 4);
    /* Data_3 reserved bits: 4-7 */

    DGN_TO_BITS(pCtrl->air_smaxfan, pu8Data[4], 0, 4);
    DGN_TO_BITS(pCtrl->air_vminfan, pu8Data[4], 4, 4);

    /* Data_5 reserved bits: 0-7 */

    /* Data_6 reserved bits: 0-7 */

    /* Data_7 reserved bits: 0-1 */
    DGN_TO_BITS(pCtrl->air_sync_frame, pu8Data[7], 2, 1);
    /* Data_7 reserved bits: 3-7 */
    pCtrl->air_temp = dometic_sharc_air_convert_temp_tosys(air_temp);

    if (pCtrl->air_esel != esel_old)
    {
        pCtrl->esel = pCtrl->air_esel;
    }

    pCtrl->wakeup = pCtrl->wtr_wakeup | pCtrl->air_wakeup;
}

/**
 *  \brief Extract parameters from SHARC water heater Ctrl frame buffer from specific page.
 *
 *
 *  \param pCtrl     out : Data structure.
 *  \param pu8Data   in  : DGN frame buffer.
 *
 */
void dometic_sharc_air_C_Extract(dometic_sharc_ctrl_t *pCtrl, const uint8_t *const pu8Data)
{
    uint8_t page;
    /* Get received control frame page ID */
    DGN_TO_BITS(page, pu8Data[7], 0, 2);
    /* Get requested info frame page ID */
    DGN_TO_BITS(air_info_page, pu8Data[7], 3, 2);
    switch (page)
    {
    case 0:
        dometic_sharc_air_C_Extract_page0(pCtrl, pu8Data);
        break;
    case 1:
    case 2:
    case 3:
    default:
        LOG(W, "LIN control frame page not supported - %d", page);
        break;
    }
}

/**
 *  \brief Extract parameters from SHARC air heater Info frame buffer.
 *
 *
 *  \param pInfo     out : Data structure.
 *  \param pu8Data   in  : DGN frame buffer.
 *
 */
void dometic_sharc_air_I_Extract(dometic_sharc_info_t *pInfo, uint8_t *pu8Data)
{
    int32_t air_temp;
    int32_t air_room_temp;
    /*           dest                      src */
    DGN_TO_BITS(air_temp, pu8Data[0], 0, 8);

    DGN_TO_BITS(pInfo->air_on, pu8Data[1], 0, 1);
    DGN_TO_BITS(pInfo->online, pu8Data[1], 1, 1);
    /* Data_1 reserved bits: 2-7 */

    DGN_TO_BITS(pInfo->esel, pu8Data[2], 0, 8);
    /* Data_2 reserved bits: 4-7 */

    DGN_TO_BITS(pInfo->air_mode, pu8Data[3], 0, 4);
    /* Data_3 reserved bits: 4-7 */

    DGN_TO_BITS(pInfo->air_smaxfan, pu8Data[4], 0, 4);
    DGN_TO_BITS(pInfo->air_vminfan, pu8Data[4], 4, 4);

    DGN_TO_BITS(air_room_temp, pu8Data[5], 0, 8);

    DGN_TO_BITS(pInfo->air_ac_status, pu8Data[6], 0, 1);
    DGN_TO_BITS(pInfo->air_time_off, pu8Data[6], 1, 1);
    DGN_TO_BITS(pInfo->air_time_on, pu8Data[6], 2, 1);
    DGN_TO_BITS(pInfo->air_wtr_time_on, pu8Data[6], 3, 1);
    /* Data_6 reserved bits: 4-7 */

    DGN_TO_BITS(pInfo->air_local_change, pu8Data[7], 1, 1);
    /* Data_7 reserved bits: 1-6 */
    DGN_TO_BITS(pInfo->air_ci_error, pu8Data[7], 7, 1);
    pInfo->air_room_temp = dometic_sharc_air_convert_itemp_tosys(air_room_temp);
    pInfo->air_temp = dometic_sharc_air_convert_temp_tosys(air_temp);
}

/**
 *  \brief Stuff parameters for SHARC air heater into a frame buffer.
 *
 *
 *  \param pu8Data   out : DGN frame buffer.
 *  \param pCtrl     in  : Data structure.
 *
 */
void dometic_sharc_air_C_Stuff(uint8_t *pu8Data, dometic_sharc_ctrl_t *pCtrl)
{
    int32_t air_temp;

    air_temp = dometic_sharc_air_convert_temp_tolin(pCtrl->air_temp);

    /* Clear */
    (void)memset(pu8Data, 0, DOMETIC_AC_CTRL_SIZE);

    /*           dest        src */
    BITS_TO_DGN(pu8Data[0], air_temp, 0, 8);

    BITS_TO_DGN(pu8Data[1], pCtrl->air_on, 0, 1);
    BITS_TO_DGN(pu8Data[1], pCtrl->wakeup, 1, 1);
    /* Data_1 reserved bits: 2-7 */

    BITS_TO_DGN(pu8Data[2], pCtrl->air_esel, 0, 8);
    /* Data_2 reserved bits: 4-7 */

    BITS_TO_DGN(pu8Data[3], pCtrl->air_mode, 0, 4);
    /* Data_3 reserved bits: 4-7 */

    BITS_TO_DGN(pu8Data[4], pCtrl->air_smaxfan, 0, 4);
    BITS_TO_DGN(pu8Data[4], pCtrl->air_vminfan, 4, 4);

    /* Data_5 reserved bits: 0-7 */

    /* Data_6 reserved bits: 0-7 */

    /* Data_7 reserved bits: 0-1 */
    BITS_TO_DGN(pu8Data[7], pCtrl->air_sync_frame, 2, 1);
    /* Data_7 reserved bits: 3-7 */
}

/**
 *  \brief Extract parameters from SHARC air heater Info frame buffer page 0.
 *
 *
 *  \param pu8Data   out : DGN frame buffer.
 *  \param pInfo     in  : Data structure.
 *
 */
void dometic_sharc_air_I_Stuff_page0(uint8_t *pu8Data, dometic_sharc_info_t *pInfo)
{
    int32_t air_temp;
    int32_t air_room_temp;
    uint8_t active_error_flag;

    active_error_flag = pInfo->wtr_errcd_1 || pInfo->wtr_errcd_2 || pInfo->wtr_errcd_3 || pInfo->wtr_errcd_4;

    air_temp = dometic_sharc_air_convert_temp_tolin(pInfo->air_temp);
    air_room_temp = dometic_sharc_air_convert_itemp_tolin(pInfo->air_room_temp);
    /* Clear */
    (void)memset(pu8Data, 0, DOMETIC_AC_CTRL_SIZE);

    BITS_TO_DGN(pu8Data[0], air_temp, 0, 8);

    BITS_TO_DGN(pu8Data[1], pInfo->air_on, 0, 1);
    BITS_TO_DGN(pu8Data[1], pInfo->online, 1, 1);
    /* Data_1 reserved bits: 2-7 */

    BITS_TO_DGN(pu8Data[2], pInfo->esel, 0, 8);
    /* Data_2 reserved bits: 4-7 */

    BITS_TO_DGN(pu8Data[3], pInfo->air_mode, 0, 4);
    /* Data_3 reserved bits: 4-7 */

    BITS_TO_DGN(pu8Data[4], pInfo->air_smaxfan, 0, 4);
    BITS_TO_DGN(pu8Data[4], pInfo->air_vminfan, 4, 4);
    /* Data_4 reserved bits: 4-7 */

    BITS_TO_DGN(pu8Data[5], air_room_temp, 0, 8);

    BITS_TO_DGN(pu8Data[6], pInfo->air_ac_status, 0, 1);
    BITS_TO_DGN(pu8Data[6], pInfo->air_time_off, 1, 1);
    BITS_TO_DGN(pu8Data[6], pInfo->air_time_on, 2, 1);
    BITS_TO_DGN(pu8Data[6], pInfo->air_wtr_time_on, 3, 1);
    /* Data_6 reserved bits: 4-7 */

    BITS_TO_DGN(pu8Data[7], pInfo->air_local_change, 1, 1);
    BITS_TO_DGN(pu8Data[7], 0x00, 1, 2);
    BITS_TO_DGN(pu8Data[7], active_error_flag, 3, 1);
    /* Data_7 reserved bits: 1-6 */
    BITS_TO_DGN(pu8Data[7], pInfo->air_ci_error, 7, 1);
}

/**
 *  \brief  Select stuffing function according to requested info page
 *
 *
 *  \param pu8Data   in  : DGN frame buffer.
 *  \param pInfo     out : Data structure.
 *
 */
void dometic_sharc_air_I_Stuff(uint8_t *pu8Data, dometic_sharc_info_t *pInfo)
{
    uint8_t page;
    /* Get requested info page ID from master */
    page = dometic_get_info_page(DOMETIC_SHARC_AIR_INFO_ID);
    switch (page)
    {
    case 0:
        dometic_sharc_air_I_Stuff_page0(pu8Data, pInfo);
        break;
    case 1:
    case 2:
    case 3:
    default:
        LOG(W, "LIN information frame page not supported - %d", page);
        break;
    }
}

/**
 *  \brief Extract data from LIN frame function.
 *
 *  This function will extract parameters from Shape AC Info frame buffer.
 *
 *
 *  \param pInfo     out: Extracted parameters
 *  \param pu8Data   in : DGN frame buffer
 *
 */
void dometic_shape_ac_I_Extract(dometic_shape_info_t *pInfo, const uint8_t *pu8Data)
{
    int32_t temp;
    int32_t mode_tmp;
    int32_t internal_temp_tmp;
    int32_t off_min_ms_tmp;
    int32_t off_min_ls_tmp;
    int32_t on_min_ms_tmp;
    int32_t on_min_ls_tmp;
    uint8_t light_sent;
    uint8_t no_init;

    /*           dest                       src         pos size */
    DGN_TO_BITS(mode_tmp, pu8Data[0], 0, 3);
    DGN_TO_BITS(pInfo->sleep_mode, pu8Data[0], 3, 1);
    DGN_TO_BITS(pInfo->timer_off_mode, pu8Data[0], 4, 1);
    DGN_TO_BITS(pInfo->timer_on_mode, pu8Data[0], 5, 1);
    /* Data_0 reserved bits: 6 */
    DGN_TO_BITS(pInfo->power, pu8Data[0], 7, 1);

    DGN_TO_BITS(pInfo->light_external, pu8Data[1], 0, 1);
    DGN_TO_BITS(pInfo->fan_speed_mode, pu8Data[1], 1, 3);
    DGN_TO_BITS(temp, pu8Data[1], 4, 7);

    DGN_TO_BITS(on_min_ms_tmp, pu8Data[2], 0, 8);

    DGN_TO_BITS(off_min_ms_tmp, pu8Data[3], 0, 8);

    DGN_TO_BITS(on_min_ls_tmp, pu8Data[4], 0, 4);
    DGN_TO_BITS(off_min_ls_tmp, pu8Data[4], 4, 4);

    DGN_TO_BITS(light_sent, pu8Data[5], 0, 3);
    DGN_TO_BITS(pInfo->ac0_heat_enable, pu8Data[5], 3, 1);
    DGN_TO_BITS(pInfo->ac0_light_enable, pu8Data[5], 4, 1);
    DGN_TO_BITS(pInfo->remote_ctrl_status, pu8Data[5], 5, 1);
    /* Data_5 reserved bits: 6-7 */

    DGN_TO_BITS(pInfo->heater_run, pu8Data[6], 0, 1);
    DGN_TO_BITS(pInfo->comp_run, pu8Data[6], 1, 1);
    DGN_TO_BITS(internal_temp_tmp, pu8Data[6], 2, 6);
    /* Data_6 reserved bits: 3-7 */

    DGN_TO_BITS(pInfo->local_change, pu8Data[7], 0, 1);
    DGN_TO_BITS(pInfo->no_main, pu8Data[7], 1, 1);
    DGN_TO_BITS(pInfo->error, pu8Data[7], 2, 1);
    DGN_TO_BITS(pInfo->timer_on_req, pu8Data[7], 3, 1);
    DGN_TO_BITS(pInfo->timer_off_req, pu8Data[7], 4, 1);
    DGN_TO_BITS(pInfo->remote_on, pu8Data[7], 5, 1);
    DGN_TO_BITS(no_init, pu8Data[7], 6, 1);
    DGN_TO_BITS(pInfo->ci_error, pu8Data[7], 7, 1);

    /* Call conversion functions for needed signals */
    pInfo->no_init = !(0x01 & no_init);
    pInfo->mode = dometic_shape_convert_mode_tosys(mode_tmp);
    pInfo->temp = dometic_shape_convert_temp_tosys(temp);
    pInfo->int_temp = dometic_shape_convert_itemp_tosys(internal_temp_tmp);
    dometic_shape_convert_time_tosys(off_min_ms_tmp,
                                     off_min_ls_tmp,
                                     on_min_ms_tmp,
                                     on_min_ls_tmp,
                                     &pInfo->off_timer_min,
                                     &pInfo->on_timer_bits);
    dometic_shape_convert_light_tosys(&pInfo->light_internal, &pInfo->light_dmr, light_sent);
    dometic_shape_convert_actuators_tosys(pInfo->heater_run,
                                          pInfo->comp_run,
                                          0,
                                          &pInfo->actuators);
}

void dometic_shape_ac_ctrl_extract(
    dometic_shape_ac_ctrl_bundle_t *bundle,
    const uint8_t data[DOMETIC_SHAPE_DATA_LENGTH])
{
    bundle->rd.md = EXTRACT_BITS(data[0], 0, 3);
    bundle->rd.sleep = EXTRACT_BITS(data[0], 3, 1);
    bundle->rd.toffa = EXTRACT_BITS(data[0], 4, 1);
    bundle->rd.tona = EXTRACT_BITS(data[0], 5, 1);
    bundle->rd.on = EXTRACT_BITS(data[0], 7, 1);
    bundle->rd.elgt = EXTRACT_BITS(data[1], 0, 1);
    bundle->rd.fspd = EXTRACT_BITS(data[1], 1, 3);
    bundle->rd.ttemp = EXTRACT_BITS(data[1], 4, 4);
    /* 1-to-1 split-field extraction mapping */
    bundle->rd.tonm = (EXTRACT_BITS(data[2], 0, 8) << 8u) | EXTRACT_BITS(data[4], 0, 4);
    /* 1-to-1 split-field extraction mapping */
    bundle->rd.toffm = (EXTRACT_BITS(data[3], 0, 8) << 8u) | EXTRACT_BITS(data[4], 4, 4);
    /* 1-to-many extraction mapping */
    bundle->rd.lgt_dmr = EXTRACT_BITS(data[5], 0, 3);
    bundle->rd.hfavl = EXTRACT_BITS(data[5], 3, 1);
    bundle->rd.lfavl = EXTRACT_BITS(data[5], 4, 1);
    bundle->rd.remctrl = EXTRACT_BITS(data[5], 5, 1);
    bundle->st.remctrl = EXTRACT_BITS(data[7], 0, 1);
    /* many-to-1 extraction mapping */
    bundle->st.actext = (EXTRACT_BITS(data[7], 5, 1) << 1u) | EXTRACT_BITS(data[7], 4, 1);
    bundle->protocol.is_sync_frame = EXTRACT_BITS(data[7], 2, 1);
}

void dometic_shape_ac_info_stuff(
    const dometic_shape_ac_info_bundle_t *bundle,
    uint8_t data[DOMETIC_SHAPE_DATA_LENGTH])
{
    data[0] = STUFF_BITS(bundle->rd.on, 7, 1) |
              STUFF_BITS(bundle->rd.tona, 5, 1) |
              STUFF_BITS(bundle->rd.toffa, 4, 1) |
              STUFF_BITS(bundle->rd.sleep, 3, 1) |
              STUFF_BITS(bundle->rd.md, 0, 3);
    data[1] = STUFF_BITS(bundle->rd.ttemp, 4, 4) |
              STUFF_BITS(bundle->rd.fspd, 1, 3) |
              STUFF_BITS(bundle->rd.elgt, 0, 1);
    /* 1-to-1 split-field stuffing mapping */
    data[2] = STUFF_BITS(bundle->rd.tonm >> 8u, 0, 8);
    /* 1-to-1 split-field stuffing mapping */
    data[3] = STUFF_BITS(bundle->rd.toffm >> 8u, 0, 8);
    /* 1-to-many stuffing mapping */
    data[4] = STUFF_BITS(bundle->rd.toffm, 4, 4) | STUFF_BITS(bundle->rd.tonm, 0, 4);
    data[5] = STUFF_BITS(bundle->rd.remctrl, 5, 1) |
              STUFF_BITS(bundle->rd.lfavl, 4, 1) |
              STUFF_BITS(bundle->rd.hfavl, 3, 1) |
              /* 1-to-many stuffing mapping */
              STUFF_BITS(bundle->rd.lgt_dmr, 0, 3);
    data[6] = STUFF_BITS(bundle->st.itemp, 2, 6) | STUFF_BITS(bundle->st.actext, 0, 2);
    data[7] =
        STUFF_BITS(bundle->st.ci_error, 7, 1) |
        STUFF_BITS(bundle->st.not_init, 6, 1) |
        STUFF_BITS(bundle->st.remctrl, 5, 1) |
        /* many-to-1 stuffing mapping */
        STUFF_BITS(bundle->st.status, 1, 2) |
        STUFF_BITS(bundle->protocol.is_local_change_frame, 0, 1);
}

void dometic_shape_ac2_ctrl_extract(
    dometic_shape_ac2_ctrl_bundle_t *bundle,
    const uint8_t data[DOMETIC_SHAPE_DATA_LENGTH])
{
    bundle->icl.input_current_limit = EXTRACT_BITS(data[0], 0, 4);
}

void dometic_shape_ac2_info_stuff(
    const dometic_shape_ac2_info_bundle_t *bundle,
    uint8_t data[DOMETIC_SHAPE_DATA_LENGTH])
{
    data[0] = STUFF_BITS(bundle->icl.input_current_limit, 0, 4);
}

/**
 *  \brief Extract data from LIN frame function.
 *
 *  This function will extract parameters from Shape AC Ctrl frame buffer.
 *
 *
 *  \param pCtrl     out: Extracted parameters
 *  \param pu8Data   in : DGN frame buffer
 *
 */
void dometic_shape_ac_C_Extract(dometic_shape_ctrl_t *pCtrl, const uint8_t *pu8Data)
{
    int32_t temp;
    int32_t mode_tmp;
    int32_t off_min_ms_tmp;
    int32_t off_min_ls_tmp;
    int32_t on_min_ms_tmp;
    int32_t on_min_ls_tmp;
    uint8_t light_sent;

    /*           dest                       src         pos size */
    DGN_TO_BITS(mode_tmp, pu8Data[0], 0, 3);
    DGN_TO_BITS(pCtrl->sleep_mode, pu8Data[0], 3, 1);
    DGN_TO_BITS(pCtrl->timer_off_mode, pu8Data[0], 4, 1);
    DGN_TO_BITS(pCtrl->timer_on_mode, pu8Data[0], 5, 1);
    /* Data_0 reserved bits: 6 */
    DGN_TO_BITS(pCtrl->power, pu8Data[0], 7, 1);

    DGN_TO_BITS(pCtrl->light_external, pu8Data[1], 0, 1);
    DGN_TO_BITS(pCtrl->fan_speed_mode, pu8Data[1], 1, 3);
    DGN_TO_BITS(temp, pu8Data[1], 4, 4);

    DGN_TO_BITS(on_min_ms_tmp, pu8Data[2], 0, 8);

    DGN_TO_BITS(off_min_ms_tmp, pu8Data[3], 0, 8);

    DGN_TO_BITS(on_min_ls_tmp, pu8Data[4], 0, 4);
    DGN_TO_BITS(off_min_ls_tmp, pu8Data[4], 4, 4);

    /* Data_4 reserved bits: 4-7 */

    DGN_TO_BITS(light_sent, pu8Data[5], 0, 3);
    DGN_TO_BITS(pCtrl->ac0_heat_enable, pu8Data[5], 3, 1);
    DGN_TO_BITS(pCtrl->ac0_light_enable, pu8Data[5], 4, 1);
    DGN_TO_BITS(pCtrl->remote_ctrl_status, pu8Data[5], 5, 1);
    /* Data_5 reserved bits: 6-7 */

    /* Data_6 reserved bits: 0-7 */

    DGN_TO_BITS(pCtrl->remote_ctrl_dis, pu8Data[7], 0, 1);
    /* Data_7 reserved bits: 1 */
    DGN_TO_BITS(pCtrl->sync_frame, pu8Data[7], 2, 1);
    DGN_TO_BITS(pCtrl->timer_update, pu8Data[7], 3, 1);
    DGN_TO_BITS(pCtrl->set_inverter_off, pu8Data[7], 4, 1);
    DGN_TO_BITS(pCtrl->set_inverter_on, pu8Data[7], 5, 1);
    /* Data_7 reserved bits: 6-7 */

    /* Call conversion functions for needed signals */
    pCtrl->temp = dometic_shape_convert_temp_tosys(temp);
    pCtrl->mode = dometic_shape_convert_mode_tosys(mode_tmp);
    dometic_shape_convert_time_tosys(off_min_ms_tmp,
                                     off_min_ls_tmp,
                                     on_min_ms_tmp,
                                     on_min_ls_tmp,
                                     &pCtrl->off_timer_min,
                                     &pCtrl->on_timer_min);
    dometic_shape_convert_light_tosys(&pCtrl->light_internal, &pCtrl->light_dmr, light_sent);
    dometic_shape_convert_actuators_tosys(0,
                                          0,
                                          pCtrl->set_inverter_on,
                                          &pCtrl->actuators);
}

/**
 *  \brief Stuff data to LIN frame function.
 *
 *  This function will stuff the parameters for Shape AC into a Info frame buffer.
 *
 *
 *  \param pu8Data    out: LIN frame buffer
 *  \param pInfo      in : Parameters to stuff
 *
 */
void dometic_shape_ac_I_Stuff(uint8_t *pu8Data, const dometic_shape_info_t *pInfo)
{
    int32_t temp;
    int32_t mode_tmp;
    int32_t internal_temp_tmp;
    int32_t off_min_ms_tmp;
    int32_t off_min_ls_tmp;
    int32_t on_min_ms_tmp;
    int32_t on_min_ls_tmp;
    uint8_t light_sent;
    uint8_t no_init;
    uint8_t heater_run;
    uint8_t comp_run;
    uint8_t error;
    uint8_t no_main;

    /* Call conversion functions for needed signals */
    no_init = !(0x01 & pInfo->no_init);
    dometic_shape_convert_time_tolin(&off_min_ms_tmp,
                                     &off_min_ls_tmp,
                                     &on_min_ms_tmp,
                                     &on_min_ls_tmp,
                                     pInfo->off_timer_min,
                                     pInfo->on_timer_bits);
    mode_tmp = dometic_shape_convert_mode_tolin(pInfo->mode);
    temp = dometic_shape_convert_temp_tolin(pInfo->temp);
    internal_temp_tmp = dometic_shape_convert_itemp_tolin(pInfo->int_temp);
    light_sent = dometic_shape_convert_light_tolin(pInfo->light_internal, pInfo->light_dmr);
    dometic_shape_convert_actuators_tolin(&heater_run,
                                          &comp_run,
                                          NULL,
                                          NULL,
                                          pInfo->actuators);
    dometic_shape_convert_err_tolin(pInfo->status, &error, &no_main);
    /* Clear */
    (void)memset(pu8Data, 0, DOMETIC_SHAPE_AC_INFO_SIZE);

    /*          dest        source                     pos size */
    BITS_TO_DGN(pu8Data[0], mode_tmp, 0, 3);
    BITS_TO_DGN(pu8Data[0], pInfo->sleep_mode, 3, 1);
    BITS_TO_DGN(pu8Data[0], pInfo->timer_off_mode, 4, 1);
    BITS_TO_DGN(pu8Data[0], pInfo->timer_on_mode, 5, 1);
    /* Data_0 reserved bits: 6 */
    BITS_TO_DGN(pu8Data[0], pInfo->power, 7, 1);

    BITS_TO_DGN(pu8Data[1], pInfo->light_external, 0, 1);
    BITS_TO_DGN(pu8Data[1], pInfo->fan_speed_mode, 1, 3);
    BITS_TO_DGN(pu8Data[1], temp, 4, 4);

    BITS_TO_DGN(pu8Data[2], on_min_ms_tmp, 0, 8);

    BITS_TO_DGN(pu8Data[3], off_min_ms_tmp, 0, 8);

    BITS_TO_DGN(pu8Data[4], on_min_ls_tmp, 0, 4);
    BITS_TO_DGN(pu8Data[4], off_min_ls_tmp, 4, 4);

    BITS_TO_DGN(pu8Data[5], light_sent, 0, 3);
    BITS_TO_DGN(pu8Data[5], pInfo->ac0_heat_enable, 3, 1);
    BITS_TO_DGN(pu8Data[5], pInfo->ac0_light_enable, 4, 1);
    BITS_TO_DGN(pu8Data[5], pInfo->remote_ctrl_status, 5, 1);
    /* Data_5 reserved bits: 6-7 */

    /* Data_6 reserved bits: 0 */
    BITS_TO_DGN(pu8Data[6], heater_run, 0, 1);
    BITS_TO_DGN(pu8Data[6], comp_run, 1, 1);
    BITS_TO_DGN(pu8Data[6], internal_temp_tmp, 2, 6);

    BITS_TO_DGN(pu8Data[7], pInfo->local_change, 0, 1);
    BITS_TO_DGN(pu8Data[7], no_main, 1, 1);
    BITS_TO_DGN(pu8Data[7], error, 2, 1);
    BITS_TO_DGN(pu8Data[7], pInfo->timer_on_req, 3, 1);
    BITS_TO_DGN(pu8Data[7], pInfo->timer_off_req, 4, 1);
    BITS_TO_DGN(pu8Data[7], pInfo->remote_on, 5, 1);
    BITS_TO_DGN(pu8Data[7], no_init, 6, 1);
    BITS_TO_DGN(pu8Data[7], pInfo->ci_error, 7, 1);
}

/**
 *  \brief Stuff data to LIN frame function.
 *
 *  This function will stuff the parameters for Shape AC into a Ctrl frame buffer.
 *
 *
 *  \param pu8Data    out: LIN frame buffer
 *  \param pCtrl      in : Parameters to stuff
 */
void dometic_shape_ac_C_Stuff(uint8_t *pu8Data, dometic_shape_ctrl_t *pCtrl)
{
    int32_t temp;
    int32_t mode_tmp;
    int32_t off_min_ms_tmp;
    int32_t off_min_ls_tmp;
    int32_t on_min_ms_tmp;
    int32_t on_min_ls_tmp;
    uint8_t light_sent;

    /* Call conversion functions for needed signals */
    dometic_shape_convert_time_tolin(&off_min_ms_tmp,
                                     &off_min_ls_tmp,
                                     &on_min_ms_tmp,
                                     &on_min_ls_tmp,
                                     pCtrl->off_timer_min,
                                     pCtrl->on_timer_min);
    mode_tmp = dometic_shape_convert_mode_tolin(pCtrl->mode);
    temp = dometic_shape_convert_temp_tolin(pCtrl->temp);
    light_sent = dometic_shape_convert_light_tolin(pCtrl->light_internal, pCtrl->light_dmr);
    dometic_shape_convert_actuators_tolin(NULL,
                                          NULL,
                                          &pCtrl->set_inverter_on,
                                          &pCtrl->set_inverter_off,
                                          pCtrl->actuators);

    /* Clear */
    (void)memset(pu8Data, 0, DOMETIC_SHAPE_AC_CTRL_SIZE);

    /*           dest                       src         pos size */
    BITS_TO_DGN(pu8Data[0], mode_tmp, 0, 3);
    BITS_TO_DGN(pu8Data[0], pCtrl->sleep_mode, 3, 1);
    BITS_TO_DGN(pu8Data[0], pCtrl->timer_off_mode, 4, 1);
    BITS_TO_DGN(pu8Data[0], pCtrl->timer_on_mode, 5, 1);
    /* Data_0 reserved bits: 6 */
    BITS_TO_DGN(pu8Data[0], pCtrl->power, 7, 1);

    BITS_TO_DGN(pu8Data[1], pCtrl->light_external, 0, 1);
    BITS_TO_DGN(pu8Data[1], pCtrl->fan_speed_mode, 1, 3);
    BITS_TO_DGN(pu8Data[1], temp, 4, 4);

    BITS_TO_DGN(pu8Data[2], on_min_ms_tmp, 0, 8);

    BITS_TO_DGN(pu8Data[3], off_min_ms_tmp, 0, 8);

    BITS_TO_DGN(pu8Data[4], on_min_ls_tmp, 0, 4);
    BITS_TO_DGN(pu8Data[4], off_min_ls_tmp, 4, 4);

    BITS_TO_DGN(pu8Data[5], light_sent, 0, 3);
    BITS_TO_DGN(pu8Data[5], pCtrl->ac0_heat_enable, 3, 1);
    BITS_TO_DGN(pu8Data[5], pCtrl->ac0_light_enable, 4, 1);
    BITS_TO_DGN(pu8Data[5], pCtrl->remote_ctrl_status, 5, 1);
    /* Data_5 reserved bits: 6-7 */

    /* Data_6 reserved bits: 0-7 */

    BITS_TO_DGN(pu8Data[7], pCtrl->remote_ctrl_dis, 0, 1);
    /* Data_7 reserved bits: 1 */
    BITS_TO_DGN(pu8Data[7], pCtrl->sync_frame, 2, 1);
    BITS_TO_DGN(pu8Data[7], pCtrl->timer_update, 3, 1);
    BITS_TO_DGN(pu8Data[7], pCtrl->set_inverter_off, 4, 1);
    BITS_TO_DGN(pu8Data[7], pCtrl->set_inverter_on, 5, 1);
    /* Data_7 reserved bits: 6-7 */
}

void dometic_shape_var_I_Extract(dometic_shape_var_info_t *pInfo, const uint8_t *pu8Data)
{
    /* TODO: Implement the function */
}

void dometic_shape_var_C_Extract(dometic_shape_var_ctrl_t *pCtrl, const uint8_t *pu8Data)
{
    /* TODO: Implement the function */
}

void dometic_shape_var_I_Stuff(uint8_t *pu8Data, const dometic_shape_var_info_t *pInfo)
{
    /* TODO: Implement the function */
}

void dometic_shape_var_C_Stuff(uint8_t *pu8Data, const dometic_shape_var_ctrl_t *pCtrl)
{
    /* TODO: Implement the function */
}

/**
 *  \brief Convert Air heater target temp from LIN value to system value
 *  before publishing.
 *
 *  \param i32Value    Input temperature raw value. Values below -50 will be
 *                     overriden to -50, as the min definiton of the HTR0ATEMP
 *                     RVC DGN is min=-50.
 *
 *  \retval res        Converted value.
 *
 */
static int32_t dometic_sharc_air_convert_temp_tosys(int32_t i32Value)
{
    int32_t res = 0;

    /* Valid range -64°C … +62.5°C (0x00 … 0xFD), 0.5 °C/bit */

    if ((i32Value >= 0) && (i32Value <= 253))
    {
        int32_t min_rvc_dgn_target_temp_val = 0x1C;   // -50
        if (i32Value <= min_rvc_dgn_target_temp_val)  // below -50
        {
            i32Value = min_rvc_dgn_target_temp_val;
        }

        res = ((i32Value * 10) / 2 - 640) * 100; /* (i32Value / 2 - 64) * 1000 */
    }

    return res;
}

/**
 *  \brief Convert Air heater target temperature from system value to LIN value.
 *
 *
 *  \param i32Value    DDM temperature physical value.
 *  \retval res        Converted value.
 *
 */
static int32_t dometic_sharc_air_convert_temp_tolin(int32_t i32Value)
{
    int32_t res = 0;

    if ((i32Value >= -64000) && (i32Value <= 62500))
    {
        res = i32Value / 500 + 128; /* (i32Value / 1000 + 64) * 2 */
    }

    return res;
}

/**
 *  \brief Convert Air heater room temperature from system value to LIN value.
 *
 *
 *  \param i32Value    DDM temperature physical value.
 *  \retval res        Converted value.
 *
 */
static int32_t dometic_sharc_air_convert_itemp_tolin(int32_t i32Value)
{
    int32_t res = 0;

    if (i32Value < -50000)
    {
        /* Limit out of range to -50C: (-50000 / 1000) + 50 = 0 */
        res = 0;
    }
    else if (i32Value > 150000)
    {
        /* Limit out of range to 150C: (150000 / 1000) + 50 = 200 */
        res = 200;
    }
    else
    {
        /* Convert DDM value to LIN value */
        res = (i32Value / 1000) + 50;
    }
    return res;
}

/**
 *  \brief Convert Air heater room temperature from LIN value to system value.
 *
 *
 *  \param i32Value    DDM temperature raw value.
 *  \retval res        Converted value.
 *
 */
static int32_t dometic_sharc_air_convert_itemp_tosys(int32_t i32Value)
{
    int32_t res = 0;

    if ((i32Value >= 0) && (i32Value <= 255))
    {
        res = (i32Value - 50) * 1000;
    }

    return res;
}

/**
 *  \brief Convert Air heater water temp from LIN value to system value
 *  before publishing.
 *
 *
 *  \param i32Value    DDM water temperature raw value.
 *  \retval res        Converted value.
 *
 */
static int32_t dometic_sharc_air_convert_wtrtemp_tosys(int32_t i32Value)
{
    int32_t res = 0;

    /* Map LIN temperature modes to DDM temperature modes */
    switch (i32Value)
    {
    case LIN_WTRTEMP_ECO:
        res = WTRTEMP_ECO;
        break;
    case LIN_WTRTEMP_HOT:
        res = WTRTEMP_HOT;
        break;
    case LIN_WTRTEMP_BOOST:
        res = WTRTEMP_BOOST;
        break;
    case LIN_WTRTEMP_OFF:
        res = WTRTEMP_OFF;
        break;
    default:
        res = WTRTEMP_OFF;
        break;
    }

    return res;
}

/**
 *  \brief Convert Air heater water temp from system value to LIN value.
 *
 *
 *  \param i32Value    DDM water temperature physical value.
 *  \retval res        Converted value.
 *
 */
static int32_t dometic_sharc_air_convert_wtrtemp_tolin(int32_t i32Value)
{
    int32_t res = 0;

    /* Map DDM temperature modes to LIN temperature modes */
    switch (i32Value)
    {
    case WTRTEMP_ECO:
        res = LIN_WTRTEMP_ECO;
        break;
    case WTRTEMP_HOT:
        res = LIN_WTRTEMP_HOT;
        break;
    case WTRTEMP_BOOST:
        res = LIN_WTRTEMP_BOOST;
        break;
    case WTRTEMP_OFF:
        res = LIN_WTRTEMP_OFF;
        break;
    default:
        res = LIN_WTRTEMP_OFF;
        break;
    }

    return res;
}

/**
 *  \brief Extract parameters from Diagnostic Dometic generic PID
 *  Info frame buffer.
 *
 *
 *  \param pInfo   Pointer to info frame data structure.
 *  \param pu8Data Pointer to LIN frame data.
 *
 */
void dometic_gen_pid_diag_I_Extract(dometic_gen_pid_diag_info_t *pInfo, uint8_t *pu8Data)
{
    /*           dest                          src */
    DGN_TO_BITS(pInfo->nad, pu8Data[0], 0, 8);
    DGN_TO_BITS(pInfo->pci, pu8Data[1], 0, 8);
    DGN_TO_BITS(pInfo->rsid, pu8Data[2], 0, 8);
    DGN_TO_WORD(pInfo->supplier_id, pu8Data[3]);
    DGN_TO_WORD(pInfo->func_id, pu8Data[5]);
    DGN_TO_BITS(pInfo->var_id, pu8Data[7], 0, 8);
}

/**
 *  \brief Extract parameters from Diagnostic Dometic SHARC PID
 *  Info frame buffer.
 *
 *
 *  \param pInfo   Pointer to info frame data structure.
 *  \param pu8Data Pointer to LIN frame data.
 *
 */
void dometic_sharc_pid_diag_I_Extract(dometic_sharc_pid_diag_info_t *pInfo, uint8_t *pu8Data)
{
    /*           dest                    src      start   size/bit */
    DGN_TO_BITS(pInfo->nad, pu8Data[0], 0, 8);
    DGN_TO_BITS(pInfo->pci, pu8Data[1], 0, 8);
    DGN_TO_BITS(pInfo->rsid, pu8Data[2], 0, 8);
    DGN_TO_WORD(pInfo->supplier_id, pu8Data[3]);
    DGN_TO_WORD(pInfo->func_id, pu8Data[5]);
    DGN_TO_BITS(pInfo->var_id, pu8Data[7], 0, 8);
}

/**
 *  \brief Stuff parameters for Diagnostic Dometic SHARC Serial
 *  into a frame buffer.
 *
 *
 *  \param pu8Data  Pointer to LIN frame data.
 *  \param pCtrl    Pointer to control frame data structure.
 *
 */
void dometic_sharc_serial_diag_C_Stuff(uint8_t *pu8Data, dometic_sharc_serial_diag_ctrl_t *pCtrl)
{
    /* Clear */
    (void)memset(pu8Data, 0, DOMETIC_REQ_FRAME_SIZE_DIAG);

    /* Fill */
    /*              dest          src            start   size/bit */
    BITS_TO_DGN(pu8Data[0], pCtrl->nad, 0, 8);
    BITS_TO_DGN(pu8Data[1], pCtrl->pci, 0, 8);
    BITS_TO_DGN(pu8Data[2], pCtrl->sid, 0, 8);
    BITS_TO_DGN(pu8Data[3], pCtrl->data1, 0, 8);
    WORD_TO_DGN(pu8Data[4], pCtrl->supplier_id);
    WORD_TO_DGN(pu8Data[6], pCtrl->func_id);
}

/**
 *  \brief Extract parameters from Diagnostic Dometic SHARC Serial
 *  Info frame buffer.
 *
 *
 *  \param pInfo   Pointer to info frame data structure.
 *  \param pu8Data Pointer to LIN frame data.
 *
 */
void dometic_sharc_serial_diag_I_Extract(dometic_sharc_serial_diag_info_t *pInfo, uint8_t *pu8Data)
{
    /*           dest                    src      start   size/bit */
    DGN_TO_BITS(pInfo->nad, pu8Data[0], 0, 8);
    DGN_TO_BITS(pInfo->pci, pu8Data[1], 0, 8);
    DGN_TO_BITS(pInfo->rsid, pu8Data[2], 0, 8);
    DGN_TO_DWRD(pInfo->serial_number, pu8Data[3]);
    DGN_TO_BITS(pInfo->data8, pu8Data[7], 0, 8);
}

/**
 *  \brief Stuff parameters for Dometic SHARC Assign NAD
 *  into a frame buffer.
 *
 *
 *  \param pu8Data  Pointer to LIN frame data.
 *  \param pCtrl    Pointer to control frame data structure.
 *
 */
void dometic_sharc_assignnad_diag_C_Stuff(uint8_t *pu8Data, dometic_sharc_nad_diag_ctrl_t *pCtrl)
{
    /* Clear */
    (void)memset(pu8Data, 0, DOMETIC_REQ_FRAME_SIZE_DIAG);

    /* Fill */
    /*           dest              src           start   size/bit */
    BITS_TO_DGN(pu8Data[0], pCtrl->nad, 0, 8);
    BITS_TO_DGN(pu8Data[1], pCtrl->pci, 0, 8);
    BITS_TO_DGN(pu8Data[2], pCtrl->sid, 0, 8);
    WORD_TO_DGN(pu8Data[3], pCtrl->supplier_id);
    WORD_TO_DGN(pu8Data[5], pCtrl->func_id);
    BITS_TO_DGN(pu8Data[7], pCtrl->new_nad, 0, 8);
}

/**
 *  \brief Extract parameters from Diagnostic Dometic SHARC Assign NAD
 *  Info frame buffer.
 *
 *
 *  \param pInfo   Pointer to info frame data structure.
 *  \param pu8Data Pointer to LIN frame data.
 *
 */
void dometic_sharc_assignnad_diag_I_Extract(dometic_sharc_nad_diag_info_t *pInfo, uint8_t *pu8Data)
{
    /*           dest                    src          start   size/bit */
    DGN_TO_BITS(pInfo->nad, pu8Data[0], 0, 8);
    DGN_TO_BITS(pInfo->pci, pu8Data[1], 0, 8);
    DGN_TO_BITS(pInfo->rsid, pu8Data[2], 0, 8);
    DGN_TO_BITS(pInfo->data1, pu8Data[3], 0, 8);
    DGN_TO_BITS(pInfo->data2, pu8Data[4], 0, 8);
    DGN_TO_BITS(pInfo->data3, pu8Data[5], 0, 8);
    DGN_TO_BITS(pInfo->data4, pu8Data[6], 0, 8);
    DGN_TO_BITS(pInfo->data5, pu8Data[7], 0, 8);
}

/**
 *  \brief Stuff parameters for Diagnostic Dometic SHARC Assign Frame
 *  into a frame buffer.
 *
 *
 *  \param pu8Data  Pointer to LIN frame data.
 *  \param pCtrl    Pointer to control frame data structure.
 *
 */
void dometic_sharc_assignframe_diag_C_Stuff(uint8_t *pu8Data, dometic_sharc_frame_diag_ctrl_t *pCtrl)
{
    /* Clear */
    (void)memset(pu8Data, 0, DOMETIC_REQ_FRAME_SIZE_DIAG);

    /* Fill */
    /*           dest            src              start   size/bit */
    BITS_TO_DGN(pu8Data[0], pCtrl->nad, 0, 8);
    BITS_TO_DGN(pu8Data[1], pCtrl->pci, 0, 8);
    BITS_TO_DGN(pu8Data[2], pCtrl->sid, 0, 8);
    BITS_TO_DGN(pu8Data[3], pCtrl->start_index, 0, 8);
    DWRD_TO_DGN(pu8Data[4], pCtrl->product_id);
}

/**
 *  \brief Extract parameters from Dometic SHARC Diagnostic Assign Frame
 *  Info frame buffer.
 *
 *
 *  \param pInfo   Pointer to info frame data structure.
 *  \param pu8Data Pointer to LIN frame data.
 *
 */
void dometic_sharc_assignframe_diag_I_Extract(dometic_sharc_frame_diag_info_t *pInfo, uint8_t *pu8Data)
{
    /*           dest                    src        start   size/bit */
    DGN_TO_BITS(pInfo->nad, pu8Data[0], 0, 8);
    DGN_TO_BITS(pInfo->pci, pu8Data[1], 0, 8);
    DGN_TO_BITS(pInfo->rsid, pu8Data[2], 0, 8);
    DGN_TO_BITS(pInfo->data1, pu8Data[3], 0, 8);
    DGN_TO_BITS(pInfo->data2, pu8Data[4], 0, 8);
    DGN_TO_BITS(pInfo->data3, pu8Data[5], 0, 8);
    DGN_TO_BITS(pInfo->data4, pu8Data[6], 0, 8);
    DGN_TO_BITS(pInfo->data5, pu8Data[7], 0, 8);
}

/**
 *  \brief Extract parameters from Diagnostic Dometic Negative
 *  Info frame buffer.
 *
 *
 *  \param pInfo   Pointer to info frame data structure.
 *  \param pu8Data Pointer to LIN frame data.
 *
 */
void dometic_generic_negative_diag_I_Extract(dometic_gen_neg_diag_info_t *pInfo, uint8_t *pu8Data)
{
    /*           dest                    src      start   size/bit */
    DGN_TO_BITS(pInfo->nad, pu8Data[0], 0, 8);
    DGN_TO_BITS(pInfo->pci, pu8Data[1], 0, 8);
    DGN_TO_BITS(pInfo->rsid, pu8Data[2], 0, 8);
    DGN_TO_BITS(pInfo->req_sid, pu8Data[3], 0, 8);
    DGN_TO_BITS(pInfo->data2, pu8Data[4], 0, 8);
    DGN_TO_BITS(pInfo->data3, pu8Data[5], 0, 8);
    DGN_TO_BITS(pInfo->data4, pu8Data[6], 0, 8);
    DGN_TO_BITS(pInfo->data5, pu8Data[7], 0, 8);
}

static INVENT_V1_LEDBRIGHT_DDM invent_v1_lin_to_ddm_inv_dim0lvl(INVENT_V1_LEDBRIGHT_LIN lin_ledstrip_bright)
{
    INVENT_V1_LEDBRIGHT_DDM ledbright_ddm;

    switch (lin_ledstrip_bright)
    {
    case INV_LIN_LEDBRIGHT_0:
        ledbright_ddm = INV_LEDBRIGHT_0PER;
        break;
    case INV_LIN_LEDBRIGHT_1:
        ledbright_ddm = INV_LEDBRIGHT_5PER;
        break;
    case INV_LIN_LEDBRIGHT_4:
        ledbright_ddm = INV_LEDBRIGHT_40PER;
        break;
    case INV_LIN_LEDBRIGHT_10:
        ledbright_ddm = INV_LEDBRIGHT_100PER;
        break;
    default:
        ledbright_ddm = INV_LEDBRIGHT_0PER;
        break;
    }
#ifdef DEBUG_LOG_APPLIN
    LOG(I, "LED bright len:%d, ddm:%d", lin_ledstrip_bright, ledbright_ddm);
#endif
    return ledbright_ddm;
}

static INVENT_V1_LEDBRIGHT_LIN invent_v1_ddm_to_lin_inv_dim0lvl(INVENT_V1_LEDBRIGHT_DDM ddm_md)
{
    INVENT_V1_LEDBRIGHT_LIN ledbright_lin;

    switch (ddm_md)
    {
    case INV_LEDBRIGHT_0PER:
        ledbright_lin = INV_LIN_LEDBRIGHT_0;
        break;
    case INV_LEDBRIGHT_5PER:
        ledbright_lin = INV_LIN_LEDBRIGHT_1;
        break;
    case INV_LEDBRIGHT_40PER:
        ledbright_lin = INV_LIN_LEDBRIGHT_4;
        break;
    case INV_LEDBRIGHT_100PER:
        ledbright_lin = INV_LIN_LEDBRIGHT_10;
        break;
    default:
        ledbright_lin = 0;
        break;
    }
    return ledbright_lin;
}

size_t dometic_invent_v1_conv_ledbright_to_dim0lvl(const void *conv_data, size_t conv_data_size, void *ddm_data, size_t ddm_data_size)
{
    /* md member is uint8_t type */
    const uint8_t *dometic_data = conv_data;
    /* The DIM0LVL is int32_t type */
    int32_t *dim0lvl_data = ddm_data;

    /* Ensure data sizes are correct */
    TRUE_CHECK_RETURN0(sizeof(*dometic_data) == conv_data_size);
    TRUE_CHECK_RETURN0(sizeof(*dim0lvl_data) <= ddm_data_size);
    /* Convert according to spec */
    *dim0lvl_data = invent_v1_lin_to_ddm_inv_dim0lvl(*dometic_data);
    /* Return that we have written some bytes to ddm data */
    return sizeof(*dim0lvl_data);
}

void dometic_invent_v1_conv_dim0lvl_to_ledbright(const void *const *ddm_data, const size_t *ddm_data_size, size_t conv_data_size, void *conv_data)
{
    /* Access only DDM 0 */
    /* md member is uint8_t type */
    uint8_t *dometic_data = conv_data;
    /* The DIM0LVL is int32_t type */
    const int32_t *dim0lvl_data = ddm_data[0];

    /* Ensure data sizes are correct */
    TRUE_CHECK_RETURN(sizeof(*dometic_data) == conv_data_size);
    TRUE_CHECK_RETURN(sizeof(*dim0lvl_data) == ddm_data_size[0]);
    /* Convert according to spec */
    *dometic_data = invent_v1_ddm_to_lin_inv_dim0lvl(*dim0lvl_data);
}

/**
 *  \brief Conversion function for Error code
 *
 *  This function will split the Error code into 4 bytes for LIN data
 *
 *  \param inv_errcode_byte0    Byte 0 (LSB) of the error code
 *  \param inv_errcode_byte1    Byte 1 of the error code
 *  \param inv_errcode_byte2    Byte 2 of the error code
 *  \param inv_errcode_byte3    Byte 3 (MSB) of the error code
 *  \param inv_errorcode        Complete error code
 */
static void dometic_invent_convert_errcode_tolin(uint8_t *inv_errcode_byte0,
                                                 uint8_t *inv_errcode_byte1,
                                                 uint8_t *inv_errcode_byte2,
                                                 uint8_t *inv_errcode_byte3,
                                                 const uint32_t inv_errorcode)
{
#ifdef DEBUG_LOG_APPLIN
    LOG(I, "errcodelin:%d", inv_errorcode);
#endif
    /* stuff filter reset max count to lin bytes */
    *inv_errcode_byte3 = (uint8_t)(inv_errorcode >> 24) & 0xFF;  // (*inv_errcode_byte2 << 16) || (*inv_errcode_byte1 << 8) || (*inv_errcode_byte0);
    *inv_errcode_byte2 = (uint8_t)(inv_errorcode >> 16) & 0xFF;
    *inv_errcode_byte1 = (uint8_t)(inv_errorcode >> 8) & 0xFF;
    *inv_errcode_byte0 = (uint8_t)(inv_errorcode)&0xFF;
}

void dometic_invent_v1_ctrl_extract(
    dometic_invent_v1_ctrl_bundle_t *bundle,
    const uint8_t data[DOMETIC_INVENT_V1_DATA_LENGTH])
{
    /*           dest                           src         pos size */
    bundle->rd.inv_pwr = EXTRACT_BITS(data[0], 0, 1);
    bundle->rd.inv_mode = EXTRACT_BITS(data[0], 1, 2);
    bundle->rd.inv_ledstrip_bright = EXTRACT_BITS(data[0], 3, 4);

    bundle->rd.inv_filtreset = EXTRACT_BITS(data[1], 0, 2);
    bundle->rd.inv_stormodeavl = EXTRACT_BITS(data[1], 2, 1);
    /* Remote Data_1 reserved bits: 5-7 */
    /* Remote Data_2 reserved bits: 0-7 */
    /* Remote Data_3 reserved bits: 0-7 */
    /* Remote Data_4 reserved bits: 0-7 */
    /* Remote Data_5 reserved bits: 0-7 */
    /* Remote Data_6 reserved bits: 0-7 */
    /* Remote Data_7 reserved bits: 5-7 */

    bundle->st.ctrl_page = EXTRACT_BITS(data[7], 0, 1);
    bundle->protocol.is_sync_frame = EXTRACT_BITS(data[7], 2, 1);
    bundle->st.info_pageid = EXTRACT_BITS(data[7], 3, 2);

#ifdef DEBUG_LOG_APPLIN
    LOG(I, "FromLIN:%d", bundle->rd.inv_ledstrip_bright);

    LOG(I, "C_Extract: pwr:%x, mode:%x, led:%x, filter:%x, storage:%x, sync:%x",
        bundle->rd.inv_pwr, bundle->rd.inv_mode, bundle->rd.inv_ledstrip_bright,
        bundle->rd.inv_filtreset, bundle->rd.inv_stormodeavl, bundle->protocol.is_sync_frame);
#endif
}

void dometic_invent_v1_info_stuff(
    const dometic_invent_v1_info_bundle_t *bundle,
    uint8_t data[DOMETIC_INVENT_V1_DATA_LENGTH])
{
    uint8_t inv_errcode_by0;
    uint8_t inv_errcode_by1;
    uint8_t inv_errcode_by2;
    uint8_t inv_errcode_by3;

    dometic_invent_convert_errcode_tolin(&inv_errcode_by0,
                                         &inv_errcode_by1,
                                         &inv_errcode_by2,
                                         &inv_errcode_by3,
                                         bundle->st.inv_errcode);

    data[0] = STUFF_BITS(bundle->rd.inv_ledstrip_bright, 3, 4) |
              STUFF_BITS(bundle->rd.inv_mode, 1, 2) |
              STUFF_BITS(bundle->rd.inv_pwr, 0, 1);
    data[1] = STUFF_BITS(bundle->st.inv_airqua, 5, 3) |
              STUFF_BITS(bundle->rd.inv_stormodeavl, 2, 1) |
              STUFF_BITS(bundle->rd.inv_filtreset, 0, 2);
    data[2] = STUFF_BITS(bundle->st.inv_ionizstat, 5, 1) |
              STUFF_BITS(bundle->st.inv_wifistat, 2, 3) |
              STUFF_BITS(bundle->st.inv_pwrsrcstat, 0, 2);

    data[3] = STUFF_BITS(inv_errcode_by0, 0, 8);
    data[4] = STUFF_BITS(inv_errcode_by1, 0, 8);
    data[5] = STUFF_BITS(inv_errcode_by2, 0, 8);
    data[6] = STUFF_BITS(inv_errcode_by3, 0, 8);

    data[7] = STUFF_BITS(bundle->protocol.response_error, 7, 1) |
              STUFF_BITS(bundle->protocol.active_error_flag, 3, 1) |
              STUFF_BITS(bundle->protocol.info_page, 1, 2) |
              STUFF_BITS(bundle->protocol.is_local_change_frame, 0, 1);

#ifdef DEBUG_LOG_APPLIN
    LOG(I, "LIN Stuff-D0:%x, D1:%x, D2:%x, D3:%x, D4:%x, D5:%x, D6:%x, D7:%x",
        data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
#endif
}

/**Lin dev start Bridge NRX **/

/*
Control Frame
byte0.bit3-bit4: 0: Performance Mode 1: Quiet Mode 2: Eco Mode 3: Freezer Mode
byte1.bit0-bit3: 0: step 1  1:step 2  2:step  3:step 4  4:step 5
byte7.bit7: 1:Fridge on  0:Fridge off
*/
void dometic_nrx_v1_ctrl_extract(dometic_nrx_v1_ctrl_bundle_t *bundle, const uint8_t data[DOMETIC_NRX_V1_DATA_LENGTH])
{
    /*           dest                  src         pos size */
    bundle->rd.nrx_userMode = EXTRACT_BITS(data[0], 3, 2);
    bundle->rd.nrx_setTemp = EXTRACT_BITS(data[1], 0, 4);
    bundle->rd.nrx_power = EXTRACT_BITS(data[7], 7, 1);

#ifdef DEBUG_LOG_APPLIN
    LOG(D, "LIN extract-D0:%x, D1:%x, D2:%x, D3:%x, D4:%x, D5:%x, D6:%x, D7:%x",
        data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
#endif
}

/*
Information Frame
byte0.bit3-bit4: 0: Performance Mode 1: Quiet Mode 2: Eco Mode 3: Freezer Mode
byte0.bit6: 1:compressor on  0:compressor off
byte1.bit0-bit3: 0: step 1  1:step 2  2:step  3:step 4  4:step 5
byte4-byte5: Fresh Temper
byte7.bit7: 1:Fridge on  0:Fridge off
byte6.bit0-bit6: Error code

 *  \brief Conversion function for Error code
       --LIN--                                       --UART--
0x00  No errors                                    No errors
0x01  W19-Low battery voltage             Battery voltage fault
0x02  E32--Condenser fan over current     Fan overcurrent protection
0x04  E33-Compressor startup failure      Compressor startup failure
0x08  E34-Compressor stall protection     Compressor stall protection
0x10  E35--Controller overheat protection Controller Overheat Protection
0x20  W10-Door open timeout               Open door timeout fault
0x40  W02-Fresh NTC defective             NTC1 sensor error
0x80
*/
void dometic_nrx_v1_info_stuff(const dometic_nrx_v1_info_bundle_t *bundle, uint8_t data[DOMETIC_NRX_V1_DATA_LENGTH])
{
    data[0] = STUFF_BITS(bundle->rd.nrx_userMode, 3, 2) |
              STUFF_BITS(1, 5, 1) |  // CI BUS 0: Turn Off 1: Turn On
              STUFF_BITS(bundle->st.nrx_comprStatus, 6, 1);
    data[1] = STUFF_BITS(bundle->rd.nrx_setTemp, 0, 4);
    data[3] = STUFF_BITS(1, 0, 1);                                                        // Cooling type 0:ABSORPTION, 1:Compressor
    uint8_t FreshTemp = dometic_nrx_bridge_convert_temp_tolin(bundle->st.nrx_Fresh_Temp); /* act temp = FreshTemp / 2 - 64 */
    data[4] = STUFF_BITS(FreshTemp, 0, 8);
    // data[5] = //Reserved
    data[6] = STUFF_BITS(bundle->st.nrx_Error, 0, 7) |
              STUFF_BITS(1, 7, 1); /* Error code is different with Dometic Generic CI-BUS protocol, So AIarm Type should be 1 */
    data[7] = STUFF_BITS(bundle->rd.nrx_power, 7, 1);

#ifdef DEBUG_LOG_APPLIN
    LOG(D, "LIN Stuff-D0:%x, D1:%x, D2:%x, D3:%x, D4:%x, D5:%x, D6:%x, D7:%x",
        data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
#endif
}

/**
 *  \brief Convert NRX Bridge fresh temperature from system value to LIN value.
 *
 *
 *  \param i32Value    DDM temperature physical value.
 *  \retval res        Converted value.
 *
 */
static int32_t dometic_nrx_bridge_convert_temp_tolin(int32_t i32Value)
{
    int32_t res = 0;

    if ((i32Value >= -64000) && (i32Value <= 62500))
    {
        res = i32Value / 500 + 128; /* (i32Value / 1000 + 64) * 2 */
    }

    return res;
}

/**end Bridge NRX**/
