/*! \file ame_calibration_xfer.h
	\brief ame_calibration_xfer module - header file.
AME - Accelerometer Mathematical Engine is an acronym used in the naming of all modules that support calculation of the tilt angle.

This  module provides data import/export functions. It converts calibration data [q coefficients structure] to an array of bytes, and vice versa,
it generates calibration data from an imported array of bytes. It also has a functions for generating and validating checksum value.

*/
#ifndef AME_CALIBRATION_XFER_H_
#define AME_CALIBRATION_XFER_H_
#include "ame_calibration.h"
#include "neo_matrix.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
enum ame_calibration_xfer__error
{
    AME_CALIBRATION_XFER__ERROR_NONE = 0,
    AME_CALIBRATION_XFER__ERROR_IMPORT,
    AME_CALIBRATION_XFER__ERROR_EXPORT,
    AME_CALIBRATION_XFER__ERROR_CHECK,
    AME_CALIBRATION_XFER__ERROR_SIZE

};

enum ame_calibration_xfer__error ame_calibration_xfer__validate_checksum(const struct ame_calibration__data *data,
                                                                         uint16_t                            checksum);
uint16_t                         ame_calibration_xfer__calculate_checksum(const struct ame_calibration__data *data);
enum ame_calibration_xfer__error ame_calibration_xfer__validate_checksum_tilt(const struct ame_calibration__tilt_data *tilt_data , 
                                                                                                     uint16_t checksum);
uint16_t ame_calibration_xfer__calculate_checksum_tilt(const struct ame_calibration__tilt_data *tilt_data);

enum ame_calibration_xfer__error ame_calibration_xfer__export(struct ame_calibration__calibration *calibration,
                                                              uint8_t *                            array);
enum ame_calibration_xfer__error ame_calibration_xfer__import(struct ame_calibration__calibration *calibration,
                                                              uint8_t *                            array);
enum ame_calibration_xfer__error ame_calibration_xfer__export_tilt(struct ame_calibration__tilt_calibration *cal_tilt_data,
                                                              uint8_t *                            array);
enum ame_calibration_xfer__error ame_calibration_xfer__import_tilt(struct ame_calibration__tilt_calibration *cal_tilt_data,
                                                              uint8_t *                            array);

#endif /* AME_CALIBRATION_XFER_H_ */
