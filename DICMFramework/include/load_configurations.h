/**
 * \file        load_configurations.h
 * \date        2022-09-27
 * \author      (BB) Borjan Bozhinovski <borjan.bozhinovski@seavus.com>
 * \brief       Configurations loader.
 * 
 * Loads custom configurations located in non-volatile memory, that are not consisted
 * of DDM2 parameters. Each descriptor has it's own implementation of the loading functionality
 * that should populate the intended connector with specific data.
 *
 * \li          2022-09-27 (BB) Initial implementation
 *
 * \copyright   Dometic Group
 *              This source file and the information contained in it are
 *              confidential and proprietary to Dometic Group
 *              The reproduction or disclosure, in whole or in part,
 *              to anyone outside of Dometic Group without the written
 *              approval of a Dometic Group officer under a Non-Disclosure
 *              Agreement is expressly prohibited.
 *
 *              All rights reserved
 */

#ifndef LOAD_CONFIGURATION_H_
#define LOAD_CONFIGURATION_H_

#include <stdint.h>
#include "configuration.h"

#if defined(LOAD_CONFIGURATIONS)
 //!< Describes the location of the configuration
typedef enum _LOAD_CONFIGURATION__LOCATION
{
    LOAD_CONFIGURATION__LOCATION_ROM = 0,
    LOAD_CONFIGURATION__LOCATION_FILE_SYSTEM = 1,
} LOAD_CONFIGURATION__LOCATION;

/*! \~ Static configuration structre

    Describes the elements required to define the static
    configuration beeing loaded from ROM memory.
*/
struct load_configurations__static_configuration
{
    const void * configuration;                     //!< Pointer to an array of structures that holds configuration elements.
    uint32_t configuration_size;                    //!< Specifies the size of configuration array. Usually ELEMENTS macro is used to calculate array size.
    const char * configuration_name;                //!< The name of the configuration
};

/*! \~  Configuration structure

    Describes the elements required to define the configuration beeing loaded.
*/
struct load_configurations__configuration
{
    LOAD_CONFIGURATION__LOCATION configuration_location_type;                   /*< \~ Describes the location of the configuration.
                                                                                   Configuration can be statically defined as an array located in ROM(LOAD_CONFIGURATION__LOCATION_ROM)
                                                                                   or can be defined in a file located on the filesystem(LOAD_CONFIGURATION__LOCATION_FILE_SYSTEM)
                                                                                */
    const struct load_configurations__static_configuration * static_config;     //!< Pointer to static configuration located on ROM.
};

/*! \~  Load configurations descriptor

    Describes the elements requried to load custom configurations from memory.
    Custom configurations are NOT consisted of DDM2 parameters that can be loaded
    and set trough the broker. All the parameters present in the configurations
    should be loaded manualy to the dedicated connector trough the custom function
    that has to be implemented for each descriptor separately.
*/
struct load_configurations__descriptor
{
    const struct load_configurations__configuration * custom_configurations;                        /*!< \~ Pointer to an array of structures that holds
                                                                                                        the configurations that need to be loaded.
                                                                                                    */
    uint32_t custom_configurations_size;                                                            /*!< \~ Specifies the size of custom_configurations array.
                                                                                                        Usually ELEMENTS macro is used to calculate array size.
                                                                                                    */
    int (* load_custom_configurations)(const struct load_configurations__configuration * config);   //!< Specifies the loading function which has to be implemented for each descriptor
    const char * descriptor_name;                                                                   //!< The name of the descriptor
};

/*! \~  Load configurations handle structure

    This structure keeps list of descriptors whose
    configurations should be loaded from memory.
 */
struct load_configurations
{
    const struct load_configurations__descriptor * descriptors;
};


/*! \brief Load custom configurations

    Load configurations listed in \a load_configurations__configuration array for
    each of the descriptors listed in the \a load_configurations__descriptors.
    It should be invoked by the \a board_initialization() function of each configuration
    source file that intends to initialize connectors to a specific starting state.

    Example:
    shape_amb_3.c
        #include "load_configurations.h"
        ...
        void board_initialization(void)
        {
            ...
            #if defined(LOAD_CONFIGURATIONS)
                load_configurations();
            #endif
            ...
        }
 */
void load_configurations(void);

#else
void load_configurations(void) {}
#endif
#endif //LOAD_CONFIGURATION_H_