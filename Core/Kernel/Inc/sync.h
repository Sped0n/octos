#ifndef __SYNC_H__
#define __SYNC_H__

#include <stdint.h>

#include "list.h"
#include "tcb.h"

typedef struct Sema {
    int32_t Count;
    List_t BlockedList;
} Sema_t;

typedef struct Mutex {
    TCB_t *Owner;
    List_t BlockedList;
} Mutex_t;

void sema_init(Sema_t *sem, int32_t initial_count);
void sema_acquire(Sema_t *sem);
void sema_release(Sema_t *sem);

void mutex_init(Mutex_t *mutex);
void mutex_acquire(Mutex_t *mutex);
void mutex_release(Mutex_t *mutex);

#endif
