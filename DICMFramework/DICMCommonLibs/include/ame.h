/*! \file ame.h
 *	\brief Accelerometer Mathematical Engine header file.
 AME - Accelerometer Mathematical Engine is an acronym used in the naming of all modules that support calculation of the tilt angle.

This module performs tilt angle calculations. It is also used to perform static calibration and calculate 
the rotation matrix required by the tilt angle algorithm 
 */
#ifndef AME_H_
#define AME_H_
#include "ame_calibration.h"
#include "neo_matrix.h"
#include <stdbool.h>

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
struct ame_calibration__calibration;

// Data types
enum ame__error
{
    AME__ERROR_NONE = 0,
    AME__ERROR_ANGLE,
    AME__ERROR_NEOMATRIX,
    AME__ERROR_VECTOR_SIZE
};

/** Function prototypes *******************************************************/
void            ame__init(void);
enum ame__error ame__update_values(float x, float y, float z);
enum ame__error ame__update_r_static(neo_matrix *mat);
enum ame__error ame__update_r_xy(neo_matrix *mat);
enum ame__error ame__calculate_flat_cal_parameters(struct ame_calibration__data *data);
enum ame__error ame__calculate_tilt_cal_parameters(struct ame_calibration__tilt_data *cal_tilt_data, bool position_longitudinal);
enum ame__error ame__qdata_to_matrix(struct ame_calibration__data *data, neo_matrix *mat);
void ame__create_xy_rotation_matrix(neo_matrix *mat, float theta,bool position_longitudinal);
enum ame__error ame__get_tilt_angle(float *angle_lon,float *angle_lat);

#endif /* AME_H_ */
