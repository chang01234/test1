
#ifndef EDS_PORT_DEFINITION_H_
#define EDS_PORT_DEFINITION_H_

#include <stddef.h>
#include "iGeneralDefinitions.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define CRITICAL_METHOD_MUTEX           1
#define CRITICAL_METHOD_ISR             2

#define CRITICAL_METHOD                 CRITICAL_METHOD_MUTEX


typedef struct eds_port__sleep
{
    StaticSemaphore_t sem;
    SemaphoreHandle_t handle;
} eds_port__sleep_t;

typedef struct eds_port__critical
{
#if (CRITICAL_METHOD == CRITICAL_METHOD_ISR)
    portMUX_TYPE mux;
#elif (CRITICAL_METHOD == CRITICAL_METHOD_MUTEX)
    /* NOTE:
     * EDS library expects this structure to be defined. When using mutex method we actually don't
     * have a use for this structure. Since the structure needs to have at least one member we use
     * a dummy member here.
     */
    int dummy;
#endif
} eds_port__critical_t;

#endif /* EDS_PORT_DEFINITION_H_ */
