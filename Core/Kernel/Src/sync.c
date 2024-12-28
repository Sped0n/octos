#include <stddef.h>

#include "Arch/stm32f4xx/Inc/api.h"
#include "list.h"
#include "sync.h"
#include "task.h"

extern TCB_t *current_tcb;
extern List_t ready_list;

static void block_current_task(List_t *blocked_list) {
    list_insert(blocked_list, &(current_tcb->StateListItem));
    task_yield();
}

static void unblock_one_task(List_t *blocked_list) {
    ListItem_t *head = list_head(blocked_list);
    list_remove(head);
    list_insert(&ready_list, head);
}

void sema_init(Sema_t *sema, int32_t initial_count) {
    sema->Count = initial_count;
    list_init(&(sema->BlockedList));
}

void sema_acquire(Sema_t *sema) {
    ENTER_CRITICAL();

    sema->Count--;
    if (sema->Count < 0)
        block_current_task(&(sema->BlockedList));

    EXIT_CRITICAL();
}

void sema_release(Sema_t *sema) {
    ENTER_CRITICAL();

    sema->Count++;

    if (sema->Count > 0) {
        __enable_irq();
        return;
    }

    if (sema->BlockedList.Length > 0)
        unblock_one_task(&(sema->BlockedList));

    EXIT_CRITICAL();
}

// Mutex functions
void mutex_init(Mutex_t *mutex) {
    mutex->Owner = NULL;
    list_init(&(mutex->BlockedList));
}

void mutex_acquire(Mutex_t *mutex) {
    ENTER_CRITICAL();

    if (mutex->Owner == NULL)
        mutex->Owner = current_tcb;
    else if (mutex->Owner == current_tcb)
        ;
    else
        block_current_task(&(mutex->BlockedList));

    EXIT_CRITICAL();
}

void mutex_release(Mutex_t *mutex) {
    if (mutex->Owner != current_tcb)
        return;

    EXIT_CRITICAL();

    mutex->Owner = NULL;
    if (mutex->BlockedList.Length > 0)
        unblock_one_task(&(mutex->BlockedList));

    EXIT_CRITICAL();
}
