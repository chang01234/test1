//------------------------------------------------------------------------------
// Module:      RVCDGN.c
//------------------------------------------------------------------------------
// Description: Provides PGN structure definitions.
//------------------------------------------------------------------------------
// COPYRIGHT:   Seastar Solutions
//              3831, No 6 Road
//              Richmond, BC
//              Canada V6V 1P6
//
//              This source file and the information contained in it are
//              confidential and proprietary to Seastar Solutions Inc.
//              The reproduction or disclosure, in whole or in part,
//              to anyone outside of Seastar Solutions without the written
//              approval of a Seastar Solutions officer under a Non-Disclosure
//              Agreement is expressly prohibited.
//
//              All rights reserved
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
// Libraries
#include <string.h>

// Own Module
#include "RVCDGN.h"

//------------------------------------------------------------------------------
// Local definitions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Local types
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Local variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Local function prototypes
//------------------------------------------------------------------------------
static void   rvcdgn_SetString( char* pu8BufferData, uint16 u16BufferSize, const char* pcString );
static uint16 rvcdgn_AppendString( char* pu8BufferData, uint16 u16BufferIndex, uint16 u16BufferSize, const char* pcString );

//-----------------------------------------------------------------------------
// Function:   RVCDGN_DGN_130762_StuffInit
//-----------------------------------------------------------------------------
// Purpose:    Initialization of a DM_RV Stuffing session
//-----------------------------------------------------------------------------
// Notes:      None.
//-----------------------------------------------------------------------------
// Parameters: See below
//-----------------------------------------------------------------------------
// Return:     None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130762_StuffInit
(
    RVCDGN_zDGN_130762_Session* pzSession,    // DM_RV session instance address
    uint8*                      pu8Data,      // DM_RV frame buffer address
    uint16                      u16Capacity   // DM_RV frame buffer size
)
{
	// Init session with default header (can be replaced later)
    pzSession->pu8Data     = pu8Data;
    pzSession->pu8Data[0]  = 0;
	pzSession->pu8Data[1]  = 0;
    pzSession->u16Index    = 2;
    pzSession->u16Capacity = u16Capacity;
}

//-----------------------------------------------------------------------------
// Function:   RVCDGN_DGN_130762_StuffHeader
//-----------------------------------------------------------------------------
// Purpose:    Stuff DM_RV header parameters into a frame buffer.
//-----------------------------------------------------------------------------
// Notes:      None.
//-----------------------------------------------------------------------------
// Parameters: See below
//-----------------------------------------------------------------------------
// Return:     None.
//-----------------------------------------------------------------------------
extern void RVCDGN_DGN_130762_StuffHeader
(
    RVCDGN_zDGN_130762_Session* pzSession, // DMRV session instance address
	RVCDGN_zDGN_130762_Header*  pzHeader   // DMRV header structure address
)
{
	// Clear
	(void)memset(pzSession->pu8Data, 0, RVCDGN_DGN_130762_HEADER_SIZE);

	// Fill
    //           dest                   source                      start  size
    BITS_TO_DGN( pzSession->pu8Data[0], pzHeader->u2OperatingStatus1,   0,    2 );
    BITS_TO_DGN( pzSession->pu8Data[0], pzHeader->u2OperatingStatus2,   2,    2 );
    BITS_TO_DGN( pzSession->pu8Data[0], pzHeader->u2YellowLampStatus,   4,    2 );
    BITS_TO_DGN( pzSession->pu8Data[0], pzHeader->u2RedLampStatus,      6,    2 );
    BITS_TO_DGN( pzSession->pu8Data[1], pzHeader->u8DSA,                0,    8 );
}

//-----------------------------------------------------------------------------
// Function:   RVCDGN_DGN_130762_StuffDTC
//-----------------------------------------------------------------------------
// Purpose:    Stuff of consecutive DTC parameters into a frame buffer
//-----------------------------------------------------------------------------
// Notes:      None.
//-----------------------------------------------------------------------------
// Parameters: See below
//-----------------------------------------------------------------------------
// Return:     TRUE: A DTC has been stored, FALSE: No more DTC
//-----------------------------------------------------------------------------
extern bool RVCDGN_DGN_130762_StuffDTC
(
    RVCDGN_zDGN_130762_Session* pzSession, // DMRV session instance address
	RVCDGN_zDGN_130762_DTC*     pzDtc      // DMRV DTC structure address
)
{
	uint8  u8SPN_MSB = (pzDtc->u32SPN>>11)&0xFF;
	uint8  u8SPN_ISB = (pzDtc->u32SPN>> 3)&0xFF;
	uint8  u8SPN_LSB = (pzDtc->u32SPN>> 0)&0x07;
	uint8* pu8Data   = &pzSession->pu8Data[pzSession->u16Index];

    // Check if there is space available for a new DTC
    if ( (pzSession->u16Index+RVCDGN_DGN_130762_DTC_SIZE+RVCDGN_DGN_130762_FOOTER_SIZE) > pzSession->u16Capacity )
    {
        // Not enough space to store DTC
        return FALSE;
    }

    // Saturate Occurence Count to maximum allowed
    if (pzDtc->u8OccurCount > 126)
    {
    	pzDtc->u8OccurCount = 126;
    }

	// Clear
	(void)memset(pu8Data, 0, RVCDGN_DGN_130762_DTC_SIZE);

    // Store
    //           dest         source              start  size
    BITS_TO_DGN( pu8Data[0],  u8SPN_MSB,              0,    8 );
    BITS_TO_DGN( pu8Data[1],  u8SPN_ISB,              0,    8 );
    BITS_TO_DGN( pu8Data[2],  u8SPN_LSB,              5,    3 );
    BITS_TO_DGN( pu8Data[2],  pzDtc->u8FMI,           0,    5 );
    BITS_TO_DGN( pu8Data[3],  pzDtc->u8OccurCount,    0,    7 );
    BITS_TO_DGN( pu8Data[3],  1,                      7,    1 );
    BITS_TO_DGN( pu8Data[4],  pzDtc->u8DSAExtension,  0,    8 );

    // Increase index
    pzSession->u16Index += RVCDGN_DGN_130762_DTC_SIZE;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Function:   RVCDGN_DGN_130762_StuffFooter
//-----------------------------------------------------------------------------
// Purpose:    Stuff DM_RV footer into a frame buffer.
//-----------------------------------------------------------------------------
// Notes:      None.
//-----------------------------------------------------------------------------
// Parameters: See below
//-----------------------------------------------------------------------------
// Return:     None.
//-----------------------------------------------------------------------------
extern void RVCDGN_DGN_130762_StuffFooter
(
    RVCDGN_zDGN_130762_Session* pzSession  // DMRV session instance address
)
{
	// Add footer
	if ( pzSession->u16Index < pzSession->u16Capacity  )
	{
		pzSession->pu8Data[pzSession->u16Index] = 0xFF;
		pzSession->u16Index++;
	}

	// Padding (at least 1 frame)
	while ( pzSession->u16Index < RVCDGN_SINGLE_FRAME_SIZE  )
	{
		pzSession->pu8Data[pzSession->u16Index] = 0xFF;
		pzSession->u16Index++;
	}
}

//-----------------------------------------------------------------------------
// Function:   RVCDGN_DGN_130762_StuffSize
//-----------------------------------------------------------------------------
// Purpose:    Return the actual size of the DM_RV message (stuffing session)
//-----------------------------------------------------------------------------
// Notes:      None.
//-----------------------------------------------------------------------------
// Parameters: DMRV session instance address
//-----------------------------------------------------------------------------
// Return:     Size
//-----------------------------------------------------------------------------
uint16 RVCDGN_DGN_130762_StuffSize
(
	RVCDGN_zDGN_130762_Session *pzSession  // DM_RV session instance address
)
{
	// Message size must be at least one frame
    uint16 u16Result = pzSession->u16Index;
    if ( u16Result < RVCDGN_SINGLE_FRAME_SIZE )
    {
        u16Result = RVCDGN_SINGLE_FRAME_SIZE;
    }
    return u16Result;
}

//-----------------------------------------------------------------------------
// Function:   RVCDGN_DGN_130762_ExtractInit
//-----------------------------------------------------------------------------
// Purpose:    Initialization of a DM_RV Extracting session
//-----------------------------------------------------------------------------
// Notes:      None.
//-----------------------------------------------------------------------------
// Parameters: See below
//-----------------------------------------------------------------------------
// Return:     None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130762_ExtractInit
(
    RVCDGN_zDGN_130762_Session* pzSession,  // DM_RV session instance address
    uint8*                      pu8Data,    // DM_RV frame buffer address
    uint16                      u16Capacity // DM_RV frame buffer size
)
{
    pzSession->pu8Data     = pu8Data;
    pzSession->u16Index    = 0;
    pzSession->u16Capacity = u16Capacity;
}

//-----------------------------------------------------------------------------
// Function:   RVCDGN_DGN_130762_ExtractHeader
//-----------------------------------------------------------------------------
// Purpose:    Extraction of the header portion of a DM_RV structure
//-----------------------------------------------------------------------------
// Notes:      None.
//-----------------------------------------------------------------------------
// Parameters: See below
//-----------------------------------------------------------------------------
// Return:     None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130762_ExtractHeader
(
    RVCDGN_zDGN_130762_Session* pzSession, // DMRV session instance address
	RVCDGN_zDGN_130762_Header*  pzHeader   // DMRV header structure address
)
{
    //           dest                           src                   start  size
    DGN_TO_BITS( pzHeader->u2OperatingStatus1,  pzSession->pu8Data[0],    0,    2 );
    DGN_TO_BITS( pzHeader->u2OperatingStatus2,  pzSession->pu8Data[0],    2,    2 );
    DGN_TO_BITS( pzHeader->u2YellowLampStatus,  pzSession->pu8Data[0],    4,    2 );
    DGN_TO_BITS( pzHeader->u2RedLampStatus,     pzSession->pu8Data[0],    6,    2 );
    DGN_TO_BITS( pzHeader->u8DSA,               pzSession->pu8Data[1],    0,    8 );

    pzSession->u16Index = RVCDGN_DGN_130762_HEADER_SIZE;
}

//-----------------------------------------------------------------------------
// Function:   RVCDGN_DGN_130762_ExtractDTC
//-----------------------------------------------------------------------------
// Purpose:    Extraction of consecutive DTCs of a DM_RV structure
//-----------------------------------------------------------------------------
// Notes:      None.
//-----------------------------------------------------------------------------
// Parameters: See below
//-----------------------------------------------------------------------------
// Return:     TRUE: A DTC has been stored, FALSE: No more DTC
//-----------------------------------------------------------------------------
bool RVCDGN_DGN_130762_ExtractDTC
(
	RVCDGN_zDGN_130762_Session* pzSession, // DM_RV session instance address
    RVCDGN_zDGN_130762_DTC*     pzDtc      // DM_RV DTC structure address
)
{
	uint8 u8SPN_MSB, u8SPN_ISB, u8SPN_LSB;
	uint8* pu8Data = &pzSession->pu8Data[pzSession->u16Index];

    // Check if there is a new DTC
    if ( (pzSession->u16Index + RVCDGN_DGN_130762_DTC_SIZE) > pzSession->u16Capacity )
    {
        // No more DTC in structure
        return FALSE;
    }

    // Extract
    //           dest                   src         start  size
    DGN_TO_BITS( u8SPN_MSB,             pu8Data[0],     0,    8  );
    DGN_TO_BITS( u8SPN_ISB,             pu8Data[1],     0,    8  );
    DGN_TO_BITS( u8SPN_LSB,             pu8Data[2],     5,    3  );
    DGN_TO_BITS( pzDtc->u8FMI,          pu8Data[2],     0,    5  );
    DGN_TO_BITS( pzDtc->u8OccurCount,   pu8Data[3],     0,    7  );
    DGN_TO_BITS( pzDtc->u8DSAExtension, pu8Data[4],     0,    8  );

    // Merge SPN
    pzDtc->u32SPN  = ((uint32)u8SPN_MSB<<11);
    pzDtc->u32SPN |= ((uint32)u8SPN_ISB<< 3);
    pzDtc->u32SPN |= ((uint32)u8SPN_LSB<< 0)&7;

    // Set DTC pointer to next
    pzSession->u16Index += RVCDGN_DGN_130762_DTC_SIZE;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Function:   RVCDGN_DGN_59904_Stuff (ISO request)
//-----------------------------------------------------------------------------
// Purpose:    Stuff parameters from DGN 59904.
//-----------------------------------------------------------------------------
// Notes:      None.
//-----------------------------------------------------------------------------
// Return:     None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_59904_Stuff
(
    uint8             *pu8Dest,   // out: Destination buffer.
    RVCDGN_zDGN_59904 *pzSrc      // in : Source structure.
)
{
    // Clear
    (void)memset( pu8Dest, 0, RVCDGN_DGN_59904_SIZE );

    // Fill
    pu8Dest[0] = (pzSrc->u32DGN >>  0) & 0xFF;
    pu8Dest[1] = (pzSrc->u32DGN >>  8) & 0xFF;
    pu8Dest[2] = (pzSrc->u32DGN >> 16) & 0xFF;
}

//-----------------------------------------------------------------------------
// Function:   RVCDGN_DGN_60928_Extract
//-----------------------------------------------------------------------------
// Purpose:    Extract parameters from DGN 60928 frame buffer.
//-----------------------------------------------------------------------------
// Notes:      None.
//-----------------------------------------------------------------------------
// Parameters: See below
//-----------------------------------------------------------------------------
// Return:     None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_60928_Extract
(
    RVCDGN_zDGN_60928 *pzDgn60928,  // out: Extracted parameters
    uint8             *pu8Data      // in : DGN frame buffer
)
{
    // Special case: 21 bit extraction
    pzDgn60928->u32UniqueNumber  = pu8Data[0];
    pzDgn60928->u32UniqueNumber |= ( (uint32)pu8Data[1] << 8 ) & 0x0000FF00;
    pzDgn60928->u32UniqueNumber |= ( (uint32)pu8Data[2] << 16 ) & 0x001F0000;

    // Special case: 11 bit extraction
    pzDgn60928->u16ManufCode  = ( pu8Data[2] >> 5 ) & 0x07;
    pzDgn60928->u16ManufCode |= ( (uint16)pu8Data[3] << 3 ) & 0x07F8;

    //           dest                                 src             start,  size
    DGN_TO_BITS( pzDgn60928->u3NodeInstance,          pu8Data[4],     0,      3   );
    DGN_TO_BITS( pzDgn60928->u5FuncInstance,          pu8Data[4],     3,      5   );
    DGN_TO_BITS( pzDgn60928->u8Function,              pu8Data[5],     0,      8   );
    DGN_TO_BITS( pzDgn60928->u1Reserved,              pu8Data[6],     0,      1   );
    DGN_TO_BITS( pzDgn60928->u7VehicleSystem,         pu8Data[6],     1,      7   );
    DGN_TO_BITS( pzDgn60928->u4VehicleSystemInstance, pu8Data[7],     0,      4   );
    DGN_TO_BITS( pzDgn60928->u3IndustryGroup,         pu8Data[7],     4,      3   );
    DGN_TO_BITS( pzDgn60928->u1ArbAddrCapable,        pu8Data[7],     7,      1   );
}
//-----------------------------------------------------------------------------
// Function:   RVCDGN_DGN_60928_Stuff
//-----------------------------------------------------------------------------
// Purpose:    Stuff DGN 60928 parameters into a frame buffer.
//-----------------------------------------------------------------------------
// Notes:      None.
//-----------------------------------------------------------------------------
// Parameters: See below
//-----------------------------------------------------------------------------
// Return:     None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_60928_Stuff
(
    uint8             *pu8Data,     // out: DGN frame buffer
    RVCDGN_zDGN_60928 *pzDgn60928   // in : Parameters to stuff
)
{
    // Clear
    (void)memset( pu8Data, 0, RVCDGN_DGN_60928_SIZE );

    // Special case: 21 bit stuffing
    pu8Data[0]  = (uint8)( pzDgn60928->u32UniqueNumber );
    pu8Data[1] |= (uint8)( pzDgn60928->u32UniqueNumber >> 8 );
    pu8Data[2] |= (uint8)( (pzDgn60928->u32UniqueNumber >> 16 ) & 0x1F );

    // Special case: 11 bit stuffing
    pu8Data[2] |= (uint8)( ( pzDgn60928->u16ManufCode << 5 ) & 0xE0 );
    pu8Data[3]  = (uint8)( pzDgn60928->u16ManufCode >> 3 );

    //           dest         source                            start    size
    BITS_TO_DGN( pu8Data[4],  pzDgn60928->u3NodeInstance,    		0,      3  );
    BITS_TO_DGN( pu8Data[4],  pzDgn60928->u5FuncInstance,    		3,      5  );
    BITS_TO_DGN( pu8Data[5],  pzDgn60928->u8Function,      		    0,      8  );
    BITS_TO_DGN( pu8Data[6],  pzDgn60928->u1Reserved,               0,      1  );
    BITS_TO_DGN( pu8Data[6],  pzDgn60928->u7VehicleSystem,          1,      7  );
    BITS_TO_DGN( pu8Data[7],  pzDgn60928->u4VehicleSystemInstance,	0,      4  );
    BITS_TO_DGN( pu8Data[7],  pzDgn60928->u3IndustryGroup,    		4,      3  );
    BITS_TO_DGN( pu8Data[7],  pzDgn60928->u1ArbAddrCapable,   		7,      1  );
}

//-----------------------------------------------------------------------------
// Function:   RVCDGN_DGN_65242_Extract
//-----------------------------------------------------------------------------
// Purpose:    Extract parameters from DGN 65242.
//-----------------------------------------------------------------------------
// Notes:      None.
//-----------------------------------------------------------------------------
// Return:     None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_65242_Extract
(
    RVCDGN_zDGN_65242* pzDest,   // out: Destination structure
    uint8*             pu8Src,   // in:  Source buffer (will overwrite separator)
	uint16             u16Size   // in:  Source size
)
{
	uint16 u16Index, u16FieldId, u16FieldStart, u16FieldEnd, u16FieldSize;
	char* pcFieldData;

	// Clear
	memset(pzDest, 0, sizeof(RVCDGN_zDGN_65242));

	// Scan
	u16FieldId    = 0;
	u16FieldStart = 1;
	u16FieldEnd   = u16FieldStart;
	for (u16Index=1; u16Index<u16Size; u16Index++)
	{
		// End of field?
		if (pu8Src[u16Index]=='*')
		{
			u16FieldEnd = u16Index;
		}
		else if ((u16Index+1)==u16Size)
		{
			u16FieldEnd = u16Index+1;
		}

		// New field?
		if (u16FieldEnd>u16FieldStart)
		{
			// Select field
			switch (u16FieldId)
			{
				case 0:  pcFieldData = &pzDest->cId1[0]; break;
				case 1:  pcFieldData = &pzDest->cId2[0]; break;
				case 2:  pcFieldData = &pzDest->cId3[0]; break;
				case 3:  pcFieldData = &pzDest->cId4[0]; break;
				default: pcFieldData = NULL;             break;
			}

			// Fill field
			if (pcFieldData != NULL)
			{
				u16FieldSize = u16FieldEnd-u16FieldStart;
				if (u16FieldSize>(RVCDGN_DGN_65242_FIELD_SIZE-1))
				{
					u16FieldSize = (RVCDGN_DGN_65242_FIELD_SIZE-1);
				}
				(void)memcpy(pcFieldData, &pu8Src[u16FieldStart], u16FieldSize);
				pcFieldData = NULL;
			}

			// Next field
			u16FieldId++;
			u16FieldStart = u16Index + 1;
			u16FieldEnd   = u16FieldStart;
		}
	}
}

//-----------------------------------------------------------------------------
// Function:   RVCDGN_DGN_65242_Set
//-----------------------------------------------------------------------------
// Purpose:    Set parameter to DGN 65242.
//-----------------------------------------------------------------------------
// Notes:      None.
//-----------------------------------------------------------------------------
// Return:     None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_65242_Set
(
	RVCDGN_zDGN_65242* pzDgn65242, // out:  Destination buffer
	const char*        pcId1,      // in:   Identifier #1
	const char*        pcId2,      // in:   Identifier #2
	const char*        pcId3,      // in:   Identifier #3
	const char*        pcId4       // in:   Identifier #4
)
{
	rvcdgn_SetString( pzDgn65242->cId1,     sizeof(pzDgn65242->cId1),     pcId1 );
	rvcdgn_SetString( pzDgn65242->cId2,     sizeof(pzDgn65242->cId2),     pcId2 );
	rvcdgn_SetString( pzDgn65242->cId3,     sizeof(pzDgn65242->cId3),     pcId3 );
	rvcdgn_SetString( pzDgn65242->cId4,     sizeof(pzDgn65242->cId4),     pcId4 );
}

//-----------------------------------------------------------------------------
// Function:   RVCDGN_DGN_65242_Stuff
//-----------------------------------------------------------------------------
// Purpose:    Stuff parameters from DGN 65242.
//-----------------------------------------------------------------------------
// Notes:      None.
//-----------------------------------------------------------------------------
// Return:     None.
//-----------------------------------------------------------------------------
uint16 RVCDGN_DGN_65242_Stuff
(
    uint8*             pu8Dest,     // In/out:  Destination buffer
	RVCDGN_zDGN_65242* pzSrc        // in:      Source structure
)
{
	uint16 u16Index = 1;
	pu8Dest[0] = 4;
	u16Index = rvcdgn_AppendString((char*)pu8Dest, u16Index, RVCDGN_DGN_65242_SIZE-1, pzSrc->cId1);
	u16Index = rvcdgn_AppendString((char*)pu8Dest, u16Index, RVCDGN_DGN_65242_SIZE  , "*");
	u16Index = rvcdgn_AppendString((char*)pu8Dest, u16Index, RVCDGN_DGN_65242_SIZE-1, pzSrc->cId2);
	u16Index = rvcdgn_AppendString((char*)pu8Dest, u16Index, RVCDGN_DGN_65242_SIZE  , "*");
	u16Index = rvcdgn_AppendString((char*)pu8Dest, u16Index, RVCDGN_DGN_65242_SIZE-1, pzSrc->cId3);
	u16Index = rvcdgn_AppendString((char*)pu8Dest, u16Index, RVCDGN_DGN_65242_SIZE  , "*");
	u16Index = rvcdgn_AppendString((char*)pu8Dest, u16Index, RVCDGN_DGN_65242_SIZE-1, pzSrc->cId4);
	u16Index = rvcdgn_AppendString((char*)pu8Dest, u16Index, RVCDGN_DGN_65242_SIZE  , "*");
	while ( u16Index < RVCDGN_SINGLE_FRAME_SIZE )
	{
		pu8Dest[u16Index++] = 0;
	}
	return u16Index;
}

//-----------------------------------------------------------------------------
// Function:   RVCDGN_DGN_65259_Extract
//-----------------------------------------------------------------------------
// Purpose:    Extract parameters from DGN 65259.
//-----------------------------------------------------------------------------
// Notes:      None.
//-----------------------------------------------------------------------------
// Return:     None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_65259_Extract
(
    RVCDGN_zDGN_65259* pzDest,   // out: Destination structure
    uint8*             pu8Src,   // in:  Source buffer (will overwrite separator)
	uint16             u16Size   // in:  Source size
)
{
	uint16 u16Index, u16FieldId, u16FieldStart, u16FieldEnd, u16FieldSize;
	char* pcFieldData;

	// Clear
	memset(pzDest, 0, sizeof(RVCDGN_zDGN_65259));

	// Scan
	u16FieldId    = 0;
	u16FieldStart = 0;
	u16FieldEnd   = u16FieldStart;
	for (u16Index=0; u16Index<u16Size; u16Index++)
	{
		// End of field?
		if (pu8Src[u16Index]=='*')
		{
			u16FieldEnd = u16Index;
		}
		else if ((u16Index+1)==u16Size)
		{
			u16FieldEnd = u16Index+1;
		}

		// New field?
		if (u16FieldEnd>u16FieldStart)
		{
			// Select field
			switch (u16FieldId)
			{
				case 0:  pcFieldData = &pzDest->cMake[0];         break;
				case 1:  pcFieldData = &pzDest->cModel[0];        break;
				case 2:  pcFieldData = &pzDest->cSerialNumber[0]; break;
				case 3:  pcFieldData = &pzDest->cUnitNumber[0];   break;
				default: pcFieldData = NULL;                      break;
			}

			// Fill field
			if (pcFieldData!=NULL)
			{
				u16FieldSize = u16FieldEnd-u16FieldStart;
				if (u16FieldSize>(RVCDGN_DGN_65259_FIELD_SIZE-1))
				{
					u16FieldSize = (RVCDGN_DGN_65259_FIELD_SIZE-1);
				}
				(void)memcpy(pcFieldData, &pu8Src[u16FieldStart], u16FieldSize);
				pcFieldData = NULL;
			}

			// Next field
			u16FieldId++;
			u16FieldStart = u16Index + 1;
			u16FieldEnd   = u16FieldStart;
		}
	}
}

//-----------------------------------------------------------------------------
// Function:   RVCDGN_DGN_65259_Set
//-----------------------------------------------------------------------------
// Purpose:    Set parameter to DGN 65259.
//-----------------------------------------------------------------------------
// Notes:      None.
//-----------------------------------------------------------------------------
// Return:     None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_65259_Set
(
	RVCDGN_zDGN_65259* pzDgn65259,     // out:  Destination buffer
	const char*        pcMake,         // in:   Make string
	const char*        pcModel,        // in:   Model string
	const char*        pcSerialNumber, // in:   Serial number string
	const char*        pcUnitNumber    // in:   Unit number string
)
{
	rvcdgn_SetString(pzDgn65259->cMake,         sizeof(pzDgn65259->cMake),         pcMake);
	rvcdgn_SetString(pzDgn65259->cModel,        sizeof(pzDgn65259->cModel),        pcModel);
	rvcdgn_SetString(pzDgn65259->cSerialNumber, sizeof(pzDgn65259->cSerialNumber), pcSerialNumber);
	rvcdgn_SetString(pzDgn65259->cUnitNumber,   sizeof(pzDgn65259->cUnitNumber),   pcUnitNumber);
}

//-----------------------------------------------------------------------------
// Function:   RVCDGN_DGN_65259_Stuff
//-----------------------------------------------------------------------------
// Purpose:    Stuff parameters from DGN 65259.
//-----------------------------------------------------------------------------
// Notes:      None.
//-----------------------------------------------------------------------------
// Return:     None.
//-----------------------------------------------------------------------------
uint16 RVCDGN_DGN_65259_Stuff
(
    uint8*             pu8Dest,     // In/out:  Destination buffer
	RVCDGN_zDGN_65259* pzSrc        // in:      Source structure
)
{
	uint16 u16Index = 0;
	u16Index = rvcdgn_AppendString((char*)pu8Dest, u16Index, RVCDGN_DGN_65259_SIZE-1, pzSrc->cMake);
	u16Index = rvcdgn_AppendString((char*)pu8Dest, u16Index, RVCDGN_DGN_65259_SIZE  , "*");
	u16Index = rvcdgn_AppendString((char*)pu8Dest, u16Index, RVCDGN_DGN_65259_SIZE-1, pzSrc->cModel);
	u16Index = rvcdgn_AppendString((char*)pu8Dest, u16Index, RVCDGN_DGN_65259_SIZE  , "*");
	u16Index = rvcdgn_AppendString((char*)pu8Dest, u16Index, RVCDGN_DGN_65259_SIZE-1, pzSrc->cSerialNumber);
	u16Index = rvcdgn_AppendString((char*)pu8Dest, u16Index, RVCDGN_DGN_65259_SIZE  , "*");
	u16Index = rvcdgn_AppendString((char*)pu8Dest, u16Index, RVCDGN_DGN_65259_SIZE-1, pzSrc->cUnitNumber);
	u16Index = rvcdgn_AppendString((char*)pu8Dest, u16Index, RVCDGN_DGN_65259_SIZE  , "*");
	while ( u16Index<RVCDGN_SINGLE_FRAME_SIZE )
	{
		pu8Dest[u16Index++] = 0;
	}
	return u16Index;
}

//-----------------------------------------------------------------------------
// Function:   RVCDGN_DGN_130972_Extract
//-----------------------------------------------------------------------------
// Purpose:    Extract parameters from DGN 130972 frame buffer.
//-----------------------------------------------------------------------------
// Notes:      None.
//-----------------------------------------------------------------------------
// Parameters: See below
//-----------------------------------------------------------------------------
// Return:     None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130972_Extract
(
	RVCDGN_zDGN_130972 *pzDgn,  // out: Extracted parameters
    uint8              *pu8Data   // in : DGN frame buffer
)
{
    //           dest                            src         start   size
	DGN_TO_BITS( pzDgn->u8Instance,              pu8Data[0],     0,     8 );
    DGN_TO_WORD( pzDgn->u16AmbientTemp,          pu8Data[1]);
    DGN_TO_BITS( pzDgn->u8Reserved1,             pu8Data[3],     0,     8 );
    DGN_TO_WORD( pzDgn->u16Reserved2,            pu8Data[4]);
    DGN_TO_WORD( pzDgn->u16Reserved3,            pu8Data[6]);
}
//-----------------------------------------------------------------------------
// Function:   RVCDGN_DGN_130972_Stuff
//-----------------------------------------------------------------------------
// Purpose:    Stuff DGN 130972 parameters into a frame buffer.
//-----------------------------------------------------------------------------
// Notes:      None.
//-----------------------------------------------------------------------------
// Parameters: See below
//-----------------------------------------------------------------------------
// Return:     None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130972_Stuff
(
    uint8              *pu8Data,  // out: DGN frame buffer
	RVCDGN_zDGN_130972 *pzDgn     // in : Parameters to stuff
)
{
    // Clear
    (void)memset( pu8Data, 0, RVCDGN_DGN_130972_SIZE );

    //           dest         source                start  size
    BITS_TO_DGN( pu8Data[0],  pzDgn->u8Instance,        0,    8 );
    WORD_TO_DGN( pu8Data[1],  pzDgn->u16AmbientTemp             );
    BITS_TO_DGN( pu8Data[3],  0xFF,                     0,    8 );
    WORD_TO_DGN( pu8Data[4],  0xFFFF                            );
    WORD_TO_DGN( pu8Data[6],  0xFFFF                            );
}


//-----------------------------------------------------------------------------
// Function:   RVCDGN_DGN_131071_Extract
//-----------------------------------------------------------------------------
// Purpose:    Extract parameters from DGN 131071 frame buffer.
//-----------------------------------------------------------------------------
// Notes:      None.
//-----------------------------------------------------------------------------
// Parameters: See below
//-----------------------------------------------------------------------------
// Return:     None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_131071_Extract
(
	RVCDGN_zDGN_131071 *pzDgn131071,  // out: Extracted parameters
    uint8              *pu8Data       // in : DGN frame buffer
)
{
    //           dest                            src         start   size
    DGN_TO_BITS( pzDgn131071->u8Year,            pu8Data[0],     0,     8 );
    DGN_TO_BITS( pzDgn131071->u8Month,           pu8Data[1],     0,     8 );
    DGN_TO_BITS( pzDgn131071->u8Day,             pu8Data[2],     0,     8 );
    DGN_TO_BITS( pzDgn131071->u8DayOfWeek,       pu8Data[3],     0,     8 );
    DGN_TO_BITS( pzDgn131071->u8Hour,            pu8Data[4],     0,     8 );
    DGN_TO_BITS( pzDgn131071->u8Minute,          pu8Data[5],     0,     8 );
    DGN_TO_BITS( pzDgn131071->u8Second,          pu8Data[6],     0,     8 );
    DGN_TO_BITS( pzDgn131071->u8TimeZone,        pu8Data[7],     0,     8 );
}
//-----------------------------------------------------------------------------
// Function:   RVCDGN_DGN_131071_Stuff
//-----------------------------------------------------------------------------
// Purpose:    Stuff DGN 131071 parameters into a frame buffer.
//-----------------------------------------------------------------------------
// Notes:      None.
//-----------------------------------------------------------------------------
// Parameters: See below
//-----------------------------------------------------------------------------
// Return:     None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_131071_Stuff
(
    uint8              *pu8Data,  // out: DGN frame buffer
	RVCDGN_zDGN_131071 *pzDgn     // in : Parameters to stuff
)
{
    // Clear
    (void)memset( pu8Data, 0, RVCDGN_DGN_131071_SIZE );

    //           dest          source                start   size
    BITS_TO_DGN( pu8Data[0],   pzDgn->u8Year,            0,     8 );
    BITS_TO_DGN( pu8Data[1],   pzDgn->u8Month,           0,     8 );
    BITS_TO_DGN( pu8Data[2],   pzDgn->u8Day,             0,     8 );
    BITS_TO_DGN( pu8Data[3],   pzDgn->u8DayOfWeek,       0,     8 );
    BITS_TO_DGN( pu8Data[4],   pzDgn->u8Hour,            0,     8 );
    BITS_TO_DGN( pu8Data[5],   pzDgn->u8Minute,          0,     8 );
    BITS_TO_DGN( pu8Data[6],   pzDgn->u8Second,          0,     8 );
    BITS_TO_DGN( pu8Data[7],   pzDgn->u8TimeZone,        0,     8 );
}

//-----------------------------------------------------------------------------
// Function:   rvcdgn_SetString
//-----------------------------------------------------------------------------
// Purpose:    Set string to buffer
//-----------------------------------------------------------------------------
// Notes:      None.
//-----------------------------------------------------------------------------
// Return:     None.
//-----------------------------------------------------------------------------
static void rvcdgn_SetString
(
	char*       pcBufferData,  // in/out:  Buffer data
	uint16      u16BufferSize, // in:      Buffer size
	const char* pcString       // in:      String input
)
{
	uint16 u16Index = 0;

	// Data
	while ( u16Index < (u16BufferSize-1) )
	{
		if ( (pcString[u16Index]==0) || (pcString[u16Index]==0xFF) )
		{
			break;
		}
		pcBufferData[u16Index] = pcString[u16Index];
		u16Index++;
	}

	// Padding
	while ( u16Index < u16BufferSize )
	{
		pcBufferData[u16Index] = 0;
		u16Index++;
	}
}

//-----------------------------------------------------------------------------
// Function:   rvcdgn_AppendString
//-----------------------------------------------------------------------------
// Purpose:    Append string to a buffer
//-----------------------------------------------------------------------------
// Notes:      None.
//-----------------------------------------------------------------------------
// Return:     New buffer index.
//-----------------------------------------------------------------------------
static uint16 rvcdgn_AppendString
(
	char*       pu8BufferData,  // in/out:  Buffer data
	uint16      u16BufferIndex, // in:      Buffer index
	uint16      u16BufferSize,  // in:      Buffer size
	const char* pcString        // in:      Data to append
)
{
	uint16 u16Size    = strlen((char*)pcString);
	uint16 u16SizeMax = u16BufferSize - u16BufferIndex;
	if ( u16Size > u16SizeMax )
	{
		u16Size = u16SizeMax;
	}
	(void)memcpy(&pu8BufferData[u16BufferIndex], pcString, u16Size);
	return u16BufferIndex+u16Size;
}

