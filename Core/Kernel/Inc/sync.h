#ifndef __SYNC_H__
#define __SYNC_H__

#include <stddef.h>

#include "Arch/stm32f4xx/Inc/api.h"
#include "list.h"
#include "task.h"

typedef struct Sema {
    int32_t Count;
    List_t BlockedList;
} Sema_t;

typedef struct Mutex {
    TCB_t *Owner;
    List_t BlockedList;
} Mutex_t;

extern TCB_t *current_tcb;
extern List_t *ready_list;

OCTOS_INLINE static inline void block_current_task(List_t *blocked_list) {
    list_insert(blocked_list, &(current_tcb->StateListItem));
    task_yield();
}

OCTOS_INLINE static inline void unblock_one_task(List_t *blocked_list) {
    ListItem_t *head = list_head(blocked_list);
    list_remove(head);
    list_insert(ready_list, head);
}

OCTOS_INLINE inline void sema_init(Sema_t *sema, int32_t initial_count) {
    sema->Count = initial_count;
    list_init(&(sema->BlockedList));
}

OCTOS_INLINE static inline void sema_acquire(Sema_t *sema) {
    OCTOS_ENTER_CRITICAL();

    sema->Count--;
    if (sema->Count < 0)
        block_current_task(&(sema->BlockedList));

    OCTOS_EXIT_CRITICAL();
}

OCTOS_INLINE static inline void sema_release(Sema_t *sema) {
    OCTOS_ENTER_CRITICAL();

    sema->Count++;

    if (sema->Count > 0) {
        __enable_irq();
        return;
    }

    if (sema->BlockedList.Length > 0)
        unblock_one_task(&(sema->BlockedList));

    OCTOS_EXIT_CRITICAL();
}

OCTOS_INLINE inline void mutex_init(Mutex_t *mutex) {
    mutex->Owner = NULL;
    list_init(&(mutex->BlockedList));
}

OCTOS_INLINE static inline void mutex_acquire(Mutex_t *mutex) {
    OCTOS_ENTER_CRITICAL();

    if (mutex->Owner == NULL)
        mutex->Owner = current_tcb;
    else if (mutex->Owner == current_tcb)
        ;
    else
        block_current_task(&(mutex->BlockedList));

    OCTOS_EXIT_CRITICAL();
}

OCTOS_INLINE static inline void mutex_release(Mutex_t *mutex) {
    if (mutex->Owner != current_tcb)
        return;

    OCTOS_EXIT_CRITICAL();

    mutex->Owner = NULL;
    if (mutex->BlockedList.Length > 0)
        unblock_one_task(&(mutex->BlockedList));

    OCTOS_EXIT_CRITICAL();
}

#endif
