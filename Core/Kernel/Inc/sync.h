#ifndef __SYNC_H__
#define __SYNC_H__

#include <stddef.h>
#include <stdint.h>

#include "list.h"
#include "task.h"

#define syncUNLOCKED ((int8_t) -1)
#define syncLOCKED_UNMODIFIED ((int8_t) 0)

/**
 * @brief Synchronization Core structure definition
 */
typedef struct SyncCore {
    List_t BlockedList; /*!< List of blocked tasks */
    int8_t Lock;        /*!< Lock state of the synchronization object */
} SyncCore_t;

/**
 * @brief Semaphore structure definition
 */
typedef struct Sema {
    int32_t Count;   /*!< Semaphore count */
    SyncCore_t Core; /*!< Synchronization core */
} Sema_t;

/**
 * @brief Mutex structure definition
 */
typedef struct Mutex {
    TCB_t *Owner;    /*!< Pointer to the task that owns the mutex */
    SyncCore_t Core; /*!< Synchronization core */
} Mutex_t;

/* Semaphore -----------------------------------------------------------------*/
void sema_init(Sema_t *sema, int32_t initial_count);
bool sema_acquire(Sema_t *sema, uint32_t timeout_ticks);
void sema_release(Sema_t *sema);
bool sema_acquire_from_isr(Sema_t *sema);
void sema_release_from_isr(Sema_t *sema, bool *const switch_required);
/* Mutex ---------------------------------------------------------------------*/
void mutex_init(Mutex_t *mutex);
bool mutex_acquire(Mutex_t *mutex, uint32_t timeout_ticks);
bool mutex_release(Mutex_t *mutex);

#endif
