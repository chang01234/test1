#include "configuration.h"

#include "connector_lindev.h"
#include "lindev_generic.h"

#if defined(CONNECTOR_LINDEV_SHAPE)
#define LINDEV_GENERIC_CONTEXT          &shape_lindev_generic
extern lindev_generic_t shape_lindev_generic;
#elif defined(CONNECTOR_LINDEV_SHARC)
#define LINDEV_GENERIC_CONTEXT          &sharc_lindev_generic
extern lindev_generic_t sharc_lindev_generic;
#elif defined(CONNECTOR_LINDEV_INVENT)
#define LINDEV_GENERIC_CONTEXT          &invent_lindev_generic
extern lindev_generic_t invent_lindev_generic;
#elif defined(CONNECTOR_LINDEV_NRX)
#define LINDEV_GENERIC_CONTEXT          &nrx_lindev_generic
extern lindev_generic_t nrx_lindev_generic;
#else
#error "Define CONNECTOR_LINDEV_SHAPE or CONNECTOR_LINDEV_SHARC or CONNECTOR_LINDEV_INVENT or CONNECTOR_LINDEV_NRX macro to enable the product LINDEV implementation"
#endif

#if !defined(LINDEV_GENERIC_CONTEXT)
#error "Macro LINDEV_GENERIC_CONTEXT must be defined in connector_lindev.h to point to correct lindev_generic context structure"
#endif

void connector_lindev_enable(void)
{
    lindev_generic_enable(LINDEV_GENERIC_CONTEXT);
}

void connector_lindev_disable(void)
{
    lindev_generic_disable(LINDEV_GENERIC_CONTEXT);
}
