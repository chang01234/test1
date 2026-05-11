/*!
    \file	ringbuffer2.c
    \brief	Ringbuffer2 source
            Single-consumer multiple-producer ring buffer that can handle multiple bytes per transaction
            Lock is optional if single-producer

    \author Jens Björnhager
*/

#include "ringbuffer2.h"

#include <stdlib.h>  //calloc(), malloc(), free()
#include <string.h>  //memcpy()

#define RB2_MASK(pprb) (pprb->buffer_size - 1)  //!< \~ Wraparound mask

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))  //!< \~ Minimum value macro
#endif

/*! \brief Initialize dynamically allocated ringbuffer
    \param pprb Pointer to ringbuffer pointer
    \param size Size of ringbuffer (must be power of 2)
    \param lock_cb Lock callback (optional)
    \param context Context for lock callback (optional)
*/
RINGBUFFER2 *ringbuffer2_create(RINGBUFFER2 **const pprb, const size_t size, const RINGBUFFER2_LOCK_CB lock_cb, void *context)
{
    if (pprb == NULL)
    {
        return NULL;
    }

    if ((size == 0) || (size & (size - 1)))  // size must be nonzero and a power of 2
    {
        return NULL;
    }

    *pprb = calloc(1, sizeof(RINGBUFFER2));  // allocate metadata structure

    if ((*pprb) == NULL)
    {
        return NULL;
    }

    (*pprb)->pdata = (uint8_t *)malloc(size);  // allocate data storage

    if ((*pprb)->pdata == NULL)
    {
        free(*pprb);

        return NULL;
    }

    (*pprb)->buffer_size = size;  // initialize metadata
    (*pprb)->lock_callback = lock_cb;
    (*pprb)->lock_callback_context = context;

    return *pprb;
}

/*! \brief Initialize statically allocated ringbuffer
    \param prb Pointer to ringbuffer
    \param lock_cb Lock callback (optional)
    \param context Context for lock callback (optional)
*/
RINGBUFFER2 *ringbuffer2_init_static(RINGBUFFER2 *const prb, const RINGBUFFER2_LOCK_CB lock_cb, void *context)
{
    if ((prb == NULL) || (prb->pdata == NULL) || (prb->buffer_size) || (prb->buffer_size & (prb->buffer_size - 1)))  // size must be nonzero and a power of 2
    {
        return NULL;
    }

    prb->read_index = 0;  // make sure read/write indices are zeroed
    prb->write_index = 0;
    prb->lock_callback = lock_cb;  // initialize meatadata
    prb->lock_callback_context = context;

    return prb;
}

/*! \brief Add data to ringbuffer
    \param prb Pointer to ringbuffer
    \param data Data to add
    \param size Size of data
    \return true if success, false if not enough space
*/
int ringbuffer2_write(RINGBUFFER2 *const prb, const uint8_t *const data, const size_t size)
{
    if (prb == NULL)
    {
        return 0;
    }

    if (!size)
    {
        return 1;  // Allow writing 0 bytes, no-op
    }

    const size_t Free_space = (prb->buffer_size - 1) - ((prb->write_index - prb->read_index) & RB2_MASK(prb));

    if (size > Free_space)
    {
        return 0;
    }

    if ((prb->lock_callback == NULL) || prb->lock_callback(RINGBUFFER2_LOCK, prb->lock_callback_context))  // lock for writing (if necessary)
    {
        const size_t Size1 = prb->buffer_size - prb->write_index;
        const size_t Write_size1 = MIN(size, Size1);

        memcpy(&prb->pdata[prb->write_index], data, Write_size1);
        prb->write_index += Write_size1;
        prb->write_index &= RB2_MASK(prb);

        const size_t Size2 = size - Write_size1;
        const size_t Write_size2 = MIN(size - Write_size1, Size2);

        if (Write_size2 > 0)
        {
            memcpy(prb->pdata, &data[Size1], Write_size2);
            prb->write_index += Write_size2;
        }

        if (prb->lock_callback != NULL)
        {
            prb->lock_callback(RINGBUFFER2_UNLOCK, prb->lock_callback_context);
        }
    }

    return 1;
}

/*! \brief Read from ringbuffer
    \param prb Pointer to ringbuffer
    \param data Pointer to output buffer
    \param size Size of output buffer
    \return bytes copied to output buffer
*/
size_t ringbuffer2_read(RINGBUFFER2 *const prb, uint8_t *const data, size_t const size)
{
    if (prb == NULL)
    {
        return 0;
    }

    if (prb->write_index >= prb->read_index)  // no wraparound, contiguous read to write index
    {
        const size_t Size1 = prb->write_index - prb->read_index;
        const size_t Read_size1 = MIN(size, Size1);

        memcpy(data, &prb->pdata[prb->read_index], Read_size1);
        prb->read_index += Read_size1;
        prb->read_index &= RB2_MASK(prb);

        return Read_size1;
    }

    const size_t Size1 = prb->buffer_size - prb->read_index;  // wraparound, read to end of buffer
    const size_t Read_size1 = MIN(size, Size1);

    memcpy(data, &prb->pdata[prb->read_index], Read_size1);
    prb->read_index += Read_size1;
    prb->read_index &= RB2_MASK(prb);

    if (prb->read_index)
    {
        return Read_size1;
    }

    const size_t Size2 = prb->write_index;  // wraparound, read from beginning of buffer
    const size_t Read_size2 = MIN(size - Read_size1, Size2);

    memcpy(&data[Read_size1], prb->pdata, Read_size2);
    prb->read_index = Read_size2;

    return Read_size1 + Read_size2;
}

/*! \brief Clear ringbuffer
    \param prb Pointer to ringbuffer
*/
void ringbuffer2_clear(RINGBUFFER2 *const prb)
{
    if (prb != NULL)
    {
        prb->read_index = 0;
        prb->write_index = 0;
    }
}

/*! \brief Destroy ringbuffer
    \param prb Pointer to ringbuffer
*/
void ringbuffer2_destroy(RINGBUFFER2 *const prb)
{
    if (prb != NULL)
    {
        if (prb->pdata != NULL)
        {
            free(prb->pdata);
            prb->pdata = NULL;
        }

        free(prb);
    }
}
