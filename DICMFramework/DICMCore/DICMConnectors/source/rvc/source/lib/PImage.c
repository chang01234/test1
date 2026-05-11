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

//------------------------------------------------------------------------------
// Private Definitions
//------------------------------------------------------------------------------
static int is_string_type(const PIMAGE_eTABLE index);

//------------------------------------------------------------------------------
// Private Types
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Private Variables
//------------------------------------------------------------------------------
static const PIMAGE_eTABLE PImage_strings[] =
{
        VAR_DGN_65259_IN_MAKE, VAR_DGN_65259_IN_MODEL, VAR_DGN_65259_IN_SNR, VAR_DGN_65259_IN_UNIT
};

static EXT_RAM_ATTR int32 pimage_Variables[VAR_ENUM_COUNT];
typedef struct _PIMAGE_ENTRY
{
	char data[RVCDGN_DGN_65259_FIELD_SIZE + 1];
	uint8_t size;
} PIMAGE_ENTRY;

static EXT_RAM_ATTR PIMAGE_ENTRY pimage_strings[ELEMENTS(PImage_strings)];
//------------------------------------------------------------------------------
// Public Variables
//------------------------------------------------------------------------------

/**
 * @brief
 *
 * @param index Variable index to check
 * @return >= 0 for found string index, -1 if index is not a string index
 */
static int is_string_type(const PIMAGE_eTABLE index)
{
    for (int i = 0; i < (int)ELEMENTS(PImage_strings); i++)
    {
        if (index == PImage_strings[i])
        {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Initializes the process image.
 *
 */
void PIMAGE_Initialize( void )
{
	(void)memset(&pimage_Variables, 0, sizeof(pimage_Variables));
	(void)memset(&pimage_strings, 0, sizeof(pimage_strings));
}

/**
 * @brief Get process image value
 *
 * @param eIndex[in] Variable index
 * @return Value as 32 bits integer
 */
int32 PIMAGE_GetValue(PIMAGE_eTABLE eIndex)
{
    int32 Result;
	if ((eIndex > VAR_RVC_NONE) && (eIndex < VAR_ENUM_COUNT))
	{
		Result = pimage_Variables[eIndex];
	}
	else
	{
		Result = 0;
	}
	return Result;
}

/**
 * @brief Get process image value.
 *
 * @param eIndex[in] Variable index
 * @return Value as signed 8 bits.
 */
int8 PIMAGE_s8GetValue(PIMAGE_eTABLE eIndex)
{
	uint16 Result;
	int32  Value;
	if ((eIndex > VAR_RVC_NONE) && (eIndex < VAR_ENUM_COUNT))
	{
		Value = pimage_Variables[eIndex];
		if (Value < INT8_MIN)
		{
			Result = INT8_MIN;
		}
		else if (Value > INT8_MAX)
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

/**
 * @brief Get process image value.
 *
 * @param eIndex[in] Variable index
 * @return Value as 16 bits integer.
 */
int16 PIMAGE_s16GetValue(PIMAGE_eTABLE eIndex)
{
	uint16 Result;
	int32  Value;
	if ((eIndex > VAR_RVC_NONE) && (eIndex < VAR_ENUM_COUNT))
	{
		Value = pimage_Variables[eIndex];
		if (Value < INT16_MIN)
		{
			Result = INT16_MIN;
		}
		else if (Value > INT16_MAX)
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

/**
 * @brief Get process image value.
 *
 * @param eIndex[in] Variable index
 * @return Value as unsigned 8 bits integer.
 */
uint8 PIMAGE_u8GetValue(PIMAGE_eTABLE eIndex)
{
	uint16 Result;
	int32  Value;
	if ((eIndex > VAR_RVC_NONE) && (eIndex < VAR_ENUM_COUNT))
	{
		Value = pimage_Variables[eIndex];
		if (Value < 0)
		{
			Result = 0;
		}
		else if (Value > UINT8_MAX)
		{
			Result = UINT8_MAX;
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

/**
 * @brief Get process image value.
 *
 * @param eIndex[in] Variable index
 * @return Value as unsigned 16 bits.
 */
uint16 PIMAGE_u16GetValue(PIMAGE_eTABLE eIndex)
{
	uint16 Result;
	int32  Value;
	if ((eIndex > VAR_RVC_NONE) && (eIndex < VAR_ENUM_COUNT))
	{
		Value = pimage_Variables[eIndex];
		if (Value < 0)
		{
			Result = 0;
		}
		else if (Value > UINT16_MAX)
		{
			Result = UINT16_MAX;
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

/**
 * @briefSet process image value.
 *
 * @param eIndex[in] Variable index
 * @param i32Value[in] Variable value
 */
void PIMAGE_SetValue(PIMAGE_eTABLE eIndex, int32 i32Value)
{
	if ((eIndex > VAR_RVC_NONE) && (eIndex < VAR_ENUM_COUNT))
	{
		pimage_Variables[eIndex] = i32Value;
		//LOG(I, "Setting index=%d to=0x%x store=0x%x", eIndex, (int)i32Value, (int)pimage_Variables[eIndex]);
	}
}

/**
 * @brief Set process image device string.
 *
 * @param eIndex[in] variable index, need to be a string type
 * @param string[in] string to be stored in PIMAGE
 */
void PIMAGE_SetString(const PIMAGE_eTABLE eIndex, const char * string)
{
    int sIndex = 0;
    if ((sIndex = is_string_type(eIndex)) != -1)
    {
        strcpy(pimage_strings[sIndex].data, string);
    }
}


/**
 * @brief Get process image device string.
 *
 * @param eIndex[in] variable index, need to be a string type
 * @param string[out] string to copy data from PIMAGE
 */
void PIMAGE_GetString(const PIMAGE_eTABLE eIndex, char *string)
{
    int sIndex = 0;
    if (NULL != string)
    {
        string[0] = '\0';
        if ((sIndex = is_string_type(eIndex)) != -1)
        {
            strcpy(string, pimage_strings[sIndex].data);
        }
    }
}
