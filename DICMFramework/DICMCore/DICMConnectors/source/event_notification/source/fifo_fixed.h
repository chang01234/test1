/**
 * \file        fifo_fixed.h
 * \date        2024-06-27
 * \author      (NR) Nenad Radulovic (nenad.b.radulovic@gmail.com)
 *
 * \brief       Fixed size FIFO interface
 *
 * \li          2024-06-27  (NR) Initial implementation
 *
 * \copyright   Dometic Group
 *              This source file and the information contained in it are
 *              confidential and proprietary to Dometic Group
 *              The reproduction or disclosure, in whole or in part,
 *              to anyone outside of Dometic Group without the written
 *              approval of a Dometic Group officer under a Non-Disclosure
 *              Agreement is expressly prohibited.
 *
 *              All rights reserved
 */

#ifndef FIFO_FIXED_H_
#define FIFO_FIXED_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @brief FIFO fixed structure.
 *
 * This structure represents a FIFO of fixed size.
 */
typedef struct fifo_fixed
{
    void * items;                       //!< Pointer to the buffer storing the elements.
    uint32_t head;                      //!< Index of the head of the FIFO.
    uint32_t tail;                      //!< Index of the tail of the FIFO.
    uint32_t occupied_items;            //!< Number of items currently in the FIFO.
    uint32_t free_items;                //!< Number of free items in the FIFO.
    uint32_t max_items;                 //!< Maximum number of items the FIFO can hold.
    size_t item_size;                   //!< Size of each item in the FIFO in bytes.
} fifo_fixed_t;

/**
 * @brief Initialize the FIFO.
 *
 * When calling this function provide the buffer pointed by @a items which can
 * hold @a max_items item instances.
 *
 * @param fifo Pointer to the FIFO structure to initialize.
 * @param items Pointer to buffer which stores the items.
 * @param item_size Size of each item in the FIFO.
 * @param max_items Maximum number of items the FIFO can hold.
 */
void fifo_fixed_init(fifo_fixed_t * fifo, void * items, size_t item_size, uint32_t max_items);

/**
 * @brief Deinitialize the FIFO.
 *
 * @param fifo Pointer to the FIFO structure to deinitialize.
 */
void fifo_fixed_deinit(fifo_fixed_t *fifo);

/**
 * @brief Enqueue an item to the back of the FIFO.
 *
 * Put the @a item to the back of the queue, First In - First Out (FIFO) model.
 * If the queue is full when this function is called, then the oldest item is
 * dropped from the queue.
 *
 * @param fifo Pointer to the FIFO structure.
 * @param item Pointer to the item to enqueue.
 */
void fifo_fixed_enqueue_back(fifo_fixed_t *fifo, const void *item);

/**
 * @brief Enqueue an item to the front of the FIFO.
 *
 * Put the @a item to the fron of the queue, Last In - First Out (LIFO) model.
 * If the queue is full when this function is called, then the oldest item is
 * dropped from the queue.
 *
 * @param fifo Pointer to the FIFO structure.
 * @param item Pointer to the item to enqueue.
 */
void fifo_fixed_enqueue_front(fifo_fixed_t *fifo, const void *item);

/**
 * @brief Dequeue an item from the front of the FIFO.
 *
 * @param fifo Pointer to the FIFO structure.
 * @param item Pointer to the location to store the dequeued item.
 * @note Before calling this function ensure the queue is not empty using
 * function @ref fifo_fixed_is_empty() function.
 */
void fifo_fixed_dequeue(fifo_fixed_t *fifo, void *item);

/**
 * @brief Delete all items that are stored in the FIFO.
 *
 * @param fifo Pointer to the FIFO structure.
 */
void fifo_fixed_delete_all(fifo_fixed_t *fifo);

/**
 * @brief Get the item size in bytes.
 *
 * @param fifo Pointer to the FIFO structure.
 * @return size_t Item size in bytes.
 */
inline size_t fifo_fixed_get_item_size(const fifo_fixed_t * fifo)
{
    return fifo->item_size;
}

/**
 * @brief Get the number of occupied items in the FIFO.
 *
 * @param fifo Pointer to the FIFO structure.
 * @return uint32_t How many items are stored in FIFO.
 */
inline uint32_t fifo_fixed_get_occupied_items(const fifo_fixed_t * fifo)
{
    return fifo->occupied_items;
}

/**
 * @brief Check if FIFO is empty.
 *
 * @param fifo Pointer to the FIFO structure.
 * @return true FIFO is empty, contains no item in FIFO.
 * @return false FIFO is not empty, contains at least one item in FIFO.
 */
inline bool fifo_fixed_is_empty(const fifo_fixed_t * fifo)
{
    return fifo->occupied_items == 0;
}

/**
 * @brief Check if FIFO is full.
 *
 * @param fifo Pointer to the FIFO structure.
 * @return true No free space if available in FIFO. Inserting new item would
 *         drop the oldest item.
 * @return false There is at least one free slot for the item.
 */
inline bool fifo_fixed_is_full(const fifo_fixed_t * fifo)
{
    return fifo->free_items == 0;
}

/**
 * @brief Peek into FIFO.
 *
 * Use this function to peek into a desired item. Items are indexed from 0 to
 * N - 1, where N is equal to @ref fifo_fixed_get_occupied_items. Item at index
 * 0 represents an item at tail position, while the item at index N - 1
 * represents an item at head position.
 *
 * @note Before calling this function ensure FIFO contains at least one entry. For
 *       this use @ref fifo_fixed_is_empty function.
 *
 * @note Ensure the index @a index is smaller than the number returned by
 *       @ref fifo_fixed_get_occupied_items function.
 *
 * @param fifo Pointer to the FIFO structure.
 * @param index Index of the item to peek.
 * @return Pointer to stored FIFO item(s). Item is not removed from FIFO, it is
 *         only accessed.
 */
const void * fifo_fixed_peek(const fifo_fixed_t * fifo, uint32_t index);

#endif /* FIFO_FIXED_H_ */