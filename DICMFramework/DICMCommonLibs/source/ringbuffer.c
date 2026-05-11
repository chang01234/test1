#include "ringbuffer.h"
#include <string.h>

#define PRB_EMPTY(rb)	(rb->read_index == rb->write_index)						//!< \~ Is ringbuffer empty?
#define PRB_FULL(rb)	(rb->read_index == ((rb->write_index + 1) & RB_MASK))	//!< \~ Is ringbuffer full?

/*! \brief Initialize (zero-fill) ringbuffer
	\param rb Pointer to ringbuffer
*/
void ringbuffer_init(RINGBUFFER * const rb)
{
	memset(rb, 0, sizeof(RINGBUFFER));
}

/*! \brief Add a value to ringbuffer
	\param rb Pointer to ringbuffer
	\param value Value to add
	\return (false) Error
	\return (true) Success
*/
int ringbuffer_push(RINGBUFFER * const rb, const uint8_t value)
{
	if PRB_FULL(rb)
	{
		return 0;
	}

    rb->data[rb->write_index] = value;
	rb->write_index = (rb->write_index + 1) & RB_MASK;

	return 1;
}

/*! \brief Commit a read, deallocate position
	\param rb Pointer to ringbuffer
	\param value [out] Out buffer
	\return (false) Error
	\return (true) Success
*/
int ringbuffer_pop(RINGBUFFER * const rb, uint8_t * const value)
{
	if PRB_EMPTY(rb)
	{
		return 0;
	}

    *value = rb->data[rb->read_index];
	rb->read_index = (rb->read_index + 1) & RB_MASK;

	return 1;
}

/*! \brief Deallocate positions
	\param rb Pointer to ringbuffer
	\param count Number of bytes to deallocate
*/
void ringbuffer_remove(RINGBUFFER * const rb, const int count)
{
	rb->read_index = (rb->read_index + count) & RB_MASK;
}
