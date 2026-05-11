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
static void rvcdgn_SetString(char *pu8BufferData, uint16 u16BufferSize, const char *pcString);
static uint16 rvcdgn_AppendString(char *pu8BufferData, uint16 u16BufferIndex, uint16 u16BufferSize, const char *pcString);

//-----------------------------------------------------------------------------
// Function:   RVCDGN_DGN_130762_Stuff
//-----------------------------------------------------------------------------
// Purpose:    Stuff DM_RV parameters into a frame buffer.
//-----------------------------------------------------------------------------
// Notes:      None.
//-----------------------------------------------------------------------------
// Parameters: See below
//-----------------------------------------------------------------------------
// Return:     None.
//-----------------------------------------------------------------------------
extern void RVCDGN_DGN_130762_Stuff(
    uint8 *pu8Dest,
    RVCDGN_zDGN_130762 *pzSrc)
{
    // Clear
    (void)memset(pu8Dest, 0, RVCDGN_DGN_130762_SIZE);

    // Fill
    //           dest                   source          start  size
    BITS_TO_DGN(pu8Dest[0], pzSrc->u2OperatingStatus1, 0, 2);
    BITS_TO_DGN(pu8Dest[0], pzSrc->u2OperatingStatus2, 2, 2);
    BITS_TO_DGN(pu8Dest[0], pzSrc->u2YellowLampStatus, 4, 2);
    BITS_TO_DGN(pu8Dest[0], pzSrc->u2RedLampStatus, 6, 2);
    BITS_TO_DGN(pu8Dest[1], pzSrc->u8DSA, 0, 8);
    BITS_TO_DGN(pu8Dest[2], pzSrc->u8SPN_MSB, 0, 8);
    BITS_TO_DGN(pu8Dest[3], pzSrc->u8SPN_ISB, 0, 8);
    BITS_TO_DGN(pu8Dest[4], pzSrc->u3SPN_LSB, 5, 3);
    BITS_TO_DGN(pu8Dest[4], pzSrc->u5FMI, 0, 5);
    BITS_TO_DGN(pu8Dest[5], pzSrc->u7OccurCount, 0, 7);
    BITS_TO_DGN(pu8Dest[5], 1, 7, 1);
    BITS_TO_DGN(pu8Dest[6], pzSrc->u8DSAExtension, 0, 8);
    BITS_TO_DGN(pu8Dest[7], pzSrc->u4BankSelect, 0, 4);
    BITS_TO_DGN(pu8Dest[7], 0xF, 4, 8);
}

//-----------------------------------------------------------------------------
// Function:   RVCDGN_DGN_130762_Extract
//-----------------------------------------------------------------------------
// Purpose:    Extraction of the DM_RV parameter
//-----------------------------------------------------------------------------
// Notes:      None.
//-----------------------------------------------------------------------------
// Parameters: See below
//-----------------------------------------------------------------------------
// Return:     None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130762_Extract(
    RVCDGN_zDGN_130762 *pzDest,
    uint8 *pu8Src)
{
    //           dest                           src                   start  size
    DGN_TO_BITS(pzDest->u2OperatingStatus1, pu8Src[0], 0, 2);
    DGN_TO_BITS(pzDest->u2OperatingStatus2, pu8Src[0], 2, 2);
    DGN_TO_BITS(pzDest->u2YellowLampStatus, pu8Src[0], 4, 2);
    DGN_TO_BITS(pzDest->u2RedLampStatus, pu8Src[0], 6, 2);
    DGN_TO_BITS(pzDest->u8DSA, pu8Src[1], 0, 8);
    DGN_TO_BITS(pzDest->u8SPN_MSB, pu8Src[2], 0, 8);
    DGN_TO_BITS(pzDest->u8SPN_ISB, pu8Src[3], 0, 8);
    DGN_TO_BITS(pzDest->u3SPN_LSB, pu8Src[4], 5, 3);
    DGN_TO_BITS(pzDest->u5FMI, pu8Src[4], 0, 5);
    DGN_TO_BITS(pzDest->u7OccurCount, pu8Src[5], 0, 7);
    DGN_TO_BITS(pzDest->u8DSAExtension, pu8Src[6], 0, 8);
    DGN_TO_BITS(pzDest->u4BankSelect, pu8Src[7], 0, 4);
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
void RVCDGN_DGN_59904_Stuff(
    uint8 *pu8Dest,           // out: Destination buffer.
    RVCDGN_zDGN_59904 *pzSrc  // in : Source structure.
)
{
    // Clear
    (void)memset(pu8Dest, 0, RVCDGN_DGN_59904_SIZE);

    // Fill
    pu8Dest[0] = (pzSrc->u32DGN >> 0) & 0xFF;
    pu8Dest[1] = (pzSrc->u32DGN >> 8) & 0xFF;
    pu8Dest[2] = (pzSrc->u32DGN >> 16) & 0xFF;
    pu8Dest[3] = 0xFF;
    pu8Dest[4] = 0xFF;
    pu8Dest[5] = 0xFF;
    pu8Dest[6] = 0xFF;
    pu8Dest[7] = pzSrc->da;  // used as temp param for requested destination address
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
void RVCDGN_DGN_60928_Extract(
    RVCDGN_zDGN_60928 *pzDgn60928,  // out: Extracted parameters
    uint8 *pu8Data                  // in : DGN frame buffer
)
{
    // Special case: 21 bit extraction
    pzDgn60928->u32UniqueNumber = pu8Data[0];
    pzDgn60928->u32UniqueNumber |= ((uint32)pu8Data[1] << 8) & 0x0000FF00;
    pzDgn60928->u32UniqueNumber |= ((uint32)pu8Data[2] << 16) & 0x001F0000;

    // Special case: 11 bit extraction
    pzDgn60928->u16ManufCode = (pu8Data[2] >> 5) & 0x07;
    pzDgn60928->u16ManufCode |= ((uint16)pu8Data[3] << 3) & 0x07F8;

    //           dest                                 src             start,  size
    DGN_TO_BITS(pzDgn60928->u3NodeInstance, pu8Data[4], 0, 3);
    DGN_TO_BITS(pzDgn60928->u5FuncInstance, pu8Data[4], 3, 5);
    DGN_TO_BITS(pzDgn60928->u8Function, pu8Data[5], 0, 8);
    DGN_TO_BITS(pzDgn60928->u1Reserved, pu8Data[6], 0, 1);
    DGN_TO_BITS(pzDgn60928->u7VehicleSystem, pu8Data[6], 1, 7);
    DGN_TO_BITS(pzDgn60928->u4VehicleSystemInstance, pu8Data[7], 0, 4);
    DGN_TO_BITS(pzDgn60928->u3IndustryGroup, pu8Data[7], 4, 3);
    DGN_TO_BITS(pzDgn60928->u1ArbAddrCapable, pu8Data[7], 7, 1);
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
void RVCDGN_DGN_60928_Stuff(
    uint8 *pu8Data,                // out: DGN frame buffer
    RVCDGN_zDGN_60928 *pzDgn60928  // in : Parameters to stuff
)
{
    // Clear
    (void)memset(pu8Data, 0, RVCDGN_DGN_60928_SIZE);

    // Special case: 21 bit stuffing
    pu8Data[0] = (uint8)(pzDgn60928->u32UniqueNumber);
    pu8Data[1] |= (uint8)(pzDgn60928->u32UniqueNumber >> 8);
    pu8Data[2] |= (uint8)((pzDgn60928->u32UniqueNumber >> 16) & 0x1F);

    // Special case: 11 bit stuffing
    pu8Data[2] |= (uint8)((pzDgn60928->u16ManufCode << 5) & 0xE0);
    pu8Data[3] = (uint8)(pzDgn60928->u16ManufCode >> 3);

    //           dest         source                            start    size
    BITS_TO_DGN(pu8Data[4], pzDgn60928->u3NodeInstance, 0, 3);
    BITS_TO_DGN(pu8Data[4], pzDgn60928->u5FuncInstance, 3, 5);
    BITS_TO_DGN(pu8Data[5], pzDgn60928->u8Function, 0, 8);
    BITS_TO_DGN(pu8Data[6], pzDgn60928->u1Reserved, 0, 1);
    BITS_TO_DGN(pu8Data[6], pzDgn60928->u7VehicleSystem, 1, 7);
    BITS_TO_DGN(pu8Data[7], pzDgn60928->u4VehicleSystemInstance, 0, 4);
    BITS_TO_DGN(pu8Data[7], pzDgn60928->u3IndustryGroup, 4, 3);
    BITS_TO_DGN(pu8Data[7], pzDgn60928->u1ArbAddrCapable, 7, 1);
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
void RVCDGN_DGN_65242_Extract(
    RVCDGN_zDGN_65242 *pzDest,  // out: Destination structure
    uint8 *pu8Src,              // in:  Source buffer (will overwrite separator)
    uint16 u16Size              // in:  Source size
)
{
    uint16 u16Index, u16FieldId, u16FieldStart, u16FieldEnd, u16FieldSize;
    char *pcFieldData;

    // Clear
    memset(pzDest, 0, sizeof(RVCDGN_zDGN_65242));

    // Scan
    u16FieldId = 0;
    u16FieldStart = 1;
    u16FieldEnd = u16FieldStart;
    for (u16Index = 1; u16Index < u16Size; u16Index++)
    {
        // End of field?
        if (pu8Src[u16Index] == '*')
        {
            u16FieldEnd = u16Index;
        }
        else if ((u16Index + 1) == u16Size)
        {
            u16FieldEnd = u16Index + 1;
        }

        // New field?
        if (u16FieldEnd > u16FieldStart)
        {
            // Select field
            switch (u16FieldId)
            {
            case 0:
                pcFieldData = &pzDest->cId1[0];
                break;
            case 1:
                pcFieldData = &pzDest->cId2[0];
                break;
            case 2:
                pcFieldData = &pzDest->cId3[0];
                break;
            case 3:
                pcFieldData = &pzDest->cId4[0];
                break;
            default:
                pcFieldData = NULL;
                break;
            }

            // Fill field
            if (pcFieldData != NULL)
            {
                u16FieldSize = u16FieldEnd - u16FieldStart;
                if (u16FieldSize > (RVCDGN_DGN_65242_FIELD_SIZE - 1))
                {
                    u16FieldSize = (RVCDGN_DGN_65242_FIELD_SIZE - 1);
                }
                (void)memcpy(pcFieldData, &pu8Src[u16FieldStart], u16FieldSize);
                pcFieldData = NULL;
            }

            // Next field
            u16FieldId++;
            u16FieldStart = u16Index + 1;
            u16FieldEnd = u16FieldStart;
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
void RVCDGN_DGN_65242_Set(
    RVCDGN_zDGN_65242 *pzDgn65242,  // out:  Destination buffer
    const char *pcId1,              // in:   Identifier #1
    const char *pcId2,              // in:   Identifier #2
    const char *pcId3,              // in:   Identifier #3
    const char *pcId4               // in:   Identifier #4
)
{
    rvcdgn_SetString(pzDgn65242->cId1, sizeof(pzDgn65242->cId1), pcId1);
    rvcdgn_SetString(pzDgn65242->cId2, sizeof(pzDgn65242->cId2), pcId2);
    rvcdgn_SetString(pzDgn65242->cId3, sizeof(pzDgn65242->cId3), pcId3);
    rvcdgn_SetString(pzDgn65242->cId4, sizeof(pzDgn65242->cId4), pcId4);
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
uint16 RVCDGN_DGN_65242_Stuff(
    uint8 *pu8Dest,           // In/out:  Destination buffer
    RVCDGN_zDGN_65242 *pzSrc  // in:      Source structure
)
{
    uint16 u16Index = 1;
    pu8Dest[0] = 4;
    u16Index = rvcdgn_AppendString((char *)pu8Dest, u16Index, RVCDGN_DGN_65242_SIZE - 1, pzSrc->cId1);
    u16Index = rvcdgn_AppendString((char *)pu8Dest, u16Index, RVCDGN_DGN_65242_SIZE, "*");
    u16Index = rvcdgn_AppendString((char *)pu8Dest, u16Index, RVCDGN_DGN_65242_SIZE - 1, pzSrc->cId2);
    u16Index = rvcdgn_AppendString((char *)pu8Dest, u16Index, RVCDGN_DGN_65242_SIZE, "*");
    u16Index = rvcdgn_AppendString((char *)pu8Dest, u16Index, RVCDGN_DGN_65242_SIZE - 1, pzSrc->cId3);
    u16Index = rvcdgn_AppendString((char *)pu8Dest, u16Index, RVCDGN_DGN_65242_SIZE, "*");
    u16Index = rvcdgn_AppendString((char *)pu8Dest, u16Index, RVCDGN_DGN_65242_SIZE - 1, pzSrc->cId4);
    u16Index = rvcdgn_AppendString((char *)pu8Dest, u16Index, RVCDGN_DGN_65242_SIZE, "*");
    while (u16Index < RVCDGN_SINGLE_FRAME_SIZE)
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
void RVCDGN_DGN_65259_Extract(
    RVCDGN_zDGN_65259 *pzDest,  // out: Destination structure
    uint8 *pu8Src,              // in:  Source buffer (will overwrite separator)
    uint16 u16Size              // in:  Source size
)
{
    uint16 u16Index, u16FieldId, u16FieldStart, u16FieldEnd, u16FieldSize;
    char *pcFieldData;

    // Clear
    memset(pzDest, 0, sizeof(RVCDGN_zDGN_65259));
    uint8_t occurrence_asterisk = 0;

    /* Per RV-C spec, there should be three asterisks even if the fields are empty,
       but some devices are not following this strictly, so the additional asterisks
       are replaced with dashes in order to not lose device information */

    for (int i = 0; i < u16Size; i++)
    {
        if (pu8Src[i] == '*')
        {
            if (occurrence_asterisk < 3)
            {
                occurrence_asterisk++;
            }
            else
            {
                pu8Src[i] = '-';
            }
        }
    }

    if (occurrence_asterisk == 3)
    {
        // Scan
        u16FieldId = 0;
        u16FieldStart = 0;
        u16FieldEnd = u16FieldStart;
        for (u16Index = 0; u16Index < u16Size; u16Index++)
        {
            // End of field?
            if (pu8Src[u16Index] == '*')
            {
                u16FieldEnd = u16Index;
            }
            else if ((u16Index + 1) == u16Size)
            {
                u16FieldEnd = u16Index + 1;
            }

            // New field?
            if (u16FieldEnd > u16FieldStart)
            {
                // Select field
                switch (u16FieldId)
                {
                case 0:
                    pcFieldData = &pzDest->cMake[0];
                    break;
                case 1:
                    pcFieldData = &pzDest->cModel[0];
                    break;
                case 2:
                    pcFieldData = &pzDest->cSerialNumber[0];
                    break;
                case 3:
                    pcFieldData = &pzDest->cUnitNumber[0];
                    break;
                default:
                    pcFieldData = NULL;
                    break;
                }

                // Fill field
                if (pcFieldData != NULL)
                {
                    u16FieldSize = u16FieldEnd - u16FieldStart;
                    if (u16FieldSize > (RVCDGN_DGN_65259_FIELD_SIZE - 1))
                    {
                        u16FieldSize = (RVCDGN_DGN_65259_FIELD_SIZE - 1);
                    }
                    (void)memcpy(pcFieldData, &pu8Src[u16FieldStart], u16FieldSize);
                    pcFieldData = NULL;
                }

                // Next field
                u16FieldId++;
                u16FieldStart = u16Index + 1;
                u16FieldEnd = u16FieldStart;
            }
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
void RVCDGN_DGN_65259_Set(
    RVCDGN_zDGN_65259 *pzDgn65259,  // out:  Destination buffer
    const char *pcMake,             // in:   Make string
    const char *pcModel,            // in:   Model string
    const char *pcSerialNumber,     // in:   Serial number string
    const char *pcUnitNumber        // in:   Unit number string
)
{
    rvcdgn_SetString(pzDgn65259->cMake, sizeof(pzDgn65259->cMake), pcMake);
    rvcdgn_SetString(pzDgn65259->cModel, sizeof(pzDgn65259->cModel), pcModel);
    rvcdgn_SetString(pzDgn65259->cSerialNumber, sizeof(pzDgn65259->cSerialNumber), pcSerialNumber);
    rvcdgn_SetString(pzDgn65259->cUnitNumber, sizeof(pzDgn65259->cUnitNumber), pcUnitNumber);
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
uint16 RVCDGN_DGN_65259_Stuff(
    uint8 *pu8Dest,           // In/out:  Destination buffer
    RVCDGN_zDGN_65259 *pzSrc  // in:      Source structure
)
{
    uint16 u16Index = 0;
    u16Index = rvcdgn_AppendString((char *)pu8Dest, u16Index, RVCDGN_DGN_65259_SIZE - 1, pzSrc->cMake);
    u16Index = rvcdgn_AppendString((char *)pu8Dest, u16Index, RVCDGN_DGN_65259_SIZE, "*");
    u16Index = rvcdgn_AppendString((char *)pu8Dest, u16Index, RVCDGN_DGN_65259_SIZE - 1, pzSrc->cModel);
    u16Index = rvcdgn_AppendString((char *)pu8Dest, u16Index, RVCDGN_DGN_65259_SIZE, "*");
    u16Index = rvcdgn_AppendString((char *)pu8Dest, u16Index, RVCDGN_DGN_65259_SIZE - 1, pzSrc->cSerialNumber);
    u16Index = rvcdgn_AppendString((char *)pu8Dest, u16Index, RVCDGN_DGN_65259_SIZE, "*");
    u16Index = rvcdgn_AppendString((char *)pu8Dest, u16Index, RVCDGN_DGN_65259_SIZE - 1, pzSrc->cUnitNumber);

    while (u16Index < RVCDGN_SINGLE_FRAME_SIZE)
    {
        pu8Dest[u16Index++] = 0;
    }
    pu8Dest[u16Index] = 0;  // Last null. Allocate one byte more than RVCDGN_DGN_65259_SIZE
    return u16Index;
}

void RVCDGN_DGN_59392_Extract(RVCDGN_zDGN_59392 *pzDgn, uint8 *pu8Data)
{
    DGN_TO_BITS(pzDgn->u8Code, pu8Data[0], 0, 8);
    DGN_TO_BITS(pzDgn->u8Instance, pu8Data[1], 0, 8);
    DGN_TO_BITS(pzDgn->u4instanceBank, pu8Data[2], 0, 4);
    DGN_TO_BITS(pzDgn->u8SourceAddress, pu8Data[4], 0, 8);
    pzDgn->u24Dgn = ((uint32)pu8Data[7] << 16) | ((uint32)pu8Data[6] << 8) | (uint32)pu8Data[5];
}

// extern void RVCDGN_DGN_98048_Extract(RVCDGN_zDGN_98048 *pzDgn, uint8 *pu8Data); Not implemented

//-----------------------------------------------------------------------------
// Function:   RVCDGN_DGN_98048_Stuff (General reset)
//-----------------------------------------------------------------------------
// Purpose:    Stuff DGN 98048 parameters into a frame buffer.
//-----------------------------------------------------------------------------
// Notes:      None.
//-----------------------------------------------------------------------------
// Parameters: See below
//-----------------------------------------------------------------------------
// Return:     None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_98048_Stuff(uint8 *pu8Data, RVCDGN_zDGN_98048 *pzDgn)
{
    memset(pu8Data, 0, RVCDGN_DGN_98048_SIZE);
    //           dest - src - start - size
    BITS_TO_DGN(pu8Data[0], pzDgn->u2Reboot, 0, 2);
    BITS_TO_DGN(pu8Data[0], pzDgn->u2ClearFaults, 2, 2);
    BITS_TO_DGN(pu8Data[0], pzDgn->u2ResetToDefault, 4, 2);
    BITS_TO_DGN(pu8Data[0], pzDgn->u2ResetStatistics, 6, 2);
    BITS_TO_DGN(pu8Data[1], pzDgn->u2TestMode, 0, 2);
    BITS_TO_DGN(pu8Data[1], pzDgn->u2ResetToOEMSettings, 2, 2);
    BITS_TO_DGN(pu8Data[1], pzDgn->u2RebootEnterBootloader, 4, 2);
    BITS_TO_DGN(pu8Data[1], 0xFF, 6, 2);
    pu8Data[2] = 0xFF;
    pu8Data[3] = 0xFF;
    pu8Data[4] = 0xFF;
    pu8Data[5] = 0xFF;
    pu8Data[6] = 0xFF;
    pu8Data[7] = pzDgn->da;  // temp storage
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
void RVCDGN_DGN_130972_Extract(
    RVCDGN_zDGN_130972 *pzDgn,  // out: Extracted parameters
    uint8 *pu8Data              // in : DGN frame buffer
)
{
    //           dest                            src         start   size
    DGN_TO_BITS(pzDgn->u8Instance, pu8Data[0], 0, 8);
    DGN_TO_WORD(pzDgn->u16AmbientTemp, pu8Data[1]);
    DGN_TO_BITS(pzDgn->u8Reserved1, pu8Data[3], 0, 8);
    DGN_TO_WORD(pzDgn->u16Reserved2, pu8Data[4]);
    DGN_TO_WORD(pzDgn->u16Reserved3, pu8Data[6]);
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
void RVCDGN_DGN_130972_Stuff(
    uint8 *pu8Data,            // out: DGN frame buffer
    RVCDGN_zDGN_130972 *pzDgn  // in : Parameters to stuff
)
{
    // Clear
    (void)memset(pu8Data, 0, RVCDGN_DGN_130972_SIZE);

    //           dest         source                start  size
    BITS_TO_DGN(pu8Data[0], pzDgn->u8Instance, 0, 8);
    WORD_TO_DGN(pu8Data[1], pzDgn->u16AmbientTemp);
    BITS_TO_DGN(pu8Data[3], 0xFF, 0, 8);
    WORD_TO_DGN(pu8Data[4], 0xFFFF);
    WORD_TO_DGN(pu8Data[6], 0xFFFF);
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
void RVCDGN_DGN_131071_Extract(
    RVCDGN_zDGN_131071 *pzDgn131071,  // out: Extracted parameters
    uint8 *pu8Data                    // in : DGN frame buffer
)
{
    //           dest                            src         start   size
    DGN_TO_BITS(pzDgn131071->u8Year, pu8Data[0], 0, 8);
    DGN_TO_BITS(pzDgn131071->u8Month, pu8Data[1], 0, 8);
    DGN_TO_BITS(pzDgn131071->u8Day, pu8Data[2], 0, 8);
    DGN_TO_BITS(pzDgn131071->u8DayOfWeek, pu8Data[3], 0, 8);
    DGN_TO_BITS(pzDgn131071->u8Hour, pu8Data[4], 0, 8);
    DGN_TO_BITS(pzDgn131071->u8Minute, pu8Data[5], 0, 8);
    DGN_TO_BITS(pzDgn131071->u8Second, pu8Data[6], 0, 8);
    DGN_TO_BITS(pzDgn131071->u8TimeZone, pu8Data[7], 0, 8);
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
void RVCDGN_DGN_131071_Stuff(
    uint8 *pu8Data,            // out: DGN frame buffer
    RVCDGN_zDGN_131071 *pzDgn  // in : Parameters to stuff
)
{
    // Clear
    (void)memset(pu8Data, 0, RVCDGN_DGN_131071_SIZE);

    //           dest          source                start   size
    BITS_TO_DGN(pu8Data[0], pzDgn->u8Year, 0, 8);
    BITS_TO_DGN(pu8Data[1], pzDgn->u8Month, 0, 8);
    BITS_TO_DGN(pu8Data[2], pzDgn->u8Day, 0, 8);
    BITS_TO_DGN(pu8Data[3], pzDgn->u8DayOfWeek, 0, 8);
    BITS_TO_DGN(pu8Data[4], pzDgn->u8Hour, 0, 8);
    BITS_TO_DGN(pu8Data[5], pzDgn->u8Minute, 0, 8);
    BITS_TO_DGN(pu8Data[6], pzDgn->u8Second, 0, 8);
    BITS_TO_DGN(pu8Data[7], pzDgn->u8TimeZone, 0, 8);
}
//-----------------------------------------------------------------------------
// Function:   RVCDGN_DGN_130530_Extract
//-----------------------------------------------------------------------------
// Purpose:    Extract parameters from DGN 130530 frame buffer.
//-----------------------------------------------------------------------------
// Notes:      None.
//-----------------------------------------------------------------------------
// Parameters: See below
//-----------------------------------------------------------------------------
// Return:     None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130530_Extract(
    RVCDGN_zDGN_130530 *pzDgn,  // out: Extracted parameters
    uint8 *pu8Data              // in : DGN frame buffer
)
{
    //           dest                            src         start   size
    DGN_TO_BITS(pzDgn->u8Instance, pu8Data[0], 0, 8);
    DGN_TO_BITS(pzDgn->u8DomeCommand, pu8Data[1], 0, 8);
    DGN_TO_BITS(pzDgn->u8DesiredDomePosition, pu8Data[2], 0, 8);
    DGN_TO_BITS(pzDgn->u2RainSensorOverride, pu8Data[3], 0, 2);
    DGN_TO_BITS(pzDgn->u2SetpointCtrldDome, pu8Data[3], 2, 2);
    DGN_TO_BITS(pzDgn->u2AutoCloseDomeonFanOff, pu8Data[3], 4, 2);
    DGN_TO_BITS(pzDgn->u2AutoFanOffonDomeClose, pu8Data[3], 6, 2);
    DGN_TO_BITS(pzDgn->u2FanSpeedIncDec, pu8Data[4], 0, 2);
    DGN_TO_BITS(pzDgn->u6FanSpeedIncDecStep, pu8Data[4], 2, 6);
    DGN_TO_BITS(pzDgn->u8Reserved1[0], pu8Data[5], 0, 8);
    DGN_TO_BITS(pzDgn->u8Reserved1[1], pu8Data[6], 0, 8);
    DGN_TO_BITS(pzDgn->u8Reserved1[2], pu8Data[7], 0, 8);
}
void RVCDGN_DGN_130530_Stuff(uint8 *pu8Data, RVCDGN_zDGN_130530 *pzDgn)
{
    // Clear
    (void)memset(pu8Data, 0, RVCDGN_DGN_130530_SIZE);

    //           dest         source                        start  size
    BITS_TO_DGN(pu8Data[0], pzDgn->u8Instance, 0, 8);
    BITS_TO_DGN(pu8Data[1], pzDgn->u8DomeCommand, 0, 8);
    BITS_TO_DGN(pu8Data[2], pzDgn->u8DesiredDomePosition, 0, 8);
    BITS_TO_DGN(pu8Data[3], pzDgn->u2RainSensorOverride, 0, 2);
    BITS_TO_DGN(pu8Data[3], pzDgn->u2SetpointCtrldDome, 2, 2);
    BITS_TO_DGN(pu8Data[3], pzDgn->u2AutoCloseDomeonFanOff, 4, 2);
    BITS_TO_DGN(pu8Data[3], pzDgn->u2AutoFanOffonDomeClose, 6, 2);
    BITS_TO_DGN(pu8Data[4], pzDgn->u2FanSpeedIncDec, 0, 2);
    BITS_TO_DGN(pu8Data[4], pzDgn->u6FanSpeedIncDecStep, 2, 6);
    BITS_TO_DGN(pu8Data[5], pzDgn->u8Reserved1[0], 0, 8);
    BITS_TO_DGN(pu8Data[6], pzDgn->u8Reserved1[1], 0, 8);
    BITS_TO_DGN(pu8Data[7], pzDgn->u8Reserved1[2], 0, 8);
}

//-----------------------------------------------------------------------------
// Function:   RVCDGN_DGN_130531_Stuff
//-----------------------------------------------------------------------------
// Purpose:    Stuff DGN 130531 parameters into a frame buffer.
//-----------------------------------------------------------------------------
// Notes:      None.
//-----------------------------------------------------------------------------
// Parameters: See below
//-----------------------------------------------------------------------------
// Return:     None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130531_Stuff(
    uint8 *pu8Data,            // out: DGN frame buffer
    RVCDGN_zDGN_130531 *pzDgn  // in : Parameters to stuff
)
{
    // Clear
    (void)memset(pu8Data, 0, RVCDGN_DGN_130531_SIZE);

    //           dest         source                        start  size
    BITS_TO_DGN(pu8Data[0], pzDgn->u8Instance, 0, 8);
    BITS_TO_DGN(pu8Data[1], pzDgn->u8DomeMode, 0, 8);
    BITS_TO_DGN(pu8Data[2], pzDgn->u8DomePosition, 0, 8);
    BITS_TO_DGN(pu8Data[3], pzDgn->u2RainSensor, 0, 2);
    BITS_TO_DGN(pu8Data[3], pzDgn->u2RainSensorOverride, 2, 2);
    BITS_TO_DGN(pu8Data[3], pzDgn->u2SetpointCtrldDomeState, 4, 2);
    BITS_TO_DGN(pu8Data[3], pzDgn->u2AutoCloseDomeonFanOff, 6, 2);
    BITS_TO_DGN(pu8Data[4], pzDgn->u2AutoFanOffonDomeClose, 0, 2);
    BITS_TO_DGN(pu8Data[4], pzDgn->u6FanStepsSupported, 2, 6);
    BITS_TO_DGN(pu8Data[5], pzDgn->u8Reserved1[0], 0, 8);
    BITS_TO_DGN(pu8Data[6], pzDgn->u8Reserved1[1], 0, 8);
    BITS_TO_DGN(pu8Data[7], pzDgn->u8Reserved1[2], 0, 8);
}
void RVCDGN_DGN_130531_Extract(RVCDGN_zDGN_130531 *pzDgn, uint8 *pu8Data)
{
    //           dest                            src         start   size
    DGN_TO_BITS(pzDgn->u8Instance, pu8Data[0], 0, 8);
    DGN_TO_BITS(pzDgn->u8DomeMode, pu8Data[1], 0, 8);
    DGN_TO_BITS(pzDgn->u8DomePosition, pu8Data[2], 0, 8);
    DGN_TO_BITS(pzDgn->u2RainSensor, pu8Data[3], 0, 2);
    DGN_TO_BITS(pzDgn->u2RainSensorOverride, pu8Data[3], 2, 2);
    DGN_TO_BITS(pzDgn->u2SetpointCtrldDomeState, pu8Data[3], 4, 2);
    DGN_TO_BITS(pzDgn->u2AutoCloseDomeonFanOff, pu8Data[3], 6, 2);
    DGN_TO_BITS(pzDgn->u2AutoFanOffonDomeClose, pu8Data[4], 0, 2);
    DGN_TO_BITS(pzDgn->u6FanStepsSupported, pu8Data[4], 2, 6);
    DGN_TO_BITS(pzDgn->u8Reserved1[0], pu8Data[5], 0, 8);
    DGN_TO_BITS(pzDgn->u8Reserved1[1], pu8Data[6], 0, 8);
    DGN_TO_BITS(pzDgn->u8Reserved1[2], pu8Data[7], 0, 8);
}

//-----------------------------------------------------------------------------
// Function:   RVCDGN_DGN_130726_Extract
//-----------------------------------------------------------------------------
// Purpose:    Extract parameters from DGN 130726 frame buffer.
//-----------------------------------------------------------------------------
// Notes:      None.
//-----------------------------------------------------------------------------
// Parameters: See below
//-----------------------------------------------------------------------------
// Return:     None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130726_Extract(
    RVCDGN_zDGN_130726 *pzDgn,  // out: Extracted parameters
    uint8 *pu8Data              // in : DGN frame buffer
)
{
    //           dest                            src         start   size
    DGN_TO_BITS(pzDgn->u8Instance, pu8Data[0], 0, 8);
    DGN_TO_BITS(pzDgn->u2SystemStatus, pu8Data[1], 0, 2);
    DGN_TO_BITS(pzDgn->u2FanMode, pu8Data[1], 2, 2);
    DGN_TO_BITS(pzDgn->u2SpeedMode, pu8Data[1], 4, 2);
    DGN_TO_BITS(pzDgn->u2Light, pu8Data[1], 6, 2);
    DGN_TO_BITS(pzDgn->u8FanSpeedSetting, pu8Data[2], 0, 8);
    DGN_TO_BITS(pzDgn->u2WindDirectionSwitch, pu8Data[3], 0, 2);
    DGN_TO_BITS(pzDgn->u4Reserved1, pu8Data[3], 2, 4);
    DGN_TO_BITS(pzDgn->u2Reserved2, pu8Data[3], 6, 2);
    DGN_TO_WORD(pzDgn->u16AmbientTemp, pu8Data[4]);
    DGN_TO_WORD(pzDgn->u16SetPoint, pu8Data[6]);
}
void RVCDGN_DGN_130726_Stuff(uint8 *pu8Data, RVCDGN_zDGN_130726 *pzDgn)
{
    // Clear
    (void)memset(pu8Data, 0, RVCDGN_DGN_130726_SIZE);

    //           dest         source                        start  size
    BITS_TO_DGN(pu8Data[0], pzDgn->u8Instance, 0, 8);
    BITS_TO_DGN(pu8Data[1], pzDgn->u2SystemStatus, 0, 2);
    BITS_TO_DGN(pu8Data[1], pzDgn->u2FanMode, 2, 2);
    BITS_TO_DGN(pu8Data[1], pzDgn->u2SpeedMode, 4, 2);
    BITS_TO_DGN(pu8Data[1], pzDgn->u2Light, 6, 2);
    BITS_TO_DGN(pu8Data[2], pzDgn->u8FanSpeedSetting, 0, 8);
    BITS_TO_DGN(pu8Data[3], pzDgn->u2WindDirectionSwitch, 0, 2);
    BITS_TO_DGN(pu8Data[3], pzDgn->u4Reserved1, 2, 4);
    BITS_TO_DGN(pu8Data[3], pzDgn->u2Reserved2, 6, 2);
    WORD_TO_DGN(pu8Data[4], pzDgn->u16AmbientTemp);
    WORD_TO_DGN(pu8Data[6], pzDgn->u16SetPoint);
}

//-----------------------------------------------------------------------------
// Function:   RVCDGN_DGN_130727_Stuff
//-----------------------------------------------------------------------------
// Purpose:    Stuff DGN 130727 parameters into a frame buffer.
//-----------------------------------------------------------------------------
// Notes:      None.
//-----------------------------------------------------------------------------
// Parameters: See below
//-----------------------------------------------------------------------------
// Return:     None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130727_Stuff(
    uint8 *pu8Data,            // out: DGN frame buffer
    RVCDGN_zDGN_130727 *pzDgn  // in : Parameters to stuff
)
{
    // Clear
    (void)memset(pu8Data, 0, RVCDGN_DGN_130727_SIZE);

    //           dest         source                        start  size
    BITS_TO_DGN(pu8Data[0], pzDgn->u8Instance, 0, 8);
    BITS_TO_DGN(pu8Data[1], pzDgn->u2SystemStatus, 0, 2);
    BITS_TO_DGN(pu8Data[1], pzDgn->u2FanMode, 2, 2);
    BITS_TO_DGN(pu8Data[1], pzDgn->u2SpeedMode, 4, 2);
    BITS_TO_DGN(pu8Data[1], pzDgn->u2Light, 6, 2);
    BITS_TO_DGN(pu8Data[2], pzDgn->u8FanSpeedSetting, 0, 8);
    BITS_TO_DGN(pu8Data[3], pzDgn->u2WindDirectionSwitch, 0, 2);
    BITS_TO_DGN(pu8Data[3], pzDgn->u4DomePosition, 2, 4);
    BITS_TO_DGN(pu8Data[3], pzDgn->u2Reserved1, 6, 2);
    WORD_TO_DGN(pu8Data[4], pzDgn->u16AmbientTemp);
    WORD_TO_DGN(pu8Data[6], pzDgn->u16SetPoint);
}
void RVCDGN_DGN_130727_Extract(RVCDGN_zDGN_130727 *pzDgn, uint8 *pu8Data)
{
    //           dest                            src         start   size
    DGN_TO_BITS(pzDgn->u8Instance, pu8Data[0], 0, 8);
    DGN_TO_BITS(pzDgn->u2SystemStatus, pu8Data[1], 0, 2);
    DGN_TO_BITS(pzDgn->u2FanMode, pu8Data[1], 2, 2);
    DGN_TO_BITS(pzDgn->u2SpeedMode, pu8Data[1], 4, 2);
    DGN_TO_BITS(pzDgn->u2Light, pu8Data[1], 6, 2);
    DGN_TO_BITS(pzDgn->u8FanSpeedSetting, pu8Data[2], 0, 8);
    DGN_TO_BITS(pzDgn->u2WindDirectionSwitch, pu8Data[3], 0, 2);
    DGN_TO_BITS(pzDgn->u4DomePosition, pu8Data[3], 2, 4);
    DGN_TO_BITS(pzDgn->u2Reserved1, pu8Data[3], 6, 2);
    DGN_TO_WORD(pzDgn->u16AmbientTemp, pu8Data[4]);
    DGN_TO_WORD(pzDgn->u16SetPoint, pu8Data[6]);
}
//-----------------------------------------------------------------------------
// Function:    RVC_DGN_130514_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters in DGN 130514(REFRIGERATOR_COMMAND)
// pzDest: out: Destination structure
// pu8Src: in:  Source buffer
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVC_DGN_130514_Extract(RVCDGN_zDGN_130514 *pzDest, uint8 *pu8Src)
{
    //           dest                             src       start size
    DGN_TO_BITS(pzDest->u6Instance, pu8Src[0], 0, 6);
    DGN_TO_BITS(pzDest->u2Cavity, pu8Src[0], 6, 2);
    DGN_TO_BITS(pzDest->u2Light, pu8Src[1], 0, 2);
    DGN_TO_WORD(pzDest->u16SetTemperature, pu8Src[4]);
    DGN_TO_BITS(pzDest->u4FuelSource, pu8Src[6], 0, 4);
    DGN_TO_BITS(pzDest->u4RefrigeratorMode, pu8Src[6], 4, 4);
}
//-----------------------------------------------------------------------------
// Function:    RVC_DGN_130515_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters in DGN 130515(REFRIGERATOR_STATUS)
// pu8Dest: out:  Destination buffer
// pzSrc: in:   Source structure
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVC_DGN_130515_Stuff(uint8 *pu8Dest, RVCDGN_zDGN_130515 *pzSrc)
{
    // Clear
    (void)memset(pu8Dest, 0x00, RVC_DGN_130515_SIZE);

    // Fill
    //           dest             source                   start size
    BITS_TO_DGN(pu8Dest[0], pzSrc->u6Instance, 0, 6);
    BITS_TO_DGN(pu8Dest[0], pzSrc->u2Cavity, 6, 2);
    BITS_TO_DGN(pu8Dest[1], pzSrc->u2Light, 0, 2);
    BITS_TO_DGN(pu8Dest[1], pzSrc->u2DoorSwitch, 2, 2);
    WORD_TO_DGN(pu8Dest[2], pzSrc->u16CurrentTemperature);
    WORD_TO_DGN(pu8Dest[4], pzSrc->u16SetTemperature);
    BITS_TO_DGN(pu8Dest[6], pzSrc->u4FuelSource, 0, 4);
    BITS_TO_DGN(pu8Dest[6], pzSrc->u4RefrigeratorMode, 4, 4);
    BITS_TO_DGN(pu8Dest[7], pzSrc->u8CompressorSpeed, 0, 8);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_131069_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 131069
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_131069_Extract(RVCDGN_zDGN_131069 *pzDest, uint8 *pu8Src)
{
    //           dest                          src        start size
    DGN_TO_BITS(pzDest->u8DCInstance, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u8DevicePriority, pu8Src[1], 0, 8);
    DGN_TO_WORD(pzDest->u16DCVoltage, pu8Src[2]);
    DGN_TO_DWRD(pzDest->u32DCCurrent, pu8Src[4]);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_131068_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 131068
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_131068_Extract(RVCDGN_zDGN_131068 *pzDest, uint8 *pu8Src)
{
    //           dest                                src       start size
    DGN_TO_BITS(pzDest->u8DCInstance, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u8DevicePriority, pu8Src[1], 0, 8);
    DGN_TO_WORD(pzDest->u16SourceTemperature, pu8Src[2]);
    DGN_TO_BITS(pzDest->u8StateOfCharge, pu8Src[4], 0, 8);
    DGN_TO_WORD(pzDest->u16TimeRemaining, pu8Src[5]);
    DGN_TO_BITS(pzDest->u2TimeRemainingInterpr, pu8Src[7], 0, 2);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_131067_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 131067
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_131067_Extract(RVCDGN_zDGN_131067 *pzDest, uint8 *pu8Src)
{
    //           dest                                src       start size
    DGN_TO_BITS(pzDest->u8DCInstance, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u8DevicePriority, pu8Src[1], 0, 8);
    DGN_TO_BITS(pzDest->u8StateOfHealth, pu8Src[2], 0, 8);
    DGN_TO_WORD(pzDest->u16CapacityRemaining, pu8Src[3]);
    DGN_TO_BITS(pzDest->u8RelativeCapacity, pu8Src[5], 0, 8);
    DGN_TO_WORD(pzDest->u16ACRMSRipple, pu8Src[6]);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130761_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130761
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130761_Extract(RVCDGN_zDGN_130761 *pzDest, uint8 *pu8Src)
{
    //           dest                                src       start size
    DGN_TO_BITS(pzDest->u8DCInstance, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u8DevicePriority, pu8Src[1], 0, 8);
    DGN_TO_BITS(pzDest->u8DesiredChargeState, pu8Src[2], 0, 8);
    DGN_TO_WORD(pzDest->u16DesiredDCVoltage, pu8Src[3]);
    DGN_TO_WORD(pzDest->u16DesiredDCCurrent, pu8Src[5]);
    DGN_TO_BITS(pzDest->u8BatteryType, pu8Src[7], 0, 8);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130760_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130760
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130760_Extract(RVCDGN_zDGN_130760 *pzDest, uint8 *pu8Src)
{
    //           dest                          src        start size
    DGN_TO_BITS(pzDest->u8DCInstance, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u8DevicePriority, pu8Src[1], 0, 8);
    DGN_TO_DWRD(pzDest->u32HPDCVoltage, pu8Src[2]);
    // Bytes 6-7 are deprecated and ignored
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130759_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130759
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130759_Extract(RVCDGN_zDGN_130759 *pzDest, uint8 *pu8Src)
{
    //           dest                                src       start size
    DGN_TO_BITS(pzDest->u8DCInstance, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u8DevicePriority, pu8Src[1], 0, 8);
    DGN_TO_BITS(pzDest->u2HighVoltageLimitStatus, pu8Src[2], 0, 2);
    DGN_TO_BITS(pzDest->u2HighVoltageDisconnectStatus, pu8Src[2], 2, 2);
    DGN_TO_BITS(pzDest->u2LowVoltageLimitStatus, pu8Src[2], 4, 2);
    DGN_TO_BITS(pzDest->u2LowVoltageDisconnectStatus, pu8Src[2], 6, 2);
    DGN_TO_BITS(pzDest->u2LowSOCLimitStatus, pu8Src[3], 0, 2);
    DGN_TO_BITS(pzDest->u2LowSOCDisconnectStatus, pu8Src[3], 2, 2);
    DGN_TO_BITS(pzDest->u2LowDCSourceTempLimitStatus, pu8Src[3], 4, 2);
    DGN_TO_BITS(pzDest->u2LowDCSourceTempDisconnectStatus, pu8Src[3], 6, 2);
    DGN_TO_BITS(pzDest->u2HighDCSourceTempLimitStatus, pu8Src[4], 0, 2);
    DGN_TO_BITS(pzDest->u2HighDCSourceTempDisconnectStatus, pu8Src[4], 2, 2);
    DGN_TO_BITS(pzDest->u2HighCurrentDCSourceLimit, pu8Src[4], 4, 2);
    DGN_TO_BITS(pzDest->u2HighCurrentDCSourceDisconnect, pu8Src[4], 6, 2);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130732_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130732
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130732_Extract(RVCDGN_zDGN_130732 *pzDest, uint8 *pu8Src)
{
    //           dest                                src       start size
    DGN_TO_BITS(pzDest->u8DCInstance, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u8DevicePriority, pu8Src[1], 0, 8);
    DGN_TO_WORD(pzDest->u16TodayInputAmpHours, pu8Src[2]);
    DGN_TO_WORD(pzDest->u16TodayOutputAmpHours, pu8Src[4]);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130731_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130731
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130731_Extract(RVCDGN_zDGN_130731 *pzDest, uint8 *pu8Src)
{
    //           dest                                src       start size
    DGN_TO_BITS(pzDest->u8DCInstance, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u8DevicePriority, pu8Src[1], 0, 8);
    DGN_TO_WORD(pzDest->u16YesterdayInputAmpHours, pu8Src[2]);
    DGN_TO_WORD(pzDest->u16YesterdayOutputAmpHours, pu8Src[4]);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130730_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130730
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130730_Extract(RVCDGN_zDGN_130730 *pzDest, uint8 *pu8Src)
{
    //           dest                                src       start size
    DGN_TO_BITS(pzDest->u8DCInstance, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u8DevicePriority, pu8Src[1], 0, 8);
    DGN_TO_WORD(pzDest->u16DayBeforeYestInputAmpHours, pu8Src[2]);
    DGN_TO_WORD(pzDest->u16DayBeforeYestOutputAmpHours, pu8Src[4]);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130729_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130729
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130729_Extract(RVCDGN_zDGN_130729 *pzDest, uint8 *pu8Src)
{
    //           dest                                src       start size
    DGN_TO_BITS(pzDest->u8DCInstance, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u8DevicePriority, pu8Src[1], 0, 8);
    DGN_TO_WORD(pzDest->u16LastSevenDaysInputAmpHours, pu8Src[2]);
    DGN_TO_WORD(pzDest->u16LastSevenDaysOutputAmpHours, pu8Src[4]);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130725_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130725
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130725_Extract(RVCDGN_zDGN_130725 *pzDest, uint8 *pu8Src)
{
    //           dest                                src       start size
    DGN_TO_BITS(pzDest->u8DCInstance, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u8DevicePriority, pu8Src[1], 0, 8);
    DGN_TO_BITS(pzDest->u2DischargeStatus, pu8Src[2], 0, 2);
    DGN_TO_BITS(pzDest->u2ChargeStatus, pu8Src[2], 2, 2);
    DGN_TO_BITS(pzDest->u2ChargeDetected, pu8Src[2], 4, 2);
    DGN_TO_BITS(pzDest->u2ReserveStatus, pu8Src[2], 6, 2);
    DGN_TO_WORD(pzDest->u16FullCapacity, pu8Src[3]);
    DGN_TO_WORD(pzDest->u16DCPower, pu8Src[5]);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130552_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130552
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130552_Extract(RVCDGN_zDGN_130552 *pzDest, uint8 *pu8Src)
{
    //           dest                                src       start size
    DGN_TO_BITS(pzDest->u8DCInstance, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u8DevicePriority, pu8Src[1], 0, 8);
    DGN_TO_WORD(pzDest->u16Cycles, pu8Src[2]);
    DGN_TO_WORD(pzDest->u16DeepsetDischargeDepth, pu8Src[4]);
    DGN_TO_WORD(pzDest->u16AverageDischargeDepth, pu8Src[6]);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130535_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130535
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130535_Extract(RVCDGN_zDGN_130535 *pzDest, uint8 *pu8Src)
{
    //           dest                                src       start size
    DGN_TO_BITS(pzDest->u8DCInstance, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u8DevicePriority, pu8Src[1], 0, 8);
    DGN_TO_WORD(pzDest->u16LowestDCSourceVoltage, pu8Src[2]);
    DGN_TO_WORD(pzDest->u16HighestDCSourceVoltage, pu8Src[4]);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130551_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130551
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130551_Extract(RVCDGN_zDGN_130551 *pzDest, uint8_t *pu8Src)
{
    //           dest                        src       start size
    DGN_TO_BITS(pzDest->u8DCInstance, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u8PeukertExponent, pu8Src[1], 0, 8);
    DGN_TO_BITS(pzDest->u8TempCoefficient, pu8Src[2], 0, 8);
    DGN_TO_BITS(pzDest->u8ChargeEfficiency, pu8Src[3], 0, 8);
    DGN_TO_BITS(pzDest->u8TimeRemAveraging, pu8Src[4], 0, 8);
    DGN_TO_WORD(pzDest->u16FullCapacity, pu8Src[5]);
    DGN_TO_BITS(pzDest->u8TailCurrent, pu8Src[7], 0, 8);
}

/*
//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130551_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters to DGN 130551
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
*/
void RVCDGN_DGN_130551_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_130551 *pzSrc)
{
    // Clear buffer
    (void)memset(pu8Dest, 0, 8);

    // Stuff fields
    BITS_TO_DGN(pu8Dest[0], pzSrc->u8DCInstance, 0, 8);
    BITS_TO_DGN(pu8Dest[1], pzSrc->u8PeukertExponent, 0, 8);
    BITS_TO_DGN(pu8Dest[2], pzSrc->u8TempCoefficient, 0, 8);
    BITS_TO_DGN(pu8Dest[3], pzSrc->u8ChargeEfficiency, 0, 8);
    BITS_TO_DGN(pu8Dest[4], pzSrc->u8TimeRemAveraging, 0, 8);
    WORD_TO_DGN(pu8Dest[5], pzSrc->u16FullCapacity);
    BITS_TO_DGN(pu8Dest[7], pzSrc->u8TailCurrent, 0, 8);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130512_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130512
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130512_Extract(RVCDGN_zDGN_130512 *pzDest, uint8_t *pu8Src)
{
    //           dest                           src      start size
    DGN_TO_BITS(pzDest->u8DeviceInstance, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u8DeviceDSA, pu8Src[1], 0, 8);
    DGN_TO_BITS(pzDest->u4Function, pu8Src[2], 0, 4);
    DGN_TO_BITS(pzDest->u8PrimaryDCInstance, pu8Src[3], 0, 8);
    DGN_TO_BITS(pzDest->u8SecondaryDCInstance, pu8Src[4], 0, 8);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130724_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130724
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130724_Extract(RVCDGN_zDGN_130724 *pzDest, uint8_t *pu8Src)
{
    //           dest                                 src         start  size
    DGN_TO_BITS(pzDest->u8DCInstance, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u2DesiredPowerOnOffStatus, pu8Src[1], 0, 2);
    DGN_TO_BITS(pzDest->u2DesiredChargeOnOffStatus, pu8Src[1], 2, 2);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130724_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters to DGN 130545
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130724_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_130724 *pzSrc)
{
    // Clear buffer
    (void)memset(pu8Dest, 0, 8);

    // Stuff fields
    BITS_TO_DGN(pu8Dest[0], pzSrc->u8DCInstance, 0, 8);
    BITS_TO_DGN(pu8Dest[1], pzSrc->u2DesiredPowerOnOffStatus, 0, 2);
    BITS_TO_DGN(pu8Dest[1], pzSrc->u2DesiredChargeOnOffStatus, 2, 2);
}

/*
//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130548_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130548
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
*/
void RVCDGN_DGN_130548_Extract(RVCDGN_zDGN_130548 *pzDest, uint8_t *pu8Src)
{
    //           dest                              src        start size
    DGN_TO_BITS(pzDest->u8DCInstance, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u2ClearHistory, pu8Src[1], 0, 2);
    DGN_TO_BITS(pzDest->u2SetCapacity100, pu8Src[1], 2, 2);
    DGN_TO_BITS(pzDest->u2ResetBatteryHealth, pu8Src[1], 4, 2);
    DGN_TO_WORD(pzDest->u16ChargedVoltage, pu8Src[2]);
    DGN_TO_BITS(pzDest->u8ShuntVoltage, pu8Src[4], 0, 8);
    DGN_TO_WORD(pzDest->u16ShuntCurrent, pu8Src[5]);
    DGN_TO_BITS(pzDest->u4BatteryType, pu8Src[7], 0, 4);
}

/*
//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130548_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters to DGN 130548
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
*/
void RVCDGN_DGN_130548_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_130548 *pzSrc)
{
    // Clear buffer
    (void)memset(pu8Dest, 0, 8);

    // Stuff fields
    BITS_TO_DGN(pu8Dest[0], pzSrc->u8DCInstance, 0, 8);
    BITS_TO_DGN(pu8Dest[1], pzSrc->u2ClearHistory, 0, 2);
    BITS_TO_DGN(pu8Dest[1], pzSrc->u2SetCapacity100, 2, 2);
    BITS_TO_DGN(pu8Dest[1], pzSrc->u2ResetBatteryHealth, 4, 2);
    WORD_TO_DGN(pu8Dest[2], pzSrc->u16ChargedVoltage);
    BITS_TO_DGN(pu8Dest[4], pzSrc->u8ShuntVoltage, 0, 8);
    WORD_TO_DGN(pu8Dest[5], pzSrc->u16ShuntCurrent);
    BITS_TO_DGN(pu8Dest[7], pzSrc->u4BatteryType, 0, 4);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130526_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130526
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130526_Extract(RVCDGN_zDGN_130526 *pzDest, uint8_t *pu8Src)
{
    //           dest                           src      start size
    DGN_TO_BITS(pzDest->u8DeviceInstance, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u8DeviceDSA, pu8Src[1], 0, 8);
    DGN_TO_BITS(pzDest->u4Function, pu8Src[2], 0, 4);
    DGN_TO_BITS(pzDest->u8PrimaryDCInstance, pu8Src[3], 0, 8);
    DGN_TO_BITS(pzDest->u8SecondaryDCInstance, pu8Src[4], 0, 8);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130526_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters to DGN 130526
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130526_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_130526 *pzSrc)
{
    // Clear buffer
    (void)memset(pu8Dest, 0, 8);

    // Stuff fields
    BITS_TO_DGN(pu8Dest[0], pzSrc->u8DeviceInstance, 0, 8);
    BITS_TO_DGN(pu8Dest[1], pzSrc->u8DeviceDSA, 0, 8);
    BITS_TO_DGN(pu8Dest[2], pzSrc->u4Function, 0, 4);
    BITS_TO_DGN(pu8Dest[3], pzSrc->u8PrimaryDCInstance, 0, 8);
    BITS_TO_DGN(pu8Dest[4], pzSrc->u8SecondaryDCInstance, 0, 8);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130768_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130768
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130768_Extract(RVCDGN_zDGN_130768 *pzDest, uint8_t *pu8Src)
{
    //           dest                                src       start size
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u2CircuitStatus, pu8Src[1], 0, 2);
    DGN_TO_BITS(pzDest->u2LastCommand, pu8Src[1], 2, 2);
    DGN_TO_BITS(pzDest->u2BypassDetect, pu8Src[1], 4, 2);
    DGN_TO_BITS(pzDest->u2Reserved, pu8Src[1], 6, 2);
    DGN_TO_WORD(pzDest->u16DCSwitchedVoltage, pu8Src[2]);
    DGN_TO_DWRD(pzDest->u32DCSwitchedCurrent, pu8Src[4]);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130767_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters to DGN 130767
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130767_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_130767 *pzSrc)
{
    // Clear buffer
    (void)memset(pu8Dest, 0, RVCDGN_DGN_130767_SIZE);

    // Stuff fields
    BITS_TO_DGN(pu8Dest[0], pzSrc->u8Instance, 0, 8);
    BITS_TO_DGN(pu8Dest[1], pzSrc->u2Command, 0, 2);
    BITS_TO_DGN(pu8Dest[1], pzSrc->u6Reserved, 2, 6);
    pu8Dest[2] = 0xFF;
    pu8Dest[3] = 0xFF;
    pu8Dest[4] = 0xFF;
    pu8Dest[5] = 0xFF;
    pu8Dest[6] = 0xFF;
    pu8Dest[7] = 0xFF;
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130545_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130545
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130545_Extract(RVCDGN_zDGN_130545 *pzDest, uint8 *pu8Src)
{
    //           dest                                src       start size
    DGN_TO_BITS(pzDest->u8BatteryInstance, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u8DCInstance, pu8Src[1], 0, 8);
    DGN_TO_BITS(pzDest->u8SeriesString, pu8Src[2], 0, 8);
    DGN_TO_BITS(pzDest->u8ModuleCount, pu8Src[3], 0, 8);
    DGN_TO_BITS(pzDest->u8CellsPerModule, pu8Src[4], 0, 8);
    DGN_TO_BITS(pzDest->u4VoltageStatus, pu8Src[7], 0, 3);
    DGN_TO_BITS(pzDest->u4TemperatureStatus, pu8Src[7], 3, 3);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130808_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130808
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130808_Extract(
    RVCDGN_zDGN_130808 *pzDest,  // out: Destination structure
    uint8 *pu8Src                // in:  Source buffer
)
{
    //           dest                                src       start size
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u8CurrentScheduleInstance, pu8Src[1], 0, 8);
    DGN_TO_BITS(pzDest->u2ReducedNoiseMode, pu8Src[2], 0, 2);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130808_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters in DGN 130808
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130808_Stuff(
    uint8 *pu8Dest,            // out:  Destination buffer
    RVCDGN_zDGN_130808 *pzSrc  // in:   Source structure
)
{
    // Clear
    (void)memset(pu8Dest, 0, RVCDGN_DGN_130808_SIZE);

    // Fill
    //           dest       source                          start size
    BITS_TO_DGN(pu8Dest[0], pzSrc->u8Instance, 0, 8);
    BITS_TO_DGN(pu8Dest[1], pzSrc->u8CurrentScheduleInstance, 0, 8);
    BITS_TO_DGN(pu8Dest[2], pzSrc->u2ReducedNoiseMode, 0, 2);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130810_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 131042
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130810_Extract(
    RVCDGN_zDGN_130810 *pzDest,  // out: Destination structure
    uint8 *pu8Src                // in:  Source buffer
)
{
    //           dest                                src       start size
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u8CurrentScheduleInstance, pu8Src[1], 0, 8);
    DGN_TO_BITS(pzDest->u8NumberOfScheduleInstance, pu8Src[2], 0, 8);
    DGN_TO_BITS(pzDest->u2ReducedNoiseMode, pu8Src[3], 0, 2);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130810_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters in DGN 130810
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130810_Stuff(
    uint8 *pu8Dest,            // out:  Destination buffer
    RVCDGN_zDGN_130810 *pzSrc  // in:   Source structure
)
{
    // Clear
    (void)memset(pu8Dest, 0, RVCDGN_DGN_131042_SIZE);

    // Fill
    //           dest       source                          start size
    BITS_TO_DGN(pu8Dest[0], pzSrc->u8Instance, 0, 8);
    BITS_TO_DGN(pu8Dest[1], pzSrc->u8CurrentScheduleInstance, 0, 8);
    BITS_TO_DGN(pu8Dest[2], pzSrc->u8NumberOfScheduleInstance, 0, 8);
    BITS_TO_DGN(pu8Dest[3], pzSrc->u2ReducedNoiseMode, 0, 2);
}

// //-----------------------------------------------------------------------------
// // Function:    RVCDGN_DGN_130807_Extract
// //-----------------------------------------------------------------------------
// // Description: Extract parameters from DGN 130807
// //-----------------------------------------------------------------------------
// // Return:      None.
// //-----------------------------------------------------------------------------
// void RVCDGN_DGN_130807_Extract
// (
//  RVCDGN_zDGN_130807* pzDest,   // out: Destination structure
//     uint8*               pu8Src    // in:  Source buffer
// )
// {
//     //           dest                                src       start size
//     DGN_TO_BITS( pzDest->u8Instance,                 pu8Src[0],    0,   8 );
//     DGN_TO_BITS( pzDest->u8SheduleModeInstance,      pu8Src[1],    0,   8 );
//     DGN_TO_BITS( pzDest->u8Start_Hour,               pu8Src[2],    0,   8 );
//     DGN_TO_BITS( pzDest->u8StartMinute,              pu8Src[3],    0,   8 );
//     DGN_TO_WORD( pzDest->u16SetPointTempHeat,        pu8Src[4]            );
//     DGN_TO_WORD( pzDest->u16SetPointTempCool,        pu8Src[6]            );
// }

// //-----------------------------------------------------------------------------
// // Function:    RVCDGN_DGN_130807_Stuff
// //-----------------------------------------------------------------------------
// // Description: Stuff parameters in DGN 130807
// //-----------------------------------------------------------------------------
// // Return:      None.
// //-----------------------------------------------------------------------------
// void RVCDGN_DGN_130807_Stuff
// (
//     uint8*               pu8Dest,   // out:  Destination buffer
//  RVCDGN_zDGN_130807* pzSrc      // in:   Source structure
// )
// {
//     // Clear
//     (void)memset( pu8Dest, 0, RVCDGN_DGN_130807_SIZE );

//  // Fill
//     //           dest       source                          start size
//     BITS_TO_DGN( pu8Dest[0], pzSrc->u8Instance,               0,   8 );
//     BITS_TO_DGN( pu8Dest[1], pzSrc->u8SheduleModeInstance,    0,   8 );
//     BITS_TO_DGN( pu8Dest[2], pzSrc->u8Start_Hour,             0,   8 );
//     BITS_TO_DGN( pu8Dest[3], pzSrc->u8StartMinute,            0,   8 );
//     WORD_TO_DGN( pu8Dest[4], pzSrc->u16SetPointTempHeat              );
//     WORD_TO_DGN( pu8Dest[6], pzSrc->u16SetPointTempCool              );
// }

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_131042_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 131042
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_131042_Extract(
    RVCDGN_zDGN_131042 *pzDest,  // out: Destination structure
    uint8 *pu8Src                // in:  Source buffer
)
{
    //           dest                                src       start size
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u4OperatingMode, pu8Src[1], 0, 4);
    DGN_TO_BITS(pzDest->u2FanMode, pu8Src[1], 4, 2);
    DGN_TO_BITS(pzDest->u2SheduleMode, pu8Src[1], 6, 2);
    DGN_TO_BITS(pzDest->u8FanSpeed, pu8Src[2], 0, 8);
    DGN_TO_WORD(pzDest->u16SetPointTempHeat, pu8Src[3]);
    DGN_TO_WORD(pzDest->u16SetPointTempCool, pu8Src[5]);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_131042_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters in DGN 131042
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_131042_Stuff(
    uint8 *pu8Dest,            // out:  Destination buffer
    RVCDGN_zDGN_131042 *pzSrc  // in:   Source structure
)
{
    // Clear
    (void)memset(pu8Dest, 0, RVCDGN_DGN_131042_SIZE);

    // Fill
    //           dest       source                          start size
    BITS_TO_DGN(pu8Dest[0], pzSrc->u8Instance, 0, 8);
    BITS_TO_DGN(pu8Dest[1], pzSrc->u4OperatingMode, 0, 4);
    BITS_TO_DGN(pu8Dest[1], pzSrc->u2FanMode, 4, 2);
    BITS_TO_DGN(pu8Dest[1], pzSrc->u2SheduleMode, 6, 2);
    BITS_TO_DGN(pu8Dest[2], pzSrc->u8FanSpeed, 0, 8);
    WORD_TO_DGN(pu8Dest[3], pzSrc->u16SetPointTempHeat);
    WORD_TO_DGN(pu8Dest[5], pzSrc->u16SetPointTempCool);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130778_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130778
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130778_Extract(
    RVCDGN_zDGN_130778 *pzDest,  // out: Destination structure
    uint8 *pu8Src                // in:  Source buffer
)
{
    //           dest                                    src       start size
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u8Group, pu8Src[1], 0, 8);
    DGN_TO_BITS(pzDest->u8OperatingStatusBrightness, pu8Src[2], 0, 8);
    DGN_TO_BITS(pzDest->u2LockStatus, pu8Src[3], 0, 2);
    DGN_TO_BITS(pzDest->u2OvercurrentStatus, pu8Src[3], 2, 2);
    DGN_TO_BITS(pzDest->u2OverrideStatus, pu8Src[3], 4, 2);
    DGN_TO_BITS(pzDest->u2EnableStatus, pu8Src[3], 6, 2);
    DGN_TO_BITS(pzDest->u8DelayDuration, pu8Src[4], 0, 8);
    DGN_TO_BITS(pzDest->u8LastCommand, pu8Src[5], 0, 8);
    DGN_TO_BITS(pzDest->u2InterlockStatus, pu8Src[6], 0, 2);
    DGN_TO_BITS(pzDest->u2LoadStatus, pu8Src[6], 2, 2);
    DGN_TO_BITS(pzDest->u2ReservedField1, pu8Src[6], 4, 6);
    DGN_TO_BITS(pzDest->u2UnderCurrent, pu8Src[6], 6, 8);
    DGN_TO_BITS(pzDest->u8MasterMemoryValue, pu8Src[7], 0, 8);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130778_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters in DGN 130778
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130778_Stuff(
    uint8 *pu8Dest,            // out:  Destination buffer
    RVCDGN_zDGN_130778 *pzSrc  // in:   Source structure
)
{
    // Clear
    (void)memset(pu8Dest, 0, RVCDGN_DGN_130778_SIZE);

    // Fill
    //             dest       source                                start size
    BITS_TO_DGN(pu8Dest[0], pzSrc->u8Instance, 0, 8);
    BITS_TO_DGN(pu8Dest[1], pzSrc->u8Group, 0, 8);
    BITS_TO_DGN(pu8Dest[2], pzSrc->u8OperatingStatusBrightness, 0, 8);
    BITS_TO_DGN(pu8Dest[3], pzSrc->u2LockStatus, 0, 2);
    BITS_TO_DGN(pu8Dest[3], pzSrc->u2OvercurrentStatus, 2, 2);
    BITS_TO_DGN(pu8Dest[3], pzSrc->u2OverrideStatus, 4, 2);
    BITS_TO_DGN(pu8Dest[3], pzSrc->u2EnableStatus, 6, 2);
    BITS_TO_DGN(pu8Dest[4], pzSrc->u8DelayDuration, 0, 8);
    BITS_TO_DGN(pu8Dest[5], pzSrc->u8LastCommand, 0, 8);
    BITS_TO_DGN(pu8Dest[6], pzSrc->u2InterlockStatus, 0, 2);
    BITS_TO_DGN(pu8Dest[6], pzSrc->u2LoadStatus, 2, 2);
    BITS_TO_DGN(pu8Dest[6], 0x03, 4, 6);
    BITS_TO_DGN(pu8Dest[6], pzSrc->u2UnderCurrent, 6, 8);
    BITS_TO_DGN(pu8Dest[7], pzSrc->u8MasterMemoryValue, 0, 8);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130779_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130779
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130779_Extract(
    RVCDGN_zDGN_130779 *pzDest,  // out: Destination structure
    uint8 *pu8Src                // in:  Source buffer
)
{
    //           dest                                src       start size
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u8Group, pu8Src[1], 0, 8);
    DGN_TO_BITS(pzDest->u8DesiredLevelBrightness, pu8Src[2], 0, 8);
    DGN_TO_BITS(pzDest->u8Command, pu8Src[3], 0, 8);
    DGN_TO_BITS(pzDest->u8DelayDuration, pu8Src[4], 0, 8);
    DGN_TO_BITS(pzDest->u2InterlockStatus, pu8Src[5], 0, 2);
    DGN_TO_BITS(pzDest->u6ReservedField1, pu8Src[5], 2, 6);
    DGN_TO_BITS(pzDest->u8RampTime, pu8Src[6], 0, 8);
    DGN_TO_BITS(pzDest->u8ReservedField2, pu8Src[7], 0, 8);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130779_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters in DGN   130779
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130779_Stuff(
    uint8 *pu8Dest,            // out:  Destination buffer
    RVCDGN_zDGN_130779 *pzSrc  // in:   Source structure
)
{
    // Clear
    (void)memset(pu8Dest, 0, RVCDGN_DGN_130779_SIZE);

    // Fill
    //           dest       source                               start size
    BITS_TO_DGN(pu8Dest[0], pzSrc->u8Instance, 0, 8);
    BITS_TO_DGN(pu8Dest[1], pzSrc->u8Group, 0, 8);
    BITS_TO_DGN(pu8Dest[2], pzSrc->u8DesiredLevelBrightness, 0, 8);
    BITS_TO_DGN(pu8Dest[3], pzSrc->u8Command, 0, 8);
    BITS_TO_DGN(pu8Dest[4], pzSrc->u8DelayDuration, 0, 8);
    BITS_TO_DGN(pu8Dest[5], pzSrc->u2InterlockStatus, 0, 2);
    BITS_TO_DGN(pu8Dest[5], pzSrc->u6ReservedField1, 2, 6);
    WORD_TO_DGN(pu8Dest[6], pzSrc->u6ReservedField1);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130805_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130805
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130805_Extract(
    RVCDGN_zDGN_130805 *pzDest,  // out: Destination structure
    uint8 *pu8Src                // in:  Source buffer
)
{
    //           dest                                src       start size
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u8ScheduleModeInstance, pu8Src[1], 0, 8);
    DGN_TO_BITS(pzDest->u8StartHour, pu8Src[2], 0, 8);
    DGN_TO_BITS(pzDest->u8StartMinute, pu8Src[3], 0, 8);
    DGN_TO_WORD(pzDest->u16SetpointTempHeat, pu8Src[4]);
    DGN_TO_WORD(pzDest->u16SetpointTempCool, pu8Src[6]);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130805_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters in DGN   130805
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130805_Stuff(
    uint8 *pu8Dest,            // out:  Destination buffer
    RVCDGN_zDGN_130805 *pzSrc  // in:   Source structure
)
{
    // Clear
    (void)memset(pu8Dest, 0, RVCDGN_DGN_130805_SIZE);

    // Fill
    //           dest       source                               start size
    BITS_TO_DGN(pu8Dest[0], pzSrc->u8Instance, 0, 8);
    BITS_TO_DGN(pu8Dest[1], pzSrc->u8ScheduleModeInstance, 0, 8);
    BITS_TO_DGN(pu8Dest[2], pzSrc->u8StartHour, 0, 8);
    BITS_TO_DGN(pu8Dest[3], pzSrc->u8StartMinute, 0, 8);
    WORD_TO_DGN(pu8Dest[4], pzSrc->u16SetpointTempHeat);
    WORD_TO_DGN(pu8Dest[6], pzSrc->u16SetpointTempCool);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130804_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130804
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130804_Extract(
    RVCDGN_zDGN_130804 *pzDest,  // out: Destination structure
    uint8 *pu8Src                // in:  Source buffer
)
{
    //           dest                                src       start size
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u8ScheduleModeInstance, pu8Src[1], 0, 8);
    DGN_TO_BITS(pzDest->u2Sunday, pu8Src[2], 0, 2);
    DGN_TO_BITS(pzDest->u2Monday, pu8Src[2], 2, 2);
    DGN_TO_BITS(pzDest->u2Tuesday, pu8Src[2], 4, 2);
    DGN_TO_BITS(pzDest->u2Wednesday, pu8Src[2], 6, 2);
    DGN_TO_BITS(pzDest->u2Thursday, pu8Src[3], 0, 2);
    DGN_TO_BITS(pzDest->u2Friday, pu8Src[3], 2, 2);
    DGN_TO_BITS(pzDest->u2Saturday, pu8Src[3], 4, 2);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130804_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters in DGN   130804
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130804_Stuff(
    uint8 *pu8Dest,            // out:  Destination buffer
    RVCDGN_zDGN_130804 *pzSrc  // in:   Source structure
)
{
    // Clear
    (void)memset(pu8Dest, 0, RVCDGN_DGN_130804_SIZE);

    // Fill
    //           dest       source                               start size
    BITS_TO_DGN(pu8Dest[0], pzSrc->u8Instance, 0, 8);
    BITS_TO_DGN(pu8Dest[1], pzSrc->u8ScheduleModeInstance, 0, 8);
    BITS_TO_DGN(pu8Dest[2], pzSrc->u2Sunday, 0, 2);
    BITS_TO_DGN(pu8Dest[2], pzSrc->u2Monday, 2, 2);
    BITS_TO_DGN(pu8Dest[2], pzSrc->u2Tuesday, 4, 2);
    BITS_TO_DGN(pu8Dest[2], pzSrc->u2Wednesday, 6, 2);
    BITS_TO_DGN(pu8Dest[3], pzSrc->u2Thursday, 0, 2);
    BITS_TO_DGN(pu8Dest[3], pzSrc->u2Friday, 2, 2);
    BITS_TO_DGN(pu8Dest[3], pzSrc->u2Saturday, 4, 2);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_131041_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters in DGN   131041
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_131041_Stuff(
    uint8 *pu8Dest,            // out:  Destination buffer
    RVCDGN_zDGN_131041 *pzSrc  // in:   Source structure
)
{
    // Clear
    (void)memset(pu8Dest, 0, RVCDGN_DGN_131041_SIZE);

    // Fill
    //           dest       source                               start size
    BITS_TO_DGN(pu8Dest[0], pzSrc->u8Instance, 0, 8);
    BITS_TO_DGN(pu8Dest[1], pzSrc->u8OperatingMode, 0, 8);
    BITS_TO_DGN(pu8Dest[2], pzSrc->u8MaxFanSpeed, 0, 8);
    BITS_TO_DGN(pu8Dest[3], pzSrc->u8MaxAirConOutLvl, 0, 8);
    BITS_TO_DGN(pu8Dest[4], pzSrc->u8FanSpeed, 0, 8);
    BITS_TO_DGN(pu8Dest[5], pzSrc->u8AirConOutLvl, 0, 8);
    BITS_TO_DGN(pu8Dest[6], pzSrc->u8DeadBand, 0, 8);
    BITS_TO_DGN(pu8Dest[7], pzSrc->u8SecondStageDeadBand, 0, 8);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130505_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters in DGN 130505
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130505_Stuff(
    uint8 *pu8Dest,            // out:  Destination buffer
    RVCDGN_zDGN_130505 *pzSrc  // in:   Source structure
)
{
    // Clear
    (void)memset(pu8Dest, 0, RVCDGN_DGN_130505_SIZE);

    // Fill
    //           dest       source                               start size
    BITS_TO_DGN(pu8Dest[0], pzSrc->u8Instance, 0, 8);
    BITS_TO_DGN(pu8Dest[1], pzSrc->u4CompressorStatus, 0, 4);
    BITS_TO_DGN(pu8Dest[1], pzSrc->NotInUseB1, 4, 4);
    BITS_TO_DGN(pu8Dest[2], pzSrc->NotInUseB21, 0, 3);
    BITS_TO_DGN(pu8Dest[2], pzSrc->u2ReducedNoiseMode, 3, 2);
    BITS_TO_DGN(pu8Dest[2], pzSrc->NotInUseB22, 5, 3);
    WORD_TO_DGN(pu8Dest[3], pzSrc->u16ExtTemp);
    WORD_TO_DGN(pu8Dest[5], pzSrc->u16CoilTemp);
    BITS_TO_DGN(pu8Dest[7], pzSrc->u2CoilTempError, 0, 2);
    BITS_TO_DGN(pu8Dest[7], pzSrc->u2CoilFreezeDetected, 2, 2);
    BITS_TO_DGN(pu8Dest[7], pzSrc->u2ExtTempError, 4, 2);
    BITS_TO_DGN(pu8Dest[7], pzSrc->u2DefrostCycleActive, 6, 2);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_131041_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 131041
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_131041_Extract(RVCDGN_zDGN_131041 *pzDest, uint8 *pu8Src)
{
    //           dest                                src       start size
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u8OperatingMode, pu8Src[1], 0, 8);
    DGN_TO_BITS(pzDest->u8MaxFanSpeed, pu8Src[2], 0, 8);
    DGN_TO_BITS(pzDest->u8MaxAirConOutLvl, pu8Src[3], 0, 8);
    DGN_TO_BITS(pzDest->u8FanSpeed, pu8Src[4], 0, 8);
    DGN_TO_BITS(pzDest->u8AirConOutLvl, pu8Src[5], 0, 8);
    DGN_TO_BITS(pzDest->u8DeadBand, pu8Src[6], 0, 8);
    DGN_TO_BITS(pzDest->u8SecondStageDeadBand, pu8Src[7], 0, 8);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130971_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130971
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130971_Extract(
    RVCDGN_zDGN_130971 *pzDest,  // out: Destination structure
    uint8 *pu8Src                // in:  Source buffer
)
{
    //           dest                                src       start size
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u8OperatingMode, pu8Src[1], 0, 8);
    DGN_TO_BITS(pzDest->u8MaxHeatOutputLevel, pu8Src[2], 0, 8);
    DGN_TO_BITS(pzDest->u8HeatOutputLevel, pu8Src[3], 0, 8);
    DGN_TO_BITS(pzDest->u8DeadBand, pu8Src[4], 0, 8);
    DGN_TO_BITS(pzDest->u8SecondStageDeadBand, pu8Src[5], 0, 8);
    DGN_TO_BITS(pzDest->u8FanSpeed, pu8Src[6], 0, 8);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130971_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters in DGN 130971
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130971_Stuff(
    uint8 *pu8Dest,            // out:  Destination buffer
    RVCDGN_zDGN_130971 *pzSrc  // in:   Source structure
)
{
    // Clear
    (void)memset(pu8Dest, 0, RVCDGN_DGN_130971_SIZE);

    // Fill
    //           dest       source                          start size
    BITS_TO_DGN(pu8Dest[0], pzSrc->u8Instance, 0, 8);
    BITS_TO_DGN(pu8Dest[1], pzSrc->u8OperatingMode, 0, 8);
    BITS_TO_DGN(pu8Dest[2], pzSrc->u8MaxHeatOutputLevel, 0, 8);
    BITS_TO_DGN(pu8Dest[3], pzSrc->u8HeatOutputLevel, 0, 8);
    BITS_TO_DGN(pu8Dest[4], pzSrc->u8DeadBand, 0, 8);
    BITS_TO_DGN(pu8Dest[5], pzSrc->u8SecondStageDeadBand, 0, 8);
    BITS_TO_DGN(pu8Dest[6], pzSrc->u8FanSpeed, 0, 8);
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
static void rvcdgn_SetString(
    char *pcBufferData,    // in/out:  Buffer data
    uint16 u16BufferSize,  // in:      Buffer size
    const char *pcString   // in:      String input
)
{
    uint16 u16Index = 0;

    // Data
    while (u16Index < (u16BufferSize - 1))
    {
        if ((pcString[u16Index] == 0) || (pcString[u16Index] == (const char)0xFF))
        {
            break;
        }
        pcBufferData[u16Index] = pcString[u16Index];
        u16Index++;
    }

    // Padding
    while (u16Index < u16BufferSize)
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
static uint16 rvcdgn_AppendString(
    char *pu8BufferData,    // in/out:  Buffer data
    uint16 u16BufferIndex,  // in:      Buffer index
    uint16 u16BufferSize,   // in:      Buffer size
    const char *pcString    // in:      Data to append
)
{
    uint16 u16Size = strlen((char *)pcString);
    uint16 u16SizeMax = u16BufferSize - u16BufferIndex;
    if (u16Size > u16SizeMax)
    {
        u16Size = u16SizeMax;
    }
    (void)memcpy(&pu8BufferData[u16BufferIndex], pcString, u16Size);
    return u16BufferIndex + u16Size;
}

// Function:    RVCDGN_DGN_130776_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130776
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130776_Extract(RVCDGN_zDGN_130776 *pzDest, uint8_t *pu8Src)
{
    //           dest                                 src         start  size
    DGN_TO_BITS(pzDest->u8ManufacturerCodeLSB, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u3ManufacturerCodeMSB, pu8Src[1], 0, 3);
    DGN_TO_BITS(pzDest->u5FunctionInstance, pu8Src[1], 3, 5);
    DGN_TO_BITS(pzDest->u8Function, pu8Src[2], 0, 8);
    DGN_TO_BITS(pzDest->u8FirmwareRevision, pu8Src[3], 0, 8);
    DGN_TO_BITS(pzDest->u8ConfigTypeLSB, pu8Src[4], 0, 8);
    DGN_TO_BITS(pzDest->u8ConfigType, pu8Src[5], 0, 8);
    DGN_TO_BITS(pzDest->u8ConfigTypeMSB, pu8Src[6], 0, 8);
    DGN_TO_BITS(pzDest->u8ConfigRevision, pu8Src[7], 0, 8);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130739_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130739
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130739_Extract(RVCDGN_zDGN_130739 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_WORD(pzDest->u16ChargeVoltage, pu8Src[1]);
    DGN_TO_WORD(pzDest->u16ChargeCurrent, pu8Src[3]);
    DGN_TO_BITS(pzDest->u8ChargeCurrentPercentOfMaximum, pu8Src[5], 0, 8);
    DGN_TO_BITS(pzDest->u8OperatingState, pu8Src[6], 0, 8);
    DGN_TO_BITS(pzDest->u2PowerUpState, pu8Src[7], 0, 2);
    DGN_TO_BITS(pzDest->u2ClearHistory, pu8Src[7], 2, 2);
    DGN_TO_BITS(pzDest->u4ForceCharge, pu8Src[7], 4, 4);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130737_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters in DGN 130737
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130737_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_130737 *pzSrc)
{
    (void)memset(pu8Dest, 0, RVCDGN_DGN_130737_SIZE);

    BITS_TO_DGN(pu8Dest[0], pzSrc->u8Instance, 0, 8);
    BITS_TO_DGN(pu8Dest[1], pzSrc->u8SolarChargeControllerStatus, 0, 8);
    BITS_TO_DGN(pu8Dest[2], pzSrc->u2DefaultStateOnPowerUp, 0, 2);
    BITS_TO_DGN(pu8Dest[2], pzSrc->u2ClearHistory, 2, 2);
    BITS_TO_DGN(pu8Dest[2], pzSrc->u4ForceCharge, 4, 4);
    //! \~ u8Reserved1[5]
    pu8Dest[3] = 0xFF;
    pu8Dest[4] = 0xFF;
    pu8Dest[5] = 0xFF;
    pu8Dest[6] = 0xFF;
    pu8Dest[7] = 0xFF;
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130737_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130737
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130737_Extract(RVCDGN_zDGN_130737 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u8SolarChargeControllerStatus, pu8Src[1], 0, 8);
    DGN_TO_BITS(pzDest->u2DefaultStateOnPowerUp, pu8Src[2], 0, 2);
    DGN_TO_BITS(pzDest->u2ClearHistory, pu8Src[2], 2, 2);
    DGN_TO_BITS(pzDest->u4ForceCharge, pu8Src[2], 4, 4);
    pzDest->u8Reserved1[0] = 0xFF;
    pzDest->u8Reserved1[1] = 0xFF;
    pzDest->u8Reserved1[2] = 0xFF;
    pzDest->u8Reserved1[3] = 0xFF;
    pzDest->u8Reserved1[4] = 0xFF;
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130693_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130693
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130693_Extract(RVCDGN_zDGN_130693 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_WORD(pzDest->u16RatedBatteryVoltage, pu8Src[1]);
    DGN_TO_WORD(pzDest->u16RatedChargingCurrent, pu8Src[3]);
    DGN_TO_WORD(pzDest->u16SupportedBatteryTypes, pu8Src[5]);
    DGN_TO_BITS(pzDest->u2VendorDefinedProprietaryType1, pu8Src[7], 0, 2);
    DGN_TO_BITS(pzDest->u2VendorDefinedProprietaryType2, pu8Src[7], 2, 2);
    DGN_TO_BITS(pzDest->u4Reserved1, 0XFF, 4, 4);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130692_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130692
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130692_Extract(RVCDGN_zDGN_130692 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_WORD(pzDest->u16RatedSolarInputVoltage, pu8Src[1]);
    DGN_TO_WORD(pzDest->u16RatedSolarInputCurrent, pu8Src[3]);
    DGN_TO_WORD(pzDest->u16RatedSolarOverPower, pu8Src[5]);
    pzDest->u8Reserved1 = 0xFF;
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130691_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130691
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130691_Extract(RVCDGN_zDGN_130691 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_WORD(pzDest->u16TodayAmpHoursToBattery, pu8Src[1]);
    DGN_TO_WORD(pzDest->u16YesterdayAmpHoursToBattery, pu8Src[3]);
    DGN_TO_WORD(pzDest->u16DayBeforeYesterdayAmpHoursToBattery, pu8Src[5]);
    pzDest->u8Reserved1 = 0xFF;
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130690_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130690
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130690_Extract(RVCDGN_zDGN_130690 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_WORD(pzDest->u16Last7DaysAmpHoursToBattery, pu8Src[1]);
    DGN_TO_DWRD(pzDest->u32CumulativePowerGeneration, pu8Src[3]);
    pzDest->u8Reserved1 = 0xFF;
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130689_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130689
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130689_Extract(RVCDGN_zDGN_130689 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_WORD(pzDest->u16TotalNumberOfOperatingDays, pu8Src[1]);
    DGN_TO_WORD(pzDest->u16SolarChargeControllerMeasuredTemperature, pu8Src[3]);
    pzDest->u8Reserved1[0] = 0xFF;
    pzDest->u8Reserved1[1] = 0xFF;
    pzDest->u8Reserved1[2] = 0xFF;
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130688_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130688
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130688_Extract(RVCDGN_zDGN_130688 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    pzDest->u8Reserved1 = 0xFF;
    pzDest->u8Reserved2 = 0xFF;
    DGN_TO_WORD(pzDest->u16MeasuredVoltage, pu8Src[3]);
    DGN_TO_WORD(pzDest->u16MeasuredCurrent, pu8Src[5]);
    DGN_TO_BITS(pzDest->u8MeasuredTemperature, pu8Src[7], 0, 8);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130559_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130559
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130559_Extract(RVCDGN_zDGN_130559 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_WORD(pzDest->u16SolarArrayMeasuredVoltage, pu8Src[1]);
    DGN_TO_WORD(pzDest->u16SolarArrayMeasuredInputCurrent, pu8Src[3]);
    pzDest->u8Reserved1[0] = 0xFF;
    pzDest->u8Reserved1[1] = 0xFF;
    pzDest->u8Reserved1[2] = 0xFF;
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130738_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130738
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130738_Extract(RVCDGN_zDGN_130738 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u8ChargingAlgorithm, pu8Src[1], 0, 8);
    DGN_TO_BITS(pzDest->u8ControllerMode, pu8Src[2], 0, 8);
    DGN_TO_BITS(pzDest->u2BatterySensorPresent, pu8Src[3], 0, 2);
    DGN_TO_BITS(pzDest->u2LinkageMode, pu8Src[3], 2, 2);
    DGN_TO_BITS(pzDest->u4BatteryType, pu8Src[3], 4, 4);
    DGN_TO_WORD(pzDest->u16BatteryBankSize, pu8Src[4]);
    pzDest->u8Reserved1 = 0xFF;
    DGN_TO_BITS(pzDest->u8MaximumChargingCurrent, pu8Src[7], 0, 8);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130736_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters in DGN 130736
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130736_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_130736 *pzSrc)
{
    (void)memset(pu8Dest, 0, RVCDGN_DGN_130736_SIZE);

    BITS_TO_DGN(pu8Dest[0], pzSrc->u8Instance, 0, 8);
    BITS_TO_DGN(pu8Dest[1], pzSrc->u8ChargingAlgorithm, 0, 8);
    BITS_TO_DGN(pu8Dest[2], pzSrc->u8ControllerMode, 0, 8);
    BITS_TO_DGN(pu8Dest[3], pzSrc->u2BatterySensorPresent, 0, 2);
    BITS_TO_DGN(pu8Dest[3], pzSrc->u2LinkageMode, 2, 2);
    BITS_TO_DGN(pu8Dest[3], pzSrc->u4BatteryType, 4, 4);
    WORD_TO_DGN(pu8Dest[4], pzSrc->u16BatteryBankSize);
    //! \~u8Reserved1
    pu8Dest[6] = 0xFF;
    BITS_TO_DGN(pu8Dest[7], pzSrc->u8MaximumChargingCurrent, 0, 8);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130736_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130736
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130736_Extract(RVCDGN_zDGN_130736 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u8ChargingAlgorithm, pu8Src[1], 0, 8);
    DGN_TO_BITS(pzDest->u8ControllerMode, pu8Src[2], 0, 8);
    DGN_TO_BITS(pzDest->u2BatterySensorPresent, pu8Src[3], 0, 2);
    DGN_TO_BITS(pzDest->u2LinkageMode, pu8Src[3], 2, 2);
    DGN_TO_BITS(pzDest->u4BatteryType, pu8Src[3], 4, 4);
    DGN_TO_WORD(pzDest->u16BatteryBankSize, pu8Src[4]);
    pzDest->u8Reserved1 = 0xFF;
    DGN_TO_BITS(pzDest->u8MaximumChargingCurrent, pu8Src[7], 0, 8);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130558_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters in DGN 130558
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130558_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_130558 *pzSrc)
{
    (void)memset(pu8Dest, 0, RVCDGN_DGN_130558_SIZE);

    BITS_TO_DGN(pu8Dest[0], pzSrc->u8Instance, 0, 8);
    WORD_TO_DGN(pu8Dest[1], pzSrc->u16BulkAbsorptionVoltage);
    WORD_TO_DGN(pu8Dest[3], pzSrc->u16FloatVoltage);
    WORD_TO_DGN(pu8Dest[5], pzSrc->u16ChargeReturnVoltage);
    //! \~ u8Reserved1
    pu8Dest[7] = 0xFF;
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130558_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130558
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130558_Extract(RVCDGN_zDGN_130558 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_WORD(pzDest->u16BulkAbsorptionVoltage, pu8Src[1]);
    DGN_TO_WORD(pzDest->u16FloatVoltage, pu8Src[3]);
    DGN_TO_WORD(pzDest->u16ChargeReturnVoltage, pu8Src[5]);
    pzDest->u8Reserved1 = 0xFF;
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130556_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters in DGN 130556
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130556_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_130556 *pzSrc)
{
    (void)memset(pu8Dest, 0, RVCDGN_DGN_130556_SIZE);

    BITS_TO_DGN(pu8Dest[0], pzSrc->u8Instance, 0, 8);
    WORD_TO_DGN(pu8Dest[1], pzSrc->u16UnderVoltageWarningVoltage);
    WORD_TO_DGN(pu8Dest[3], pzSrc->u16BatteryHighVoltageLimitVoltage);
    WORD_TO_DGN(pu8Dest[5], pzSrc->u16BatteryLowVoltageLimitVoltage);
    //! \~ u8Reserved1
    pu8Dest[7] = 0xFF;
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130556_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130556
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130556_Extract(RVCDGN_zDGN_130556 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_WORD(pzDest->u16UnderVoltageWarningVoltage, pu8Src[1]);
    DGN_TO_WORD(pzDest->u16BatteryHighVoltageLimitVoltage, pu8Src[3]);
    DGN_TO_WORD(pzDest->u16BatteryLowVoltageLimitVoltage, pu8Src[5]);
    pzDest->u8Reserved1 = 0xFF;
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130554_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters in DGN 130554
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130554_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_130554 *pzSrc)
{
    (void)memset(pu8Dest, 0, RVCDGN_DGN_130554_SIZE);

    BITS_TO_DGN(pu8Dest[0], pzSrc->u8Instance, 0, 8);
    WORD_TO_DGN(pu8Dest[1], pzSrc->u16BatteryHighVoltageLimitReturnVoltage);
    WORD_TO_DGN(pu8Dest[3], pzSrc->u16BatteryLowVoltageLimitReturnVoltage);
    BITS_TO_DGN(pu8Dest[5], pzSrc->u8BatteryLowVoltageLimitTimeDelay, 0, 8);
    BITS_TO_DGN(pu8Dest[6], pzSrc->u8AbsorptionDuration, 0, 8);
    BITS_TO_DGN(pu8Dest[7], pzSrc->u8TemperatureCompensationFactor, 0, 8);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130554_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130554
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130554_Extract(RVCDGN_zDGN_130554 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_WORD(pzDest->u16BatteryHighVoltageLimitReturnVoltage, pu8Src[1]);
    DGN_TO_WORD(pzDest->u16BatteryLowVoltageLimitReturnVoltage, pu8Src[3]);
    DGN_TO_BITS(pzDest->u8BatteryLowVoltageLimitTimeDelay, pu8Src[5], 0, 8);
    DGN_TO_BITS(pzDest->u8AbsorptionDuration, pu8Src[6], 0, 8);
    DGN_TO_BITS(pzDest->u8TemperatureCompensationFactor, pu8Src[7], 0, 8);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130511_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters in DGN 130511
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130511_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_130511 *pzSrc)
{
    (void)memset(pu8Dest, 0, RVCDGN_DGN_130511_SIZE);

    BITS_TO_DGN(pu8Dest[0], pzSrc->u8Instance, 0, 8);
    BITS_TO_DGN(pu8Dest[1], pzSrc->u8ChargerPriority, 0, 8);
    BITS_TO_DGN(pu8Dest[2], pzSrc->u8ExternalTemperatureSensorHighTemperatureLimit, 0, 8);
    BITS_TO_DGN(pu8Dest[3], pzSrc->u8ExternalTemperatureSensorLowTemperatureLimit, 0, 8);
    //! \~ u8Reserved1[4]
    pu8Dest[4] = 0xFF;
    pu8Dest[5] = 0xFF;
    pu8Dest[6] = 0xFF;
    pu8Dest[7] = 0xFF;
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130511_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130511
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130511_Extract(RVCDGN_zDGN_130511 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u8ChargerPriority, pu8Src[1], 0, 8);
    DGN_TO_BITS(pzDest->u8ExternalTemperatureSensorHighTemperatureLimit, pu8Src[2], 0, 8);
    DGN_TO_BITS(pzDest->u8ExternalTemperatureSensorLowTemperatureLimit, pu8Src[3], 0, 8);
    pzDest->u8Reserved1[0] = 0xFF;
    pzDest->u8Reserved1[1] = 0xFF;
    pzDest->u8Reserved1[2] = 0xFF;
    pzDest->u8Reserved1[3] = 0xFF;
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130735_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130735
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130735_Extract(RVCDGN_zDGN_130735 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_WORD(pzDest->u16TimeRemaining, pu8Src[1]);
    DGN_TO_BITS(pzDest->u2PreChargingStatus, pu8Src[3], 0, 2);
    DGN_TO_BITS(pzDest->u6Reserved1, 0xFF, 2, 6);
    DGN_TO_BITS(pzDest->u8TimeSinceLastEqualization, pu8Src[4], 0, 8);
    pzDest->u8Reserved2[0] = 0xFF;
    pzDest->u8Reserved2[1] = 0xFF;
    pzDest->u8Reserved2[2] = 0xFF;
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130734_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters in DGN 130734
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130734_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_130734 *pzSrc)
{
    (void)memset(pu8Dest, 0, RVCDGN_DGN_130734_SIZE);

    BITS_TO_DGN(pu8Dest[0], pzSrc->u8Instance, 0, 8);
    WORD_TO_DGN(pu8Dest[1], pzSrc->u16EqualizationVoltage);
    WORD_TO_DGN(pu8Dest[3], pzSrc->u16EqualizationTime);
    BITS_TO_DGN(pu8Dest[5], pzSrc->u8EqualizationInterval, 0, 8);
    //! \~ u8Reserved1[2]
    pu8Dest[6] = 0xFF;
    pu8Dest[7] = 0xFF;
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130734_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130734
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130734_Extract(RVCDGN_zDGN_130734 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_WORD(pzDest->u16EqualizationVoltage, pu8Src[1]);
    DGN_TO_WORD(pzDest->u16EqualizationTime, pu8Src[3]);
    DGN_TO_BITS(pzDest->u8EqualizationInterval, pu8Src[5], 0, 8);
    pzDest->u8Reserved1[0] = 0xFF;
    pzDest->u8Reserved1[1] = 0xFF;
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_131031_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 131031
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_131031_Extract(RVCDGN_zDGN_131031 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u4Instance, pu8Src[0], 0, 4);
    DGN_TO_BITS(pzDest->u2Line, pu8Src[0], 4, 2);
    DGN_TO_BITS(pzDest->u2InputOutput, pu8Src[0], 6, 2);
    DGN_TO_WORD(pzDest->u16RMSVoltage, pu8Src[1]);
    DGN_TO_WORD(pzDest->u16RMSCurrent, pu8Src[3]);
    DGN_TO_WORD(pzDest->u16Frequency, pu8Src[5]);
    DGN_TO_BITS(pzDest->u2FaultOpenGround, pu8Src[7], 0, 2);
    DGN_TO_BITS(pzDest->u2FaultOpenNeutral, pu8Src[7], 2, 2);
    DGN_TO_BITS(pzDest->u2FaultReversePolarity, pu8Src[7], 4, 2);
    DGN_TO_BITS(pzDest->u2FaultGroundCurrent, pu8Src[7], 6, 2);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_131030_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 131030
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_131030_Extract(RVCDGN_zDGN_131030 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u4Instance, pu8Src[0], 0, 4);
    DGN_TO_BITS(pzDest->u2Line, pu8Src[0], 4, 2);
    DGN_TO_BITS(pzDest->u2InputOutput, pu8Src[0], 6, 2);
    DGN_TO_WORD(pzDest->u16PeakVoltage, pu8Src[1]);
    DGN_TO_WORD(pzDest->u16PeakCurrent, pu8Src[3]);
    DGN_TO_WORD(pzDest->u16GroundCurrent, pu8Src[5]);
    DGN_TO_BITS(pzDest->u8Capacity, pu8Src[7], 0, 8);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_131029_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 131029
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_131029_Extract(RVCDGN_zDGN_131029 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u4Instance, pu8Src[0], 0, 4);
    DGN_TO_BITS(pzDest->u2Line, pu8Src[0], 4, 2);
    DGN_TO_BITS(pzDest->u2InputOutput, pu8Src[0], 6, 2);
    DGN_TO_BITS(pzDest->u2Waveform, pu8Src[1], 0, 2);
    DGN_TO_BITS(pzDest->u4PhaseStatus, pu8Src[1], 2, 4);
    DGN_TO_BITS(pzDest->u2Reserved1, 0xFF, 6, 2);
    DGN_TO_WORD(pzDest->u16RealPower, pu8Src[2]);
    DGN_TO_WORD(pzDest->u16ReactivePower, pu8Src[4]);
    DGN_TO_BITS(pzDest->u8HarmonicDistortion, pu8Src[6], 0, 8);
    DGN_TO_BITS(pzDest->u8ComplementaryLeg, pu8Src[7], 0, 8);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130959_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130959
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130959_Extract(RVCDGN_zDGN_130959 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u4Instance, pu8Src[0], 0, 4);
    DGN_TO_BITS(pzDest->u2Line, pu8Src[0], 4, 2);
    DGN_TO_BITS(pzDest->u2InputOutput, pu8Src[0], 6, 2);
    DGN_TO_BITS(pzDest->u8VoltageFault, pu8Src[1], 0, 8);
    DGN_TO_BITS(pzDest->u2FaultSurgeProtection, pu8Src[2], 0, 2);
    DGN_TO_BITS(pzDest->u2FaultHighFrequency, pu8Src[2], 2, 2);
    DGN_TO_BITS(pzDest->u2FaultLowFrequency, pu8Src[2], 4, 2);
    DGN_TO_BITS(pzDest->u2BypassModeActive, pu8Src[2], 6, 2);
    DGN_TO_BITS(pzDest->u4QualificationStatus, pu8Src[3], 0, 4);
    DGN_TO_BITS(pzDest->u4Reserved1, 0xFF, 4, 4);
    pzDest->u8Reserved2[0] = 0xFF;
    pzDest->u8Reserved2[1] = 0xFF;
    pzDest->u8Reserved2[2] = 0xFF;
    pzDest->u8Reserved2[3] = 0xFF;
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130792_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130792
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130792_Extract(RVCDGN_zDGN_130792 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_WORD(pzDest->u16DCVoltage, pu8Src[1]);
    DGN_TO_WORD(pzDest->u16DCAmperage, pu8Src[3]);
    pzDest->u8Reserved1[0] = 0xFF;
    pzDest->u8Reserved1[1] = 0xFF;
    pzDest->u8Reserved1[2] = 0xFF;
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_131028_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 131028
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_131028_Extract(RVCDGN_zDGN_131028 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u8Status, pu8Src[1], 0, 8);
    DGN_TO_BITS(pzDest->u2BatteryTemperatureSensorPresent, pu8Src[2], 0, 2);
    DGN_TO_BITS(pzDest->u2LoadSenseEnabled, pu8Src[2], 2, 2);
    DGN_TO_BITS(pzDest->u2InverterEnabled, pu8Src[2], 4, 2);
    DGN_TO_BITS(pzDest->u2PassThroughEnable, pu8Src[2], 6, 2);
    DGN_TO_BITS(pzDest->u2GeneratorSupportEnabled, pu8Src[3], 0, 2);
    DGN_TO_BITS(pzDest->u6Reserved1, 0xFF, 2, 6);
    pzDest->u8Reserved2[0] = 0xFF;
    pzDest->u8Reserved2[1] = 0xFF;
    pzDest->u8Reserved2[2] = 0xFF;
    pzDest->u8Reserved2[3] = 0xFF;
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_131027_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters in DGN 131027
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_131027_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_131027 *pzSrc)
{
    (void)memset(pu8Dest, 0, RVCDGN_DGN_131027_SIZE);

    BITS_TO_DGN(pu8Dest[0], pzSrc->u8Instance, 0, 8);
    BITS_TO_DGN(pu8Dest[1], pzSrc->u2InverterEnabled, 0, 2);
    BITS_TO_DGN(pu8Dest[1], pzSrc->u2LoadSenseEnabled, 2, 2);
    BITS_TO_DGN(pu8Dest[1], pzSrc->u2PassThroughEnable, 4, 2);
    BITS_TO_DGN(pu8Dest[1], pzSrc->u2GeneratorSupportEnabled, 6, 2);
    //! \~ u8Reserved1[5]
    pu8Dest[2] = 0xFF;
    pu8Dest[3] = 0xFF;
    pu8Dest[4] = 0xFF;
    pu8Dest[5] = 0xFF;
    pu8Dest[6] = 0xFF;
    BITS_TO_DGN(pu8Dest[7], pzSrc->u2InverterEnableOnStartup, 0, 2);
    BITS_TO_DGN(pu8Dest[7], pzSrc->u2LoadSenseEnableOnStartup, 2, 2);
    BITS_TO_DGN(pu8Dest[7], pzSrc->u2ACPassThroughEnableOnStartup, 4, 2);
    BITS_TO_DGN(pu8Dest[7], pzSrc->u2GeneratorSupportEnableOnStartup, 6, 2);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_131027_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 131027
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_131027_Extract(RVCDGN_zDGN_131027 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u2InverterEnabled, pu8Src[1], 0, 2);
    DGN_TO_BITS(pzDest->u2PassThroughEnable, pu8Src[1], 2, 2);
    DGN_TO_BITS(pzDest->u2PassThroughEnable, pu8Src[1], 4, 2);
    DGN_TO_BITS(pzDest->u2GeneratorSupportEnabled, pu8Src[1], 6, 2);
    pzDest->u8Reserved1[0] = 0xFF;
    pzDest->u8Reserved1[1] = 0xFF;
    pzDest->u8Reserved1[2] = 0xFF;
    pzDest->u8Reserved1[3] = 0xFF;
    pzDest->u8Reserved1[4] = 0xFF;
    DGN_TO_BITS(pzDest->u2InverterEnableOnStartup, pu8Src[7], 0, 2);
    DGN_TO_BITS(pzDest->u2LoadSenseEnableOnStartup, pu8Src[7], 2, 2);
    DGN_TO_BITS(pzDest->u2ACPassThroughEnableOnStartup, pu8Src[7], 4, 2);
    DGN_TO_BITS(pzDest->u2GeneratorSupportEnableOnStartup, pu8Src[7], 6, 2);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_131026_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 131026
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_131026_Extract(RVCDGN_zDGN_131026 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_WORD(pzDest->u16LoadSensePowerThreshold, pu8Src[1]);
    DGN_TO_WORD(pzDest->u16LoadSenseInterval, pu8Src[3]);
    DGN_TO_WORD(pzDest->u16DCSourceShutdownVoltageMinimum, pu8Src[5]);
    DGN_TO_BITS(pzDest->u2InverterEnableOnStartup, pu8Src[7], 0, 2);
    DGN_TO_BITS(pzDest->u2LoadSenseEnableOnStartup, pu8Src[7], 2, 2);
    DGN_TO_BITS(pzDest->u2ACPassThroughEnableOnStartup, pu8Src[7], 4, 2);
    DGN_TO_BITS(pzDest->u2Reserved1, 0xFF, 6, 2);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_131024_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters in DGN 131024
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_131024_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_131024 *pzSrc)
{
    (void)memset(pu8Dest, 0, RVCDGN_DGN_131024_SIZE);

    BITS_TO_DGN(pu8Dest[0], pzSrc->u8Instance, 0, 8);
    WORD_TO_DGN(pu8Dest[1], pzSrc->u16LoadSensePowerThreshold);
    WORD_TO_DGN(pu8Dest[3], pzSrc->u16LoadSenseInterval);
    WORD_TO_DGN(pu8Dest[5], pzSrc->u16DCSourceShutdownVoltageMinimum);
    //! \~u8Reserved1
    pu8Dest[7] = 0xFF;
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_131024_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 131024
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_131024_Extract(RVCDGN_zDGN_131024 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_WORD(pzDest->u16LoadSensePowerThreshold, pu8Src[1]);
    DGN_TO_WORD(pzDest->u16LoadSenseInterval, pu8Src[3]);
    DGN_TO_WORD(pzDest->u16DCSourceShutdownVoltageMinimum, pu8Src[5]);
    pzDest->u8Reserved1 = 0xFF;
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_131025_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters in DGN 131025
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_131025_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_131025 *pzSrc)
{
    (void)memset(pu8Dest, 0, RVCDGN_DGN_131025_SIZE);

    BITS_TO_DGN(pu8Dest[0], pzSrc->u8Instance, 0, 8);
    WORD_TO_DGN(pu8Dest[1], pzSrc->u16DCSourceShutdownVoltageMaximum);
    WORD_TO_DGN(pu8Dest[3], pzSrc->u16DCSourceWarningVoltageMinimum);
    WORD_TO_DGN(pu8Dest[5], pzSrc->u16DCSourceWarningVoltageMaximum);
    //! \~u8Reserved1
    pu8Dest[7] = 0xFF;
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_131025_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 131025
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_131025_Extract(RVCDGN_zDGN_131025 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_WORD(pzDest->u16DCSourceShutdownVoltageMaximum, pu8Src[1]);
    DGN_TO_WORD(pzDest->u16DCSourceWarningVoltageMinimum, pu8Src[3]);
    DGN_TO_WORD(pzDest->u16DCSourceWarningVoltageMaximum, pu8Src[5]);
    pzDest->u8Reserved1 = 0xFF;
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130766_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters in DGN 130766
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130766_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_130766 *pzSrc)
{
    (void)memset(pu8Dest, 0, RVCDGN_DGN_130766_SIZE);

    BITS_TO_DGN(pu8Dest[0], pzSrc->u8Instance, 0, 8);
    WORD_TO_DGN(pu8Dest[1], pzSrc->u16DCSourceShutdownDelay);
    BITS_TO_DGN(pu8Dest[3], pzSrc->u8StackMode, 0, 8);
    WORD_TO_DGN(pu8Dest[4], pzSrc->u16DCSourceShutdownRecoveryLevel);
    WORD_TO_DGN(pu8Dest[6], pzSrc->u16GeneratorSupportEngageCurrent);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130766_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130766
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130766_Extract(RVCDGN_zDGN_130766 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_WORD(pzDest->u16DCSourceShutdownDelay, pu8Src[1]);
    DGN_TO_BITS(pzDest->u8StackMode, pu8Src[3], 0, 8);
    DGN_TO_WORD(pzDest->u16DCSourceShutdownRecoveryLevel, pu8Src[4]);
    DGN_TO_WORD(pzDest->u16GeneratorSupportEngageCurrent, pu8Src[6]);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130715_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters in DGN 130715
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130715_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_130715 *pzSrc)
{
    (void)memset(pu8Dest, 0, RVCDGN_DGN_130715_SIZE);

    BITS_TO_DGN(pu8Dest[0], pzSrc->u8Instance, 0, 8);
    WORD_TO_DGN(pu8Dest[1], pzSrc->u16OutputACVoltage);
    BITS_TO_DGN(pu8Dest[3], pzSrc->u8OutputFrequency, 0, 8);
    WORD_TO_DGN(pu8Dest[4], pzSrc->u16ACOutputPowerLimit);
    WORD_TO_DGN(pu8Dest[6], pzSrc->u16ACOutputTimeLimit);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130715_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130715
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130715_Extract(RVCDGN_zDGN_130715 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_WORD(pzDest->u16OutputACVoltage, pu8Src[1]);
    DGN_TO_BITS(pzDest->u8OutputFrequency, pu8Src[3], 0, 8);
    DGN_TO_WORD(pzDest->u16ACOutputPowerLimit, pu8Src[4]);
    DGN_TO_WORD(pzDest->u16ACOutputTimeLimit, pu8Src[6]);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130749_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130749
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130749_Extract(RVCDGN_zDGN_130749 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_WORD(pzDest->u16FET1Temperature, pu8Src[1]);
    DGN_TO_WORD(pzDest->u16TransformerTemperature, pu8Src[3]);
    DGN_TO_WORD(pzDest->u16FET2Temperature, pu8Src[5]);
    pzDest->u8Reserved1 = 0xFF;
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130507_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130507
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130507_Extract(RVCDGN_zDGN_130507 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_WORD(pzDest->u16ControlPowerBoardTemperature, pu8Src[1]);
    DGN_TO_WORD(pzDest->u16CapacitorTemperature, pu8Src[3]);
    DGN_TO_WORD(pzDest->u16AmbientTemperature, pu8Src[5]);
    pzDest->u8Reserved1 = 0xFF;
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_131018_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 131018
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_131018_Extract(RVCDGN_zDGN_131018 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u4Instance, pu8Src[0], 0, 4);
    DGN_TO_BITS(pzDest->u2Line, pu8Src[0], 4, 2);
    DGN_TO_BITS(pzDest->u2InputOutput, pu8Src[0], 6, 2);
    DGN_TO_WORD(pzDest->u16RMSVoltage, pu8Src[1]);
    DGN_TO_WORD(pzDest->u16RMSCurrent, pu8Src[3]);
    DGN_TO_WORD(pzDest->u16Frequency, pu8Src[5]);
    DGN_TO_BITS(pzDest->u2FaultOpenGround, pu8Src[7], 0, 2);
    DGN_TO_BITS(pzDest->u2FaultOpenNeutral, pu8Src[7], 2, 2);
    DGN_TO_BITS(pzDest->u2FaultReversePolarity, pu8Src[7], 4, 2);
    DGN_TO_BITS(pzDest->u2FaultGroundCurrent, pu8Src[7], 6, 2);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_131016_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 131016
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_131016_Extract(RVCDGN_zDGN_131016 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u4Instance, pu8Src[0], 0, 4);
    DGN_TO_BITS(pzDest->u2Line, pu8Src[0], 4, 2);
    DGN_TO_BITS(pzDest->u2InputOutput, pu8Src[0], 6, 2);
    DGN_TO_BITS(pzDest->u2Waveform, pu8Src[1], 0, 2);
    DGN_TO_BITS(pzDest->u4PhaseStatus, pu8Src[1], 2, 4);
    DGN_TO_BITS(pzDest->u2Reserved1, 0xFF, 6, 2);
    DGN_TO_WORD(pzDest->u16RealPower, pu8Src[2]);
    DGN_TO_WORD(pzDest->u16ReactivePower, pu8Src[4]);
    DGN_TO_BITS(pzDest->u8HarmonicDistortion, pu8Src[6], 0, 8);
    DGN_TO_BITS(pzDest->u8ComplementaryLeg, pu8Src[7], 0, 8);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_131015_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 131015
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_131015_Extract(RVCDGN_zDGN_131015 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_WORD(pzDest->u16ChargeVoltage, pu8Src[1]);
    DGN_TO_WORD(pzDest->u16ChargeCurrent, pu8Src[3]);
    DGN_TO_BITS(pzDest->u8ChargeCurrentPercentOfMaximum, pu8Src[5], 0, 8);
    DGN_TO_BITS(pzDest->u8OperatingState, pu8Src[6], 0, 8);
    DGN_TO_BITS(pzDest->u2DefaultStateOnPowerUp, pu8Src[7], 0, 2);
    DGN_TO_BITS(pzDest->u2AutoRechargeEnable, pu8Src[7], 2, 2);
    DGN_TO_BITS(pzDest->u4ForceCharge, pu8Src[7], 4, 4);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_131013_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters in DGN 131013
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_131013_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_131013 *pzSrc)
{
    (void)memset(pu8Dest, 0, RVCDGN_DGN_131013_SIZE);

    BITS_TO_DGN(pu8Dest[0], pzSrc->u8Instance, 0, 8);
    BITS_TO_DGN(pu8Dest[1], pzSrc->u8Status, 0, 8);
    BITS_TO_DGN(pu8Dest[2], pzSrc->u2DefaultStateOnPowerUp, 0, 2);
    BITS_TO_DGN(pu8Dest[2], pzSrc->u2AutoRechargeEnable, 2, 2);
    BITS_TO_DGN(pu8Dest[2], pzSrc->u4ForceCharge, 4, 4);
    WORD_TO_DGN(pu8Dest[3], pzSrc->u16ControlVoltageForCCCVMode);
    WORD_TO_DGN(pu8Dest[5], pzSrc->u16ControlCurrentForCCCVMode);
    //! \~ pzSrc->u8Reserved1
    pu8Dest[7] = 0xFF;
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_131013_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 131013
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_131013_Extract(RVCDGN_zDGN_131013 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u8Status, pu8Src[1], 0, 8);
    DGN_TO_BITS(pzDest->u2DefaultStateOnPowerUp, pu8Src[2], 0, 2);
    DGN_TO_BITS(pzDest->u2AutoRechargeEnable, pu8Src[2], 2, 2);
    DGN_TO_BITS(pzDest->u4ForceCharge, pu8Src[2], 4, 4);
    DGN_TO_WORD(pzDest->u16ControlVoltageForCCCVMode, pu8Src[3]);
    DGN_TO_WORD(pzDest->u16ControlCurrentForCCCVMode, pu8Src[5]);
    pzDest->u8Reserved1 = 0xFF;
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130723_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130723
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130723_Extract(RVCDGN_zDGN_130723 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    pzDest->u8Reserved1 = 0xFF;
    DGN_TO_BITS(pzDest->u8ChargerPriority, pu8Src[2], 0, 8);
    DGN_TO_WORD(pzDest->u16ChargingVoltage, pu8Src[3]);
    DGN_TO_WORD(pzDest->u16ChargingCurrent, pu8Src[5]);
    DGN_TO_BITS(pzDest->u8ChargerTemperature, pu8Src[7], 0, 8);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_131014_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 131014
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_131014_Extract(RVCDGN_zDGN_131014 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u8ChargingAlgorithm, pu8Src[1], 0, 8);
    DGN_TO_BITS(pzDest->u8ChargerMode, pu8Src[2], 0, 8);
    DGN_TO_BITS(pzDest->u2BatterySensorPresent, pu8Src[3], 0, 2);
    DGN_TO_BITS(pzDest->u2ChargerInstallationLine, pu8Src[3], 2, 2);
    DGN_TO_BITS(pzDest->u4BatteryType, pu8Src[3], 4, 4);
    DGN_TO_WORD(pzDest->u16BatteryBankSize, pu8Src[4]);
    DGN_TO_WORD(pzDest->u16MaximumChargingCurrent, pu8Src[6]);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_131012_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters in DGN 131012
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_131012_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_131012 *pzSrc)
{
    (void)memset(pu8Dest, 0, RVCDGN_DGN_131012_SIZE);

    BITS_TO_DGN(pu8Dest[0], pzSrc->u8Instance, 0, 8);
    BITS_TO_DGN(pu8Dest[1], pzSrc->u8ChargingAlgorithm, 0, 8);
    BITS_TO_DGN(pu8Dest[2], pzSrc->u8ChargerMode, 0, 8);
    BITS_TO_DGN(pu8Dest[3], pzSrc->u2BatterySensorPresent, 0, 2);
    BITS_TO_DGN(pu8Dest[3], pzSrc->u2ChargerInstallationLine, 2, 2);
    //! \~ u4Reserved1
    BITS_TO_DGN(pu8Dest[3], 0xFF, 4, 4);
    WORD_TO_DGN(pu8Dest[4], pzSrc->u16BatteryBankSize);
    BITS_TO_DGN(pu8Dest[6], pzSrc->u4BatteryType, 0, 4);
    //! \~ u4Reserved2
    BITS_TO_DGN(pu8Dest[6], 0xFF, 4, 4);
    BITS_TO_DGN(pu8Dest[7], pzSrc->u8MaximumChargingCurrent, 0, 8);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_131012_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 131012
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_131012_Extract(RVCDGN_zDGN_131012 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u8ChargingAlgorithm, pu8Src[1], 0, 8);
    DGN_TO_BITS(pzDest->u8ChargerMode, pu8Src[2], 0, 8);
    DGN_TO_BITS(pzDest->u2BatterySensorPresent, pu8Src[3], 0, 2);
    DGN_TO_BITS(pzDest->u2ChargerInstallationLine, pu8Src[3], 2, 2);
    DGN_TO_BITS(pzDest->u4Reserved1, 0xFF, 4, 4);
    DGN_TO_WORD(pzDest->u16BatteryBankSize, pu8Src[4]);
    DGN_TO_BITS(pzDest->u4BatteryType, pu8Src[6], 0, 4);
    DGN_TO_BITS(pzDest->u4Reserved2, 0xFF, 4, 4);
    DGN_TO_BITS(pzDest->u8MaximumChargingCurrent, pu8Src[7], 0, 8);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130966_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters in DGN 130966
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130966_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_130966 *pzSrc)
{
    (void)memset(pu8Dest, 0, RVCDGN_DGN_130966_SIZE);

    BITS_TO_DGN(pu8Dest[0], pzSrc->u8Instance, 0, 8);
    BITS_TO_DGN(pu8Dest[1], pzSrc->u8MaximumChargeCurrentAsPercent, 0, 8);
    BITS_TO_DGN(pu8Dest[2], pzSrc->u8ChargeRateLimitAsPercentOfBankSize, 0, 8);
    BITS_TO_DGN(pu8Dest[3], pzSrc->u8ShoreBreakerSize, 0, 8);
    BITS_TO_DGN(pu8Dest[4], pzSrc->u8DefaultBatteryTemperature, 0, 8);
    WORD_TO_DGN(pu8Dest[5], pzSrc->u16RechargeVoltage);
    //! \~u8Reserved1
    pu8Dest[7] = 0xFF;
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130966_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130966
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130966_Extract(RVCDGN_zDGN_130966 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u8MaximumChargeCurrentAsPercent, pu8Src[1], 0, 8);
    DGN_TO_BITS(pzDest->u8ChargeRateLimitAsPercentOfBankSize, pu8Src[2], 0, 8);
    DGN_TO_BITS(pzDest->u8ShoreBreakerSize, pu8Src[3], 0, 8);
    DGN_TO_BITS(pzDest->u8DefaultBatteryTemperature, pu8Src[4], 0, 8);
    DGN_TO_WORD(pzDest->u16RechargeVoltage, pu8Src[5]);
    pzDest->u8Reserved1 = 0xFF;
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130764_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters in DGN 130764
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130764_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_130764 *pzSrc)
{
    (void)memset(pu8Dest, 0, RVCDGN_DGN_130764_SIZE);

    BITS_TO_DGN(pu8Dest[0], pzSrc->u8Instance, 0, 8);
    WORD_TO_DGN(pu8Dest[1], pzSrc->u16BulkVoltage);
    WORD_TO_DGN(pu8Dest[3], pzSrc->u16AbsorptionVoltage);
    WORD_TO_DGN(pu8Dest[5], pzSrc->u16FloatVoltage);
    BITS_TO_DGN(pu8Dest[7], pzSrc->u8TemperatureCompensationConstant, 0, 8);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130764_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130764
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130764_Extract(RVCDGN_zDGN_130764 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_WORD(pzDest->u16BulkVoltage, pu8Src[1]);
    DGN_TO_WORD(pzDest->u16AbsorptionVoltage, pu8Src[3]);
    DGN_TO_WORD(pzDest->u16FloatVoltage, pu8Src[5]);
    DGN_TO_BITS(pzDest->u8TemperatureCompensationConstant, pu8Src[7], 0, 8);
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130751_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters in DGN 130751
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130751_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_130751 *pzSrc)
{
    (void)memset(pu8Dest, 0, RVCDGN_DGN_130751_SIZE);

    BITS_TO_DGN(pu8Dest[0], pzSrc->u8Instance, 0, 8);
    WORD_TO_DGN(pu8Dest[1], pzSrc->u16BulkTime);
    WORD_TO_DGN(pu8Dest[3], pzSrc->u16AbsorptionTime);
    WORD_TO_DGN(pu8Dest[5], pzSrc->u16FloatTime);
    //! \~ u8Reserved1
    pu8Dest[7] = 0xFF;
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130751_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130751
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130751_Extract(RVCDGN_zDGN_130751 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_WORD(pzDest->u16BulkTime, pu8Src[1]);
    DGN_TO_WORD(pzDest->u16AbsorptionTime, pu8Src[3]);
    DGN_TO_WORD(pzDest->u16FloatTime, pu8Src[5]);
    pzDest->u8Reserved1 = 0xFF;
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130968_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters in DGN 130968
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130968_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_130968 *pzSrc)
{
    (void)memset(pu8Dest, 0, RVCDGN_DGN_130968_SIZE);

    BITS_TO_DGN(pu8Dest[0], pzSrc->u8Instance, 0, 8);
    WORD_TO_DGN(pu8Dest[1], pzSrc->u16EqualizationVoltage);
    WORD_TO_DGN(pu8Dest[3], pzSrc->u16EqualizationTime);
    //! \~u8Reserved1[3]
    pu8Dest[5] = 0xFF;
    pu8Dest[6] = 0xFF;
    pu8Dest[7] = 0xFF;
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130968_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130968
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130968_Extract(RVCDGN_zDGN_130968 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_WORD(pzDest->u16EqualizationVoltage, pu8Src[1]);
    DGN_TO_WORD(pzDest->u16EqualizationTime, pu8Src[3]);
    pzDest->u8Reserved1[0] = 0xFF;
    pzDest->u8Reserved1[1] = 0xFF;
    pzDest->u8Reserved1[2] = 0xFF;
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130953_Stuff
//-----------------------------------------------------------------------------
// Description: Stuff parameters in DGN 130953
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130953_Stuff(uint8_t *pu8Dest, RVCDGN_zDGN_130953 *pzSrc)
{
    (void)memset(pu8Dest, 0, RVCDGN_DGN_130953_SIZE);

    BITS_TO_DGN(pu8Dest[0], pzSrc->u8Instance, 0, 8);
    BITS_TO_DGN(pu8Dest[1], pzSrc->u8ExtremeLowVoltageLevel, 0, 8);
    BITS_TO_DGN(pu8Dest[2], pzSrc->u8LowVoltageLevel, 0, 8);
    BITS_TO_DGN(pu8Dest[3], pzSrc->u8HighVoltageLevel, 0, 8);
    BITS_TO_DGN(pu8Dest[4], pzSrc->u8ExtremeHighVoltageLevel, 0, 8);
    BITS_TO_DGN(pu8Dest[5], pzSrc->u8QualificationTime, 0, 8);
    BITS_TO_DGN(pu8Dest[6], pzSrc->u2BypassMode, 0, 2);
    //! \~ u6Reserved1
    BITS_TO_DGN(pu8Dest[6], 0xFF, 2, 6);
    //! \~ u8Reserved2
    pu8Dest[7] = 0xFF;
}

//-----------------------------------------------------------------------------
// Function:    RVCDGN_DGN_130953_Extract
//-----------------------------------------------------------------------------
// Description: Extract parameters from DGN 130953
//-----------------------------------------------------------------------------
// Return:      None.
//-----------------------------------------------------------------------------
void RVCDGN_DGN_130953_Extract(RVCDGN_zDGN_130953 *pzDest, uint8_t *pu8Src)
{
    DGN_TO_BITS(pzDest->u8Instance, pu8Src[0], 0, 8);
    DGN_TO_BITS(pzDest->u8ExtremeLowVoltageLevel, pu8Src[1], 0, 8);
    DGN_TO_BITS(pzDest->u8LowVoltageLevel, pu8Src[2], 0, 8);
    DGN_TO_BITS(pzDest->u8HighVoltageLevel, pu8Src[3], 0, 8);
    DGN_TO_BITS(pzDest->u8ExtremeHighVoltageLevel, pu8Src[4], 0, 8);
    DGN_TO_BITS(pzDest->u8QualificationTime, pu8Src[5], 0, 8);
    DGN_TO_BITS(pzDest->u2BypassMode, pu8Src[6], 0, 2);
    DGN_TO_BITS(pzDest->u6Reserved1, 0xFF, 2, 6);
    pzDest->u8Reserved2 = 0xFF;
}
