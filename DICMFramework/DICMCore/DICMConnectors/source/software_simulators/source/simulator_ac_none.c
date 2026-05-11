/**
 * \file
 * \date		2022-07-05
 * \author	  (NR) Nenad Radulovic (nenad.radulovic@gmail.com)
 * \brief	   Simulator for AC DDM2 class - variant `none` implementation
 *
 * Implementation of tables to simulate AC DDM2 class - variant `none`
 *
 * \li		  2022-07-05  (NR) Initial implementation
 *
 * \copyright   Dometic Group
 *			  This source file and the information contained in it are
 *			  confidential and proprietary to Dometic Group
 *			  The reproduction or disclosure, in whole or in part,
 *			  to anyone outside of Dometic Group without the written
 *			  approval of a Dometic Group officer under a Non-Disclosure
 *			  Agreement is expressly prohibited.
 *
 *			  All rights reserved
 */
#include "configuration.h"
#include "ddm2.h"  // ELEMENTS
#include "ddm2_parameter_list.h"
#include "ddm_entry.h"
#include "ddm_store.h"
#include "esp_attr.h"  // EXT_RAM_ATTR
#include "software_simulator.h"
#include <stdint.h>
#define SOFTWARE_SIMULATOR_DEFAULTS__MIN_STACK 4096
#include "simulator_ac_none.h"
#include "software_simulator_defaults.h"
#ifndef SOFTWARE_SIMULATOR_AC0VER
#define SOFTWARE_SIMULATOR_AC0VER "1.0.0"
#endif
#ifndef SOFTWARE_SIMULATOR_AC0MDL
#define SOFTWARE_SIMULATOR_AC0MDL AC0MDL_DOMETIC_FJX4000_SERIES
#endif
typedef struct AC_ERROR_T
{
    uint32_t error[2];
} PACKED AC_ERROR_T;

static AC_ERROR_T error =
    {
        .error = {0}};

static const struct ddm_store_ddm s__ddm_initial_values[] =
    {
        {.ddm_parameter = AC0AVL,
         .value =
             {
                 .storage = {.i32 = 1},
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0MD,
         .value =
             {
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0FS,
         .value =
             {
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0FSPD,
         .value =
             {
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0PWR,
         .value =
             {
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0MMD,
         .value =
             {
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0FMD,
         .value =
             {
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0LGT,
         .value =
             {
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0DMR,
         .value =
             {
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0MDL,
         .value =
             {
                 .storage = {.i32 = SOFTWARE_SIMULATOR_AC0MDL},
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0LGTBS,
         .value =
             {
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0ELGT,
         .value =
             {
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0PUR,
         .value =
             {
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0ERRC,
         .value =
             {
                 .type = DDM2_TYPE_INT32_T}},
#ifdef AC0ALTC
        {.ddm_parameter = AC0ALTC,
         .value =
             {
                 .type = DDM2_TYPE_INT32_T}},
#endif
        {.ddm_parameter = AC0ON,
         .value =
             {
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0TTEMP,
         .value =
             {
                 .storage = {.i32 = 20000},
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0ITEMP,
         .value =
             {
                 .storage = {.i32 = 20000},
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0FMD,
         .value =
             {
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0TONA,
         .value =
             {
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0TOFFA,
         .value =
             {
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0TONH,
         .value =
             {
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0TOFFH,
         .value =
             {
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0TONM,
         .value =
             {
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0TOFFM,
         .value =
             {
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0SYSU,
         .value =
             {
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0TSUP,
         .value =
             {
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0TMD,
         .value =
             {
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0SLEEP,
         .value =
             {
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0HFAVL,
         .value =
             {
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0LFAVL,
         .value =
             {
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0STATUS,
         .value =
             {
                 .storage = {.structure = &error},
                 .size = sizeof(error),
                 .type = DDM2_TYPE_STRUCT}},
        {.ddm_parameter = AC0VER,
         .value =
             {
                 .storage = {.str = SOFTWARE_SIMULATOR_AC0VER},
                 .type = DDM2_TYPE_STRING}},
        {.ddm_parameter = AC0OFFSET,
         .value =
             {
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0OPERST,
         .value =
             {
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0ECO,
         .value =
             {
                 .storage = {.i32 = 0},
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0FLAPS,
         .value =
             {
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0FEATURE,
         .value =
             {
                 .storage = {.u32 = 0},
                 .type = DDM2_TYPE_UINT32_T}},
        {.ddm_parameter = AC0ACTEXT,
         .value =
             {
                 .storage = {.u32 = 0},
                 .type = DDM2_TYPE_UINT32_T}},
        {.ddm_parameter = AC0REMCTRL,
         .value =
             {
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0TEST,
         .value =
             {
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0HTTEMP,
         .value =
             {
                 .storage = {.i32 = 20000},
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0ETEMP,
         .value =
             {
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0HMODE,
         .value =
             {
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0FTDIFF,
         .value =
             {
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0FILTER,
         .value =
             {
                 .storage = {.i32 = 3},
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0CURR,
         .value =
             {
                 .storage = {.u32 = 0},
                 .type = DDM2_TYPE_INT32_T}},
        {.ddm_parameter = AC0CURRLIM,
         .value =
             {
                 .type = DDM2_TYPE_INT32_T}},
};

static bool action__fan_speed(SOFTWARE_SIMULATOR *ss, ddm_entry_t *entry, void *args)
{
    ddm_entry_t *ac_fmd;
    bool has_changed;

    ac_fmd = ddm_store__access(software_simulator__owned_store(ss), AC0FMD);

    if (ddm_entry__value_i32(entry) == 0)
    {
        has_changed = ddm_entry__set__value_i32(ac_fmd, AC0FMD_AUTO);
        ddm_entry__set__has_changed_conditionally(ac_fmd, has_changed);
        return has_changed;
    }
    else
    {
        has_changed = ddm_entry__set__value_i32(ac_fmd, AC0FMD_ON);
        ddm_entry__set__has_changed_conditionally(ac_fmd, has_changed);
        return has_changed;
    }
}

static const SOFTWARE_SIMULATOR__ACTION_ENTRY s__actions[] =
    {
        {
            .ddm_parameter = AC0FS,
            .trigger = {.any = {AC0FS}},
            .handler = action__fan_speed,
        }};

DDM_STORE__DECLARE_EXTRAM(s__ddm_store, ELEMENTS(s__ddm_initial_values));

const SOFTWARE_SIMULATOR__DESCRIPTOR g__class_ac_none_descriptor =
    {
        .ddm_class = DDM2_PARAMETER_CLASS(AC0AVL),
        .ddm_owned_store = &s__ddm_store,
        .ddm_owned_initial_values = s__ddm_initial_values,
        .ddm_owned_initial_values_size = ELEMENTS(s__ddm_initial_values),
        .actions = s__actions,
        .actions_size = ELEMENTS(s__actions),
        .worker_priority = SOFTWARE_SIMULATOR_DEFAULTS__PRIO,
        .worker_stack_size = SOFTWARE_SIMULATOR_DEFAULTS__MIN_STACK,
        .name = "none"};
