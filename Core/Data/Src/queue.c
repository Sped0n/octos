#include <string.h>

#include "list.h"
#include "queue.h"
#include "sync.h"
#include "task.h"
#include "tcb.h"

extern TCB_t *current_tcb;

static bool copy_to_queue(Queue_t *queue, const void *item) {
    if (queue_is_full(queue)) {
        return false;
    }

    uint8_t *dest = queue->Buffer + (queue->Head * queue->ItemSize);
    memcpy(dest, item, queue->ItemSize);

    queue->Head++;
    if (queue->Head >= queue->Length) {
        queue->Head = 0;
    }

    queue->MessagesWaiting++;
    return true;
}

static bool copy_from_queue(Queue_t *queue, void *buffer) {
    if (queue_is_empty(queue)) {
        return false;
    }

    uint8_t *src = queue->Buffer + (queue->Tail * queue->ItemSize);
    memcpy(buffer, src, queue->ItemSize);

    queue->Tail++;
    if (queue->Tail >= queue->Length) {
        queue->Tail = 0;
    }

    queue->MessagesWaiting--;
    return true;
}

static void try_wake_waiting_task(List_t *event_list) {
    if (event_list->Length == 0) return;

    ListItem_t *item = list_tail(event_list);
    list_remove(item);

    TCB_t *tcb = item->Owner;
    // now item is StateListItem
    item = &(tcb->StateListItem);
    // if a tcb status is not SUSPENDED, then it has a timeout(sleeping), and we have to reset StateListItem
    // value from wake tick to priority
    if (tcb_status(tcb) != SUSPENDED) list_item_set_value(item, tcb->BasePriority);
    list_remove(item);
    list_insert_end(pending_ready_list, item);
}

void queue_init(Queue_t *queue, void *buffer, size_t item_size, size_t length) {
    queue->Buffer = buffer;
    queue->ItemSize = item_size;
    queue->Length = length;
    queue->MessagesWaiting = 0;
    queue->Head = 0;
    queue->Tail = 0;

    // Initialize task lists
    list_init(&queue->SenderList);
    list_init(&queue->ReceiverList);
}

bool queue_send(Queue_t *queue, const void *item, uint32_t timeout_ticks) {
    OCTOS_ENTER_CRITICAL();

    // Try to copy immediately if possible
    if (copy_to_queue(queue, item)) {
        try_wake_waiting_task(&queue->ReceiverList);
        OCTOS_EXIT_CRITICAL();
        return true;
    }

    list_insert(&(queue->SenderList), &(current_tcb->EventListItem));
    if (timeout_ticks == 0) {
        task_suspend();
    } else {
        task_delay(timeout_ticks);
    }

    OCTOS_EXIT_CRITICAL();

    task_yield();

    OCTOS_ENTER_CRITICAL();

    bool success = false;

    if (is_queue_ops_timeout()) {
        list_remove(&(current_tcb->EventListItem));
    } else {
        // try to send again
        success = copy_to_queue(queue, item);
        if (success)
            try_wake_waiting_task(&queue->ReceiverList);
    }

    OCTOS_EXIT_CRITICAL();
    return success;
}

bool queue_recv(Queue_t *queue, void *buffer, uint32_t timeout_ticks) {
    OCTOS_ENTER_CRITICAL();

    // Try to copy immediately if possible
    if (copy_from_queue(queue, buffer)) {
        try_wake_waiting_task(&(queue->SenderList));
        OCTOS_EXIT_CRITICAL();
        return true;
    }

    list_insert(&(queue->ReceiverList), &(current_tcb->EventListItem));
    if (timeout_ticks == 0) {
        task_suspend();
    } else {
        task_delay(timeout_ticks);
    }

    OCTOS_EXIT_CRITICAL();

    task_yield();

    OCTOS_ENTER_CRITICAL();

    bool success = false;

    if (is_queue_ops_timeout()) {
        list_remove(&(current_tcb->EventListItem));
    } else {
        // Try to receive again
        success = copy_from_queue(queue, buffer);
        if (success)
            try_wake_waiting_task(&(queue->SenderList));
    }

    OCTOS_EXIT_CRITICAL();
    return success;
}
