/*!
    \file	ringbuffer2.h
    \brief  Ringbuffer2 header
            Single-consumer multiple-producer ring buffer that can handle multiple bytes per transaction
            Lock is optional if single-producer

    \author Jens Björnhager
*/

#ifndef RINGBUFFER2_H
#define RINGBUFFER2_H

#include <stddef.h>
#include <stdint.h>

#define RINGBUFFER2_TEST 0

typedef enum RINGBUFFER2_LOCK_ENUM
{
    RINGBUFFER2_LOCK = 0,    //!< \~ Lock
    RINGBUFFER2_UNLOCK = 1,  //!< \~ Unlock
} RINGBUFFER2_LOCK_ENUM;

typedef int (*RINGBUFFER2_LOCK_CB)(const RINGBUFFER2_LOCK_ENUM operation, void *lock_callback_context);

//! \~ Circular ringbuffer struct
typedef struct RINGBUFFER2
{
    RINGBUFFER2_LOCK_CB lock_callback;  //!< \~ Lock/unlock callback
    void *lock_callback_context;        //!< \~ Context for lock/unlock callback
    size_t read_index;                  //!< \~ Read index; consumer
    size_t write_index;                 //!< \~ Write index; producer
    size_t buffer_size;                 //!< \~ size of ringbuffer (capacity + 1)
    uint8_t *pdata;                     //!< \~ Pointer to data	storage
} RINGBUFFER2;

RINGBUFFER2 *ringbuffer2_create(RINGBUFFER2 **const rb, const size_t size, const RINGBUFFER2_LOCK_CB lock_cb, void *context);
RINGBUFFER2 *ringbuffer2_init_static(RINGBUFFER2 *const rb, const RINGBUFFER2_LOCK_CB lock_cb, void *context);
int ringbuffer2_write(RINGBUFFER2 *const rb, const uint8_t *const data, const size_t size);
size_t ringbuffer2_read(RINGBUFFER2 *const rb, uint8_t *const data, size_t const size);
void ringbuffer2_clear(RINGBUFFER2 *const rb);
void ringbuffer2_destroy(RINGBUFFER2 *const rb);

#endif  // RINGBUFFER2_H
