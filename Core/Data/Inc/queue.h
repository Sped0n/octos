#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <stddef.h>
#include <stdint.h>

#include "list.h"
#include "tcb.h"
#include "utils.h"

extern TCB_t *current_tcb;

typedef struct Queue {
    uint8_t *Buffer;       // Pointer to queue storage area
    size_t ItemSize;       // Size of each item in the queue
    size_t Length;         // Maximum number of items in queue
    size_t MessagesWaiting;// Current number of items in queue
    size_t Head;           // Write index
    size_t Tail;           // Read index
    List_t SenderList;     // List of tasks waiting to send
    List_t ReceiverList;   // List of tasks waiting to receive
} Queue_t;

// Initialize a queue with given storage buffer
void queue_init(Queue_t *queue, void *buffer, size_t item_size, size_t length);

// Send an item to the queue
bool queue_send(Queue_t *queue, const void *item, uint32_t timeout_ticks);

// Receive an item from the queue
bool queue_recv(Queue_t *queue, void *buffer, uint32_t timeout_ticks);

// Check if queue is full
OCTOS_INLINE static inline bool queue_is_full(const Queue_t *queue) {
    return queue->MessagesWaiting >= queue->Length;
}

// Check if queue is empty
OCTOS_INLINE static inline bool queue_is_empty(const Queue_t *queue) {
    return queue->MessagesWaiting == 0;
}

// Get number of items in queue
OCTOS_INLINE static inline size_t queue_waiting(const Queue_t *queue) {
    return queue->MessagesWaiting;
}

// Get spaces available in queue
OCTOS_INLINE static inline size_t queue_spaces(const Queue_t *queue) {
    return queue->Length - queue->MessagesWaiting;
}

OCTOS_INLINE static inline bool is_queue_ops_timeout(void) {
    return current_tcb->EventListItem.Parent != NULL;
}

#endif
