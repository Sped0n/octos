#ifndef __ITC_H__
#define __ITC_H__

#include <stddef.h>
#include <stdint.h>

#include "list.h"
#include "queue.h"

typedef struct MsgQueue {
    Queue_t Queue;
    List_t SenderList;
    List_t ReceiverList;
} MsgQueue_t;

// Initialize message queue
void msg_queue_init(MsgQueue_t *mqueue, void *buffer, size_t item_size, size_t max_size);

// Send/receive with timeout
bool msg_queue_send(MsgQueue_t *mqueue, const void *item, uint32_t timeout_ticks);
bool msg_queue_recv(MsgQueue_t *mqueue, void *buffer, uint32_t timeout_ticks);

#endif
