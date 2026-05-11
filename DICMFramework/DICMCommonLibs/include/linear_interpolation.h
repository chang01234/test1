
/*! \file linear_interpolation.h
	\brief This header file contains common functions providing conversion like functionalities.
 */

#ifndef LINEAR_INTERPOLATION_H_

#include <stdint.h>

typedef int32_t linear_intrp_datatype;

/**
  * @brief  Calculate the rpm to derive the motor and fan using linear interpolation.
  * @param  x1.
  * @param  x2.
  * @param  y1.
  * @param  y2.
  * @param  x Coordinate.
  * @retval y cordinate
  */
linear_intrp_datatype calc_linear_interpolation(linear_intrp_datatype x1, linear_intrp_datatype x2, linear_intrp_datatype y1, linear_intrp_datatype y2, uint32_t x);

#endif /*LINEAR_INTERPOLATION_H_*/

