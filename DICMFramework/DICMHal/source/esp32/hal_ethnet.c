//------------------------------------------------------------------------------
// Module:      hal_ethnet.c
//------------------------------------------------------------------------------
// Description: To provide abstraction layer for ESP32 ethernet
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
#include <stdarg.h>
#include <stdio.h>

#include "dicm_framework_config.h"
#include "hal_ethnet.h"

#if CONFIG_DICM_TARGET_MARINE_GW_1 || CONFIG_DICM_TARGET_MAXIMUS_DCM_1
#include "esp_eth.h"
#include "esp_eth_phy.h"
#include "eth_phy_regs_struct.h"
#include "esp_event.h"
#include "driver/gpio.h"
#include "hal_cpu.h"
#include "hal_gpio.h"
#include "lwip/etharp.h"
#include "lwip/netif.h"
#include "lwip/sockets.h"
#include "esp_netif.h"

static const char *TAG = "lan8720";
#define PHY_CHECK(a, str, goto_tag, ...)                                          \
    do                                                                            \
    {                                                                             \
        if (!(a))                                                                 \
        {                                                                         \
            ESP_LOGE(TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            goto goto_tag;                                                        \
        }                                                                         \
    } while (0)

//------------------------------------------------------------------------------
// Private Types
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Private Functions
//------------------------------------------------------------------------------
static HAL_GPIO_RESULT_ENUM ethnet_esp32_control_rmii_clk(const bool enable);
static HAL_GPIO_RESULT_ENUM ethnet_esp32_control_rmii_rst(const bool assert);
static esp_err_t            ethnet_esp32_phy_init(esp_eth_phy_t *phy);
static esp_err_t            ethnet_esp32_phy_reset_hw(esp_eth_phy_t *phy); 
static esp_err_t            ethnet_esp32_phy_pwrctl(esp_eth_phy_t *phy, bool enable);
static void                 ethnet_esp32_log_ethernet(void *, esp_event_base_t, int32_t, void *);
static void                 ethnet_esp32_on_ip(void *, esp_event_base_t, int32_t, void *);
static esp_err_t            ethnet_esp32_on_lowlevel_init_done(esp_eth_handle_t);

//------------------------------------------------------------------------------
// Private Definitions
//------------------------------------------------------------------------------
#define ETHERNET_PHY_UNKNOWN  (0)
#define ETHERNET_PHY_LAN8720A (1)

#define LAN8720_PHY_ID1                           (0x0007)
#define LAN8720_PHY_ID2                           (0xc0f0)
#define LAN8720_PHY_ID2_MASK                      (0xfff0)
#define LAN8720_PHY_SPECIAL_CONTROL_STATUS_REG    (0x1f)
#define LAN8720_AUTO_NEGOTIATION_DONE             BIT(12)
#define LAN8720_DUPLEX_INDICATION_FULL            BIT(4)
#define LAN8720_SPEED_INDICATION_100T             BIT(3)
#define LAN8720_SPEED_DUPLEX_INDICATION_100T_FULL (0x18)

#define ETHERNET_MAC_TASK_STACK_SZ  (2048) // adapted from Espressif example
#define ETHERNET_MAC_TASK_PRIORITY  (15)   // adapted from Espressif example
#define ETHERNET_MAC_FLAGS          (ETH_MAC_FLAG_WORK_WITH_CACHE_DISABLE)
#define ETHERNET_CHECK_LINK_PER_MS  (2000) // adapted from Espressif example

#if CONFIG_DICM_TARGET_MARINE_GW_1 || CONFIG_DICM_TARGET_MAXIMUS_DCM_1 // FIXME: smarter platform adaptation
#define ETHERNET_PHY                ETHERNET_PHY_LAN8720A
#define ETHERNET_PHYADDR            (0)
#define ETHERNET_RESET_TIMEOUT_MS   (1000)                     // adapted from Espressif example           
#define ETHERNET_AUTONEG_TIMEOUT_MS (4000)                     // adapted from Espressif example           
#define ETHERNET_PIN_MDC            GPIO_NUM_23
#define ETHERNET_PIN_MDIO           GPIO_NUM_18
#else
#define ETHERNET_PHY                ETHERNET_PHY_UNKNOWN
#endif

//------------------------------------------------------------------------------
// Private Variables
//------------------------------------------------------------------------------
static char ethnet_esp32_szDottedQuad[16];

static eth_phy_config_t ethnet_zPhyConfig = 
{
  .phy_addr            = ETHERNET_PHYADDR,
  .reset_timeout_ms    = ETHERNET_RESET_TIMEOUT_MS,
  .autonego_timeout_ms = ETHERNET_AUTONEG_TIMEOUT_MS,
  .reset_gpio_num      = -1,
};

static eth_mac_config_t ethnet_zMacConfig =
{
  .sw_reset_timeout_ms = ETHERNET_RESET_TIMEOUT_MS,
  .rx_task_stack_size  = ETHERNET_MAC_TASK_STACK_SZ,
  .rx_task_prio        = ETHERNET_MAC_TASK_PRIORITY,
  .smi_mdc_gpio_num    = ETHERNET_PIN_MDC,
  .smi_mdio_gpio_num   = ETHERNET_PIN_MDIO,
  .flags               = ETHERNET_MAC_FLAGS,
};

static esp_eth_config_t ethnet_zConfig = 
{
  .mac                     = NULL, // populated during init
  .phy                     = NULL, // populated during init
  .check_link_period_ms    = ETHERNET_CHECK_LINK_PER_MS,
  .stack_input             = NULL, 
  .on_lowlevel_init_done   = ethnet_esp32_on_lowlevel_init_done,
  .on_lowlevel_deinit_done = NULL,
};

static uint32_t           ethnet_u32RxFrames;
static esp_netif_config_t ethnet_cfg;
static esp_netif_t *      ethnet_netif;
static esp_eth_handle_t   ethnet_xHandle;

//-----------------------------------------------------------------------------
// Function:    HAL_ETHNET_PLAT_InitializeA
//-----------------------------------------------------------------------------
// Description: 
//-----------------------------------------------------------------------------
void
HAL_ETHNET_PLAT_InitializeA( HAL_ETHNET_zControlBlock * pzCB )
{
    // establishes default parameters for all IP interfaces
    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH( );
    (void) memcpy(&ethnet_cfg, &cfg, sizeof(cfg));
    ethnet_netif = esp_netif_new(&cfg);
    
    esp_eth_set_default_handlers(ethnet_netif);
}

//-----------------------------------------------------------------------------
// Function:    HAL_ETHNET_PLAT_InitializeB
//-----------------------------------------------------------------------------
// Description: 
//-----------------------------------------------------------------------------
void 
HAL_ETHNET_PLAT_InitializeB( HAL_ETHNET_zControlBlock * pzCB )
{
    // New MAC
    ethnet_zConfig.mac = esp_eth_mac_new_esp32(&ethnet_zMacConfig);
    if (ETHERNET_PHY == ETHERNET_PHY_LAN8720A)
    {
        // New phy
        ethnet_zConfig.phy = esp_eth_phy_new_lan8720(&ethnet_zPhyConfig);

        // Intercept few calls
        ethnet_zConfig.phy->init     = ethnet_esp32_phy_init;     // callback function when PHY chip init is requested 
        ethnet_zConfig.phy->reset_hw = ethnet_esp32_phy_reset_hw; // callback function when a reset of the PHY chip is requested
        ethnet_zConfig.phy->pwrctl   = ethnet_esp32_phy_pwrctl;   // callback function when power control of the PHY chip is requested        
    }
    else
    {
        LOG(E, "Ethernet PHY not currently configured: FIXME.");
    }
    ASSERT(ethnet_zConfig.mac != NULL);
    ASSERT(ethnet_zConfig.phy != NULL);

    // Enable and reset chip
    ethnet_esp32_phy_pwrctl(NULL, true);
    ethnet_esp32_control_rmii_clk(true);
    ethnet_esp32_control_rmii_rst(true);
    hal_cpu_wait_us(2000);
    ethnet_esp32_control_rmii_rst(false);

    // this registers ethernet event handlers for LWiP, and 
    // creates a FreeRTOS task emac_task, which:
    //  - starts and stops ethernet MAC
    //  - pushes RX frames into LWiP IP_EVENT_ETH_GOT_IP
    esp_err_t result = esp_eth_driver_install(&ethnet_zConfig, &ethnet_xHandle);
    ASSERT(result == ESP_OK);

    if (result == ESP_OK)
    {
        (void) esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID,
                                          ethnet_esp32_log_ethernet, NULL);
        (void) esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP,
                                          ethnet_esp32_on_ip, NULL);
        
        esp_netif_attach(ethnet_netif, esp_eth_new_netif_glue(ethnet_xHandle));

        // signals emac_task to start
        ESP_ERROR_CHECK(esp_eth_start(ethnet_xHandle));
    }
    else
    {
        LOG(E, "Unable to install Ethernet driver");
    }
}

//-----------------------------------------------------------------------------
// Function:    HAL_ETHNET_PLAT_Deinit
//-----------------------------------------------------------------------------
// Description: 
//-----------------------------------------------------------------------------
void 
HAL_ETHNET_PLAT_Deinit( HAL_ETHNET_zControlBlock * pzCB )
{
    if (ethnet_xHandle)
    {
        esp_eth_stop(ethnet_xHandle);
        esp_eth_driver_uninstall(ethnet_xHandle);
        ethnet_xHandle = NULL;
    }
}

// ------------------------------------------------------------------------------
// Function:    HAL_ETHNET_Power_Control
// ------------------------------------------------------------------------------
// Description: Function called when the PHY chip power state must be changed
// ------------------------------------------------------------------------------
// Return:
// ------------------------------------------------------------------------------
void HAL_ETHNET_Power_Control( bool enable )
{
    uint16_t reg;

    if (enable)
    {
        // turn on the external oscillator
        (void) ethnet_esp32_control_rmii_clk(enable);

        // wait 3000us for the oscillator to stabilize
        ets_delay_us(3000);

        // turn ON the PHY
        ethnet_zConfig.phy->pwrctl(ethnet_zConfig.phy, enable);

        // wait 3000us for the PHY to power up
        ets_delay_us(3000);

        // get the current register value
        (void) HAL_ETHNET_ReadMdio(MDIO_BASIC_CONTROL, &reg);

        // enable the autonegotiation
        reg |= (MDIO_BASIC_CONTROL_AUTONEG_ENABLE |
                MDIO_BASIC_CONTROL_RESTART_AUTONEG);
        (void) HAL_ETHNET_WriteMdio(MDIO_BASIC_CONTROL, reg);
    }
    else
    {
        // get the current basic control register value
        (void) HAL_ETHNET_ReadMdio(MDIO_BASIC_CONTROL, &reg);

        // PHY datasheet says autoneg has to be disabled prior to power-down
        reg &= ~(MDIO_BASIC_CONTROL_AUTONEG_ENABLE |
                 MDIO_BASIC_CONTROL_RESTART_AUTONEG);
        (void) HAL_ETHNET_WriteMdio(MDIO_BASIC_CONTROL, reg);

        // wait 3000us for the registers to be updated
        ets_delay_us(3000);

        // turn OFF the PHY
        ethnet_zConfig.phy->pwrctl(ethnet_zConfig.phy, enable);

        // wait 3000us for the PHY to shutdown
        ets_delay_us(3000);

        // turn off the external oscillator
        (void) ethnet_esp32_control_rmii_clk(enable);
    }
}

// ------------------------------------------------------------------------------
// Function:    ethnet_esp32_control_rmii_clk
// ------------------------------------------------------------------------------
// Description: 
// ------------------------------------------------------------------------------
// Return:
// ------------------------------------------------------------------------------
static HAL_GPIO_RESULT_ENUM ethnet_esp32_control_rmii_clk( const bool enable )
{
#if CONFIG_DICM_TARGET_MARINE_GW_1
    return RMII_CLK_EN(enable ? 0 : 1);
#elif CONFIG_DICM_TARGET_MAXIMUS_DCM_1
    return RMII_CLK_EN_N(enable ? 0 : 1);
#endif
    return HAL_GPIO_ERROR_PIN_UNAVAILABLE;
}

// ------------------------------------------------------------------------------
// Function:    ethnet_esp32_control_rmii_rst
// ------------------------------------------------------------------------------
// Description: 
// ------------------------------------------------------------------------------
// Return:
// ------------------------------------------------------------------------------
static HAL_GPIO_RESULT_ENUM ethnet_esp32_control_rmii_rst(const bool assert)
{
#if CONFIG_DICM_TARGET_MARINE_GW_1
    return RMII_NRST(assert ? 0 : 1);
#elif CONFIG_DICM_TARGET_MAXIMUS_DCM_1
    return RMII_RST_N(assert ? 0 : 1);
#endif
    return HAL_GPIO_ERROR_PIN_UNAVAILABLE;
}

//-----------------------------------------------------------------------------
// Function:    HAL_ETHNET_GetMACAddress
//-----------------------------------------------------------------------------
// Description: 
//-----------------------------------------------------------------------------
uint64_t HAL_ETHNET_GetMACAddress(const int sock)
{
    struct sockaddr_in sa;
    socklen_t          slen = sizeof(sa);
    uint64_t           u64MACAddress = 0;
    if (lwip_getpeername(sock, (struct sockaddr *) &sa, &slen) >= 0)
    {
        ip4_addr_t ip4;
        inet_addr_to_ip4addr(&ip4, &sa.sin_addr);

        struct eth_addr  * pPeerEUI;
        const ip4_addr_t * pPeerIP;
        if (etharp_find_addr(NULL, &ip4, &pPeerEUI, &pPeerIP) >= 0)
        {
            (void) memset(&u64MACAddress, 0, sizeof(u64MACAddress));
            (void) memcpy(&u64MACAddress, pPeerEUI, ETH_HWADDR_LEN);
        }
    }
    return u64MACAddress;
}

//-----------------------------------------------------------------------------
// Function:    HAL_ETHNET_WriteMdio
//-----------------------------------------------------------------------------
// Description: 
//-----------------------------------------------------------------------------
int HAL_ETHNET_WriteMdio(const HAL_ETHNET_eMdioRegs reg, 
                         const uint16_t             val)
{
    esp_eth_mac_t * mac = ethnet_zConfig.mac;
    esp_err_t result = 
      mac->write_phy_reg(mac, ethnet_zPhyConfig.phy_addr, (uint32_t) reg, (uint32_t) val);
    return (unsigned int) result;
}

//-----------------------------------------------------------------------------
// Function:    HAL_ETHNET_ReadMdio
//-----------------------------------------------------------------------------
// Description: 
//-----------------------------------------------------------------------------
int HAL_ETHNET_ReadMdio(const HAL_ETHNET_eMdioRegs reg, 
                        uint16_t *                 val)
{
    uint32_t        tmp;
    esp_eth_mac_t * mac = ethnet_zConfig.mac;
    esp_err_t result = 
      mac->read_phy_reg(mac, ethnet_zPhyConfig.phy_addr, (uint32_t) reg, &tmp);
    *val = (uint16_t) tmp;
    return (unsigned int) result;
}

//-----------------------------------------------------------------------------
// Function:    ethnet_esp32_phy_init
//-----------------------------------------------------------------------------
// Description: Overrite default implementation
//-----------------------------------------------------------------------------
static esp_err_t 
ethnet_esp32_phy_init(esp_eth_phy_t *phy)
{
    esp_eth_mac_t * mac = ethnet_zConfig.mac;

    /* Power on Ethernet PHY */
    PHY_CHECK(phy->pwrctl(phy, true) == ESP_OK, "power control failed", err);

    /* Reset Ethernet PHY */
    PHY_CHECK(phy->reset_hw(phy) == ESP_OK, "reset failed", err);

    /* Check PHY ID */
    phyidr1_reg_t id1;
    phyidr2_reg_t id2;
    PHY_CHECK(mac->read_phy_reg(mac, ethnet_zPhyConfig.phy_addr, 
                                ETH_PHY_IDR1_REG_ADDR, &(id1.val)) == ESP_OK,
              "read ID1 failed", err);
    
    PHY_CHECK(mac->read_phy_reg(mac, ethnet_zPhyConfig.phy_addr, 
                                ETH_PHY_IDR2_REG_ADDR, &(id2.val)) == ESP_OK,
              "read ID2 failed", err);
    
    PHY_CHECK((id1.oui_msb == 0x7) && 
              (id2.oui_lsb == 0x30) && 
              (id2.vendor_model == 0xF), "wrong chip ID", err);

    return ESP_OK;
err:
    return ESP_FAIL;
}

//-----------------------------------------------------------------------------
// Function:    ethnet_esp32_phy_pwrctl
//-----------------------------------------------------------------------------
// Description: 
//-----------------------------------------------------------------------------
static esp_err_t 
ethnet_esp32_phy_pwrctl(esp_eth_phy_t *phy, bool enable)
{
#if CONFIG_DICM_TARGET_MARINE_GW_1 || CONFIG_DICM_TARGET_MAXIMUS_DCM_1
    RMII_PWR_EN(enable ? 1 : 0);
#endif

    return ESP_OK;
}

//-----------------------------------------------------------------------------
// Function:    ethnet_esp32_phy_reset_hw
//-----------------------------------------------------------------------------
// Description: 
//-----------------------------------------------------------------------------
static esp_err_t ethnet_esp32_phy_reset_hw(esp_eth_phy_t * phy)
{
    // assert ethernet PHY reset
    (void) ethnet_esp32_control_rmii_rst(true);
    
    if (ETHERNET_PHY == ETHERNET_PHY_LAN8720A)
    {
        ets_delay_us(200); // minimum reset assertion time for LAN8720A is 100us
    }

    // turn on the external oscillator
    (void) ethnet_esp32_control_rmii_clk(true);

    // wait 3000us for the oscillator to turn on
    ets_delay_us(3000);

    // de-assert ethernet PHY reset
    (void) ethnet_esp32_control_rmii_rst(false);
    
    if (ETHERNET_PHY == ETHERNET_PHY_LAN8720A)
    {
        ets_delay_us(2000); // settling time 
    }
    
    return ESP_OK;
}

//-----------------------------------------------------------------------------
// Function:    ethnet_esp32_log_ethernet
//-----------------------------------------------------------------------------
// Description: 
//-----------------------------------------------------------------------------
static void ethnet_esp32_log_ethernet(void *           arg, 
                                      esp_event_base_t base, 
                                      int32_t          event, 
                                      void *           data)
{
    switch(event)
    {
    case ETHERNET_EVENT_START:
      LOG(I, "Ethernet started.");
      break;
    case ETHERNET_EVENT_STOP:
      LOG(I, "Ethernet stopped.");
      break;
    case ETHERNET_EVENT_CONNECTED:
      LOG(I, "Ethernet connected.");
      ethnet_u32RxFrames = 0;
      break;
    case ETHERNET_EVENT_DISCONNECTED:
      LOG(I, "Ethernet disconnected.");
      xEventGroupClearBits(network_events, ETHERNET_IP_BIT);
      break;
    default:
      LOG(I, "Unknown ethernet event: %d.", event);
      break;
    }
}

//-----------------------------------------------------------------------------
// Function:    ethnet_esp32_on_ip
//-----------------------------------------------------------------------------
// Description: 
//-----------------------------------------------------------------------------
static void ethnet_esp32_on_ip(void *           arg, 
                               esp_event_base_t base, 
                               int32_t          event, 
                               void *           data)
{
    ip_event_got_ip_t *         pgip = (ip_event_got_ip_t *) data;
    const esp_netif_ip_info_t * ptai = &pgip->ip_info;

    switch(event)
    {
    case IP_EVENT_ETH_GOT_IP:
      LOG(I, "Ethernet On IP: %d.%d.%d.%d", IP2STR(&ptai->ip));
      (void) sprintf(ethnet_esp32_szDottedQuad, IPSTR, IP2STR(&ptai->ip));
      HAL_ETHNET_OnNewIpAddress(ethnet_esp32_szDottedQuad);

      LOG(I, "Ethernet got IP:");
      LOG(I, "  addr=%s", ethnet_esp32_szDottedQuad);
      LOG(I, "  mask=" IPSTR, IP2STR(&ptai->netmask));
      LOG(I, "    gw=" IPSTR, IP2STR(&ptai->gw));
      xEventGroupSetBits(network_events, ETHERNET_IP_BIT);
      break;
      
    default:
      LOG(I, "Unknown IP event: %d.", event);
      break;
    }
}	

//-----------------------------------------------------------------------------
// Function:    ethnet_esp32_on_lowlevel_init_done
//-----------------------------------------------------------------------------
// Description: 
//-----------------------------------------------------------------------------
static esp_err_t ethnet_esp32_on_lowlevel_init_done(esp_eth_handle_t h)
{
    esp_err_t retval = ESP_OK;

    // ensure GPIO0 pull-up is switched off
#if CONFIG_DICM_TARGET_MARINE_GW_1 || 0 // FIXME: smarter platform adaptation
    retval = gpio_pullup_dis(GPIO_NUM_0);
#endif
    return retval;
}
#else
void HAL_ETHNET_PLAT_InitializeA( HAL_ETHNET_zControlBlock * pzCB ){}
void HAL_ETHNET_PLAT_InitializeB( HAL_ETHNET_zControlBlock * pzCB ){}
#endif // CONFIG_DICM_TARGET_MARINE_GW_1 || CONFIG_DICM_TARGET_MAXIMUS_DCM_1
