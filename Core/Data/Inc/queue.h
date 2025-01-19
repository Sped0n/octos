#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "attr.h"

/**
  * @brief Queue structure definition for managing circular buffer
  */
typedef struct Queue {
    uint8_t *Buffer;   /*!< Pointer to the buffer memory */
    size_t ItemSize;   /*!< Size of each item in bytes */
    size_t MaxSize;    /*!< Maximum number of items that can be stored */
    size_t Size;       /*!< Current number of items in the queue */
    size_t WriteIndex; /*!< Index for next write operation */
    size_t ReadIndex;  /*!< Index for next read operation */
} Queue_t;

void queue_init(Queue_t *queue, void *buffer, size_t item_size,
                size_t max_size);
bool queue_send(Queue_t *queue, const void *item);
bool queue_recv(Queue_t *queue, void *buffer);

/**
  * @brief Check if queue is full
  * @param queue: Pointer to queue structure
  * @return true if queue is full, false otherwise
  */
OCTOS_INLINE static inline bool queue_is_full(const Queue_t *queue) {
    return queue->Size >= queue->MaxSize;
}

/**
  * @brief Check if queue is empty
  * @param queue: Pointer to queue structure
  * @return true if queue is empty, false otherwise
  */
OCTOS_INLINE static inline bool queue_is_empty(const Queue_t *queue) {
    return queue->Size == 0;
}

/**
  * @brief Get current number of items in queue
  * @param queue: Pointer to queue structure
  * @return Current size of queue
  */
OCTOS_INLINE static inline size_t queue_size(const Queue_t *queue) {
    return queue->Size;
}

/**
  * @brief Get number of free spaces in queue
  * @param queue: Pointer to queue structure
  * @return Number of available spaces
  */
OCTOS_INLINE static inline size_t queue_spaces(const Queue_t *queue) {
    return queue->MaxSize - queue->Size;
}

#endif
