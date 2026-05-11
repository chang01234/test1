/*! \file extflash.h
 * \brief Macros for placing data in external flash. This file is machine generated - do not edit!
 */

#ifndef EXTFLASH_H_
#define EXTFLASH_H_

/** Defines *******************************************************************/

#define EXTFLASH_HEADER	__attribute__((section(".extflashhdr")))	//!< Object will be placed at the beginning of external flash memory. Only to be used with one object.
#define EXTFLASH		__attribute__((section(".extflash")))		//!< Object will be placed somewhere in the external flash memory. Use for all other flash objects.


#endif /* EXTFLASH_H_ */
