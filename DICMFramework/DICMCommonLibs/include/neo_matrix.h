/*! \file neo_matrix.h
	\brief Neo matrix - mathematical calculations module - header file.
Neo matrix module provides essential functions for calculations where matrices take place. It contains functions for initialization of a matrix, 
set and get functions (write/read elements' value), and it provides function for matrices multiplication.

*/
#ifndef NEO_MATRIX_H_
#define NEO_MATRIX_H_

#include <stdint.h>

#define NEO_MATRIX__MAX_DIMENSION 9

enum neo_matrix__error
{
    NEO_MATRIX__ERROR_NONE = 0,
    NEO_MATRIX__ERROR_DIMENSION
};

/* Basic matrix structure , rows and columns to specify the dimension and
matrix with the elements:
 [a0 ,a1, a2
  a3, a4, a5
  a6, a7 a8]
*/
typedef struct neo_matrix
{
    uint32_t row;
    uint32_t column;
    float    elements[NEO_MATRIX__MAX_DIMENSION][NEO_MATRIX__MAX_DIMENSION];
} neo_matrix;

/** Function prototypes *******************************************************/
enum neo_matrix__error neo_matrix__init(neo_matrix *matrix, uint32_t rows, uint32_t columns);
enum neo_matrix__error neo_matrix__multiply(const neo_matrix *left, const neo_matrix *right, neo_matrix *product);
float                  neo_matrix__get(const neo_matrix *matrix, uint32_t row, uint32_t column);
void                   neo_matrix__set(neo_matrix *matrix, uint32_t row, uint32_t column, float value);
enum neo_matrix__error neo_matrix__calculate_cross_product(neo_matrix *v_1, neo_matrix *v_2, neo_matrix *crossproduct);
enum neo_matrix__error neo_matrix__calculate_dot_product(neo_matrix *v_1, neo_matrix *v_2, float *dot_product);

#endif /* NEO_MATRIX_H_ */
