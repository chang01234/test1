#include <stdio.h>
#include <string.h>

#include "ddm.h"

extern const DDMP_CALLBACKS *ddmp_callbacks;	//ddm.c, to be able to call LOCK callbacks
extern DDMP_CONNECTION_STATUS ddmp_connection_status[DDMP_INTERFACE_COUNT];	//ddm.c, to access queue from intf and putqueue

/*! \brief Initialize (zero-fill) cqueue
	\param intf Interface
	\param cq Pointer to cqueue
*/
void cqueue_init(int intf, CQUEUE *cq)
{
	DDMP_LOCK;
	{
		memset(cq, 0, sizeof(CQUEUE));
		DDMP_UNLOCK;
	}
}

/*! \brief Get current write index, the position in queue to write to
	\param cq Pointer to cqueue
	\return (negative) Error
	\return (non-negative) Write index
*/
int cqueue_write(CQUEUE *cq)
{
	if (cq == NULL)
		return CQUEUE_ERROR_ARGUMENT;
	if (CQ_FULL(cq))
		return CQUEUE_ERROR_QUEUE_FULL;
	return cq->write_index;
}

/*! \brief Commit a write, allocate a new position (locked, multiple producer)
	\param intf Interface
	\param putqueue Which queue of interface to push to
	\param data Pointer to data to push
	\return (false) Error
	\return (true) Success
*/
int cqueue_push(int intf, DDMP_QUEUE_ENUM putqueue, const DDMP_FRAME* data)
{
	CQUEUE *cq = &ddmp_connection_status[intf].queue[putqueue];

	if (cq == NULL)
		return CQUEUE_ERROR_ARGUMENT;

	DDMP_LOCK;
	{
		if (CQ_FULL(cq))
		{
			DDMP_UNLOCK;
			return CQUEUE_ERROR_QUEUE_FULL;
		}

		DDMP_FRAME *dest = &cq->frames[cq->write_index];
		*dest = *data;
		cq->write_index = (cq->write_index + 1)&(CQ_QUEUESIZE - 1);
		DDMP_UNLOCK;
	}

	return 1;
}

/*! \brief Get current read index, the position in queue to read from
	\param cq Pointer to cqueue
	\return (negative) Error
	\return (non-negative) Write index
*/
int cqueue_read(CQUEUE *cq)
{
	if (cq == NULL)
		return CQUEUE_ERROR_ARGUMENT;
	if (CQ_EMPTY(cq))
		return CQUEUE_ERROR_QUEUE_EMPTY;
	return cq->read_index;
}

/*! \brief Read data, deallocate position (single consumer)
	\param intf Interface
	\param putqueue Which queue of interface to pop
	\param data Pointer to where to put popped data
	\return (false) Error
	\return (true) Success
*/
int cqueue_pop(int intf, DDMP_QUEUE_ENUM putqueue, DDMP_FRAME* data)
{
	CQUEUE *cq = &ddmp_connection_status[intf].queue[putqueue];

	if (cq == NULL)
		return CQUEUE_ERROR_ARGUMENT;
	if CQ_EMPTY(cq)
		return CQUEUE_ERROR_QUEUE_EMPTY;

	DDMP_FRAME *src = &cq->frames[cq->read_index];
	*data = *src;
	cq->read_index = (cq->read_index + 1)&(CQ_QUEUESIZE - 1);

	return 1;
}

/*! \brief Check if queue is full
	\param cq Pointer to cqueue
	\return (negative) Error
	\return (false) Not full
	\return (true) Full
*/
int cqueue_full(CQUEUE *cq)
{
	if (cq == NULL)
		return CQUEUE_ERROR_ARGUMENT;
	return CQ_FULL(cq);
}

/*! \brief Check if queue is empty
	\param cq Pointer to cqueue
	\return (negative) Error
	\return (false) Not empty
	\return (true) Empty
*/
int cqueue_empty(CQUEUE *cq)
{
	if (cq == NULL)
		return CQUEUE_ERROR_ARGUMENT;
	return CQ_EMPTY(cq);
}

void cqueue_print(CQUEUE *cq)
{
	if (cq == NULL)
		printf("NULL\n");
	return;
	for (unsigned int i = cq->read_index; i != cq->write_index; i = ((i + 1)&(CQ_QUEUESIZE - 1)))
		printf("%d", i);
	printf("\n");
}

int cqueue_available(int intf, DDMP_QUEUE_ENUM putqueue)
{
	CQUEUE *cq = &ddmp_connection_status[intf].queue[putqueue];
	int available=0;
	DDMP_LOCK;
	{
		for (available = 0; cq->read_index!=((cq->write_index+1+available)&(CQ_QUEUESIZE-1)); available++);
		DDMP_UNLOCK;
	}

	return available;
}
