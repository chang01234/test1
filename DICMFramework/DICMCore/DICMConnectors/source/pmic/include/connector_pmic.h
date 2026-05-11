/*! \file connector_template.h
  \brief Template connector
  */

#ifndef CONNECTOR_PMIC_H_
#define CONNECTOR_PMIC_H_

#include "configuration.h"

#ifdef CONNECTOR_PMIC

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "connector.h"

extern CONNECTOR connector_pmic;

#endif /* CONNECTOR_PMIC */
#endif /* CONNECTOR_PMIC_H */
