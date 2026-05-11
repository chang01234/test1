/*! \file ame_calibration_xfer.c
	\brief AME import/export data functionalities module.
*/
#include "ame_calibration_xfer.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/*! \brief ame_calibration_xfer__float2bytes function.
 *
 * This function converts floating point variable to an uint32_t variable.
 *
 * \param f         Float value that should be converted
 * \return          uint32_t variable , consisting of the 4 bytes that form the float value
 */
static uint32_t ame_calibration_xfer__float2bytes(float f)
{
    union u
    {
        float         f_value;
        unsigned char s[4];
    };
    union u  float2byte;
    uint32_t byte_array;
    float2byte.f_value = f;
    byte_array         = (((uint32_t)(float2byte.s[3] << 24)) | ((uint32_t)(float2byte.s[2] << 16)) |
                  ((uint32_t)(float2byte.s[1] << 8)) | ((uint32_t)(float2byte.s[0])));

    return byte_array;
}

/*! \brief ame_calibration_xfer__calculate_checksum function.
 *
 * This function calculates checksum value for the 4 calibration parameters (q0-q3) , consisted in
 * the ame_calibration__data structure. The checksum is calculated by using sum-complament
 * algorithm.
 *
 * \param data      Pointer to the data structure that holds the q0-q3 values
 * \return          uint16_t variable , 2-bytes long checksum
 */
uint16_t ame_calibration_xfer__calculate_checksum(const struct ame_calibration__data *data)
{
    uint16_t sum = 0, checksum = 0;
    uint32_t array[4];
    array[0] = ame_calibration_xfer__float2bytes(data->q0);
    array[1] = ame_calibration_xfer__float2bytes(data->q1);
    array[2] = ame_calibration_xfer__float2bytes(data->q2);
    array[3] = ame_calibration_xfer__float2bytes(data->q3);
    for (int j = 0; j < 4; j++) 
    {
        for (int i = 0; i < 25; i = i + 8) 
        {
            sum += (array[j] >> i) & (0xff);
        }
    }
    checksum = ~sum;
    return checksum;
}
/*! \brief ame_calibration_xfer__calculate_checksum_tilt function.
 *
 * This function calculates checksum value for the 2 tilt calibration parameters, consisted in
 * the ame_calibration__tilt_data structure. The checksum is calculated by using sum-complament
 * algorithm.
 *
 * \param tilt_data      Pointer to the tilt calibration data structure
 * \return          uint16_t variable , 2-bytes long checksum
 */
uint16_t ame_calibration_xfer__calculate_checksum_tilt(const struct ame_calibration__tilt_data *tilt_data)
{
    uint16_t sum = 0, checksum = 0;
    sum+=ame_calibration_xfer__float2bytes(tilt_data->angle);
    sum+=ame_calibration_xfer__float2bytes(tilt_data->position);
    checksum = ~sum;
    return checksum;
}

/*! \brief ame_calibration_xfer__validate_checksum function.
 *
 * This function is used for checksum validation. It checks if the given checksum value corespondes
 * wih the ame_calibration__data structure values.
 *
 * \param data      Pointer to the data structure that holds the q0-q3 values
 * \param checksum  Checksum value, 2-bytes long
 * \return          Error Code
 */
enum ame_calibration_xfer__error ame_calibration_xfer__validate_checksum(const struct ame_calibration__data *data,
                                                                         uint16_t                            checksum)
{
    enum ame_calibration_xfer__error error;
    if (ame_calibration_xfer__calculate_checksum(data) == checksum) 
    {
        error = AME_CALIBRATION_XFER__ERROR_NONE;
    } 
    else 
    {
        error = AME_CALIBRATION_XFER__ERROR_CHECK;
    }
    return error;
}

/*! \brief ame_calibration_xfer__validate_checksum_tilt function.
 *
 * This function is used for checksum validation. It checks if the given checksum value corespondes
 * wih the ame_calibration__tilt_data structure values.
 *
 * \param tilt_data      Pointer to the data structure that holds the tilt calibration parameters
 * \param checksum       Checksum value, 2-bytes long
 * \return               Error Code
 */
enum ame_calibration_xfer__error ame_calibration_xfer__validate_checksum_tilt(const struct ame_calibration__tilt_data *tilt_data , uint16_t checksum)
{
    enum ame_calibration_xfer__error error;
    if (ame_calibration_xfer__calculate_checksum_tilt(tilt_data) == checksum) 
    {
        error = AME_CALIBRATION_XFER__ERROR_NONE;
    } 
    else 
    {
        error = AME_CALIBRATION_XFER__ERROR_CHECK;
    }
    return error;
}

/*! \brief ame_calibration_xfer__import function.
 *
 *  This function is used to import calibration parameters values and their checksum, from an
 * 18-bytes long array to the ame_calibration__calibration structure. The array is in the following
 * order:
 * ( 17 -> 0 BYTE) : Q3 (B17-B14) - Q2 (B13-B10) - Q1 (B9-B6) - Q0 (B5-B2) - CHS (B1-B0)
 *
 *  \param calibration        Pointer to the ame_calibration__calibration data structure
 *  \param array              Pointer to the array of 18 bytes, that holds the calibration
 * parameters values
 * \return                   Error code
 */
enum ame_calibration_xfer__error ame_calibration_xfer__import(struct ame_calibration__calibration *calibration,
                                                              uint8_t *array)
{
    uint32_t temp = 0u;
    calibration->checksum = (((*(array + 1)) << 8) | (*array));
    temp = ((*(array + 5) << 24) | (*(array + 4) << 16) | (*(array + 3) << 8) | (*(array + 2)));
    calibration->data.q0  = *((float *)&temp);
    temp = ((*(array + 9) << 24) | (*(array + 8) << 16) | (*(array + 7) << 8) | (*(array + 6)));
    calibration->data.q1  = *((float *)&temp);
    temp = ((*(array + 13) << 24) | (*(array + 12) << 16) | (*(array + 11) << 8) | (*(array + 10)));
    calibration->data.q2  = *((float *)&temp);
    temp = ((*(array + 17) << 24) | (*(array + 16) << 16) | (*(array + 15) << 8) | (*(array + 14)));
    calibration->data.q3  = *((float *)&temp);
    if (ame_calibration_xfer__validate_checksum(&(calibration->data), calibration->checksum) ==
        AME_CALIBRATION_XFER__ERROR_NONE) 
        {
        return AME_CALIBRATION_XFER__ERROR_NONE;
        } 
    else 
    {
        return AME_CALIBRATION_XFER__ERROR_IMPORT;
    }
}

/*! \brief ame_calibration_xfer__export function.
 *
 *  This function is used to export calibration parameters values and their checksum in a form of an
 * 18-bytes long array. The array is in the following order: ( 17 -> 0 BYTE) : Q3 (B17-B14) - Q2
 * (B13-B10) - Q1 (B9-B6) - Q0 (B5-B2) - CHS (B1-B0)
 *
 *  \param calibration        Pointer to the ame_calibration__calibration data structure
 *  \param array              Pointer to the array of 18 bytes
 * \return                   Error code
 */
enum ame_calibration_xfer__error ame_calibration_xfer__export(struct ame_calibration__calibration *calibration,
                                                              uint8_t *                            array)
{
    calibration->checksum = ame_calibration_xfer__calculate_checksum(&calibration->data);
    // 4*float values - 16 bytes + 2 bytes checksum = 18 bytes
    // Checksum bytes
    *array       = ((uint8_t)(calibration->checksum & 0xff));
    *(array + 1) = (uint8_t)(calibration->checksum >> 8 & 0xff);
    // q0
    uint32_t temp;
    temp = ame_calibration_xfer__float2bytes(calibration->data.q0);
    for (int i = 0; i < 25; i = i + 8) 
    {
        *(array + i / 8 + 2) = (uint8_t)(temp >> i & 0xff);
    }
    // q1
    temp = ame_calibration_xfer__float2bytes(calibration->data.q1);
    for (int i = 0; i < 25; i = i + 8) 
    {
        *(array + i / 8 + 6) = (uint8_t)(temp >> i & 0xff);
    }
    temp = ame_calibration_xfer__float2bytes(calibration->data.q2);
    for (int i = 0; i < 25; i = i + 8) 
    {
        *(array + i / 8 + 10) = (uint8_t)(temp >> i & 0xff);
    }
    // q3

    temp = ame_calibration_xfer__float2bytes(calibration->data.q3);
    for (int i = 0; i < 25; i = i + 8) 
    {
        *(array + i / 8 + 14) = (uint8_t)(temp >> i & 0xff);
    }
    return AME_CALIBRATION_XFER__ERROR_NONE;
}

/*! \brief ame_calibration_xfer__export_tilt function.
 *
 *  This function is used to export XY rotation matrix to an bytes array.
 *
 *  \param cal_tilt_data      Tilt calibration data structure
 *  \param array              Bytes array
 * \return                    Error code
 */
enum ame_calibration_xfer__error ame_calibration_xfer__export_tilt(struct ame_calibration__tilt_calibration *cal_tilt_data,
                                                              uint8_t *                            array)
{
    cal_tilt_data->checksum=ame_calibration_xfer__calculate_checksum_tilt(&cal_tilt_data->tilt_data);
    *array       = ((uint8_t)(cal_tilt_data->checksum & 0xff));
    *(array + 1) = (uint8_t)(cal_tilt_data->checksum >> 8 & 0xff);
    uint32_t temp=ame_calibration_xfer__float2bytes(cal_tilt_data->tilt_data.angle);
    for (int i = 0; i < 25; i = i + 8) 
    {
        *(array + i / 8 + 2) = (uint8_t)(temp >> i & 0xff);
    }
    *(array+2+sizeof(float))=cal_tilt_data->tilt_data.position;
    return AME_CALIBRATION_XFER__ERROR_NONE;
}

/*! \brief ame_calibration_xfer__import_tilt function.
 *
 *  This function is used to convert array of bytes to tilt calibration data structure
 *
 *  \param cal_tilt_data      Tilt calibration data structure
 *  \param array              Array of bytes
 * \return                    Error code
 */
enum ame_calibration_xfer__error ame_calibration_xfer__import_tilt(struct ame_calibration__tilt_calibration *cal_tilt_data,
                                                              uint8_t *array)
{
    uint32_t temp = 0;
    cal_tilt_data->checksum = (((*(array + 1)) << 8) | (*array));
    temp = (uint32_t)((*(array + 5) << 24) | (*(array + 4) << 16) | (*(array + 3) << 8) | (*(array + 2)));
    cal_tilt_data->tilt_data.angle  = *((float *)&temp);
    temp = (uint32_t)((*(array + 9) << 24) | (*(array + 8) << 16) | (*(array + 7) << 8) | (*(array + 6)));
    cal_tilt_data->tilt_data.position  = *(array + 6);
    if (ame_calibration_xfer__validate_checksum_tilt(&cal_tilt_data->tilt_data,cal_tilt_data->checksum) == AME_CALIBRATION_XFER__ERROR_NONE) 
        {
        return AME_CALIBRATION_XFER__ERROR_NONE;
        } 
    else 
    {
        return AME_CALIBRATION_XFER__ERROR_IMPORT;
    }
}
