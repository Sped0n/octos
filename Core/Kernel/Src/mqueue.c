#include <stdint.h>

#include "Arch/stm32f4xx/Inc/api.h"
#include "list.h"
#include "mqueue.h"
#include "queue.h"
#include "task.h"
#include "utils.h"

/* Private Helpers -----------------------------------------------------------*/

/**
 * @brief Increment the receive lock counter of a message queue
 * @note The lock counter is capped at the current number of tasks to
 *       ensure it does not exceed the number of tasks in the system
 * @param mqueue: Pointer to the message queue
 * @param rxlock: The current receive lock value
 * @return None
 */
OCTOS_INLINE static inline void mqueue_rxlock_increment(MsgQueue_t *mqueue,
                                                        int8_t rxlock) {
    const uint8_t current_number_of_tasks = task_get_number_of_tasks();
    /* Cap lock counter at current_number_of_tasks
     *
     * But why?
     * Beacause lock counter is actually used to keep track of task need to 
     * unblock in msg_queue_unlock, since 
     * numof(blocked task) < numof(current number of tasks), it is not 
     * necessary to let lock counter goes beyond current_number_of_tasks
     * */
    if ((uint8_t) rxlock < current_number_of_tasks) {
        OCTOS_ASSERT(rxlock < INT8_MAX);
        mqueue->RxLock = rxlock + 1;
    }
}

/**
 * @brief Increment the transmit lock counter of a message queue
 * @note The lock counter is capped at the current number of tasks to
 *       ensure it does not exceed the number of tasks in the system
 * @param mqueue: Pointer to the message queue
 * @param txlock: The current transmit lock value
 * @return None
 */
OCTOS_INLINE static inline void mqueue_txlock_increment(MsgQueue_t *mqueue,
                                                        int8_t txlock) {
    const uint8_t current_number_of_tasks = task_get_number_of_tasks();
    /* Cap lock counter at current_number_of_tasks
     *
     * But why?
     * Beacause lock counter is actually used to keep track of task need to 
     * unblock in msg_queue_unlock, since 
     * numof(blocked task) < numof(current number of tasks), it is not 
     * necessary to let lock counter goes beyond current_number_of_tasks
     * */
    if ((uint8_t) txlock < current_number_of_tasks) {
        OCTOS_ASSERT(txlock < INT8_MAX);
        mqueue->TxLock = txlock + 1;
    }
}

/**
 * @brief Lock a message queue by incrementing both receive and
 *        transmit lock counters
 * @param mqueue: Pointer to the message queue
 * @return None
 */
OCTOS_INLINE static inline void mqueue_lock(MsgQueue_t *mqueue) {
    OCTOS_ENTER_CRITICAL();

    if (mqueue->RxLock == queueUNLOCKED)
        mqueue->RxLock = queueLOCKED_UNMODIFIED;
    if (mqueue->TxLock == queueUNLOCKED)
        mqueue->TxLock = queueLOCKED_UNMODIFIED;

    OCTOS_EXIT_CRITICAL();
}

/**
 * @brief Unlock a message queue by decrementing the receive and transmit
 *        lock counters and unblocking tasks if necessary
 * @param mqueue: Pointer to the message queue
 * @return None
 */
static void mqueue_unlock(MsgQueue_t *mqueue) {
    OCTOS_ENTER_CRITICAL();

    int8_t rxlock = mqueue->RxLock;
    List_t *const sender_list = &(mqueue->SenderList);
    while (rxlock > queueLOCKED_UNMODIFIED) {
        if (sender_list->Length > 0) {
            /* The function will automatically set yield_pending for us */
            task_remove_highest_priority_from_event_list(sender_list);
        } else {
            break;
        }
        rxlock--;
    }
    mqueue->RxLock = queueUNLOCKED;

    OCTOS_EXIT_CRITICAL();

    OCTOS_ENTER_CRITICAL();

    int8_t txlock = mqueue->TxLock;
    List_t *const reciever_list = &(mqueue->ReceiverList);
    while (txlock > queueLOCKED_UNMODIFIED) {
        if (reciever_list->Length > 0) {
            /* The function will automatically set yield_pending for us */
            task_remove_highest_priority_from_event_list(reciever_list);
        } else {
            break;
        }
        txlock--;
    }
    mqueue->TxLock = queueUNLOCKED;

    OCTOS_EXIT_CRITICAL();
}

/* Public Methods ------------------------------------------------------------*/

/**
 * @brief Initializes a message queue
 * @param mqueue: Pointer to message queue to initialize
 * @param buffer: Pointer to buffer that will hold queue data
 * @param item_size_in_bytes: Size of each item in the queue
 * @param max_size: Maximum number of items in the queue
 * @return None
 */
void mqueue_init(MsgQueue_t *mqueue, void *buffer, size_t item_size_in_bytes,
                 size_t max_size) {
    queue_init(&mqueue->Queue, buffer, item_size_in_bytes, max_size);
    list_init(&mqueue->SenderList);
    list_init(&mqueue->ReceiverList);
    mqueue->RxLock = queueUNLOCKED;
    mqueue->TxLock = queueUNLOCKED;
}

/**
 * @brief Send an item to a message queue
 * @param mqueue: Pointer to the message queue
 * @param item: Pointer to the item to be sent
 * @param timeout_ticks: 
 *      The number of ticks to wait before returning if the queue is full
 * @retval true If the item was sent successfully
 * @retval false Otherwise
 */
bool mqueue_send(MsgQueue_t *mqueue, const void *item, uint32_t timeout_ticks) {
    Timeout_t timeout;
    bool timeout_set = false;

    while (true) {
        /* We have operation on queue, so a critical section 
         * is needed */
        OCTOS_ENTER_CRITICAL();

        if (queue_send(&mqueue->Queue, item)) {
            const bool switch_required =
                    task_remove_highest_priority_from_event_list(
                            &(mqueue->ReceiverList));
            OCTOS_EXIT_CRITICAL();
            if (switch_required) OCTOS_YIELD();
            return true;
        }

        if (timeout_ticks == 0) {
            OCTOS_EXIT_CRITICAL();
            return false;
        } else if (!timeout_set) {
            /* timeout_ticks == UINT32_MAX means to wait indefinitely */
            if (timeout_ticks != UINT32_MAX) task_set_timeout(&timeout);
            timeout_set = true;
        }

        OCTOS_EXIT_CRITICAL();

        task_suspend_all();
        /* Lock the queue so ISR cannot modify EventListItem */
        mqueue_lock(mqueue);

        /* Timeout has expired */
        if (timeout_ticks != UINT32_MAX &&
            task_check_timeout(&timeout, timeout_ticks)) {
            mqueue_unlock(mqueue);
            task_resume_all();
            return false;
        }

        /* Timeout has not expired */
        if (queue_is_full(&(mqueue->Queue))) {
            task_add_current_to_event_list(&(mqueue->SenderList),
                                           timeout_ticks);
            mqueue_unlock(mqueue);
            if (!task_resume_all()) OCTOS_YIELD();
        } else {
            mqueue_unlock(mqueue);
            task_resume_all();
        }
    }
}

/**
 * @brief Receive an item from a message queue
 * @param mqueue: Pointer to the message queue
 * @param buffer: Pointer to the buffer where the received item will be stored
 * @param timeout_ticks: 
 *      The number of ticks to wait before returning if the queue is empty
 * @retval true If an item was received successfully
 * @retval false otherwise
 */
bool mqueue_recv(MsgQueue_t *mqueue, void *buffer, uint32_t timeout_ticks) {
    Timeout_t timeout;
    bool timeout_set = false;

    while (true) {
        /* We have operation on queue, so a critical section 
         * is needed */
        OCTOS_ENTER_CRITICAL();

        if (queue_recv(&mqueue->Queue, buffer)) {
            const bool switch_required =
                    task_remove_highest_priority_from_event_list(
                            &mqueue->SenderList);
            OCTOS_EXIT_CRITICAL();
            if (switch_required) OCTOS_YIELD();
            return true;
        }

        if (timeout_ticks == 0) {
            OCTOS_EXIT_CRITICAL();
            return false;
        } else if (!timeout_set) {
            /* timeout_ticks == UINT32_MAX means to wait indefinitely */
            if (timeout_ticks != UINT32_MAX) task_set_timeout(&timeout);
            timeout_set = true;
        }

        OCTOS_EXIT_CRITICAL();

        task_suspend_all();
        /* Lock the queue so ISR cannot modify EventListItem */
        mqueue_lock(mqueue);

        /* Timeout has expired */
        if (timeout_ticks != UINT32_MAX &&
            task_check_timeout(&timeout, timeout_ticks)) {
            mqueue_unlock(mqueue);
            task_resume_all();
            return false;
        }

        /* Timeout has not expired */
        if (queue_is_empty(&(mqueue->Queue))) {
            task_add_current_to_event_list(&(mqueue->ReceiverList),
                                           timeout_ticks);
            mqueue_unlock(mqueue);
            if (!task_resume_all()) OCTOS_YIELD();
        } else {
            mqueue_unlock(mqueue);
            task_resume_all();
        }
    }
}

/**
 * @brief Send an item to a message queue from an ISR
 * @param mqueue: Pointer to the message queue
 * @param item: Pointer to the item to be sent
 * @param switch_required: 
 *      Pointer to a variable where the function will indicate if a context
 *      switch is required
 * @retval true If the item was sent successfully
 * @retval false Otherwise
 */
bool mqueue_send_from_isr(MsgQueue_t *mqueue, const void *item,
                          bool *const switch_required) {
    OCTOS_ASSERT_IF_INTERRUPT_PRIORITY_INVALID();

    uint32_t saved_intr_status = OCTOS_ENTER_CRITICAL_FROM_ISR();

    const bool success = queue_send(&mqueue->Queue, item);
    if (success) {
        const int8_t txlock = mqueue->TxLock;
        if (txlock == queueUNLOCKED) {
            const bool higher_priority_woken =
                    task_remove_highest_priority_from_event_list(
                            &(mqueue->ReceiverList));
            if (switch_required != NULL)
                *switch_required = higher_priority_woken;
        } else {
            mqueue_txlock_increment(mqueue, txlock);
        }
    }

    OCTOS_EXIT_CRITICAL_FROM_ISR(saved_intr_status);

    return success;
}

/**
 * @brief Receive a message from a message queue from an ISR
 * @note This function should only be called from an ISR
 * @param mqueue: Pointer to the message queue
 * @param buffer: 
 *      Pointer to the buffer where the received message will be stored
 * @param switch_required: 
 *      Pointer to a boolean to indicate if a context switch is required
 * @retval true If a message was successfully received
 * @retval false Otherwise
 */
bool mqueue_recv_from_isr(MsgQueue_t *mqueue, void *buffer,
                          bool *const switch_required) {
    OCTOS_ASSERT_IF_INTERRUPT_PRIORITY_INVALID();

    uint32_t saved_intr_status = OCTOS_ENTER_CRITICAL_FROM_ISR();

    const bool success = queue_recv(&mqueue->Queue, buffer);
    if (success) {
        const int8_t rxlock = mqueue->RxLock;
        if (rxlock == queueUNLOCKED) {
            const bool higher_priority_woken =
                    task_remove_highest_priority_from_event_list(
                            &(mqueue->SenderList));
            if (switch_required != NULL) {
                *switch_required = higher_priority_woken;
            }
        } else {
            mqueue_rxlock_increment(mqueue, rxlock);
        }
    }

    OCTOS_EXIT_CRITICAL_FROM_ISR(saved_intr_status);

    return success;
}
