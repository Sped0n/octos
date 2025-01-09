#ifndef __SYNC_H__
#define __SYNC_H__

#include <stddef.h>
#include <stdint.h>

#include "Arch/stm32f4xx/Inc/api.h"
#include "list.h"
#include "task.h"
#include "tcb.h"

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

/* helper functions */

OCTOS_INLINE static inline void block_current_task(List_t *blocked_list) {
    list_insert(blocked_list, &(current_tcb->StateListItem));
    task_yield();
}

OCTOS_INLINE static inline void unblock_one_task(List_t *blocked_list) {
    ListItem_t *tail = list_tail(blocked_list);
    list_remove(tail);
    list_insert(ready_list, tail);
}


/* semaphore */

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
        OCTOS_EXIT_CRITICAL();
        return;
    }

    if (sema->BlockedList.Length > 0)
        unblock_one_task(&(sema->BlockedList));

    OCTOS_EXIT_CRITICAL();
}

/* mutex */

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
    else {
        block_current_task(&(mutex->BlockedList));
        if (current_tcb->Priority > mutex->Owner->Priority) {
            mutex->Owner->BasePriority = current_tcb->Priority;       // update priority
            mutex->Owner->StateListItem.Value = current_tcb->Priority;// reset priority value
        }
    }

    OCTOS_EXIT_CRITICAL();
}

OCTOS_INLINE static inline void mutex_release(Mutex_t *mutex) {
    if (mutex->Owner != current_tcb)
        return;

    OCTOS_EXIT_CRITICAL();

    if (mutex->BlockedList.Length > 0) {
        mutex->Owner = list_tail(&(mutex->BlockedList))->Owner;
        unblock_one_task(&(mutex->BlockedList));
    } else
        mutex->Owner = NULL;

    tcb_recover_priority(current_tcb);

    OCTOS_EXIT_CRITICAL();
}

#endif
