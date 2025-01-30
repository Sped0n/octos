#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "queue.h"

/**
 * @brief Initialize queue structure
 * @param queue: Pointer to queue structure
 * @param buffer: Pointer to memory buffer for queue storage
 * @param item_size_in_bytes: Size of each queue item in bytes
 * @param max_size: Maximum number of items queue can hold
 * @return None
 */
void queue_init(Queue_t *queue, void *buffer, size_t item_size_in_bytes,
                size_t max_size) {
    queue->Buffer = buffer;
    queue->ItemSize = item_size_in_bytes;
    queue->MaxSize = max_size;
    queue->Size = 0;
    queue->WriteIndex = 0;
    queue->ReadIndex = 0;
}

/**
 * @brief Send item to queue
 * @param queue: Pointer to queue structure
 * @param item: Pointer to item to be added
 * @note This function instantly return
 * @retval true If item was successfully added
 * @retval false If queue is full
 */
bool queue_send(Queue_t *queue, const void *item) {
    if (queue_is_full(queue)) return false;


    uint8_t *dest = queue->Buffer + (queue->WriteIndex * queue->ItemSize);
    memcpy(dest, item, queue->ItemSize);

    queue->WriteIndex++;
    if (queue->WriteIndex >= queue->MaxSize) queue->WriteIndex = 0;

    queue->Size++;
    return true;
}

/**
 * @brief Receive item from queue
 * @param queue: Pointer to queue structure
 * @param buffer: Buffer to store received item
 * @note This function instantly return
 * @retval true If item was successfully received
 * @retval false If queue is empty
 */
bool queue_recv(Queue_t *queue, void *buffer) {
    if (queue_is_empty(queue)) return false;

    uint8_t *src = queue->Buffer + (queue->ReadIndex * queue->ItemSize);
    memcpy(buffer, src, queue->ItemSize);

    queue->ReadIndex++;
    if (queue->ReadIndex >= queue->MaxSize) queue->ReadIndex = 0;

    queue->Size--;
    return true;
}
