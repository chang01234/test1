/*! \file ame_calibration.h
 *	\brief Accelerometer Mathematical Engine Calibration module - header file.
 AME - Accelerometer Mathematical Engine is an acronym used in the naming of all modules that support calculation of the tilt angle.

This module defines the main data structure of the AME modules - calibration data structure.
 */
#ifndef AME_CALIBRATION_H_
#define AME_CALIBRATION_H_
#include "neo_matrix.h"

#include <math.h>
#include <stdio.h>
#include <stdbool.h>
// Data structures

enum ame_calibration__error
{
    AME_CALIBRATION__ERROR_NONE = 0,
};

struct ame_calibration__calibration
{
    /* R Static coeffs */
    struct ame_calibration__data
    {
        float q0;
        float q1;
        float q2;
        float q3;
    } data;
    uint16_t checksum;
};
struct ame_calibration__tilt_calibration
{
    struct  ame_calibration__tilt_data
    {
        float angle;
        bool position;
    } tilt_data;
    uint16_t checksum;
};
void ame_calibration__init(void);
#endif /* AME_CALIBRATION_H_ */
