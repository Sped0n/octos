#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "Kernel/Inc/utils.h"

typedef struct Queue {
    uint8_t *Buffer;
    size_t ItemSize;
    size_t MaxSize;
    size_t Size;
    size_t WriteIndex;
    size_t ReadIndex;
} Queue_t;

// Initialize a queue with given storage buffer
void queue_init(Queue_t *queue, void *buffer, size_t item_size, size_t max_size);

// Core queue operations
bool queue_send_from_isr(Queue_t *queue, const void *item);
bool queue_recv_from_isr(Queue_t *queue, void *buffer);

// Queue status checks
OCTOS_INLINE static inline bool queue_is_full(const Queue_t *queue) {
    return queue->Size >= queue->MaxSize;
}

OCTOS_INLINE static inline bool queue_is_empty(const Queue_t *queue) {
    return queue->Size == 0;
}

OCTOS_INLINE static inline size_t queue_size(const Queue_t *queue) {
    return queue->Size;
}

OCTOS_INLINE static inline size_t queue_spaces(const Queue_t *queue) {
    return queue->MaxSize - queue->Size;
}

#endif
