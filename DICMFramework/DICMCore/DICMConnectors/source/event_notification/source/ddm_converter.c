/**
 * \file        ddm_converter.c
 * \date        2024-07-05
 * \author      (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 *
 * \brief       Event Notification connector implementation
 *
 * Implementation of DDM converter.
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

#include "ddm_converter.h"

int ddm_converter_set_and_store(
    const ddm_converter_t * converter_table,
    size_t converter_table_elements,
    void * arg,
    const DDMP2_FRAME * in_frame,
    ddm_entry_t * out_type_ddm)
{
    int retval;

    if (in_frame->frame.control == DDMP2_CONTROL_SET)
    {
        retval = DDM_CONVERTER_ERR_NOT_FOUND;
        for (size_t i = 0u; i < converter_table_elements; i++)
        {
            /* Handle cases when DDM parameters instance is different then zero. */
            uint32_t base_instance = DDM2_PARAMETER_BASE_INSTANCE(in_frame->frame.set.parameter);
            if (base_instance == converter_table[i].parameter_id)
            {
                converter_table[i].fn(arg, in_frame, out_type_ddm);
                retval = DDM_CONVERTER_ERR_NONE;
                break;
            }
        }
    }
    else
    {
        retval = DDM_CONVERTER_ERR_INVALID_FRAME;
    }
    return retval;
}