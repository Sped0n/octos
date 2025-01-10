#ifndef __ITC_H__
#define __ITC_H__

#include <stddef.h>
#include <stdint.h>

#include "list.h"
#include "queue.h"

/**
  * @brief Message queue structure containing queue and waiting lists
  */
typedef struct MsgQueue {
    Queue_t Queue;       /*!< Underlying queue structure for message storage */
    List_t SenderList;   /*!< List of tasks waiting to send messages */
    List_t ReceiverList; /*!< List of tasks waiting to receive messages */
} MsgQueue_t;

void msg_queue_init(MsgQueue_t *mqueue, void *buffer, size_t item_size, size_t max_size);
bool msg_queue_send(MsgQueue_t *mqueue, const void *item, uint32_t timeout_ticks);
bool msg_queue_recv(MsgQueue_t *mqueue, void *buffer, uint32_t timeout_ticks);

#endif
