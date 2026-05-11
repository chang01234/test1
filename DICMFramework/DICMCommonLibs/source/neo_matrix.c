/*! \file neo_matrix.c
	\brief Mathematical calculations module.
*/
#include "neo_matrix.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/*! \brief init_mat function.
 *
 *  This function is used to allocate memory and create a matrix with given values for rows and
 * columns.
 *
 *  \param matrix        Matrix that should be initialized
 *  \param rows          Pointer to data for parameter value.
 *  \param columns       Data_size config parameter to set.
 *  \return              Error code
 */
enum neo_matrix__error neo_matrix__init(neo_matrix *matrix, uint32_t rows, uint32_t columns)
{
    if (rows > NEO_MATRIX__MAX_DIMENSION) 
    {
        return NEO_MATRIX__ERROR_DIMENSION;
    }
    if (columns > NEO_MATRIX__MAX_DIMENSION) 
    {
        return NEO_MATRIX__ERROR_DIMENSION;
    }
    matrix->row    = rows;
    matrix->column = columns;
    for (uint32_t i = 0; i < matrix->row; i++) 
    {
        for (uint32_t j = 0; j < matrix->column; j++) 
        {
            matrix->elements[i][j] = 0;
        }
    }
    return NEO_MATRIX__ERROR_NONE;
}

/*! \brief get function.
 *
 *  This function is used to get the value of the particular element on a given position in some
 * matrix
 *
 *  \param matrix           Matrix
 *  \param row              Row position of the element of interest
 *  \param column           Column position of the element of interest
 *  \return                 Element value
 */
float neo_matrix__get(const neo_matrix *matrix, uint32_t row, uint32_t column) { return matrix->elements[row][column]; }

/*! \brief set function.
 *
 *  This function is used to set the value of the particular element on a given position in some
 * matrix
 *
 * \param matrix          Matrix
 * \param row             Row position of the element of interest
 * \param column          Column position of the element of interest
 * \param value           Desired value
 */
void neo_matrix__set(neo_matrix *matrix, uint32_t row, uint32_t column, float value)
{
    matrix->elements[row][column] = value;
}

/*! \brief multiply function.
 *
 *  This function is used to multiply 2 matrices , if possible.
 *
 *  \param left        Left Matrix
 *  \param right       Right Matrix
 *  \param product     Product Matrix
 *  \return            Error Code
 */
enum neo_matrix__error neo_matrix__multiply(const neo_matrix *left, const neo_matrix *right, neo_matrix *product)
{
    enum neo_matrix__error error = NEO_MATRIX__ERROR_NONE;
    uint32_t               r1    = left->row;
    uint32_t               r2    = right->row;
    uint32_t               c1    = left->column;
    uint32_t               c2    = right->column;
    if (c1 == r2) 
    {
        neo_matrix__init(product, r1, c2);
        for (uint32_t i = 0; i < r1; i++) 
        {
            for (uint32_t j = 0; j < c2; j++) 
            {
                float de = 0.0;
                for (uint32_t k = 0; k < r2; k++) 
                {
                    de += neo_matrix__get(left, i, k) * neo_matrix__get(right, k, j);
                }
                neo_matrix__set(product, i, j, de);
            }
        }
    } 
    else 
    {
        error = NEO_MATRIX__ERROR_DIMENSION; // multiplication not available, size of the matrices
                                             // are not fitting
    }
    return error;
}

/*! \brief neo_matrix__calculate_cross_product.
 *
 *  This function is used to calculate cross product between 2 vectors.
 *
 *  \param v_1              Vector 1
 *  \param v_2              Vector 2
 *  \param crossproduc      Vector_1 x Vector_2
 *  \return                 Error Code
 */
enum neo_matrix__error neo_matrix__calculate_cross_product(neo_matrix *v_1, neo_matrix *v_2, neo_matrix *crossproduct)
{
  // Checking if the dimesnion of the input vectors is correct
  crossproduct->elements[0][0]=0.0f;
  crossproduct->elements[1][0]=0.0f;
  crossproduct->elements[2][0]=0.0f;
  if ((v_1->row!=3 || v_1->column!=1) || (v_2->row!=3 || v_2->column!=1))
  {
      return NEO_MATRIX__ERROR_DIMENSION;
  }
  crossproduct->elements[0][0]=v_1->elements[1][0]*v_2->elements[2][0]-v_2->elements[1][0]*v_1->elements[2][0];
  crossproduct->elements[1][0]=v_1->elements[2][0]*v_2->elements[0][0]-v_2->elements[2][0]*v_1->elements[0][0];
  crossproduct->elements[2][0]=v_1->elements[0][0]*v_2->elements[1][0]-v_2->elements[0][0]*v_1->elements[1][0];
  return NEO_MATRIX__ERROR_NONE;
}

/*! \brief neo_matrix__calculate_dot_product.
 *
 *  This function is used to calculate dot product between 2 vectors.
 *
 *  \param v_1              Vector 1
 *  \param v_2              Vector 2
 *  \param dot_product      Vector_1 * Vector_2
 *  \return                 Error Code
 */
enum neo_matrix__error neo_matrix__calculate_dot_product(neo_matrix *v_1, neo_matrix *v_2, float *dot_product)
{
  // Checking if the dimesnion of the input vectors is correct
  if ((v_1->row!=3 || v_1->column!=1) || (v_2->row!=3 || v_2->column!=1))
  {
      return NEO_MATRIX__ERROR_DIMENSION;
  }
  *dot_product=(v_1->elements[0][0]*v_2->elements[0][0]+v_1->elements[1][0]*v_2->elements[1][0]+v_1->elements[2][0]*v_2->elements[2][0]);
  return NEO_MATRIX__ERROR_NONE;
}