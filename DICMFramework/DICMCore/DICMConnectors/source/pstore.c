//------------------------------------------------------------------------------
// Module:      pstore.c
//------------------------------------------------------------------------------
// Description: Parameter storage

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "configuration.h"
#include "pstore.h"
#include "string.h"
#include "esp_attr.h"
//------------------------------------------------------------------------------
// Private Definitions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Private Types
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Private Variables
//------------------------------------------------------------------------------
static EXT_RAM_ATTR int32_t pstore_Variables[VAR_PSTORE_ENUM_COUNT];

//------------------------------------------------------------------------------
// Public Variables
//------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Function:    pstore_Initialize
//-----------------------------------------------------------------------------
// Description: Initializes the process image.
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void pstore_Initialize( void )
{
	(void)memset(&pstore_Variables, 0, sizeof(pstore_Variables));
}

//-----------------------------------------------------------------------------
// Function:    pstore_GetValue
//-----------------------------------------------------------------------------
// Description: Get process image value
//-----------------------------------------------------------------------------
// Return:      Value as 32 bits integer
//-----------------------------------------------------------------------------
int32_t pstore_GetValue
(
	pstore_table_t eIndex  // in : Variable index
)
{
	int32_t Result;
	if (eIndex<VAR_PSTORE_ENUM_COUNT)
	{
		Result = pstore_Variables[eIndex];
	}
	else
	{
		Result = 0;
	}
	return Result;
}

//-----------------------------------------------------------------------------
// Function:    pstore_s8GetValue
//-----------------------------------------------------------------------------
// Description: Get process image value.
//-----------------------------------------------------------------------------
// Return:      Value as signed 8 bits.
//-----------------------------------------------------------------------------
int8_t pstore_s8GetValue
(
	pstore_table_t eIndex  // in : Variable index
)
{
	uint16_t Result;
	int32_t  Value;
	if (eIndex<VAR_PSTORE_ENUM_COUNT)
	{
		Value = pstore_Variables[eIndex];
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
			Result = (uint8_t)Value;
		}
	}
	else
	{
		Result = 0;
	}
	return Result;
}

//-----------------------------------------------------------------------------
// Function:    pstore_s16GetValue
//-----------------------------------------------------------------------------
// Description: Get process image value.
//-----------------------------------------------------------------------------
// Return:      Value as 16 bits integer.
//-----------------------------------------------------------------------------
int16_t pstore_s16GetValue
(
	pstore_table_t eIndex  // in : Variable index
)
{
	uint16_t Result;
	int32_t  Value;
	if (eIndex<VAR_PSTORE_ENUM_COUNT)
	{
		Value = pstore_Variables[eIndex];
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
			Result = (int16_t)Value;
		}
	}
	else
	{
		Result = 0;
	}
	return Result;
}

//-----------------------------------------------------------------------------
// Function:    pstore_u8GetValue
//-----------------------------------------------------------------------------
// Description: Get process image value.
//-----------------------------------------------------------------------------
// Return:      Value as unsigned 8 bits integer.
//-----------------------------------------------------------------------------
uint8_t pstore_u8GetValue
(
	pstore_table_t eIndex  // in : Variable index
)
{
	uint16_t Result;
	int32_t  Value;
	if (eIndex<VAR_PSTORE_ENUM_COUNT)
	{
		Value = pstore_Variables[eIndex];
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
			Result = (uint8_t)Value;
		}
	}
	else
	{
		Result = 0;
	}
	return Result;
}

//-----------------------------------------------------------------------------
// Function:    pstore_u16GetValue
//-----------------------------------------------------------------------------
// Description: Get process image value.
//-----------------------------------------------------------------------------
// Return:      Value as unsigned 16 bits.
//-----------------------------------------------------------------------------
uint16_t pstore_u16GetValue
(
	pstore_table_t eIndex  // in : Variable index
)
{
	uint16_t Result;
	int32_t  Value;
	if (eIndex<VAR_PSTORE_ENUM_COUNT)
	{
		Value = pstore_Variables[eIndex];
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
			Result = (uint16_t)Value;
		}
	}
	else
	{
		Result = 0;
	}
	return Result;
}

//-----------------------------------------------------------------------------
// Function:    pstore_SetValue
//-----------------------------------------------------------------------------
// Description: Set process image value.
//-----------------------------------------------------------------------------
// Return:      None
//-----------------------------------------------------------------------------
int32_t pstore_SetValue
(
	pstore_table_t eIndex,   // in : Variable index
	int32_t         i32Value  // in : Variable value
)
{
	if (eIndex<VAR_PSTORE_ENUM_COUNT)
	{
		pstore_Variables[eIndex] = i32Value;
		return 1;
	}
	
	return 0;
}

