/*! \file ame.c
	\brief Accelerometer Mathematical Engine.
In order to perform calculations that include trigonometry, ideally, from the math library should be used functions with single floating point precision (i.e. cosf, arccosf).
However, during testing it is noticed that these functions do not behave as expected, resulting with wrong return value.
After debugging, the cause of this behavior was still not defined.
Therefore, double precision functions are used (i.e. cos, arccos).
Testing and debugging done in:
SEGGER Embedded Studio for ARM V5.66   
Compiler: segger-cc: version 12.4.0

*/
#include "ame.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <float.h>

static neo_matrix                          s__r_static;
static neo_matrix                          s__r_xy;
static float                               s__raw_x;
static float                               s__raw_y;
static float                               s__raw_z;
static float                               s__vector_magnitude;

void ame__init(void) {}

/*! \brief ame__update_values function.
 *
 *  This function is used for updating the raw value for X,Y,Z axis, measured by the sensor.
 *
 *  \param x        Raw value for acceleration on the X axis
 *  \param y        Raw value for acceleration on the Y axis
 *  \param z        Raw value for acceleration on the Z axis
 *  \return         Error code
 */
enum ame__error ame__update_values(float x, float y, float z)
{
    s__raw_x = x;
    s__raw_y = y;
    s__raw_z = z;
    s__vector_magnitude=(float)sqrt(pow(s__raw_x,2)+pow(s__raw_y,2)+pow(s__raw_z,2));
    return AME__ERROR_NONE;
}

/*! \brief ame__update_r_xy function.
 *
 *  This function is used for updating values of the tilt rotation matrix - XY axes alignment. 
 *
 *  \param mat      Matrix with the values
 *  \return         Error code
 */
enum ame__error ame__update_r_xy(neo_matrix *mat)
{
  neo_matrix__init(&s__r_xy, 3, 3);
  neo_matrix temp;
  neo_matrix__init(&temp, 3, 3);
  temp = *mat;
  for (unsigned int i=0;i<mat->row;i++)
  {
    for (unsigned int j=0;j<mat->column;j++)
    {
      float element=neo_matrix__get(&temp,i,j);
      neo_matrix__set(&s__r_xy,i,j,element);
    }
  }
  return AME__ERROR_NONE;
}

/*! \brief ame__update_r_static function.
 *
 *  This function is used for updating values of the flat rotation matrix - Z axis alignment. 
 *
 *  \param mat      Pointer to the matrix with the static rotation coefficients' values
 *  \return         Error code
 */
enum ame__error ame__update_r_static(neo_matrix *mat)
{
  neo_matrix__init(&s__r_static, 3, 3);
  neo_matrix temp;
  neo_matrix__init(&temp, 3, 3);
  temp = *mat;
  for (unsigned int i=0;i<mat->row;i++)
  {
  if (mat->row==1)
  {
  }
    for (unsigned int j=0;j<mat->column;j++)
    {
      float element = neo_matrix__get(&temp,i,j);
      neo_matrix__set(&s__r_static,i,j,element);
    }
  }
  return AME__ERROR_NONE;
}



/*! \brief ame__qdata_to_matrix.
 *
 *  This function calculates the matrix, for a given ame_calibration__data data structure (q0-q1 parameters).
 *
 *  \param data    Pointer to ame_calibration__data structure 
 *  \param mat     Pointer to the matrix that should be created 
 *  \return         Error code
 */
enum ame__error ame__qdata_to_matrix(struct ame_calibration__data *data, neo_matrix *mat)
{
    float q0,q1,q2,q3;
    q0 = data->q0;
    q1 = data->q1;
    q2 = data->q2;
    q3 = data->q3;
    neo_matrix__init(mat, 3, 3);
    neo_matrix__set(mat, 0, 0, 1 - 2 * (pow(q2, 2) + pow(q3, 2)));
    neo_matrix__set(mat, 0, 1, 2 * (q1 * q2 - q0 * q3));
    neo_matrix__set(mat, 0, 2, 2 * (q0 * q2 + q1 * q3));

    neo_matrix__set(mat, 1, 0, 2 * (q1 * q2 + q0 * q3));
    neo_matrix__set(mat, 1, 1, 1 - 2 * (pow(q1, 2) + pow(q3, 2)));
    neo_matrix__set(mat, 1, 2, 2 * (q2 * q3 - q0 * q1));

    neo_matrix__set(mat, 2, 0, 2 * (q1 * q3 - q0 * q2));
    neo_matrix__set(mat, 2, 1, 2 * (q0 * q1 - q2 * q3));
    neo_matrix__set(mat, 2, 2, 1 - 2 * (pow(q1, 2) + pow(q2, 2)));

    return AME__ERROR_NONE;
}


/*! \brief ame__calculate_flat_cal_parameters function.
 *
 *  This function performs the mathematical operations needed in order to calculate the flat calibration data.
 *
 *  Static calibration is process that aligns Z axes of the sensor and the vehicle where is mounted. 
 *  The alignment is achived by quaternions rotation. The coefficients [q0-q1, also named as calibration data] are 
 *  calculated using the raw axis data [x,y,z].
 *  This coefficients are necessary to calculate flat/static rotation matrix. 
 *
 * \param data           Pointer to ame_calibration__data structure, where q-values will be stored
 * \return               Error code
 */
enum ame__error ame__calculate_flat_cal_parameters(struct ame_calibration__data *data)
{

    float A_magnitude = (float)(sqrt(pow(s__raw_x, 2) + pow(s__raw_y, 2)));
    float alpha, A_normx, A_normy, A_normz;
    if (fabs(A_magnitude)  < (FLT_EPSILON + FLT_MIN)) // Only possible if x_raw=y_raw=0 -> z_raw=g=9.81 -> No rotation needed
                          // See note1 in ISE132 document
    {
        alpha   = 0.0f;
        A_normx = 0.0f;
        A_normy = 0.0f;
        A_normz = 0.0f;
    } 
    else 
    {
        A_normx = (float)((s__raw_y) / A_magnitude);
        A_normy = (float)((-s__raw_x) / A_magnitude);
        A_normz = 0.0f;
        alpha   = acos((float)s__raw_z / (float)(sqrt(pow(s__raw_x, 2) + pow(s__raw_y, 2) + pow(s__raw_z, 2))));
    }
    float q0 = cos(alpha / 2);
    float q1 = sin(alpha / 2) * A_normx;
    float q2 = sin(alpha / 2) * A_normy;
    float q3 = sin(alpha / 2) * A_normz;

    data->q0 = q0;
    data->q1 = q1;
    data->q2 = q2;
    data->q3 = q3;
    neo_matrix mat;
    neo_matrix__init(&mat, 3, 3);
    ame__qdata_to_matrix(data,&mat);
    ame__update_r_static(&mat);
    neo_matrix proba,v_raw;
    neo_matrix__init(&proba,3,1);
    neo_matrix__init(&v_raw, 3, 1);
    neo_matrix__set(&v_raw, 0, 0, s__raw_x);
    neo_matrix__set(&v_raw, 1, 0, s__raw_y);
    neo_matrix__set(&v_raw, 2, 0, s__raw_z);
    neo_matrix__multiply(&mat,&v_raw,&proba);
    
    return AME__ERROR_NONE;
}

/*! \brief ame__create_xy_rotation_matrix.
 *
 *  This function is calculating the rotation matrix around Z axis for a given angle theta, in longitudinal or lateral position.
 *
 *  \param mat                          Pointer to the new created matrix.
 *  \param theta                        Rotation angle
 *  \param position_longitudinal        Position in the moment of calibration lateral/longitudinal
 */


void ame__create_xy_rotation_matrix(neo_matrix *mat, float theta,bool position_longitudinal)
{
    if(position_longitudinal)
    {
        neo_matrix__set(mat, 0, 0, cos(theta));
        neo_matrix__set(mat, 0, 1, sin(theta));
        neo_matrix__set(mat, 0, 2, 0);

        neo_matrix__set(mat, 1, 0, -sin(theta));
        neo_matrix__set(mat, 1, 1, cos(theta));
        neo_matrix__set(mat, 1, 2, 0);

        neo_matrix__set(mat, 2, 0, 0);
        neo_matrix__set(mat, 2, 1,0);
        neo_matrix__set(mat, 2, 2, 1);
    }
    else
    {
        neo_matrix__set(mat, 0, 0, cos(theta));
        neo_matrix__set(mat, 0, 1, -sin(theta));
        neo_matrix__set(mat, 0, 2, 0);

        neo_matrix__set(mat, 1, 0, sin(theta));
        neo_matrix__set(mat, 1, 1, cos(theta));
        neo_matrix__set(mat, 1, 2, 0);

        neo_matrix__set(mat, 2, 0, 0);
        neo_matrix__set(mat, 2, 1,0);
        neo_matrix__set(mat, 2, 2, 1);
    }

}


/*! \brief ame__calculate_tilt_cal_parameters function.
 *
 *  This function performs the mathematical operations needed in order to calculate the tilt calibration data.
 *
 *  Tilt calibration is process that aligns XY plane of the sensor and the one of the vehicle where the sensor board is mounted. 
 *  The alignment is achived by rotation around Z axis. The rotation angle is calculated using the raw axis data [x,y,z] 
 *  and static rotation matrix.
 *  This coefficients are necessary to calculate tilt (XY) rotation matrix. 
 *
 *  \param cal_tilt_data          Pointer to ame_calibration__tilt_data structure that will store angle & position
 *  \param position_longitudinal  Flag that describes the position - longitudinal (true) or lateral (false)
 * \return               Error code
 */
enum ame__error ame__calculate_tilt_cal_parameters(struct ame_calibration__tilt_data *cal_tilt_data, bool position_longitudinal)
{
    cal_tilt_data->position=position_longitudinal;
    neo_matrix v_raw,v_ground,v_ideal;
    enum ame__error error;
    enum neo_matrix__error neo_error;
    //Construct v_raw vector
    neo_error = neo_matrix__init(&v_raw, 3, 1);
    if (neo_error) 
    {
        return AME__ERROR_NEOMATRIX;
    }
    neo_matrix__set(&v_raw, 0, 0, s__raw_x);
    neo_matrix__set(&v_raw, 1, 0, s__raw_y);
    neo_matrix__set(&v_raw, 2, 0, s__raw_z);
    
    //Calculate v_ground vector - v_raw + static rotation
    neo_error = neo_matrix__init(&v_ground, 3, 1);
    if (neo_error) 
    {
        return AME__ERROR_NEOMATRIX;
    }
    neo_error = neo_matrix__multiply(&s__r_static, &v_raw, &v_ground);

    if (neo_error)
    {
        return AME__ERROR_NEOMATRIX;
    }

    float theta;
    if(position_longitudinal)
    {
      theta=atan(v_ground.elements[1][0]/v_ground.elements[0][0]);
    }
    else
    {
      theta=atan(v_ground.elements[0][0]/v_ground.elements[1][0]);
    }
    cal_tilt_data->angle=theta;
    neo_matrix mat;
    neo_matrix__init(&mat, 3, 3);
    ame__create_xy_rotation_matrix(&mat,theta,position_longitudinal);
    ame__update_r_xy(&mat);
    //Check
    neo_error = neo_matrix__init(&v_ideal, 3, 1);
    neo_error = neo_matrix__multiply(&s__r_xy, &v_ground, &v_ideal);
    float lan,lon;
    error = ame__get_tilt_angle(&lon,&lan);
    return error;
}


/*! \brief ame__get_tilt_angle.
 *
 *  This function is calculating the longitudinal & lateral tilt angle, by using raw measured values from the sensor and
 *  R_static + R_XY.
 *
 *  \param angle_lon    Float pointer , where the calculated value for longitudinal angle is stored
 *  \param angle_lat    Float pointer , where the calculated value for lateral angle is stored
 *  \return             Error code
 */
enum ame__error ame__get_tilt_angle(float *angle_lon,float *angle_lat)
{
    enum neo_matrix__error error;
    neo_matrix             v_raw;
    neo_matrix             v_ground;
    neo_matrix             v_adjusted;
    float                  angle_value_lat,angle_value_lon;

    error = neo_matrix__init(&v_raw, 3, 1);
    if (error) 
    {
        return AME__ERROR_NEOMATRIX;
    }
    neo_matrix__set(&v_raw, 0, 0, s__raw_x);
    neo_matrix__set(&v_raw, 1, 0, s__raw_y);
    neo_matrix__set(&v_raw, 2, 0, s__raw_z);

    error = neo_matrix__init(&v_ground, 3, 1);
    if (error) 
    {
        return AME__ERROR_NEOMATRIX;
    }
    error = neo_matrix__multiply(&s__r_static, &v_raw, &v_ground);

    if (error)
    {
        return AME__ERROR_NEOMATRIX;
    }
    error = neo_matrix__init(&v_adjusted, 3, 1);
    if (error) 
    {
        return AME__ERROR_NEOMATRIX;
    }
    error = neo_matrix__multiply(&s__r_xy, &v_ground, &v_adjusted);

    if (error)
    {
        return AME__ERROR_NEOMATRIX;
    }
    angle_value_lon = (float)asin((float)(neo_matrix__get(&v_adjusted, 0, 0) / s__vector_magnitude));
    if (isnan(angle_value_lon)) 
    {
        return AME__ERROR_ANGLE;
    }
    else 
    {
        //*angle_lon = angle_value_lon // In Radians
        *angle_lon = angle_value_lon * (180.0 / M_PI); // In degrees
    }
   angle_value_lat = (float)asin((float)(neo_matrix__get(&v_adjusted, 1, 0) / s__vector_magnitude));
    if (isnan(angle_value_lat)) 
    {
        return AME__ERROR_ANGLE;
    }
    else 
    {
        //*angle_lat = angle_value_lat // In Radians
        *angle_lat = angle_value_lat * (180.0 / M_PI); // In degrees
    }
    return AME__ERROR_NONE;

}
