/*! \file varstate.c
 *  \brief Module for managing variables defined by HMI data.
 *
 *  The varstate module used to to init, set, and get variables defined in HMI
 *  data. It also manages the microcontroller ADC.
 *
 *  These variables are accessed by index 0 - \p num_vars. Indexes are
 *  normally assigned to variables by readpng during data generation. Code that
 *  requires access to state data uses \p varstate_get() and \p varstate_set().
 *
 *  In addition the functions \p varstate_init() and \p varstate_update() are
 *  Provided. \p varstate_init() must be called after boot before accessing
 *  state data using set or get. \p varstate_update() manages state updates from
 *  ADC data and should be called periodically.
 */

/** Includes ******************************************************************/
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "configuration.h"
#include "ddm2_parameter_list.h"
#include "hal_mem.h"
#include "varstate.h"
#include "screen.h"
#include "ddmp_uart.h"

typedef struct hmi_struct_data
{
    uint8_t data[32 + 1];   // + 1 byte len
} hmi_struct_data_t;

/** Variables *****************************************************************/
static EXT_RAM_ATTR int32_t *p_hmi_state;       //!< System state storage.
static EXT_RAM_ATTR hmi_struct_data_t *p_hmi_struct_data;   //!< System state storage. struct vars
static EXT_RAM_ATTR bool *p_var_ddmp_changed;   //!< Var updated from DDMP.
EXT_RAM_ATTR int32_t *p_default_val;            //!< Store the default values from vars.txt.
EXT_RAM_ATTR int32_t *p_ddmp_set_sub_array;     //!< Store the ddmp set values

static const VARSTATE_DEF *var_defs;            //!< List of variable definitions. Loaded during init.
static uint8_t num_vars;                        //!< Number of defined variables. Loaded during init.

static const VARSTATE_DYNMAPTABLE_DEF *p_dyn_map_table = NULL;

static void (*varstate_callback)(uint8_t var_index) = NULL;

/** Private prototypes ********************************************************/

/*! \brief Initialize values of all HMI variables.
 *
 * 	Sets all HMI variables to their default values.
 *
 * 	\param config   Struct containing varstate module configuration data.
 */
void varstate_init(const VARSTATE_CONF *config, void (*varstate_cb)(uint8_t))
{
    uint8_t num_struct_vars = 0;
    varstate_callback = varstate_cb;
	
    // Load HMI variable config.
    var_defs = (const VARSTATE_DEF *)HMI_DATA_ADDRESS(config->var_def_array);
    num_vars = config->num_var_defs;
    // Allocate buffers
    p_hmi_state = NULL;
    p_hmi_struct_data = NULL;
    p_var_ddmp_changed = NULL;
    p_default_val = NULL;
    p_ddmp_set_sub_array = NULL;
    p_hmi_state = (int32_t*)hal_mem_malloc_prefer(num_vars * sizeof(int32_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    p_var_ddmp_changed = (bool*)hal_mem_malloc_prefer(num_vars * sizeof(bool), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    p_default_val = (int32_t*)hal_mem_malloc_prefer(num_vars * sizeof(int32_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    p_ddmp_set_sub_array = (int32_t*)hal_mem_malloc_prefer(num_vars * sizeof(int32_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
    // Check that defined variables fit in allocated RAM.
    assert(p_hmi_state != NULL);
    assert(p_var_ddmp_changed != NULL);
    assert(p_default_val != NULL);
    assert(p_ddmp_set_sub_array != NULL);
    // Find out how many struct variables
    for (uint8_t i = 0; i < num_vars; ++i)
    {
        DDM2_TYPE_ENUM type;
        varstate_get_parameter_type(i, &type);
        if (type == DDM2_TYPE_STRUCT)
        {
            num_struct_vars++;
        }
    }
    if (num_struct_vars > 0)
    {
        // Allocate default space for struct data in varstate data
        p_hmi_struct_data = (hmi_struct_data_t*)hal_mem_malloc_prefer(num_struct_vars * sizeof(hmi_struct_data_t), HAL_MEM_SPIRAM, HAL_MEM_INTERNAL_RAM);
        assert(p_hmi_struct_data != NULL);
    }
    uint8_t num_struct_vars_index = 0;
    // Load default values for all HMI variables into RAM.
    for (uint8_t i = 0; i < num_vars; ++i)
    {
#ifdef HMI_VARSTATE_DEBUG_NAMES
        LOG(D, "Loading default: %d, into: %s", var_defs[i].default_value, var_defs[i].name)
#endif
        p_hmi_state[i] = var_defs[i].default_value;
        p_var_ddmp_changed[i] = false;
        p_default_val[i] = var_defs[i].default_value; //// Load default values for all HMI variables into default array
        p_ddmp_set_sub_array[i] = var_defs[i].default_value; // Load default values for all HMI variables into ddmp_set array
        DDM2_TYPE_ENUM type;
        varstate_get_parameter_type(i, &type);
        if (type == DDM2_TYPE_STRUCT)
        {
            assert(num_struct_vars_index < num_struct_vars);
            p_hmi_struct_data->data[0] = 4; // Length 4 bytes
            *(int32_t*)(&p_hmi_struct_data->data[1]) = var_defs[i].default_value;
            p_hmi_state[i] = num_struct_vars_index++;       // State variable is instead a link into struct state
        }
    }
    if (hmi_data_is_feature_supported(HMI_DATA_FEATURE_DYNAMIC_MAPPING))
    {
        // save a pointer to dynamic map table
        p_dyn_map_table = (const VARSTATE_DYNMAPTABLE_DEF *)HMI_DATA_ADDRESS(config->dyn_map_table);
    }
    else
    {
        p_dyn_map_table = NULL;
    }
}

const VARSTATE_DYNENTRY_DEF *varstate_get_dyn_table_entry(int index)
{
    if (p_dyn_map_table && (index < (int)p_dyn_map_table->len))
    {
        return (const VARSTATE_DYNENTRY_DEF *)&p_dyn_map_table->table[index];
    }
    else
    {
        return (const VARSTATE_DYNENTRY_DEF *)NULL;
    }
}

const VARSTATE_DYNENTRY_DEF *varstate_find_any_dyn_table_entry_from_var_index(uint8_t var_index)
{
    if (p_dyn_map_table)
    {
        for (int i = 0; i < (int)p_dyn_map_table->len; ++i)
        {
            if (p_dyn_map_table->table[i].var_index == var_index)
            {
                return (const VARSTATE_DYNENTRY_DEF *)&p_dyn_map_table->table[i];
            }
        }
    }
    return (const VARSTATE_DYNENTRY_DEF *)NULL;
}

/*! \brief Set value of HMI variable with provided index to value.
 *
 *  \param var_index    Index of HMI variable to set.
 *  \param value        New value for specified HMI variable.
 */
void varstate_set(uint8_t var_index, int32_t value)
{
    DDM2_TYPE_ENUM type;
    if (var_index >= num_vars)	// Check if index is valid.
    {
        LOG(E, "Variable out of range! var_index=%d, num_vars=%d", var_index, num_vars);
        return;
    }
    (void)varstate_get_parameter_type(var_index, &type);
    if (type == DDM2_TYPE_STRUCT)
    {
        uint8_t *pdata = (uint8_t*)value;
        p_hmi_struct_data[p_hmi_state[var_index]].data[0] = *pdata;
        memcpy(&p_hmi_struct_data[p_hmi_state[var_index]].data[1], &pdata[1], *pdata);
        LOG(D, "Setting DDM2_TYPE_STRUCT (var: %d) size %d", var_index, *pdata);
    }
    else
    {
        // Only run update functions if value was changed.
        if (p_hmi_state[var_index] != value)
        {
            p_hmi_state[var_index] = value;

            if (varstate_callback != NULL)
            {
                varstate_callback(var_index);
            }
            screen_update(var_index);
        }
    }
}

/*! \brief Set value of HMI variable with provided index to value.
 *
 *  \param var_index    Index of HMI variable to set.
 * 	\param value        New value for specified HMI variable.
 */
void varstate_set_no_screen_upd(uint8_t var_index, int32_t value)
{
    if (var_index >= num_vars)	// Check if index is valid.
    {
        LOG(E, "Variable out of range! var_index=%d, num_vars=%d", var_index, num_vars);
        return;
    }
    // Only run update functions if value was changed.
    if (p_hmi_state[var_index] != value)
    {
        p_hmi_state[var_index] = value;

        if (varstate_callback != NULL)
        {
            varstate_callback(var_index);
        }
    }
}

/*! \brief Set changed state of HMI variable with provided index.
 *
 *  \param var_index    Index of HMI variable to set.
 * 	\param changed      New DDMP changed flag.
 */
void varstate_set_changed(uint8_t var_index, bool changed)
{
    if (var_index >= num_vars)	// Check if index is valid.
    {
        LOG(E, "Variable out of range! var_index=%d, num_vars=%d", var_index, num_vars);
        return;
    }

    // Register DDMP change.
    p_var_ddmp_changed[var_index] = changed;
}

/*! \brief Resets the changed state flag of all variables.
 *
 * Typically used when changing to a different screen.
 */
void varstate_clear_all_changed(void)
{
    for (uint8_t i = 0; i < num_vars; ++i)
    {
        p_var_ddmp_changed[i] = false;
    }
}

/*! \brief Get value of HMI variable with provided index.
 *
 * 	\param var_index    Index of HMI variable to get.
 *
 * 	\returns    Value of specified state variable.
 */
int32_t varstate_get(uint8_t var_index)
{
    if (var_index >= num_vars)	// Check if index is valid.
    {
        LOG(E, "Variable out of range! var_index=%d, num_vars=%d", var_index, num_vars);
        return INT32_MAX;
    }
    DDM2_TYPE_ENUM type;
    (void)varstate_get_parameter_type(var_index, &type);
    if (type == DDM2_TYPE_STRUCT)
    {
        if (p_hmi_struct_data[p_hmi_state[var_index]].data[0] == 0)
        {
            LOG(I, "Variable (struct) has no data! var_index=%d", var_index);
            return INT32_MAX;
        }
        return (int32_t)p_hmi_struct_data[p_hmi_state[var_index]].data;
    }

    return p_hmi_state[var_index];
}

/*! \brief Get validate data filtered using offset and length. Function will assert if failing
 *
 *  \param[in] is_struct    indicates if data corresponds to a struct type of data storage format
 *  \param[in] p_indata     pointer to data input
 *  \param[out] p_outdata   pointer to validated and filtered data
 *  \param[in] offset       offset value, used when filtering
 *  \param[in] len          lenght of data to filter out, used when filtering
 *
 *  \returns    true if output is valid
 */
bool varstate_get_validated_data(bool *is_struct, uint8_t var_index, int32_t *p_outdata, uint8_t offset, uint8_t len)
{
    bool validated = true;
    int32_t retval = 0;
    
    DDM2_TYPE_ENUM type;
    (void)varstate_get_parameter_type(var_index, &type);
    if (type == DDM2_TYPE_STRUCT)
    {
        if ((offset == 0) && (len == 0))
        {
            // Bad argument data.
            LOG(E, "Bad arguments");
            validated = false;
        }
        else
        {
            uint8_t *p_indata = (uint8_t *)varstate_get(var_index);
            if ((int32_t)p_indata == INT32_MAX)
            {
                // Bad data
                validated = false;
            }
            // Validata data length
            else if (p_indata[0] >= (offset + len))
            {
                switch (len)
                {
                    case 1:
                        retval = p_indata[offset+1];
                        break;
                     case 2:
                        retval = *(int16_t*)(&p_indata[offset+1]);
                        break;
                     case 4:
                        retval = *(int32_t*)(&p_indata[offset+1]);
                        break;
                     default:
                        assert(0);
                }
            }
            else
            {
                retval = INT32_MAX;
                validated = false;
            }
        }
    }
    else
    {
        int32_t indata = varstate_get(var_index);
        // Validate data length
        if ((offset == 0) && (len == 0))
        {
            // No special filtering required
            retval = indata;
        }
        else if (sizeof(retval) >= (offset + len))
        {
            retval = indata;
            retval = (retval >> (8 * len * offset));
            switch (len)
            {
                case 1:
                    retval = (retval & 0xFF);
                    break;
                 case 2:
                    retval = (retval & 0xFFFF);
                    break;
                 case 4:
                    retval = (retval & 0xFFFFFFFF);
                    break;
                 default:
                    assert(0);
            }
        }
        else
        {
            LOG(E, "Bad arguments");
            validated = false;
        }
    }
    if (validated && p_outdata)
    {
        *p_outdata = retval;
    }
    if (is_struct)
    {
        *is_struct = (type == DDM2_TYPE_STRUCT);
    }
    return validated;
}

/*! \brief Get changed state of HMI variable with provided index.
 *
 * 	\param var_index    Index of HMI variable to get.
 *
 * 	\returns    Changed state of specified variable.
 */
bool varstate_get_changed(uint8_t var_index)
{
    if (var_index >= num_vars)	// Check if index is valid.
    {
        LOG(E, "Variable out of range! var_index=%d, num_vars=%d", var_index, num_vars);
        return INT32_MAX;
    }

    return p_var_ddmp_changed[var_index];
}

/*! \brief Get definition data for HMI variable with provided index.
 *
 * 	\param var_index    Index of HMI variable definition to get.
 *
 * 	\returns    Pointer to requested HMI variable definition.
 */
const VARSTATE_DEF *varstate_def_get(uint8_t var_index)
{
    if (var_index >= num_vars)	// Check if index is valid.
    {
        LOG(E, "Variable out of range! var_index=%d, num_vars=%d", var_index, num_vars);
        return NULL;
    }

    return &var_defs[var_index];
}

/* Get the first index from publish list and Last index from Subscribe list  */
void varstate_get_first_and_last_index(uint8_t * first_index, uint8_t * last_index)
{
    const VARSTATE_DDMP_PARAMETER *sub1 = (const VARSTATE_DDMP_PARAMETER *)(def_data_header.sub_list ? HMI_DATA_ADDRESS(def_data_header.sub_list) : (uint32_t)def_data_header.sub_list);
    const VARSTATE_DDMP_PARAMETER *pub1 = (const VARSTATE_DDMP_PARAMETER *)(def_data_header.pub_list ? HMI_DATA_ADDRESS(def_data_header.pub_list) : (uint32_t)def_data_header.pub_list);
    const VARSTATE_DDMP_PARAMETER *prev_pub = NULL;

    while (pub1 != NULL)
    {
        prev_pub = pub1;
        pub1 = (pub1->next_entry != NULL ? (const VARSTATE_DDMP_PARAMETER *)HMI_DATA_ADDRESS(pub1->next_entry) : NULL);
    }

    *first_index = prev_pub->var_index;

    if (sub1 != NULL)
    {
        *last_index = sub1->var_index;
    }
}

uint8_t varstate_get_var_index(uint32_t param1)
{
    const VARSTATE_DDMP_PARAMETER *sub1 = (const VARSTATE_DDMP_PARAMETER *)(def_data_header.sub_list ? HMI_DATA_ADDRESS(def_data_header.sub_list) : (uint32_t)def_data_header.sub_list);
    uint8_t index = VARSTATE_INVALID_INDEX;

    while (sub1 != NULL)
    {
        if (sub1->parameter_id == param1)
        {
            index = sub1->var_index;
            break;
        }
        sub1 = (sub1->next_entry != NULL ? (const VARSTATE_DDMP_PARAMETER *)HMI_DATA_ADDRESS(sub1->next_entry) : NULL);
    }
    return index;
}

uint8_t varstate_get_var_index_both(uint32_t param1)
{
    const VARSTATE_DDMP_PARAMETER *sub1 = (const VARSTATE_DDMP_PARAMETER *)(def_data_header.sub_list ? HMI_DATA_ADDRESS(def_data_header.sub_list) : (uint32_t)def_data_header.sub_list);
    const VARSTATE_DDMP_PARAMETER *pub1 = (const VARSTATE_DDMP_PARAMETER *)(def_data_header.pub_list ? HMI_DATA_ADDRESS(def_data_header.pub_list) : (uint32_t)def_data_header.pub_list);
    uint8_t index = VARSTATE_INVALID_INDEX;
    uint8_t found_in_sub = 0;

    while (sub1 != NULL)
    {
        if (sub1->parameter_id == param1)
        {
            index = sub1->var_index;
            found_in_sub = 1;
            break;
        }
        sub1 = (sub1->next_entry != NULL ? (const VARSTATE_DDMP_PARAMETER *)HMI_DATA_ADDRESS(sub1->next_entry) : NULL);
    }

    if (!found_in_sub)
    {
        while (pub1 != NULL)
        {
            if (pub1->parameter_id == param1)
            {
                index = pub1->var_index;
                break;
            }
            pub1 = (pub1->next_entry != NULL ? (const VARSTATE_DDMP_PARAMETER *)HMI_DATA_ADDRESS(pub1->next_entry) : NULL);
        }
    }
    return index;
}

/*! \brief Get DDMP2 Parameter ID for given Index
 *
 *  \param var_index Variable Array Index
 *  Return the Parameter ID
 */
uint32_t varstate_get_parameter_id(uint8_t var_index)
{
    const VARSTATE_DDMP_PARAMETER *sub1 = (const VARSTATE_DDMP_PARAMETER *)(def_data_header.sub_list ? HMI_DATA_ADDRESS(def_data_header.sub_list) : (uint32_t)def_data_header.sub_list);
    const VARSTATE_DDMP_PARAMETER *pub1 = (const VARSTATE_DDMP_PARAMETER *)(def_data_header.pub_list ? HMI_DATA_ADDRESS(def_data_header.pub_list) : (uint32_t)def_data_header.pub_list);
    uint32_t parameter_id = 0;
    uint8_t found_in_sub = 0;

    while (sub1 != NULL)
    {
        if (sub1->var_index == var_index)
        {
            parameter_id = sub1->parameter_id;
            found_in_sub = 1;
            break;
        }
        sub1 = (sub1->next_entry != NULL ? (const VARSTATE_DDMP_PARAMETER *)HMI_DATA_ADDRESS(sub1->next_entry) : NULL);
    }

    if (!found_in_sub)
    {
        while (pub1 != NULL)
        {
            if (pub1->var_index == var_index)
            {
                parameter_id = pub1->parameter_id;
                break;
            }
            pub1 = (pub1->next_entry != NULL ? (const VARSTATE_DDMP_PARAMETER *)HMI_DATA_ADDRESS(pub1->next_entry) : NULL);
        }
    }
    return parameter_id;
}

/*! \brief Get DDMP2 Parameter type for given Index
 *
 *  \param[in] var_index    Variable Array Index
 *  \param[out] ptype       Type of parameter
 *  \return Return the Parameter Length
 */
uint8_t varstate_get_parameter_type(uint8_t var_index, DDM2_TYPE_ENUM *ptype)
{
    const VARSTATE_DDMP_PARAMETER *sub1 = (const VARSTATE_DDMP_PARAMETER *)(def_data_header.sub_list ? HMI_DATA_ADDRESS(def_data_header.sub_list) : (uint32_t)def_data_header.sub_list);
    const VARSTATE_DDMP_PARAMETER *pub1 = (const VARSTATE_DDMP_PARAMETER *)(def_data_header.pub_list ? HMI_DATA_ADDRESS(def_data_header.pub_list) : (uint32_t)def_data_header.pub_list);
    const VARSTATE_DDMP_PARAMETER *common = NULL;
    uint8_t parameter_length = 0;
    uint8_t found_in_sub = 0;

    while (sub1 != NULL)
    {
        if (sub1->var_index == var_index)
        {
            common = sub1;
            found_in_sub = 1;
            break;
        }
        sub1 = (sub1->next_entry != NULL ? (const VARSTATE_DDMP_PARAMETER *)HMI_DATA_ADDRESS(sub1->next_entry) : NULL);
    }

    if (!found_in_sub)
    {
        while (pub1 != NULL)
        {
            if (pub1->var_index == var_index)
            {
                common = pub1;
                break;
            }
            pub1 = (pub1->next_entry != NULL ? (const VARSTATE_DDMP_PARAMETER *)HMI_DATA_ADDRESS(pub1->next_entry) : NULL);
        }
    }

    if (common != NULL)
    {
        if (ptype)
        {
            *ptype = common->type; 
        }
        switch (common->type)
        {
            case DDM2_TYPE_UINT32_T:
                /* fallthrough */
            case DDM2_TYPE_INT32_T:
                parameter_length = 4;
                break;
            case DDM2_TYPE_STRUCT:
                /* fallthrough */
            case DDM2_TYPE_OTHER:
                /* fallthrough */
            case DDM2_TYPE_STRING:
                // parameter_length is not known
                break;
            default:
                assert(0);	// Unknown parameter type.
        }
    }
    else 
    {
        // Not found
        *ptype = DDM2_TYPE_NONE;
    }
    return parameter_length;
}

/*! \brief While we are in the Favorite Screen if user pressed the Middle Button
 *  then only we are storing and retrieving the values from the EEPROM. once it is done, reset the fav_confirm_but as zero
 *
 *  \param  fav_confirm_but_arr_index.
 *
 *	\returns Nothing.
 */
void clear_fav_confirm_button(uint8_t fav_confirm_but_arr_index)
{
    p_hmi_state[fav_confirm_but_arr_index] = 0;
}

void set_hmi_state_array(uint8_t hmi_state_index, uint32_t hmi_state_value)
{
    p_hmi_state[hmi_state_index] = hmi_state_value;
}


/*! \brief Load the Default values when ddmp_set and subscribe array usage completed
 *
 * 	Load the Default values to ddmp_set_sub array
 *
 *  \returns		Nothing
 */
void load_default_values_to_ddmp_set_array(void)
{
    // Load default values.
    for (uint8_t var_arr_index = 0; var_arr_index < num_vars; ++var_arr_index)
    {
        p_ddmp_set_sub_array[var_arr_index] = p_default_val[var_arr_index];
    }
}

/*! \brief Set value of ddmp HMI variable with provided index to value.
 *
 *  \param var_index    Index of ddmp HMI variable to set.
 * 	\param value        New value for specified HMI variable.
 */
void ddmp_varstate_set(uint8_t var_index, int32_t value)
{

    assert(var_index < num_vars);	// Check if index is valid.

    // Only run update functions if value was changed.
    if (p_hmi_state[var_index] != value)
    {
        p_hmi_state[var_index] = value;

        //ddmp_uart_varstate_cb(var_index, value);
        ddmp_uart_varstate_set(var_index, value);
        screen_update(var_index);
    }
}

/*! \brief wrapper function to call ddmp_uart_set function.
 *
 * 	\param var_index    Index of ddmp HMI variable to set.
 * 	\param value        New value for specified HMI variable.
 */

void ddmp_uart_varstate_set(uint8_t var_index, int32_t value)
{
    uint32_t parameter_id = varstate_get_parameter_id(var_index);
    uint8_t parameter_length = varstate_get_parameter_type(var_index, NULL);

    if ((parameter_id > 0) && (parameter_length > 0)) //local variable no need to set
    {
        ddmp_uart_set(parameter_id, value, parameter_length);
    }
}
