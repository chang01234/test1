/*
 * utils.h
 *
 *	Created on: 27 Sep 2019
 *	Author: Jens Björnhager et al.
 */
#ifndef UTILS_H_
#define UTILS_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Swap bytes in memory (2 bytes)
void * swap2(void * const memory);

// Swap bytes in memory (4 bytes)
void * swap4(void * const memory);

// Compare memory blocks of possibly different sizes; FALSE if equal, TRUE if different
int compare_memory(const void * const mem1, const size_t size1, const void * const mem2, const size_t size2);

// Replace all occurrences of a character in a string
char * replace_char_n(char * string, int string_size, const char find, const char replace);

// Replace all occurrences of a character in a null-terminated string
char * replace_char(char * string, const char find, const char replace);

// Does string start with prefix?
int is_prefixed_with(const char * const string, const size_t string_length, const char * const prefix, const size_t prefix_length);

//! \~ Fill memory with a 32-bit value
void *memset_i32(void * memory, const int32_t value, size_t count);

//! \~ Constrain/clip an int32_t value between two limits (inclusive)
int constrain(int32_t * const value, const int32_t minimum, const int32_t maximum);

//! \~ Create hexdump in a string
size_t hexdump_string(char * hex, const size_t hex_size, const char * const tag, const char separator, const void * mem, size_t mem_size);


/**
 * @brief Converts a 2 char string in hexadecimal to uint8_t value
 *
 * @param hex string in hexadecimal
 * @return byte value of 2 character hex string input
 */
uint8_t hex2uint8(const char *hex);
/**
 * @brief Converts a 4 char string in hexadecimal to uint16_t value
 *
 * @param hex string in hexadecimal
 * @return uint16_t value of 4 character hex string input
 */
uint16_t hex2uint16(const char *hex);
/**
 * @brief Converts a 8 char string in hexadecimal to uint32_t value
 *
 * @param hex string in hexadecimal
 * @return uint32_t value of 8 character hex string input
 */
uint32_t hex2uint32(const char *hex);

//! \~ Copy array reversed
int reverse_bytes(void * restrict destination, const void * restrict const source, const size_t size);

//check if memory range is zero
int is_zero(const void * const memory, size_t size);

//convert 64 bits data to little endian order
uint64_t reverse64(void * data);

//create a bitmask from start and end bits (inclusive)
uint64_t create_mask(int start_bit, int end_bit, int bytesize);

//has data changed in the masked region?
int has_changed(const void *old_data, int old_size, const void *new_data, int new_size, int mask_start, int mask_end);

// Increment timers (prevent overflow)
uint16_t timer16_increment(uint16_t * const timer, const uint16_t elapsed);
uint32_t timer32_increment(uint32_t * const timer, const uint32_t elapsed);

// Simple timer (return TRUE if period is due)
bool timer16_is_due(uint16_t * const timer, const uint16_t elapsed, const uint16_t period);
bool timer32_is_due(uint32_t * const timer, const uint32_t elapsed, const uint32_t period);

// String
int str_pos(const char * str, const char * substr);

// Debounce
typedef struct
{
	bool     state;
	uint32_t timer;
	uint32_t delay_on;
	uint32_t delay_off;
} debounce32_t;
void debounce32_init(debounce32_t* debounce, bool state, uint32_t delay_on, uint32_t delay_off);
void debounce32_config(debounce32_t* debounce, uint32_t delay_on, uint32_t delay_off);
bool debounce32_process(debounce32_t* debounce, bool state, uint32_t elapsed);
bool debounce32_process_with_reset(debounce32_t* debounce, bool state, uint32_t elapsed);
bool debounce32_process_on(debounce32_t* debounce, uint32_t elapsed);
bool debounce32_process_off(debounce32_t* debounce, uint32_t elapsed);

// Detect invalid unintended access utilities
#if 0
typedef struct
{
	uint8_t buffer[16];
} access_t;
void access_init( access_t* access);
void access_check(access_t* access, const char* info1, int info2);
#endif

#endif /* UTILS_H_ */