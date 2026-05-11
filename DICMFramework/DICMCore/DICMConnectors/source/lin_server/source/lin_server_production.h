/**
 * @file lin_server_production.h
 * @author Borjan Bozhinovski (borjan.bozhinovski@qinshift.com)
 * @brief LIN Server production class implementation
 * @date 2024-05-14
 */

#ifndef LIN_SERVER_PRODUCTION__
#define LIN_SERVER_PRODUCTION__

//#include "ddm2.h"
#include "lin_server_definition.h"

#define IS_PRODUCTION_PARAMETER_REQUEST(parameter)              (PROD0 == DDM2_PARAMETER_CLASS(parameter) ? true : false)

int  lin_server_production_reqister_production_class_instance(const lin_server_slave_device_t * slave_device);
void lin_server_production_remove_production_class_instance(const lin_server_slave_device_t * slave_device);
void lin_server_production_handle_set(const lin_server_slave_device_t * const device, uint32_t parameter, const void * const value, size_t value_size);
void lin_server_production_handle_subscribe(const lin_server_slave_device_t * const device, uint32_t parameter);

#endif //LIN_SERVER_PRODUCTION__
