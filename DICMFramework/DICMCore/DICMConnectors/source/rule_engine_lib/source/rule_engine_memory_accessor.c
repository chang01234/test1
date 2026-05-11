#/**
 * @file rule_engine_memory_accessor.c
 *
 * @brief Rule engine memory accessor functions
 *
 * @author Borjan Bozhinovski
 * @date July 21, 2025
 */
#include "rule_engine_memory_accessor.h"
#include "configuration.h"

/**
 * @brief Get data from memory at specified offset
 */
int32_t rule_engine_memory_accessor_get_data(const void *data_ptr, const size_t data_size, const size_t offset, const size_t size, int32_t *value)
{
    // Check for null pointer and bounds
    if ((data_ptr == NULL) || (value == NULL) || (rule_engine_memory_accessor_is_in_bounds(data_size, offset, size) == false))
    {
        LOG(E, "Invalid data pointer or out of bounds access, data_size: %zu, offset: %zu, size: %zu", data_size, offset, size);
        return -1;
    }

    // Get address and read value
    const void *const addr = rule_engine_memory_accessor_get_address(data_ptr, offset, 0, 0);
    return rule_engine_memory_accessor_read_value(addr, size, value);
}

/**
 * @brief Set data in memory at specified offset
 */
int32_t rule_engine_memory_accessor_set_data(void *data_ptr, const size_t data_size, const size_t offset, const size_t size, const int32_t value)
{
    // Check for null pointer and bounds
    if ((data_ptr == NULL) || (rule_engine_memory_accessor_is_in_bounds(data_size, offset, size) == false))
    {
        LOG(E, "Invalid data pointer or out of bounds access, data_size: %zu, offset: %zu, size: %zu", data_size, offset, size);
        return -1;
    }

    // Get address and write value
    void *const addr = rule_engine_memory_accessor_get_address(data_ptr, offset, 0, 0);
    return rule_engine_memory_accessor_write_value(addr, value, size) ? 1 : -1;
}

/**
 * @brief Find a specific value in an array of structures or primitives
 */
int32_t rule_engine_memory_accessor_find_value(const void *data_ptr, const size_t data_size,
                                               const size_t offset, const size_t stride,
                                               const size_t element_size, const int32_t value,
                                               const size_t trailing_size)
{
    int32_t element_index = -1;

    if (data_size == 0)
    {
        return -1;  // No data to search
    }

    // Validate data pointer and offset
    if ((data_ptr == NULL) || (offset >= data_size))
    {
        LOG(E, "Invalid data pointer or offset\n");
        return -1;
    }

    // Validate stride
    if (stride == 0)
    {
        LOG(E, "Invalid stride value: stride cannot be 0\n");
        return -1;
    }

    // Validate trailing size
    if (trailing_size >= data_size)
    {
        LOG(E, "Invalid trailing size: %zu >= data_size %zu\n", trailing_size, data_size);
        return -1;
    }

    // Calculate the number of elements that can be safely accessed
    const size_t search_region = data_size - trailing_size;
    if (offset >= search_region)
    {
        LOG(E, "Offset %zu exceeds usable data region %zu (data_size=%zu, trailing_size=%zu)\n",
            offset, search_region, data_size, trailing_size);
        return -1;
    }

    // Validate that we have enough data for at least the first element
    const size_t min_required_bytes = offset + element_size;
    if (search_region < min_required_bytes)
    {
        LOG(E, "Insufficient data for first element: available=%zu, required=%zu\n",
            search_region, min_required_bytes);
        return -1;
    }

    // Calculate maximum number of elements we can safely access
    const size_t usable_bytes = search_region - offset - element_size;
    const size_t max_elements = (usable_bytes / stride) + 1;  // +1 to include element at index 0
    // Iterate through all array elements to find the target value
    for (size_t index = 0; index < max_elements; index++)
    {
        // Calculate address of the field within this array element
        const void *const field_addr = rule_engine_memory_accessor_get_address(data_ptr, offset, stride, index);

        // Double-check bounds to ensure we don't read beyond valid memory
        size_t field_offset = (size_t)((uint8_t *)field_addr - (uint8_t *)data_ptr);
        if (rule_engine_memory_accessor_is_in_bounds(data_size, field_offset, element_size) == false)
        {
            LOG(E, "Element access out of bounds at index %zu (offset=%zu)\n", index, field_offset);
            break;
        }

        // Read the value from this array element and compare with target
        int32_t current_value;
        if (rule_engine_memory_accessor_read_value(field_addr, element_size, &current_value) == -1)
        {
            LOG(E, "Failed to read value at index %zu (offset=%zu)\n", index, field_offset);
            break;  // Error reading value
        }

        if (current_value == value)
        {
            element_index = (int32_t)index;  // Found match - return array index
        }
    }

    return element_index;  // Return the found index or -1 if not found
}

/**
 * @brief Get the number of elements in an array within memory
 */
int32_t rule_engine_memory_accessor_get_array_length(const size_t data_size, const size_t offset, const size_t stride)
{
    // Validate inputs
    if ((data_size == 0) || (offset >= data_size))
    {
        return -1;
    }

    // Calculate number of elements
    return (int32_t)((data_size - offset) / stride);
}
