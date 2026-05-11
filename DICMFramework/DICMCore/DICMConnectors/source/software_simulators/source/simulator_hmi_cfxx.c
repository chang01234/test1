/**
 * \file
 * \date        2023-10-25
 * \author      (NR) Nenad Radulovic (nenad.radulovic@gmail.com)
 * \brief       Simulator for HMI DDM2 class - variant CFX2` implementation
 *
 * Implementation of tables to simulate HMI DDM2 class - variant `CFX2`
 *
 * \li          2022-10-25  (NR) Initial implementation
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
#include <stdint.h>

#include "ddm2.h"  // ELEMENTS
#include "ddm2_parameter_list.h"
#include "ddm_store.h"
#include "esp_attr.h"  // EXT_RAM_ATTR
#include "simulator_hmi_cfxx.h"
#include "software_simulator.h"
#include "software_simulator_defaults.h"

/**
 * @brief 		Make a concrete HMI0EVENT structure with one element inside.
 *
 * In order to be able to statically allocate and initialize HMI0EVENT structure,
 * we created this structure which has the definition the same as HMI0EVENT_T
 * but instead of flexible array member we have a real array with 1 member.
 */
typedef struct PACKED HMI0EVENT_CONCRETE_T
{
    uint32_t id;
    uint8_t data[1];
} PACKED HMI0EVENT_CONCRETE_T;

static HMI0EVENT_CONCRETE_T initial_event = {};

static const struct ddm_store_ddm s__ddm_owned_initial_values[] = {
    {.ddm_parameter = HMI0AVL, .value = {.storage = {.i32 = 1}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = HMI0VER, .value = {.storage = {.str = "version"}, .type = DDM2_TYPE_STRING}},
    {.ddm_parameter = HMI0TEMPUNIT, .value = {.storage = {.i32 = HMI0TEMPUNIT_CMETRIC}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = HMI0BACKLIGHT, .value = {.storage = {.i32 = 80}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = HMI0SOUND, .value = {.storage = {.i32 = HMI0SOUND_CLICKON}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = HMI0TIMEFORMAT, .value = {.storage = {.i32 = HMI0TIMEFORMAT_24H}, .type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = HMI0EVENT, .value = {.storage = {.structure = &initial_event}, .size = sizeof(initial_event), .type = DDM2_TYPE_STRUCT}},
    {.ddm_parameter = HMI0VARDATA, .value = {.type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = HMI0MUTE, .value = {.type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = HMI0CHILDLOCK, .value = {.type = DDM2_TYPE_INT32_T}},
    {.ddm_parameter = HMI0SCREENTIMEOUT, .value = {.storage = {.i32 = 300000}, .type = DDM2_TYPE_INT32_T}},
};

DDM_STORE__DECLARE_EXTRAM(s__ddm_owned_store, ELEMENTS(s__ddm_owned_initial_values));

const SOFTWARE_SIMULATOR__DESCRIPTOR g__class_hmi_cfxx_descriptor = {
    .ddm_class = DDM2_PARAMETER_CLASS(HMI0AVL),
    .ddm_owned_store = &s__ddm_owned_store,
    .ddm_owned_initial_values = s__ddm_owned_initial_values,
    .ddm_owned_initial_values_size = ELEMENTS(s__ddm_owned_initial_values),
    .worker_priority = SOFTWARE_SIMULATOR_DEFAULTS__PRIO,
    .worker_stack_size = SOFTWARE_SIMULATOR_DEFAULTS__MIN_STACK,
    .name = "CFXx",
};
