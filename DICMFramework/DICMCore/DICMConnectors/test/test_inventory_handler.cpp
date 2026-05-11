/*
 * test_inventory_handler.cpp
 *
 *  Created on: 14 nov. 2024
 *      Author: Andlun
 */


extern "C" {
#include "configuration.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "broker.h"
#include "inventory_handler.h"
#include "sorted_list.h"
}
#include "DICMFrameworkTestFixture.hpp"

class InventoryHandlerTestFixture : public DICMFrameworkTestFixture {
protected:

	void SetUp() override
    {
        sorted_list_clear(&sorted_list);
		DICMFrameworkTestFixture::SetUp();
    }

    void TearDown() override
    {
    	DICMFrameworkTestFixture::TearDown();
    }
    DECLARE_SORTED_LIST_PUBLIC(sorted_list, 10);
};
extern "C"
void inventory_handler_available_cb(void * argument, uint32_t device_class_instance, bool is_available)
{
    
}

TEST_F(InventoryHandlerTestFixture, init)
{
    inventory_handler_t ih = { .sorted_list = NULL, .cb_available = NULL, .cb_argument = NULL};
    EXPECT_TRUE(inventory_handler_init(NULL, NULL, NULL, NULL) == 1) << "inventory_handler_init() should return 1" << std::endl;
    EXPECT_TRUE(inventory_handler_init(&ih, NULL, NULL, NULL) == 1) << "inventory_handler_init() should return 1" << std::endl;
    EXPECT_TRUE(inventory_handler_init(&ih, &sorted_list, NULL, NULL) == 1) << "inventory_handler_init() should return 1" << std::endl;
    EXPECT_TRUE(inventory_handler_init(NULL, &sorted_list, NULL, NULL) == 1) << "inventory_handler_init() should return 1" << std::endl;
    EXPECT_TRUE(inventory_handler_init(&ih, &sorted_list, inventory_handler_available_cb, NULL) == 0) << "inventory_handler_init() should return 0" << std::endl;
    EXPECT_TRUE(inventory_handler_init(NULL, &sorted_list, inventory_handler_available_cb, NULL) == 1) << "inventory_handler_init() should return 1" << std::endl;
    EXPECT_TRUE(inventory_handler_init(&ih, NULL, inventory_handler_available_cb, NULL) == 1) << "inventory_handler_init() should return 1" << std::endl;
}

TEST_F(InventoryHandlerTestFixture, add)
{
    inventory_handler_t ih = { .sorted_list = NULL, .cb_available = NULL, .cb_argument = NULL};
    int value_count = 10;
    SORTED_LIST_VALUE_TYPE values[10];
    uint32_t device_class_instance = AC0MDL;
    inventory_handler_init(&ih, &sorted_list, inventory_handler_available_cb, NULL);
    
    EXPECT_TRUE(inventory_handler_add(&ih, device_class_instance) == 0) << "inventory_handler_add() should return 0" << std::endl;
    EXPECT_TRUE(inventory_handler_add(&ih, device_class_instance) == 0) << "inventory_handler_add() should return 0" << std::endl;
    EXPECT_TRUE(inventory_handler_add(&ih, device_class_instance) == 0) << "inventory_handler_add() should return 0" << std::endl;
    EXPECT_TRUE(inventory_handler_add(&ih, device_class_instance) == 0) << "inventory_handler_add() should return 0" << std::endl;
    EXPECT_TRUE(inventory_handler_add(&ih, device_class_instance) == 0) << "inventory_handler_add() should return 0" << std::endl;

    EXPECT_TRUE(sorted_list_multiple_get(&values[0], &value_count, ih.sorted_list, HTR0, 0) == SORTED_LIST_FAIL) << "sorted_list_multiple_get() should return SORTED_LIST_FAIL" << std::endl;
    EXPECT_TRUE(value_count == 0) << "value_count should be zero" << std::endl;
    value_count = 10;
    EXPECT_TRUE(sorted_list_multiple_get(&values[0], &value_count, ih.sorted_list, AC0, 0) == SORTED_LIST_OK) << "sorted_list_multiple_get() should return SORTED_LIST_OK" << std::endl;
    EXPECT_TRUE(value_count == 1) << "value_count should be one: " << value_count << std::endl;
    
    EXPECT_TRUE(inventory_handler_add(&ih, device_class_instance | DDM2_PARAMETER_INSTANCE(1)) == 0) << "inventory_handler_add() should return 0" << std::endl;
    value_count = 10;
    EXPECT_TRUE(sorted_list_multiple_get(&values[0], &value_count, ih.sorted_list, AC0 | DDM2_PARAMETER_INSTANCE(1), 0) == SORTED_LIST_OK) << "sorted_list_multiple_get() should return SORTED_LIST_OK" << std::endl;
    EXPECT_TRUE(value_count == 1) << "value_count should be one: " << value_count << std::endl;
}

TEST_F(InventoryHandlerTestFixture, add_any)
{
    inventory_handler_t ih = { .sorted_list = NULL, .cb_available = NULL, .cb_argument = NULL};
    int value_count = 10;
    SORTED_LIST_VALUE_TYPE values[10];
    uint32_t device_class_instance = AC0MDL;
    inventory_handler_init(&ih, &sorted_list, inventory_handler_available_cb, NULL);
    
    EXPECT_TRUE(inventory_handler_add_any(&ih, device_class_instance) == 0) << "inventory_handler_add() should return 0" << std::endl;
    EXPECT_TRUE(inventory_handler_add_any(&ih, device_class_instance) == 0) << "inventory_handler_add() should return 0" << std::endl;
    EXPECT_TRUE(inventory_handler_add_any(&ih, device_class_instance) == 0) << "inventory_handler_add() should return 0" << std::endl;
    EXPECT_TRUE(inventory_handler_add_any(&ih, device_class_instance) == 0) << "inventory_handler_add() should return 0" << std::endl;
    EXPECT_TRUE(inventory_handler_add_any(&ih, device_class_instance) == 0) << "inventory_handler_add() should return 0" << std::endl;

    EXPECT_TRUE(sorted_list_multiple_get(&values[0], &value_count, ih.sorted_list, HTR0, 0) == SORTED_LIST_FAIL) << "sorted_list_multiple_get() should return SORTED_LIST_FAIL" << std::endl;
    EXPECT_TRUE(value_count == 0) << "value_count should be zero" << std::endl;
    value_count = 10;
    EXPECT_TRUE(sorted_list_multiple_get(&values[0], &value_count, ih.sorted_list, AC0, 0) == SORTED_LIST_FAIL) << "sorted_list_multiple_get() should return SORTED_LIST_FAIL" << std::endl;
    EXPECT_TRUE(value_count == 0) << "value_count should be zero: " << value_count << std::endl;
    value_count = 10;
    EXPECT_TRUE(sorted_list_multiple_get(&values[0], &value_count, ih.sorted_list, AC0 | DDM2_PARAMETER_INSTANCE(0xFF), 0) == SORTED_LIST_OK) << "sorted_list_multiple_get() should return SORTED_LIST_OK" << std::endl;
    EXPECT_TRUE(value_count == 1) << "value_count should be one: " << value_count << std::endl;
}

TEST_F(InventoryHandlerTestFixture, remove)
{
    inventory_handler_t ih = { .sorted_list = NULL, .cb_available = NULL, .cb_argument = NULL};
    int value_count = 10;
    SORTED_LIST_VALUE_TYPE values[10];
    inventory_handler_init(&ih, &sorted_list, inventory_handler_available_cb, NULL);
    
    EXPECT_TRUE(inventory_handler_add(&ih, AC0MDL) == 0) << "inventory_handler_add() should return 0" << std::endl;
    EXPECT_TRUE(inventory_handler_add(&ih, HTR0) == 0) << "inventory_handler_add() should return 0" << std::endl;

    EXPECT_TRUE(sorted_list_multiple_get(&values[0], &value_count, ih.sorted_list, HTR0, 0) == SORTED_LIST_OK) << "sorted_list_multiple_get() should return SORTED_LIST_OK" << std::endl;
    EXPECT_TRUE(value_count == 1) << "value_count should be one" << std::endl;
    value_count = 10;
    EXPECT_TRUE(sorted_list_multiple_get(&values[0], &value_count, ih.sorted_list, AC0, 0) == SORTED_LIST_OK) << "sorted_list_multiple_get() should return SORTED_LIST_OK" << std::endl;
    EXPECT_TRUE(value_count == 1) << "value_count should be one: " << value_count << std::endl;

    EXPECT_TRUE(inventory_handler_remove(NULL, AC0) == 1) << "inventory_handler_remove() should return 1" << std::endl;
    value_count = 10;
    EXPECT_TRUE(sorted_list_multiple_get(&values[0], &value_count, ih.sorted_list, AC0, 0) == SORTED_LIST_OK) << "sorted_list_multiple_get() should return SORTED_LIST_OK" << std::endl;
    EXPECT_TRUE(value_count == 1) << "value_count should be one: " << value_count << std::endl;

    EXPECT_TRUE(inventory_handler_remove(&ih, AC0) == 0) << "inventory_handler_remove() should return 0" << std::endl;
    value_count = 10;
    EXPECT_TRUE(sorted_list_multiple_get(&values[0], &value_count, ih.sorted_list, AC0, 0) == SORTED_LIST_FAIL) << "sorted_list_multiple_get() should return SORTED_LIST_FAIL" << std::endl;
    EXPECT_TRUE(value_count == 0) << "value_count should be zero: " << value_count << std::endl;
}
