#include "queue.h"
#include <string.h>

void queue_init(Queue_t *queue, void *buffer, size_t item_size, size_t max_size) {
    queue->Buffer = buffer;
    queue->ItemSize = item_size;
    queue->MaxSize = max_size;
    queue->Size = 0;
    queue->WriteIndex = 0;
    queue->ReadIndex = 0;
}

bool queue_send_from_isr(Queue_t *queue, const void *item) {
    if (queue_is_full(queue)) {
        return false;
    }

    uint8_t *dest = queue->Buffer + (queue->WriteIndex * queue->ItemSize);
    memcpy(dest, item, queue->ItemSize);

    queue->WriteIndex++;
    if (queue->WriteIndex >= queue->MaxSize) {
        queue->WriteIndex = 0;
    }

    queue->Size++;
    return true;
}

bool queue_recv_from_isr(Queue_t *queue, void *buffer) {
    if (queue_is_empty(queue)) {
        return false;
    }

    uint8_t *src = queue->Buffer + (queue->ReadIndex * queue->ItemSize);
    memcpy(buffer, src, queue->ItemSize);

    queue->ReadIndex++;
    if (queue->ReadIndex >= queue->MaxSize) {
        queue->ReadIndex = 0;
    }

    queue->Size--;
    return true;
}
