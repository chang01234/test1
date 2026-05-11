/*
 * utils.c
 *,
 *	Created on: 27 Sep 2019
 *	Author: Jens Björnhager et al.
 */
#include <string.h>
#include "utils.h"

void * swap2(void * const memory)
{
	if (!memory)
	{
		return NULL;
	}

	uint8_t * const Data = memory;

	uint8_t tmp = Data[0];
	Data[0] = Data[1];
	Data[1] = tmp;

	return Data;
}

void * swap4(void * const memory)
{
	if (!memory)
	{
		return NULL;
	}

	uint8_t * const Data = memory;

	uint8_t tmp = Data[0];
	Data[0] = Data[3];
	Data[3] = tmp;

	tmp = Data[1];
	Data[1] = Data[2];
	Data[2] = tmp;

	return Data;
}

int compare_memory(const void * const mem1, const size_t size1, const void * const mem2, const size_t size2)
{
	if (size1 != size2)
	{
		return 1;
	}

	return memcmp(mem1, mem2, size1);
}

char * replace_char_n(char * string, int string_size, const char find, const char replace)
{
	char * current_pos = memchr(string, find, string_size);
	while (current_pos)
	{
		*current_pos = replace;
		current_pos = memchr(current_pos, find, string_size);
	}
	return string;
}

char * replace_char(char * string, const char find, const char replace)
{
	char * current_pos = strchr(string, find);
	while (current_pos)
	{
		*current_pos = replace;
		current_pos = strchr(current_pos, find);
	}
	return string;
}

int is_prefixed_with(const char * const string, const size_t string_length, const char * const prefix, const size_t prefix_length)
{
	return string_length < prefix_length ? false : !memcmp(prefix, string, prefix_length);
}

void *memset_i32(void * memory, const int32_t value, size_t count)
{
	int32_t *buf = memory;

	while (count--)
	{
		*buf++ = value;
	}

	return memory;
}

/*! \brief Constrain/clip an int32_t value between two limits (inclusive)
	\param value Value to clip
	\param minimum Minimum (inclusive)
	\param maximum Maximum (inclusive)
	\retval -1 Value was lower than minimum
	\retval 1 Value was higher than maximum
	\retval 0 Value was in range
 */
int constrain(int32_t * const value, const int32_t minimum, const int32_t maximum)
{
	if (*value < minimum)
	{
		*value = minimum;
		return -1;
	}

	if (*value > maximum)
	{
		*value = maximum;
		return 1;
	}

	return 0;
}

static const char * Hex = "0123456789abcdef";

/*! \brief Save a hexdump to a string (tag|[XXs]...)
	\param hex Destination hex string buffer
	\param hex_size Size of destination buffer 
	\param tag Optional start of string
	\param separator Optional separator between bytes
	\param mem Memory to dump
	\param mem_size Size of memory
	\return Size of resulting string
 */
size_t hexdump_string(char * hex, const size_t hex_size, const char * const tag, const char separator, const void * mem, size_t mem_size)
{
	const uint8_t *memory = mem;
	size_t separator_size = (separator != 0);
	size_t byte_size = (2 + separator_size);
	size_t tag_size = strlen(tag);
	size_t output_size = tag_size + byte_size * mem_size + 1;

	if (output_size > hex_size)
	{
		return 0;
	}

	strcpy(hex, tag);
	hex += tag_size;

	for (; mem_size; mem_size--, memory++)
	{
		*(hex++) = Hex[(*memory >> 4) & 0xf];
		*(hex++) = Hex[(*memory >> 0) & 0xf];
		if ((separator) && (mem_size != 1))
		{
			*(hex++) = separator;
		}
	}
	*hex = 0;

	return output_size;
}

static int hex2num(char c)
{
	if (c >= '0' && c <= '9')
	{
		return c - '0';
	}

	if (c >= 'a' && c <= 'f')
	{
		return c - 'a' + 10;
	}

	if (c >= 'A' && c <= 'F')
	{
		return c - 'A' + 10;
	}

	return -1;
}

uint8_t hex2uint8(const char *hex)
{
	int a, b;

	a = hex2num(*hex++);

	if (a < 0)
	{
		return -1;
	}

	b = hex2num(*hex++);

	if (b < 0)
	{
		return -1;
	}
	
	return (a << 4) | b;
}
uint16_t hex2uint16(const char *hex)
{
	int a, b;

	a = hex2uint8(hex);

	if (a < 0)
	{
		return -1;
	}

	b = hex2uint8(hex + 2);

	if (b < 0)
	{
		return -1;
	}

	return (a << 8) | b;
}
uint32_t hex2uint32(const char *hex)
{
	int a, b;

	a = hex2uint16(hex);

	if (a < 0)
	{
		return -1;
	}

	b = hex2uint16(hex + 4);

	if (b < 0)
	{
		return -1;
	}
	
	return (a << 16) | b;
}

int reverse_bytes(void * restrict destination, const void * restrict const source, const size_t size)
{
	uint8_t * const out = (uint8_t *)destination;
	const uint8_t * const in = (uint8_t *)source;

	for (int i = 0; i < (int)size; i++)
	{
		out[i] = in[size - 1 - i]; // copy bytes from one end of source to other end of destination
	}

	return 1; // return reversed data
}

int is_zero(const void * const memory, size_t size)
{
	const uint8_t *mem = memory;
	size_t reduction = 0;

	for (; size > 0; size--)
	{
		reduction += mem[size - 1];
	}

	return !reduction;
}

uint64_t reverse64(void * data) // convert 64 bits data to little endian order
{
	uint64_t *x = data;			 // x=all source data
	uint8_t *px = (uint8_t *)x;	 // px=a byte of x
	uint64_t r;					 // r=all destination data
	uint8_t *pr = (uint8_t *)&r; // pr=a byte of r
	for (int i = 0; i < (int)sizeof(uint64_t); i++)
	{
		pr[i] = px[sizeof(uint64_t) - 1 - i]; // copy bytes from one end of source to other end of destination
	}
	return r; // return reversed data
}

uint64_t create_mask(int start_bit, int end_bit, int bytesize) // create a bitmask from start and end bits (inclusive)
{															   // bytesize=valid bytes in current data
	uint64_t start_mask = -1, end_mask = -1, mask;			   // fill masks with all ones
	int max_bit = bytesize * 8 - 1;							   // highest bit in valid data

	start_mask = start_mask >> start_bit; // shift in zeroes from the left (>>= seems to do a signed right shift)

	if (end_bit >= max_bit) // if mask runs to end, return it as is
		return start_mask;

	end_mask = end_mask >> (end_bit + 1); // create mask of bits after end bit
	mask = start_mask ^ end_mask;		  // reset bits to the right of end bit

	return mask;
}

int has_changed(const void * old_data, int old_size, const void * new_data, int new_size, int mask_start, int mask_end) // has data changed in the masked region?
{
	uint64_t old_data64, new_data64, changed_data64, changed_data_reversed64;
	uint64_t mask64, changed64, undefined_mask = -1;

	old_data64 = *(uint64_t *)old_data;
	new_data64 = *(uint64_t *)new_data;

	if (old_size < 8)
		undefined_mask = undefined_mask >> 8 * old_size; // previously undefined bytes are counted as changed
	else
		undefined_mask = 0;

	changed_data64 = old_data64 ^ new_data64;			  // bits where data has changed (still little endian)
	changed_data_reversed64 = reverse64(&changed_data64); // reverse byte order of result
	changed_data_reversed64 |= undefined_mask;			  // add undefined bytes to changed mask

	mask64 = create_mask(mask_start, mask_end, new_size); // generate the bit mask of interesting bits
	changed64 = changed_data_reversed64 & mask64;		  // overlap between changed bytes and interesting bits?

	return !!changed64; // normalize bitmask to a boolean
}

/**********************************************************
 * Function:    timer16_increment
 * Description: Increment specified 16 bits timer 
 *              (do not overflow)
 *********************************************************/
uint16_t timer16_increment(uint16_t * const timer, const uint16_t elapsed)
{
	uint16_t prev = *timer;
	uint16_t next = prev + elapsed;
	if (next < prev)
	{
		next = UINT16_MAX;
	}
	*timer = next;
	return next;
}

/**********************************************************
 * Function:    timer32_increment
 * Description: Increment specified 32 bits timer 
 *              (do not overflow)
 *********************************************************/
uint32_t timer32_increment(uint32_t * const timer, const uint32_t elapsed)
{
	uint32_t prev = *timer;
	uint32_t next = prev + elapsed;
	if (next < prev)
	{
		next = UINT32_MAX;
	}
	*timer = next;
	return next;
}

/**********************************************************
 * Function:    timer16_is_due
 * Description: Update timer and return true if period is due
 *********************************************************/
bool timer16_is_due(uint16_t * const timer, const uint16_t elapsed, const uint16_t period)
{
	// Increment
	uint16_t value = timer16_increment(timer, elapsed);	

	// Due?
	bool due = false;
	if (value >= period)
	{
		value -= period;
		*timer = value;		
		due    = true;
	}

	// Result	
	return due;
}

/**********************************************************
 * Function:    timer32_is_due
 * Description: Update timer and return true if period is due
 *********************************************************/
bool timer32_is_due(uint32_t * const timer, const uint32_t elapsed, const uint32_t period)
{
	// Increment
	uint16_t value = timer32_increment(timer, elapsed);	

	// Due?
	bool due = false;
	if (value >= period)
	{
		value -= period;
		*timer = value;		
		due    = true;
	}

	// Result	
	return due;
}

/**********************************************************
 * Function:    str_pos
 * Description: Search string within another one
 * Parameters:  See below.
 * Return:      First occurence index 
 *              -1 = not found
 *********************************************************/
int str_pos
(
	const char* str,    // in: String input
	const char* substr  // in: String to search
)
{
	int result = -1;
	int len_str = strlen(str);
	int len_substr = strlen(substr);
	int pos = 0;
	int pos_max = len_str - len_substr + 1;
	while (pos < pos_max)
	{
		if (memcmp(&str[pos], substr, len_substr) == 0)
		{
			result = pos;
			break;
		}
		pos++;
	}
	return result;
}

/**********************************************************
 * Function:    debounce32_init
 * Description: Initialize debounce
 *********************************************************/
void debounce32_init(debounce32_t * debounce, bool state, uint32_t delay_on, uint32_t delay_off)
{
	debounce->state     = state;
	debounce->delay_on  = delay_on;
	debounce->delay_off = delay_off;
	debounce->timer     = 0;
}

/**********************************************************
 * Function:    debounce32_config
 * Description: Reconfigure debouncing
 *********************************************************/
void debounce32_config(debounce32_t * debounce, uint32_t delay_on, uint32_t delay_off)
{
	debounce->delay_on  = delay_on;
	debounce->delay_off = delay_off;
}

/**********************************************************
 * Function:    debounce32_process
 * Description: Debounce input
 *********************************************************/
bool debounce32_process(debounce32_t * debounce, bool state, uint32_t elapsed)
{
	uint32_t delay = debounce->state? debounce->delay_off : debounce->delay_on;
	if (debounce->state != state)
	{
		debounce->timer += elapsed;
		if (debounce->timer >= delay)
		{
			debounce->state = state;
			debounce->timer = 0;
		}
	}
	else
	{
		if (debounce->timer > elapsed)
		{
			debounce->timer -= elapsed;
		}
		else
		{
			debounce->timer = 0;
		}
	}
	return debounce->state;
}

/**********************************************************
 * Function:    debounce32_process_with_reset
 * Description: Debounce input and reset debounce timer on state change
 *********************************************************/
bool debounce32_process_with_reset(debounce32_t* debounce, bool state, uint32_t elapsed)
{
	uint32_t delay = debounce->state? debounce->delay_off : debounce->delay_on;
	if (debounce->state != state)
	{
		debounce->timer += elapsed;
		if (debounce->timer >= delay)
		{
			debounce->state = state;
			debounce->timer = 0;
		}
	}
	else
	{
		debounce->timer = 0;
	}
	return debounce->state;
}

/**********************************************************
 * Function:    debounce32_process_on
 * Description: Debounce process on
 *********************************************************/
bool debounce32_process_on(debounce32_t * debounce, uint32_t elapsed)
{
	return debounce32_process(debounce, true, elapsed);
}

/**********************************************************
 * Function:    debounce_off
 * Description: Debounce off input
 *********************************************************/
bool debounce32_process_off(debounce32_t * debounce, uint32_t elapsed)
{
	return debounce32_process(debounce, false, elapsed);
}

/**********************************************************
 * Function:    access_init
 * Description: Initialize buffer
 *********************************************************/
#if 0
void access_init(access_t* access)
{
	int len = sizeof(access->buffer);
	for (int i=0; i<len; i++)
	{
		uint8_t expected = (0x55+i);
		access->buffer[i] = expected;
	}
}
#endif

/**********************************************************
 * Function:    access_check
 * Description: Check buffer is not overwitten
 *********************************************************/
#if 0
void access_check(access_t* access, const char* info1, int info2)
{
	int len = sizeof(access->buffer);
	int res = 0;
	for (int i=0; i<len; i++)
	{
		uint8_t expected = (0x55+i);
		if (access->buffer[i] != expected)
		{
			access->buffer[i] = expected;
			res++;
		}
	}

	if (res != 0)
	{
		LOG(E, "Access violation at %s [%d] : %08x", info1, info2, (uint32_t)access);
	}
}
#endif
