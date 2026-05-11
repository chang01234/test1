/*! \file hal_i2c_master.c
	\brief I2C Master Hardware Abstraction Layer
*/

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "nrfx_twim.h"

#include "hal_i2c_master.h"

//! Persistent data for one TWI Master.
typedef struct i2c_state
{
    nrfx_twim_t     handle;
    bool            enabled;
    volatile bool   busy;
    volatile bool   error;
    int      sda;
    int      scl;
    uint32_t freq;
    bool     initialized;
}i2c_state;

//Note: NRFX HAL assumes there can be at most 3 Masters. We do the same.
//! Array of runtime state data for all TWI Masters. Indexed by instance id.
static i2c_state i2c_states[3] =
    {
        #if NRFX_CHECK(NRFX_TWIM0_ENABLED)
            [0] = {.handle = NRFX_TWIM_INSTANCE(0), .enabled = true, .sda = 0, .scl = 0, .freq = 0, .initialized = false},
        #endif
        #if NRFX_CHECK(NRFX_TWIM1_ENABLED)
            [1] = {.handle = NRFX_TWIM_INSTANCE(1), .enabled = true, .sda = 0, .scl = 0, .freq = 0, .initialized = false},
        #endif
        #if NRFX_CHECK(NRFX_TWIM2_ENABLED)
            [2] = {.handle = NRFX_TWIM_INSTANCE(2), .enabled = true, .sda = 0, .scl = 0, .freq = 0, .initialized = false},
        #endif
};

static i2c_state *i2c_interface_get(const int i2c_master_port);
static void i2c_interface_set(const int i2c_master_port, int sda, int scl, uint32_t freq);
static int i2c_xfer(i2c_state *i2c, nrfx_twim_xfer_desc_t *cmd);
static void i2c_int_handler(nrfx_twim_evt_t const *p_event, void *p_context);

/*! \brief Initialize I2C master
	
    \param i2c_master_port I2C port number
	\param sda_pin SDA pin on host
	\param scl_pin SCL pin on host
	\param freq Bitrate to use
	
    \return HAL_E_OK if successful. Standard HAL error codes otherwise
 */
int hal_i2c_master_init(const int i2c_master_port, const int sda_pin, const int scl_pin, uint32_t freq)
{
    ret_code_t err_code;
    i2c_state *i2c = i2c_interface_get(i2c_master_port);

    nrfx_twim_config_t twim_conf= {
        .scl                = scl_pin,
        .sda                = sda_pin,
        .interrupt_priority = APP_IRQ_PRIORITY_MID,
        .hold_bus_uninit    = false
    };

    switch (freq)
    {
    case 100000:
        twim_conf.frequency = NRF_TWIM_FREQ_100K;
        break;
    case 250000:
        twim_conf.frequency = NRF_TWIM_FREQ_250K;
        break;
    case 400000:
        twim_conf.frequency = NRF_TWIM_FREQ_400K;
        break;
    default:
        APP_ERROR_HANDLER(0);    //Invalid bitrate specified.
    }

    if (i2c->initialized)
    {
        if ((i2c->sda != sda_pin) || (i2c->scl != scl_pin) || (i2c->freq != freq))
        {
            APP_ERROR_HANDLER(0);   // Multiple configurations for same I2C port not allowed.
        }
    }
    else
    {
        APP_ERROR_CHECK(nrfx_twim_init(&i2c->handle, &twim_conf, i2c_int_handler, i2c));
        nrfx_twim_enable(&i2c->handle);
        
         //Update the configuration parameters for the i2c port
        i2c_interface_set(i2c_master_port, sda_pin, scl_pin, freq);   
    }
    
    return HAL_E_OK;   // If we reach this point no error occurred.
}

/*! \brief Write data to I2C device
	
    \param i2c_master_port I2C port number
	\param device_address Slave address to communicate with
	\param data_wr Data to send to slave
	\param size Size of data to send to slave
	
    \return HAL_E_OK if successful. Standard HAL error codes otherwise
 */
int hal_i2c_master_write(const int i2c_master_port, const uint16_t device_address, const uint8_t *data_wr, const size_t size)
{
    i2c_state *i2c = i2c_interface_get(i2c_master_port);
    nrfx_twim_xfer_desc_t cmd = NRFX_TWIM_XFER_DESC_TX(device_address, (uint8_t *)data_wr, size);
    
    return i2c_xfer(i2c, &cmd);
}

/*! \brief Read data from I2C device
	
    \param i2c_master_port I2C port number
	\param device_address Slave address to communicate with
	\param data_rd Data read from slave
	\param size Size of data to read from slave
	
    \return HAL_E_OK if successful. Standard HAL error codes otherwise
 */
int hal_i2c_master_read(const int i2c_master_port, const uint16_t device_address, uint8_t *const data_rd, const size_t size)
{
    i2c_state *i2c = i2c_interface_get(i2c_master_port);
    nrfx_twim_xfer_desc_t cmd = NRFX_TWIM_XFER_DESC_RX(device_address, data_rd, size);

    return i2c_xfer(i2c, &cmd);
}

/*! \brief Write and read data from I2C device in one transaction using restart bit
	\param i2c_master_port I2C port number
	\param device_address Slave address to communicate with
	\param data_wr Data to send to slave
	\param size_wr Size of data to send to slave
	\param data_rd Data read from slave
	\param size_rd Size of data to read from slave
	\return FALSE if successful
 */
int hal_i2c_master_writeread(const int i2c_master_port, const uint16_t device_address, const uint8_t *data_wr, const size_t size_wr, uint8_t *const data_rd, const size_t size_rd)
{
    i2c_state *i2c = i2c_interface_get(i2c_master_port);
    nrfx_twim_xfer_desc_t cmd = NRFX_TWIM_XFER_DESC_TXRX(device_address, (uint8_t *)data_wr, size_wr, data_rd, size_rd);
    
    return i2c_xfer(i2c, &cmd);
}

/*! \brief Validate that provided port number and return corresponding i2c_state
    
    \param i2c_master_port I2C port number
    
    \return i2c port state address
*/        
static i2c_state *i2c_interface_get(const int i2c_master_port)
{
    if (i2c_master_port >= sizeof(i2c_states)/sizeof(i2c_states[0]) 
        || !i2c_states[i2c_master_port].enabled)
    {
        APP_ERROR_HANDLER(0);    // Invalid port specified.
    }
    
    return &i2c_states[i2c_master_port];
}

/*! \brief Set the information about I2C port configuration (port number, pins, frequency)
        \param i2c_master_port I2C port number
        \param sda SDA pin
        \param scl SCL pin
        \param freq frequency
*/   
static void i2c_interface_set(const int i2c_master_port, int sda, int scl, uint32_t freq)
{
    if (i2c_master_port >= sizeof(i2c_states)/sizeof(i2c_states[0]) 
        || !i2c_states[i2c_master_port].enabled)
    {
        APP_ERROR_HANDLER(0);    // Invalid port specified.
    }
    i2c_states[i2c_master_port].sda = sda;
    i2c_states[i2c_master_port].scl = scl;
    i2c_states[i2c_master_port].freq = freq;
    i2c_states[i2c_master_port].initialized = true;
}

/*! \brief Transfer data on I2C bus

    \param i2c Struct holding state of I2C peripheral to be used
    \param cmd Transfer settings
    
    \return HAL_E_OK if successful. Standard HAL error codes otherwise
*/        
static int i2c_xfer(i2c_state *i2c, nrfx_twim_xfer_desc_t *cmd)
{
    ret_code_t err_code;
    
    i2c->busy = true;
    err_code = nrfx_twim_xfer(&i2c->handle, cmd, 0);
    switch (err_code)
    {
    case NRFX_SUCCESS:
        // Do nothing on success.
        break;
    case NRFX_ERROR_BUSY:
        return HAL_E_BUSY;
    default:
        return HAL_E_COMM;
    }
    
    while(i2c->busy)
    {
        if (i2c->error) 
        { 
            i2c->busy = false;
            i2c->error = false;
            return HAL_E_COMM;
        }
    }

    return HAL_E_OK;   // If we reach this point no error occurred.
}

static void i2c_int_handler(nrfx_twim_evt_t const *p_event, void *p_context)
{
    i2c_state *i2c = (i2c_state *)p_context;
    
    if (p_event->type == NRFX_TWIM_EVT_DONE)
    {
        i2c->busy = false;
    }
    else
    {
        i2c->error = true;
    }
}