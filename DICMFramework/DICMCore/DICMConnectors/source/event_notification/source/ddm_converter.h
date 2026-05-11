/**
 * \file        ddm_converter.h
 * \date        2024-07-05
 * \author      (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 *
 * \brief       DDM converter interface.
 *
 * \li          2024-07-05  (NR) Initial implementation
 *
 * \copyright   Dometic Group
 *              This source file and the information contained in it are
 *              confidential and proprietary to Dometic Group
 *              The reproduction or disclosure, in whole or in part,
 *              to anyone outside of Dometic Group without the written
 *              approval of a Dometic Group officer under a Non-Disclosure
 *              Agreement is expressly prohibited.
 *
 *              All rights reserved
 */

#ifndef DDM_CONVERTER_H_
#define DDM_CONVERTER_H_

#include <stdbool.h>
#include <stddef.h>
#include "ddm2.h"
#include "ddm_entry.h"

#define DDM_CONVERTER_ERR_NONE  0
#define DDM_CONVERTER_ERR_NOT_FOUND 1
#define DDM_CONVERTER_ERR_INVALID_FRAME 2

/**
 * @brief       Conversion function for converting DDM frame into a proper value
 *              in DDM store/DDM entry structures.
 */
typedef void (ddm_converter_fn)(void * arg, const DDMP2_FRAME * in_frame, ddm_entry_t * out_type_ddm);

/**
 * @brief       This is a conversion structure that describes which DDM
 *              parameter is converted by which function.
 */
typedef struct ddm_converter
{
    uint32_t parameter_id;              //!< Parameter ID which is to be converted
    ddm_converter_fn * fn;              //!< Converter function
} ddm_converter_t;

/**
 * @brief       Find a converter function and do the conversion
 *
 * @param       converter_table is a pointer to an array of @ref ddm_converter_t
 *              structures which are describing the function to DDM parameter
 *              relation. This converter table should live in ROM address space.
 * @param       converter_table_elements is a unsigned integer representing how
 *              many elements are in @a convert_table array.
 * @param       arg is a void pointer that is just passed to converter functions
 *              so the callbacks can receive additional arguments.
 * @param       in_frame is a pointer to input @ref DDMP2_FRAME that needs to be
 *              converted into a proper @ref ddm_entry_t instance.
 *
 * @return      Operation status.
 * @retval      DDM_CONVERTER_ERR_NONDE is returned when a conversion function
 *              was found and input frame was converted to output ddm_entry_t.
 * @retval      DDM_CONVERTER_ERR_NOT_FOUND is returned when no conversion
 *              function is found.
 * @retval      DDM_CONVERTER_ERR_INVALID_FRAME is returned when input frame is
 *              not a SET frame.
 */
int ddm_converter_set_and_store(
    const ddm_converter_t * converter_table,
    size_t converter_table_elements,
    void * arg,
    const DDMP2_FRAME * in_frame,
    ddm_entry_t * out_type_ddm);

#endif /* DDM_CONVERTER_H_ */