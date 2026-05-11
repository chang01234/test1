/*
 * frame_util.h
 *
 *  Created on: 17 Feb 2020
 *      Author: Stefan.Henningsohn
 */

#ifndef FRAME_UTIL_H_
#define FRAME_UTIL_H_

#include <stdint.h>

// Extraction macros
#define DGN_TO_BITS(dest,src,start,size)  (dest) = (uint8_t)(((uint8_t)(src) >> (start)) & ((uint8_t)0xffu >> (8 - (size))))
#define DGN_TO_WORD(dest,src)             dest=(((uint16_t)(*(&(src)+1)) << 8) + (uint16_t)src)
#define DGN_TO_DWRD(dest,src)             dest=(((uint32_t)(*(&(src)+3)) << 24) + ((uint32_t)(*(&(src)+2)) << 16) + ((uint32_t)(*(&(src)+1)) << 8 ) + (uint32_t)src)

// Stuffing macros (MAKE SURE TO CLEAR THE BUFFER BEFORE STUFFING)
#define BITS_TO_DGN(dest,src,start,size)  dest|=(uint8_t)(((uint8_t)src << start) & (((uint8_t)0xFF >> (8 - size)) << start));
#define WORD_TO_DGN(dest,src)             {dest=((uint8_t)src);(*(&(dest)+1))=((uint8_t)((uint16_t)src >> 8));}
#define DWRD_TO_DGN(dest,src)             {dest=((uint8_t)src);(*(&(dest)+1))=((uint8_t)((uint16_t)src >> 8));(*(&(dest)+2))=((uint8_t)((uint32_t)src >> 16));(*(&(dest)+3))=((uint8_t)((uint32_t)src >> 24));}

#endif //FRAME_UTIL_H_

