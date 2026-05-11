/*! \file dicm.h

	\brief DICM BLE definitions.
*/

#ifndef DICM_H_
#define DICM_H_

//! \~ DICM service characteristic UUIDs
typedef enum _SERVICE_UUID_ENUM
{
	UUID_SERVICE		=0x0400,	//!< \~ ICM service UUID
	UUID_INPUT,
	UUID_OUTPUT,
} SERVICE_UUID_ENUM;

//! \~ Create UUID derived from base UUID 537axxxx-0995-481f-926c-1604e23fd515
#define UUID(x) {0x15,0xd5,0x3f,0xe2,0x04,0x16,0x6c,0x92,0x1f,0x48,0x95,0x09,((unsigned)(x)&0xff),((unsigned)(x)>>8),0x7a,0x53}

#endif /* DICM_H_ */
