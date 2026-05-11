/*! \file connector_light.h
	\brief Header file for connector Onboard HMI
	\Author Sundaramoorthy-LTTS
 */

#ifndef CONNECTOR_LIGHT_H_
#define CONNECTOR_LIGHT_H_

/** Includes ******************************************************************/
#include "configuration.h"

/** Includes ******************************************************************/
#include "connector.h"

/* Macro Definitions */


/* Function pointer declaration */
typedef void (*conn_light_param_changed_t)(uint32_t ddm_param, int32_t i32value);

/* Struct Type Definitions */
typedef struct
{
    uint32_t ddm_parameter;
    DDM2_TYPE_ENUM type;
    uint8_t pub;
    uint8_t sub;
    int32_t i32Value;
    conn_light_param_changed_t cb_func;
} conn_light_param_t;

/* Extern Declarations */
extern CONNECTOR connector_light;

#endif /* CONNECTOR_LIGHT_H_ */
