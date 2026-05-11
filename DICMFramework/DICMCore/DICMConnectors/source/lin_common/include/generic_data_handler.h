/*! \file generic_data_handler.h
*
 * \brief Generic data handler
 *
 * This file can be used to define generic defines with data
 * which can be used in multiple files, to avoid redundant definitions
 * 
 */


#ifndef GENERIC_DATA_HANDLER_H_
#define GENERIC_DATA_HANDLER_H_

/**
 * Bit define
*/
#define GDH_BIT_0 0x00
#define GDH_BIT_1 0x01
#define GDH_BIT_2 0x02
#define GDH_BIT_3 0x03
#define GDH_BIT_4 0x04
#define GDH_BIT_5 0x05
#define GDH_BIT_6 0x06
#define GDH_BIT_7 0x07

/**
 * Byte define
*/
#define GDH_BYTE_0 0x00
#define GDH_BYTE_1 0x01
#define GDH_BYTE_2 0x02
#define GDH_BYTE_3 0x03
#define GDH_BYTE_4 0x04
#define GDH_BYTE_5 0x05
#define GDH_BYTE_6 0x06
#define GDH_BYTE_7 0x07

/**
 * Pointerr types define
*/
#define GDH_POINTER_TYPE_U8   1
#define GDH_POINTER_TYPE_U16  2
#define GDH_POINTER_TYPE_U32  3
#define GDH_POINTER_TYPE_INT  4
#define GDH_POINTER_TYPE_OTH  5

/**
 * Saturaion functions
*/

/**
 * Data size handling
*/
#define GDH_DATA_LEN(a) (sizeof(a) / sizeof(a[0]))

/**
 * Bit operation handling
*/
#define GDH_SET_BIT(data, bitno, set)  do { (data) &= ~(0x01u << (bitno)); (data) |= ((set) << (bitno)); } while (0u)
#define GDH_GET_BIT(data, bitno)       (((data) >> (bitno)) & 0x01u)

#endif /* GENERIC_DATA_HANDLER_H_ */
