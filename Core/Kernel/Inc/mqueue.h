#ifndef __MQUEUE_H__
#define __MQUEUE_H__

#include <stddef.h>
#include <stdint.h>

#include "list.h"
#include "queue.h"

#define queueUNLOCKED ((int8_t) -1)
#define queueLOCKED_UNMODIFIED ((int8_t) 0)

/**
  * @brief Message queue structure containing queue and waiting lists
  */
typedef struct MsgQueue {
    Queue_t Queue;       /*!< Underlying queue structure for message storage */
    List_t SenderList;   /*!< List of tasks waiting to send messages */
    List_t ReceiverList; /*!< List of tasks waiting to receive messages */
    int8_t RxLock;       /*!< Lock for receiving messages */
    int8_t TxLock;       /*!< Lock for sending messages */
} MsgQueue_t;

void mqueue_init(MsgQueue_t *mqueue, void *buffer, size_t item_size,
                 size_t max_size);
bool mqueue_send(MsgQueue_t *mqueue, const void *item, uint32_t timeout_ticks);
bool mqueue_recv(MsgQueue_t *mqueue, void *buffer, uint32_t timeout_ticks);
bool mqueue_send_from_isr(MsgQueue_t *mqueue, const void *item,
                          bool *const switch_required);
bool mqueue_recv_from_isr(MsgQueue_t *mqueue, void *buffer,
                          bool *const switch_required);

#endif
