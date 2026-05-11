//-------------------------------------------------------------------------------
// Module: hal_ethnet.c
//-------------------------------------------------------------------------------
// Description: ethernet networking support common for all targets
//-------------------------------------------------------------------------------
// COPYRIGHT:
//
// This document is the property of SeaStar Solutions Inc,
// All rights are reserved. This document or any information it
// contains may not be copied, reproduced, disclosed to others, or
// used without the written permission from SeaStar.
//
// Copyright 2019-2020
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------------------
#include "hal_ethnet.h"

//-------------------------------------------------------------------------------
// Local defines
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// Local type definitions
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// Local variables
//-------------------------------------------------------------------------------
static HAL_ETHNET_zControlBlock * hal_ethnet_pzControlBlock;

//-------------------------------------------------------------------------------
// Local function prototypes
//-------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Function:    HAL_ETHNET_Initialize
//-----------------------------------------------------------------------------
// Description: 
//-----------------------------------------------------------------------------
void
HAL_ETHNET_Initialize( HAL_ETHNET_zControlBlock * pzCB )
{
#if DICM_WITH_LIBWEBSOCKETS
    // register our logging adapter prior to any other LWS invocations
    lws_set_log_level(LLL_ERR | LLL_WARN | LLL_NOTICE, HAL_ETHNET_AdaptLwsLog);
#endif // DICM_WITH_LIBWEBSOCKETS

    if (pzCB)
    {
        hal_ethnet_pzControlBlock = pzCB;
    }

#if DICM_WITH_LIBWEBSOCKETS
    if (pzCB && pzCB->pfHookEarly)
    {
        (pzCB->pfHookEarly)(pzCB);
    }
#endif // DICM_WITH_LIBWEBSOCKETS

    HAL_ETHNET_PLAT_InitializeA(pzCB);

#if DICM_WITH_LIBWEBSOCKETS
    if (pzCB)
    {
        pzCB->pzLwsContext = lws_create_context(&pzCB->zLwsConfig);
    }
#endif // DICM_WITH_LIBWEBSOCKETS

    HAL_ETHNET_PLAT_InitializeB(pzCB);

#if DICM_WITH_LIBWEBSOCKETS
    if (pzCB && pzCB->pfHookLate)
    {
        (pzCB->pfHookLate)(pzCB);
    }

    if (pzCB)
    {
        lws_protocol_init(pzCB->pzLwsContext);
    }
#endif // DICM_WITH_LIBWEBSOCKETS
}

//-----------------------------------------------------------------------------
// Function:    HAL_ETHNET_Deinit
//-----------------------------------------------------------------------------
// Description: 
//-----------------------------------------------------------------------------
void
HAL_ETHNET_Deinit( void )
{
    HAL_ETHNET_PLAT_Deinit(hal_ethnet_pzControlBlock);

#if DICM_WITH_LIBWEBSOCKETS
    if (hal_ethnet_pzControlBlock && hal_ethnet_pzControlBlock->pzLwsContext)
    {
        lws_context_destroy(hal_ethnet_pzControlBlock->pzLwsContext);
    }
#endif // DICM_WITH_LIBWEBSOCKETS

    hal_ethnet_pzControlBlock = NULL;
}

//-----------------------------------------------------------------------------
// Function:    HAL_ETHNET_OnNewIpAddress
//-----------------------------------------------------------------------------
// Description: 
//-----------------------------------------------------------------------------
void 
HAL_ETHNET_OnNewIpAddress( const char * pszDottedQuad )
{
    if (hal_ethnet_pzControlBlock &&
        hal_ethnet_pzControlBlock->pfnOnNewIpAddress)
    {
        (hal_ethnet_pzControlBlock->pfnOnNewIpAddress)(pszDottedQuad);
    }
}

#if DICM_WITH_LIBWEBSOCKETS
//-----------------------------------------------------------------------------
// Function:    HAL_ETHNET_AdaptLwsLog
//-----------------------------------------------------------------------------
// Description: Mechanism to allow libwebsockets logging to appear on platform
//-----------------------------------------------------------------------------
void 
HAL_ETHNET_AdaptLwsLog(int          level, 
                       const char * line)
{
    // LWS log "level" is a bitmask of log levels that should apply
    if (level & (1<<LLL_ERR)) 
    {
        LOG(E, "%s", line);
    }
    else if (level & (1<<LLL_WARN)) 
    {
        LOG(W, "%s", line);
    }
    else if (level & ((1<<LLL_INFO) | (1<<LLL_NOTICE))) 
    {
        LOG(I, "%s", line);
    }
    else if (level & LLL_DEBUG) 
    {
        LOG(D, "%s", line);
    }
    else
    {
        LOG(V, "%s", line);
    }
}
#endif // DICM_WITH_LIBWEBSOCKETS
