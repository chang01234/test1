/*! \file   linear_interpolation.c
	\brief  This file implements the common conversion function needed by other app modules
*/
#include "linear_interpolation.h"

/**
  * @brief  Calculate the rpm to derive the motor and fan using linear interpolation.
  * @param  x1.
  * @param  x2.
  * @param  y1.
  * @param  y2.
  * @param  x Co-ordinate.
  * @retval y Co-ordinate
  */
linear_intrp_datatype calc_linear_interpolation(linear_intrp_datatype x1, linear_intrp_datatype x2, linear_intrp_datatype y1, linear_intrp_datatype y2, uint32_t x)
{

	/*
   		Auto mode min rpm -700 max rpm - 2800
   		IAQ range min = 0  max = 500
     	x1 = 0
	 	x2 = 500
	 	y1 = 700
	 	y2 = 2800
	 	x = 220

      	y = ((( x - x1) * (y2 -y1)) / (x2 -x1)) + y1;
	  	y = ((220 - 0) * (2800 - 700) / (500 - 0) ) + 700;
	    y = (220 * 2100 / 500) + 700;

   */
    linear_intrp_datatype numerator;
    linear_intrp_datatype denominator;
    linear_intrp_datatype y = 0u;

    numerator = ((x - x1) * (y2 - y1));
    
    denominator = x2 - x1;

    if ( denominator > 0 )
    { 
        y = (numerator / denominator) + y1;
    }

	return y;
}
