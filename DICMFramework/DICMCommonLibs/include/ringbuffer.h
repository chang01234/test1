/*
	Lockless single-consumer single-producer ring buffer that handles one byte at a time
	Jens Björnhager 2020-06-03
*/

#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <string.h>

#ifdef __IAR_SYSTEMS_ICC__
#include "stm8s.h"
#else
#include <stdint.h>
#endif

#define RB_SIZE			128		    												//!< \~ Ring buffer size (2^x)

#define RB_MASK         (RB_SIZE - 1)                                               //!< \~ Wraparound mask
#define RB_ENTRIES(rb)	((rb.write_index - rb.read_index) & RB_MASK)		        //!< \~ Data content count
#define RB_AVAIL(rb)	(MIN(RB_ENTRIES(rb), (RB_SIZE - rb.read_index)))		    //!< \~ Contiguous data available to read from buffer
#define RB_EMPTY(rb)	(rb.read_index == rb.write_index)							//!< \~ Is ringbuffer empty?
#define RB_FULL(rb)		(rb.read_index == ((rb.write_index + 1) & RB_MASK))			//!< \~ Is ringbuffer full?

//! \~ Lock-free circular ringbuffer struct
typedef struct _RINGBUFFER
{
	uint8_t read_index;		//!< \~ Read index; consumer
	uint8_t write_index;	//!< \~ Write index; producer
	uint8_t data[RB_SIZE];
} RINGBUFFER;

void ringbuffer_init(RINGBUFFER * const rb);
int ringbuffer_push(RINGBUFFER * const rb, const uint8_t value);
int ringbuffer_pop(RINGBUFFER * const rb, uint8_t * const value);
void ringbuffer_remove(RINGBUFFER * const rb, const int count);

#endif // RINGBUFFER_H
