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
    TaskHandle_t Owner; /*!< Pointer to the task that owns the mutex */
    SyncCore_t Core;    /*!< Synchronization core */
} Mutex_t;

/**
 * @brief Condition variable structure
 */
typedef struct Cond {
    SyncCore_t Core; /*!< Synchronization core for the condition variable */
} Cond_t;

/**
 * @brief Barrier structure
 */
typedef struct Barrier {
    uint16_t Parties; /*!< Number of parties required to release the barrier */
    uint16_t Count;   /*!< Current count of parties waiting at the barrier */
    SyncCore_t Core;  /*!< Condition variable used for synchronization */
} Barrier_t;

/**
 * @brief Event structure definition
 */
typedef struct Event {
    bool Flag;       /*!< Flag indicating whether the event is set or not */
    SyncCore_t Core; /*!< Synchronization core for managing blocked tasks */
} Event_t;

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
/* Condtion ------------------------------------------------------------------*/
void cond_init(Cond_t *cond);
bool cond_wait(Cond_t *cond, uint32_t timeout_ticks);
void cond_notify(Cond_t *cond);
void cond_notify_all(Cond_t *cond);
void cond_notify_from_isr(Cond_t *cond, bool *const switch_required);
void cond_notify_all_from_isr(Cond_t *cond, bool *const switch_required);
/* Barrier -------------------------------------------------------------------*/
void barrier_init(Barrier_t *barrier, uint32_t parties);
bool barrier_wait(Barrier_t *barrier, uint32_t timeout_ticks);
void barrier_reset(Barrier_t *barrier);
void barrier_reset_from_isr(Barrier_t *barrier, bool *const switch_required);
/* Event ---------------------------------------------------------------------*/
void event_init(Event_t *event);
bool event_is_set(Event_t *event);
void event_set(Event_t *event);
void event_clear(Event_t *event);
bool event_wait(Event_t *event, uint32_t timeout_ticks);
bool event_is_set_from_isr(Event_t *event);
void event_set_from_isr(Event_t *event, bool *const switch_required);
void event_clear_from_isr(Event_t *event);

#endif
