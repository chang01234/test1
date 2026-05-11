//-------------------------------------------------------------------------------
// Module: hal_ethnet.h
//-------------------------------------------------------------------------------
// Description: ethernet networking support
//-------------------------------------------------------------------------------
// COPYRIGHT:
//
// This document is the property of SeaStar Solutions Inc,
// All rights are reserved. This document or any information it
// contains may not be copied, reproduced, disclosed to others, or
// used without the written permission from SeaStar.
//
// Copyright 2019
//-------------------------------------------------------------------------------
#ifndef HAL_ETHNET_DOT_H
#define HAL_ETHNET_DOT_H

#include "dicm_framework_config.h"
#if defined(CONFIG_DICM_SUPPORT_LIBWEBSOCKETS) || 0 // FIXME: FUTURE: other non-DICM firmware?
#define DICM_WITH_LIBWEBSOCKETS 1
#else
#define DICM_WITH_LIBWEBSOCKETS 0
#endif

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if DICM_WITH_LIBWEBSOCKETS
#include "libwebsockets.h"
#include "romfs.h"
#endif

//------------------------------------------------------------------------------
// Public constants
//------------------------------------------------------------------------------
#define HAL_ETHNET_MTU                     (1500)

#define MDIO_BASIC_CONTROL_RESTART_AUTONEG (1<< 9)
#define MDIO_BASIC_CONTROL_POWER_DOWN      (1<<11)
#define MDIO_BASIC_CONTROL_AUTONEG_ENABLE  (1<<12)

#define HAL_ETHNET_JSON_RX_BUF_MAX         (2<<10) // FIXME: HACKED, derive from actual requirements

#define HAL_ETHNET_PRINTL_TAG __attribute__ ((format (printf, 2, 3)))

//------------------------------------------------------------------------------
// Public types
//------------------------------------------------------------------------------
typedef enum 
{
    MDIO_BASIC_CONTROL                  = 0x00,
    MDIO_BASIC_STATUS                   = 0x01,
    MDIO_PHY_IDENTIFIER_1               = 0x02,
    MDIO_PHY_IDENTIFIER_2               = 0x03,
    MDIO_AUTONEG_ADVERTISEMENT          = 0x04,
    MDIO_AUTONEG_LINK_PARTNER_ABILITY   = 0x05,
    MDIO_AUTONEG_EXPANSION              = 0x06,
    MDIO_AUTONEG_NEXT_PAGE              = 0x07,
    MDIO_LINK_PARTNER_NEXT_PAGE_ABILITY = 0x08,
    MDIO_MII_CONTROL                    = 0x14,
    MDIO_RXER_COUNTER                   = 0x15,
    MDIO_INTERRUPT_CONTROL_STATUS       = 0x1b,
    MDIO_PHY_CONTROL_1                  = 0x1e,
    MDIO_PHY_CONTROL_2                  = 0x1f
}
HAL_ETHNET_eMdioRegs;

typedef enum 
{
    ENET_UNKNOWN,
    ENET_GOT_ID1,
    ENET_GOT_ID2,
    ENET_RESET_COMPLETE,
    ENET_AUTONEG_SET,
    ENET_WAIT_AUTONEG,
    ENET_DISCONNECTED,
    ENET_CONNECTED,
}
HAL_ETHNET_eMediaState;

typedef void (*HAL_ETHNET_pfnOnNewIpAddress)(const char *);

#if DICM_WITH_LIBWEBSOCKETS
typedef struct
{
    struct lws_plat_file_ops base;
    romfs_t                  xRomFS;
}
HAL_ETHNET_LWS_zFileOpsEx;

struct e0ed593;
typedef void(*HAL_ETHNET_LWS_InitHook)(struct e0ed593 *);
#endif // DICM_WITH_LIBWEBSOCKETS

typedef struct e0ed593
{
#if DICM_WITH_LIBWEBSOCKETS
    // application populates prior to _Initialize, all zeros when LWS enabled but not used
    struct lws_context_creation_info zLwsConfig;
    HAL_ETHNET_LWS_InitHook          pfHookEarly; // called before _InitializeA - for populating zLwsConfig

    // this module populates during _Initialize when LWS in use, ignored otherwise
    struct lws_context *             pzLwsContext;
    HAL_ETHNET_LWS_InitHook          pfHookLate;  // called after _InitializeB - for augmenting pzLwsContext

    // application optionally populates with pfHookLate
    HAL_ETHNET_LWS_zFileOpsEx        zFileOpsRomFS;
#endif // DICM_WITH_LIBWEBSOCKETS

    // application populates prior to _Initialize, NULL when not provided
    HAL_ETHNET_pfnOnNewIpAddress pfnOnNewIpAddress;
}
HAL_ETHNET_zControlBlock;

//------------------------------------------------------------------------------
// Public variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Public prototypes
//------------------------------------------------------------------------------

// common implementation APIs for applications
void HAL_ETHNET_Initialize( HAL_ETHNET_zControlBlock * );
void HAL_ETHNET_Deinit( void );

// called by per-platform implementation
void HAL_ETHNET_OnNewIpAddress( const char * );

// implemented per-platform, used by hal_ethnet_common
void HAL_ETHNET_PLAT_InitializeA( HAL_ETHNET_zControlBlock * );
void HAL_ETHNET_PLAT_InitializeB( HAL_ETHNET_zControlBlock * );
void HAL_ETHNET_PLAT_Deinit( HAL_ETHNET_zControlBlock * );

void     HAL_ETHNET_Power_Control( bool enable );
uint64_t HAL_ETHNET_GetMACAddress(const int sock);
int      HAL_ETHNET_WriteMdio(const HAL_ETHNET_eMdioRegs reg, const uint16_t val);
int      HAL_ETHNET_ReadMdio( const HAL_ETHNET_eMdioRegs reg, uint16_t * pval);

#if DICM_WITH_LIBWEBSOCKETS
void HAL_ETHNET_AdaptLwsLog(int, const char *);
#endif

#endif /* ETHNET_DOT_H */
