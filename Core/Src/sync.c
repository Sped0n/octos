#include "sync.h"
#include "data.h"
#include "stm32f4xx.h" // IWYU pragma: keep
#include "task.h"
#include <stddef.h>

extern TCB_t *current_tcb;
extern List_t ready_list;

// Semaphore functions
void sema_init(Sema_t *sema, int32_t initial_count) {
  sema->Count = initial_count;
  list_init(&(sema->BlockedList));
}

void sema_acquire(Sema_t *sema) {
  __disable_irq();
  sema->Count--;
  if (sema->Count < 0) {
    list_remove(&(current_tcb->StateListItem));
    list_insert(&(sema->BlockedList), &(current_tcb->StateListItem));
    current_tcb->state = BLOCKED;
    task_yield();
  }
  __enable_irq();
}

void sema_release(Sema_t *sema) {
  __disable_irq();

  sema->Count++;

  if (sema->Count > 0) {
    __enable_irq();
    return;
  }

  if (sema->BlockedList.Length > 0) {
    TCB_t *temp = sema->BlockedList.End.Next->Owner;
    list_remove(sema->BlockedList.End.Next);
    list_insert(&ready_list, &(temp->StateListItem));
    temp->state = READY;
  }

  __enable_irq();
}

// Mutex functions
void mutex_init(Mutex_t *mutex) {
  mutex->Owner = NULL;
  list_init(&(mutex->BlockedList));
}

void mutex_acquire(Mutex_t *mutex) {
  __disable_irq();
  if (mutex->Owner == NULL) {
    mutex->Owner = current_tcb;
    __enable_irq();
    return;
  } else if (mutex->Owner == current_tcb) {
    // Already owns the mutex
    __enable_irq();
    return;
  } else {
    // Need to block
    list_remove(&(current_tcb->StateListItem));
    list_insert(&(mutex->BlockedList), &(current_tcb->StateListItem));
    current_tcb->state = BLOCKED;

    task_yield();

    __enable_irq();
  }
}

void mutex_release(Mutex_t *mutex) {
  if (mutex->Owner != current_tcb)
    return;

  __disable_irq();

  mutex->Owner = NULL;

  if (mutex->BlockedList.Length > 0) {
    TCB_t *temp = mutex->BlockedList.End.Next->Owner;
    list_remove(mutex->BlockedList.End.Next);
    list_insert(&ready_list, &(temp->StateListItem));
    temp->state = READY;
  }

  __enable_irq();
}
