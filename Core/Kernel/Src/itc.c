#include <string.h>

#include "itc.h"
#include "sync.h"
#include "task.h"
#include "tcb.h"


extern TCB_t *current_tcb;

static void try_wake_waiting_task(List_t *event_list) {
    if (event_list->Length == 0) return;

    ListItem_t *item = list_tail(event_list);
    list_remove(item);

    TCB_t *tcb = item->Owner;
    item = &(tcb->StateListItem);
    if (tcb_status(tcb) != SUSPENDED) {
        list_item_set_value(item, tcb->BasePriority);
    }
    list_remove(item);
    list_insert_end(pending_ready_list, item);
}

static bool is_mqueue_ops_timeout(void) {
    return current_tcb->EventListItem.Parent != NULL;
}

void msg_queue_init(MsgQueue_t *mqueue, void *buffer, size_t item_size, size_t max_size) {
    queue_init(&mqueue->Queue, buffer, item_size, max_size);
    list_init(&mqueue->SenderList);
    list_init(&mqueue->ReceiverList);
}

bool msg_queue_send(MsgQueue_t *mqueue, const void *item, uint32_t timeout_ticks) {
    OCTOS_ENTER_CRITICAL();

    if (queue_send_from_isr(&mqueue->Queue, item)) {
        try_wake_waiting_task(&mqueue->ReceiverList);
        OCTOS_EXIT_CRITICAL();
        return true;
    }

    list_insert(&(mqueue->SenderList), &(current_tcb->EventListItem));
    if (timeout_ticks == 0) {
        task_suspend();
    } else {
        task_delay(timeout_ticks);
    }

    OCTOS_EXIT_CRITICAL();

    task_yield();

    OCTOS_ENTER_CRITICAL();

    bool success = false;

    if (is_mqueue_ops_timeout()) {
        list_remove(&(current_tcb->EventListItem));
    } else {
        success = queue_send_from_isr(&mqueue->Queue, item);
        if (success) {
            try_wake_waiting_task(&mqueue->ReceiverList);
        }
    }

    OCTOS_EXIT_CRITICAL();
    return success;
}

bool msg_queue_recv(MsgQueue_t *mqueue, void *buffer, uint32_t timeout_ticks) {
    OCTOS_ENTER_CRITICAL();

    if (queue_recv_from_isr(&mqueue->Queue, buffer)) {
        try_wake_waiting_task(&mqueue->SenderList);
        OCTOS_EXIT_CRITICAL();
        return true;
    }

    list_insert(&(mqueue->ReceiverList), &(current_tcb->EventListItem));
    if (timeout_ticks == 0) {
        task_suspend();
    } else {
        task_delay(timeout_ticks);
    }

    OCTOS_EXIT_CRITICAL();

    task_yield();

    OCTOS_ENTER_CRITICAL();
    bool success = false;

    if (is_mqueue_ops_timeout()) {
        list_remove(&(current_tcb->EventListItem));
    } else {
        success = queue_recv_from_isr(&mqueue->Queue, buffer);
        if (success) {
            try_wake_waiting_task(&mqueue->SenderList);
        }
    }

    OCTOS_EXIT_CRITICAL();
    return success;
}
