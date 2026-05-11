/**
 * @file rule_engine_memory_accessor.c
 *
 * @brief Rule engine memory accessor functions
 *
 * @author Borjan Bozhinovski
 * @date July 21, 2025
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Calculate address based on base, offset, stride and index
 *
 * @param base Base pointer
 * @param offset Initial offset from base
 * @param stride Bytes between elements
 * @param index Element index
 * @return void* Calculated address
 */
static inline void *rule_engine_memory_accessor_get_address(const void *base, const size_t offset, const size_t stride, const size_t index)
{
    return (uint8_t *)base + offset + (stride * index);
}

/**
 * @brief Read a value of specified size from memory
 *
 * @param src_ptr Source pointer
 * @param size Size to read (1, 2, or 4 bytes)
 * @param value Pointer to store the read value
 * @return int32_t 1 on success, -1 on error
 */
static inline int32_t rule_engine_memory_accessor_read_value(const void *src_ptr, const size_t size, int32_t *value)
{
    switch (size)
    {
    case sizeof(int8_t):
    {
        int8_t temp_value;
        memcpy(&temp_value, src_ptr, sizeof(int8_t));
        *value = (int32_t)temp_value;
        break;
    }
    case sizeof(int16_t):
    {
        int16_t temp_value;
        memcpy(&temp_value, src_ptr, sizeof(int16_t));
        *value = (int32_t)temp_value;
        break;
    }
    case sizeof(int32_t):
    {
        int32_t temp_value;
        memcpy(&temp_value, src_ptr, sizeof(int32_t));
        *value = temp_value;
        break;
    }
    default:
        return -1;  // Unsupported size
    }

    return 1;
}

/**
 * @brief Write a value of specified size to memory
 *
 * @param dest_ptr Destination pointer
 * @param value Value to write
 * @param size Size to write (1, 2, or 4 bytes)
 * @return bool True if write succeeded
 */
static inline bool rule_engine_memory_accessor_write_value(void *dest_ptr, const int32_t value, const size_t size)
{
    switch (size)
    {
    case sizeof(int8_t):
    {
        int8_t val = (int8_t)value;
        memcpy(dest_ptr, &val, sizeof(int8_t));
        return true;
    }
    case sizeof(int16_t):
    {
        int16_t val = (int16_t)value;
        memcpy(dest_ptr, &val, sizeof(int16_t));
        return true;
    }
    case sizeof(int32_t):
    {
        memcpy(dest_ptr, &value, sizeof(int32_t));
        return true;
    }
    default:
        return false;
    }
}

/**
 * @brief Check if access is within memory bounds
 *
 * @param data_size Total size of buffer
 * @param offset Access offset
 * @param access_size Size of access
 * @return bool True if access is within bounds
 */
static inline bool rule_engine_memory_accessor_is_in_bounds(const size_t data_size, const size_t offset, const size_t access_size)
{
    // First check: prevent underflow in subtraction
    // (access_size must fit within buffer at all)
    if (access_size > data_size)
    {
        return false;
    }
    // Second check: prevent overflow in offset + access_size
    // (offset must leave enough room for access_size)
    // Safe because: access_size <= data_size, so subtraction won't underflow
    return (offset <= (data_size - access_size));
}

/**
 * @brief Read an integer value from a memory region at a given offset and size.
 *
 * @param data_ptr Pointer to the memory region (e.g., struct or array)
 * @param data_size Total size of the memory region in bytes
 * @param offset Offset in bytes from the start of the region
 * @param size Number of bytes to read (e.g., sizeof(member) or sizeof(element))
 * @param value Pointer to store the read value
 * @return int32_t 1 on success, -1 on error
 */
int32_t rule_engine_memory_accessor_get_data(const void *data_ptr, const size_t data_size, const size_t offset, const size_t size, int32_t *value);

/**
 * @brief Write an integer value to a memory region at a given offset and size.
 *
 * This function is designed to write a value to a specific offset in a memory region,
 * ensuring that the access is within bounds and the size is appropriate. The memory region
 * has to be large enough to accommodate the write operation without exceeding its bounds.
 *
 * Note: It is not designed for extending the memory region of flexible arrays.
 *
 * @param data_ptr Pointer to the memory region (e.g., struct or array)
 * @param data_size Total size of the memory region in bytes
 * @param offset Offset in bytes from the start of the region
 * @param size Number of bytes to write (e.g., sizeof(member) or sizeof(element))
 * @param value The value to write
 * @return int32_t 1 on success, -1 on error
 */
int32_t rule_engine_memory_accessor_set_data(void *data_ptr, const size_t data_size, const size_t offset, const size_t size, const int32_t value);

/**
 * @brief Search for a value in an array region within a memory buffer.
 *
 * The function traverses memory by calculating addresses as: base + offset + (index * stride),
 * reads element_size bytes at each location, and compares the interpreted value against the target.
 *
 * Memory Layout Understanding:
 * - data_ptr points to the beginning of a memory buffer containing an array
 * - Each array element occupies 'stride' bytes in memory
 * - Within each element, the field of interest starts 'offset' bytes from the element's beginning
 * - The field spans 'element_size' bytes
 * - trailing_size bytes at the end of the buffer are excluded from the search (for additional fields)
 *
 * Usage Pattern:
 * 1. Specify the memory buffer (data_ptr, data_size)
 * 2. Define array element layout (stride for element spacing, offset for field location within element)
 * 3. Specify field characteristics (element_size for field width)
 * 4. Provide search criteria (value to find, trailing_size for exclusion area)
 *
 * The function automatically calculates the maximum number of elements that can be safely accessed
 * and iterates through them sequentially until the target value is found or all elements are checked.
 *
 * @param data_ptr Pointer to the start of the data buffer containing the array
 * @param data_size Total size of the data buffer in bytes
 * @param offset Byte offset within each array element to the field being searched
 * @param stride Byte distance between consecutive array elements (must be > 0)
 * @param element_size Size in bytes of the field being read and compared (1, 2, 4, or 8 bytes)
 * @param value The target value to search for (interpreted as signed 32-bit integer)
 * @param trailing_size Number of bytes at the end of data_ptr that should be excluded from search
 * @return Array index (0-based) where value was found, or -1 if not found or error
 */
int32_t rule_engine_memory_accessor_find_value(const void *data_ptr, const size_t data_size, const size_t offset, const size_t stride, const size_t element_size, const int32_t value, const size_t trailing_size);

/**
 * @brief Calculate the number of elements in an array region within a memory buffer.
 *
 * This function is designed for C99-compliant usage with flexible arrays
 * positioned at the end of a struct. It assumes the array extends to the end of the allocated
 * memory with no trailing members after it, as required by C99 standard for flexible arrays.
 * It calculates how many elements fit in the remaining buffer space starting from the given offset.
 *
 * @param data_size Total size of the memory region in bytes
 * @param offset Offset in bytes to the start of the array region
 * @param stride Size of each array element in bytes
 * @return int32_t Number of elements that fit in the array region, or -1 on error
 */
int32_t rule_engine_memory_accessor_get_array_length(const size_t data_size, const size_t offset, const size_t stride);
