//------------------------------------------------------------------------------
// Module:      PImage.c
//------------------------------------------------------------------------------
// Description: Process Image

//------------------------------------------------------------------------------
// Copyright:   SeaStar Solutions Inc.
//              3831, No 6 Road
//              Richmond, BC
//              Canada V6V 1P6
//
//              This source file and the information contained in it are 
//              confidential and proprietary to SeaStar Solutions Inc. 
//              The reproduction or disclosure, in whole or in part, 
//              to anyone outside of SeaStar Solutions without the written
//              approval of a SeaStar Solutions officer under a Non-Disclosure 
//              Agreement is expressly prohibited.
//
//              All rights reserved
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include <string.h>
#include "PImage.h"
#include "RVCDGN.h"

#ifdef CONNECTOR_PREM_RVC

//------------------------------------------------------------------------------
// Private Definitions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Private Types
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Private Variables
//------------------------------------------------------------------------------
static int32 pimage_Variables[VAR_ENUM_COUNT]; 
typedef struct _PIMAGE_ENTRY
{
	char data[RVCDGN_DGN_65259_FIELD_SIZE];
	uint8_t size;
} PIMAGE_ENTRY;

static PIMAGE_ENTRY pimage_strings[STRINGS_ENUM_COUNT];
//------------------------------------------------------------------------------
// Public Variables
//------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Function:    PIMAGE_Initialize
//-----------------------------------------------------------------------------
// Description: Initializes the process image.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void PIMAGE_Initialize( void )
{
	(void)memset(&pimage_Variables, 0, sizeof(pimage_Variables));
}

//-----------------------------------------------------------------------------
// Function:    PIMAGE_GetValue
//-----------------------------------------------------------------------------
// Description: Get process image value
//-----------------------------------------------------------------------------
// Return:      Value as 32 bits integer
//-----------------------------------------------------------------------------
int32 PIMAGE_GetValue
(
	PIMAGE_eTABLE eIndex  // in : Variable index
)
{
	int32 Result;
	if (eIndex<VAR_ENUM_COUNT)
	{
		Result = pimage_Variables[eIndex];
	}
	else
	{
		Result = 0;
	}
	return Result;
}

//-----------------------------------------------------------------------------
// Function:    PIMAGE_s8GetValue
//-----------------------------------------------------------------------------
// Description: Get process image value.
//-----------------------------------------------------------------------------
// Return:      Value as signed 8 bits.
//-----------------------------------------------------------------------------
int8 PIMAGE_s8GetValue
(
	PIMAGE_eTABLE eIndex  // in : Variable index
)
{
	uint16 Result;
	int32  Value;
	if (eIndex<VAR_ENUM_COUNT)
	{
		Value = pimage_Variables[eIndex];
		if (Value<INT8_MIN)
		{
			Result = INT8_MIN;
		}
		else if (Value>INT8_MAX)
		{
			Result = INT8_MAX;
		}
		else
		{
			Result = (uint8)Value;
		}
	}
	else
	{
		Result = 0;
	}
	return Result;
}

//-----------------------------------------------------------------------------
// Function:    PIMAGE_s16GetValue
//-----------------------------------------------------------------------------
// Description: Get process image value.
//-----------------------------------------------------------------------------
// Return:      Value as 16 bits integer.
//-----------------------------------------------------------------------------
int16 PIMAGE_s16GetValue
(
	PIMAGE_eTABLE eIndex  // in : Variable index
)
{
	uint16 Result;
	int32  Value;
	if (eIndex<VAR_ENUM_COUNT)
	{
		Value = pimage_Variables[eIndex];
		if (Value<INT16_MIN)
		{
			Result = INT16_MIN;
		}
		else if (Value>INT16_MAX)
		{
			Result = INT16_MAX;
		}
		else
		{
			Result = (int16)Value;
		}
	}
	else
	{
		Result = 0;
	}
	return Result;
}

//-----------------------------------------------------------------------------
// Function:    PIMAGE_u8GetValue
//-----------------------------------------------------------------------------
// Description: Get process image value.
//-----------------------------------------------------------------------------
// Return:      Value as unsigned 8 bits integer.
//-----------------------------------------------------------------------------
uint8 PIMAGE_u8GetValue
(
	PIMAGE_eTABLE eIndex  // in : Variable index
)
{
	uint16 Result;
	int32  Value;
	if (eIndex<VAR_ENUM_COUNT)
	{
		Value = pimage_Variables[eIndex];
		if (Value<0)
		{
			Result = 0;
		}
		else if (Value>255)
		{
			Result = 255;
		}
		else
		{
			Result = (uint8)Value;
		}
	}
	else
	{
		Result = 0;
	}
	return Result;
}

//-----------------------------------------------------------------------------
// Function:    PIMAGE_u16GetValue
//-----------------------------------------------------------------------------
// Description: Get process image value.
//-----------------------------------------------------------------------------
// Return:      Value as unsigned 16 bits.
//-----------------------------------------------------------------------------
uint16 PIMAGE_u16GetValue
(
	PIMAGE_eTABLE eIndex  // in : Variable index
)
{
	uint16 Result;
	int32  Value;
	if (eIndex<VAR_ENUM_COUNT)
	{
		Value = pimage_Variables[eIndex];
		if (Value<0)
		{
			Result = 0;
		}
		else if (Value>65535)
		{
			Result = 65535;
		}
		else
		{
			Result = (uint16)Value;
		}
	}
	else
	{
		Result = 0;
	}
	return Result;
}

//-----------------------------------------------------------------------------
// Function:    PIMAGE_SetValue
//-----------------------------------------------------------------------------
// Description: Set process image value.
//-----------------------------------------------------------------------------
// Return:      None
//-----------------------------------------------------------------------------
void PIMAGE_SetValue
(
	PIMAGE_eTABLE eIndex,   // in : Variable index
	int32         i32Value  // in : Variable value
)
{
	if (eIndex<VAR_ENUM_COUNT)
	{
		pimage_Variables[eIndex] = i32Value;
		//LOG(I, "Setting index=%d to=0x%x store=0x%x", eIndex, (int)i32Value, (int)pimage_Variables[eIndex]);
	}
}

//-----------------------------------------------------------------------------
// Function:    PIMAGE_SetString
//-----------------------------------------------------------------------------
// Description: Set process image device string.
//-----------------------------------------------------------------------------
// Return:      None
//-----------------------------------------------------------------------------
void PIMAGE_SetString
(
	const PIMAGE_eTABLE_SRTING eIndex, // In index of string data
	const char * string                // In string to be stored in PIMAGE
)
{
	strcpy(pimage_strings[eIndex].data, string);
}


//-----------------------------------------------------------------------------
// Function:    PIMAGE_GetString
//-----------------------------------------------------------------------------
// Description: Get process image device string.
//-----------------------------------------------------------------------------
// Return:      None
//-----------------------------------------------------------------------------
void PIMAGE_GetString
(
	const PIMAGE_eTABLE_SRTING eIndex, // In index of string data
	char * string                // Out string to copy data from PIMAGE
)
{
	strcpy(string, pimage_strings[eIndex].data);
}
#endif // CONNECTOR_PREM_RVC