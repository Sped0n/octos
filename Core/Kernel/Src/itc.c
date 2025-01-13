#include <stdint.h>

#include "Arch/stm32f4xx/Inc/api.h"
#include "itc.h"
#include "list.h"
#include "task.h"
#include "utils.h"

/**
  * @brief Wakes up first task in waiting list if any exists
  * @param event_list Pointer to list of waiting tasks
  * @retval None
  */
static void try_wake_waiting_task(List_t *event_list) {
    if (event_list->Length == 0) return;

    ListItem_t *item = list_tail(event_list);
    list_remove(item);

    TCB_t *tcb = item->Owner;
    item = &(tcb->StateListItem);
    if (task_status(tcb) != SUSPENDED) {
        list_item_set_value(item, tcb->Priority);
    }
    list_remove(item);
    task_add_to_ready_list(item->Owner);
}

// TODO: add try_wake_waiting_task_from_isr

/**
  * @brief Initializes a message queue
  * @param mqueue Pointer to message queue to initialize
  * @param buffer Pointer to buffer that will hold queue data
  * @param item_size Size of each item in the queue
  * @param max_size Maximum number of items in the queue
  * @retval None
  */
void msg_queue_init(MsgQueue_t *mqueue, void *buffer, size_t item_size,
                    size_t max_size) {
    queue_init(&mqueue->Queue, buffer, item_size, max_size);
    list_init(&mqueue->SenderList);
    list_init(&mqueue->ReceiverList);
}

/**
  * @brief Sends a message to the queue
  * @param mqueue Pointer to message queue
  * @param item Pointer to item to send
  * @param timeout_ticks Maximum time to wait if queue is full (0 for infinite)
  * @retval true if message was sent successfully, false if timed out
  */
bool msg_queue_send(MsgQueue_t *mqueue, const void *item,
                    uint32_t timeout_ticks) {
    Timeout_t timeout;

    OCTOS_ENTER_CRITICAL();

    if (queue_send(&mqueue->Queue, item)) {
        try_wake_waiting_task(&mqueue->ReceiverList);
        OCTOS_EXIT_CRITICAL();
        return true;
    }

    list_insert(&(mqueue->SenderList), &(current_tcb->EventListItem));
    if (timeout_ticks == 0) {
        task_suspend(current_tcb);
    } else {
        task_set_timeout(&timeout);
        task_delay(timeout_ticks);
    }

    OCTOS_EXIT_CRITICAL();

    task_yield();

    OCTOS_ENTER_CRITICAL();

    bool success = false;

    if (timeout_ticks == 0 || !task_check_timeout(&timeout, timeout_ticks)) {
        success = queue_send(&mqueue->Queue, item);
        if (success) { try_wake_waiting_task(&mqueue->ReceiverList); }
    }

    OCTOS_EXIT_CRITICAL();
    return success;
}

/**
  * @brief Receives a message from the queue
  * @param mqueue Pointer to message queue
  * @param buffer Pointer where received item will be stored
  * @param timeout_ticks Maximum time to wait if queue is empty (0 for infinite)
  * @retval true if message was received successfully, false if timed out
  */
bool msg_queue_recv(MsgQueue_t *mqueue, void *buffer, uint32_t timeout_ticks) {
    Timeout_t timeout;

    OCTOS_ENTER_CRITICAL();

    if (queue_recv(&mqueue->Queue, buffer)) {
        try_wake_waiting_task(&mqueue->SenderList);
        OCTOS_EXIT_CRITICAL();
        return true;
    }

    list_insert(&(mqueue->ReceiverList), &(current_tcb->EventListItem));
    if (timeout_ticks == 0) {
        task_suspend(current_tcb);
    } else {
        task_set_timeout(&timeout);
        task_delay(timeout_ticks);
    }

    OCTOS_EXIT_CRITICAL();

    task_yield();

    OCTOS_ENTER_CRITICAL();
    bool success = false;

    if (timeout_ticks == 0 || !task_check_timeout(&timeout, timeout_ticks)) {
        success = queue_recv(&mqueue->Queue, buffer);
        if (success) try_wake_waiting_task(&mqueue->SenderList);
    }

    OCTOS_EXIT_CRITICAL();
    return success;
}

/**
  * @brief Sends a message to the queue from ISR context
  * @param mqueue Pointer to message queue
  * @param item Pointer to item to send
  * @retval true if message was sent successfully, false if queue is full
  * @note This function must only be called from ISR context
  */
bool msg_queue_send_from_isr(MsgQueue_t *mqueue, const void *item) {
    OCTOS_ASSERT_IF_INTERRUPT_PRIORITY_INVALID();

    uint32_t saved_intr_status = OCTOS_ENTER_CRITICAL_FROM_ISR();

    bool success = queue_send(&mqueue->Queue, item);
    if (success) try_wake_waiting_task(&mqueue->ReceiverList);

    OCTOS_EXIT_CRITICAL_FROM_ISR(saved_intr_status);

    return success;
}

/**
  * @brief Receives a message from the queue from ISR context
  * @param mqueue Pointer to message queue
  * @param buffer Pointer where received item will be stored
  * @retval true if message was received successfully, false if queue is empty
  * @note This function must only be called from ISR context
  */
bool msg_queue_recv_from_isr(MsgQueue_t *mqueue, void *buffer) {
    OCTOS_ASSERT_IF_INTERRUPT_PRIORITY_INVALID();

    uint32_t saved_intr_status = OCTOS_ENTER_CRITICAL_FROM_ISR();

    bool success = queue_recv(&mqueue->Queue, buffer);
    if (success) try_wake_waiting_task(&mqueue->SenderList);

    OCTOS_EXIT_CRITICAL_FROM_ISR(saved_intr_status);

    return success;
}
