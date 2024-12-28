#include <stddef.h>

#include "Arch/stm32f4xx/Inc/api.h"
#include "list.h"
#include "sync.h"
#include "task.h"

extern TCB_t *current_tcb;
extern List_t ready_list;

static void block_current_task(List_t *blocked_list) {
    list_insert(blocked_list, &(current_tcb->StateListItem));
    current_tcb->State = BLOCKED;
    task_yield();
}

static void unblock_one_task(List_t *blocked_list) {
    ListItem_t *head = list_head(blocked_list);
    list_remove(head);
    list_insert(&ready_list, head);
    TCB_t *head_tcb = head->Owner;
    head_tcb->State = READY;
}

void sema_init(Sema_t *sema, int32_t initial_count) {
    sema->Count = initial_count;
    list_init(&(sema->BlockedList));
}

void sema_acquire(Sema_t *sema) {
    MICROS_DISABLE_IRQ();

    sema->Count--;
    if (sema->Count < 0)
        block_current_task(&(sema->BlockedList));

    MICROS_ENABLE_IRQ();
}

void sema_release(Sema_t *sema) {
    MICROS_DISABLE_IRQ();

    sema->Count++;

    if (sema->Count > 0) {
        __enable_irq();
        return;
    }

    if (sema->BlockedList.Length > 0)
        unblock_one_task(&(sema->BlockedList));

    MICROS_ENABLE_IRQ();
}

// Mutex functions
void mutex_init(Mutex_t *mutex) {
    mutex->Owner = NULL;
    list_init(&(mutex->BlockedList));
}

void mutex_acquire(Mutex_t *mutex) {
    MICROS_DISABLE_IRQ();

    if (mutex->Owner == NULL)
        mutex->Owner = current_tcb;
    else if (mutex->Owner == current_tcb)
        ;
    else
        block_current_task(&(mutex->BlockedList));

    MICROS_ENABLE_IRQ();
}

void mutex_release(Mutex_t *mutex) {
    if (mutex->Owner != current_tcb)
        return;

    MICROS_DISABLE_IRQ();

    mutex->Owner = NULL;
    if (mutex->BlockedList.Length > 0)
        unblock_one_task(&(mutex->BlockedList));

    MICROS_ENABLE_IRQ();
}
