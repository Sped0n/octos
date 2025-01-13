#ifndef __SYNC_H__
#define __SYNC_H__

#include <stddef.h>
#include <stdint.h>

#include "Arch/stm32f4xx/Inc/api.h"
#include "attr.h"
#include "list.h"
#include "task.h"
#include "tcb.h"

/**
  * @brief Semaphore structure definition
  */
typedef struct Sema {
    int32_t Count;      /*!< Current count of the semaphore */
    List_t BlockedList; /*!< List of tasks blocked waiting for this semaphore */
} Sema_t;

/**
  * @brief Mutex structure definition
  */
typedef struct Mutex {
    TCB_t *Owner;       /*!< Current owner task of the mutex */
    List_t BlockedList; /*!< List of tasks blocked waiting for this mutex */
} Mutex_t;


/* Helper Functions ----------------------------------------------------------*/

OCTOS_INLINE static inline void block_current_task(List_t *blocked_list) {
    OCTOS_ENTER_CRITICAL();

    TCB_t *tcb = task_get_current();
    list_insert(blocked_list, &(tcb->EventListItem));
    task_suspend(tcb);

    OCTOS_EXIT_CRITICAL();
}

OCTOS_INLINE static inline uint8_t unblock_one_task(List_t *blocked_list) {
    OCTOS_ENTER_CRITICAL();

    ListItem_t *tail = list_tail(blocked_list);
    list_remove(tail);
    TCB_t *tcb = tail->Owner;
    list_remove(&(tcb->StateListItem));
    task_add_to_ready_list(tcb);

    OCTOS_EXIT_CRITICAL();

    return tcb->RootPriority;
}


/* Semaphore -----------------------------------------------------------------*/

/**
  * @brief Initialize a semaphore with specified initial count
  * @param sema Pointer to semaphore structure
  * @param initial_count Initial value for semaphore counter
  * @retval None
  */
OCTOS_INLINE inline void sema_init(Sema_t *sema, int32_t initial_count) {
    sema->Count = initial_count;
    list_init(&(sema->BlockedList));
}

/**
  * @brief Acquire (take) a semaphore
  * @note Will block the current task if semaphore count becomes negative
  * @note This function must be called from task context
  * @param sema Pointer to semaphore structure
  * @retval None
  */
OCTOS_INLINE static inline void sema_acquire(Sema_t *sema) {
    OCTOS_ENTER_CRITICAL();

    sema->Count--;
    if (sema->Count < 0) block_current_task(&(sema->BlockedList));

    OCTOS_EXIT_CRITICAL();

    if (sema->Count < 0) task_yield();
}

/**
  * @brief Releases a semaphore, incrementing its count and potentially unblocking a waiting task
  * @note This function must be called from task context
  * @note May result in a context switch if a higher priority task is unblocked
  * @param sema Pointer to the semaphore to be released
  * @retval None
  */
OCTOS_INLINE static inline void sema_release(Sema_t *sema) {
    bool higher_priority_task_woken = false;

    OCTOS_ENTER_CRITICAL();

    sema->Count++;

    if (sema->Count > 0) {
        OCTOS_EXIT_CRITICAL();
        return;
    }

    if (sema->BlockedList.Length > 0) {
        higher_priority_task_woken = unblock_one_task(&(sema->BlockedList)) >
                                     current_tcb->RootPriority;
    }

    OCTOS_EXIT_CRITICAL();

    if (higher_priority_task_woken) task_yield();
}

/**
  * @brief Attempts to acquire a semaphore from an ISR context
  * @param sema Pointer to the semaphore to be acquired
  * @retval true if semaphore was successfully acquired, false otherwise
  */
OCTOS_INLINE static inline bool sema_acquire_from_isr(Sema_t *sema) {
    OCTOS_ASSERT_IF_INTERRUPT_PRIORITY_INVALID();

    int32_t saved_intr_status = OCTOS_ENTER_CRITICAL_FROM_ISR();
    if (sema->Count <= 0) {
        OCTOS_EXIT_CRITICAL_FROM_ISR(saved_intr_status);
        return false;
    }

    sema->Count--;
    OCTOS_EXIT_CRITICAL_FROM_ISR(saved_intr_status);
    return true;
}

/**
  * @brief Releases a semaphore from an ISR context
  * @note May trigger a context switch if a higher priority task is unblocked
  * @param sema Pointer to the semaphore to be released
  * @retval None
  */
OCTOS_INLINE static inline void sema_release_from_isr(Sema_t *sema) {
    OCTOS_ASSERT_IF_INTERRUPT_PRIORITY_INVALID();

    bool switch_required = false;

    uint32_t saved_intr_status = OCTOS_ENTER_CRITICAL_FROM_ISR();

    sema->Count++;

    if (sema->Count > 0) {
        OCTOS_EXIT_CRITICAL_FROM_ISR(saved_intr_status);
        return;
    }

    if (sema->BlockedList.Length > 0) {
        switch_required = unblock_one_task(&(sema->BlockedList)) >
                          current_tcb->RootPriority;
    }

    OCTOS_EXIT_CRITICAL_FROM_ISR(saved_intr_status);

    task_yield_from_isr(switch_required);
}

/* Mutex ---------------------------------------------------------------------*/

/**
  * @brief Initialize a mutex
  * @param mutex Pointer to mutex structure
  * @retval None
  */
OCTOS_INLINE inline void mutex_init(Mutex_t *mutex) {
    mutex->Owner = NULL;
    list_init(&(mutex->BlockedList));
}

OCTOS_INLINE static inline void mutex_acquire(Mutex_t *mutex) {
    bool switch_required = false;

    OCTOS_ENTER_CRITICAL();

    if (mutex->Owner == NULL) mutex->Owner = current_tcb;
    else if (mutex->Owner == current_tcb)
        ;
    else {
        switch_required = true;
        block_current_task(&(mutex->BlockedList));
    }

    OCTOS_EXIT_CRITICAL();

    if (switch_required) task_yield();
}

/**
  * @brief Release a mutex owned by current task
  * @note This function must be called from task context
  * @param mutex Pointer to the mutex to be released
  * @retval true if mutex was successfully released
  * @retval false if mutex is not owned by current task
  */
OCTOS_INLINE static inline bool mutex_release(Mutex_t *mutex) {
    bool higher_priority_task_woken = false;

    OCTOS_ENTER_CRITICAL();

    if (mutex->Owner != current_tcb) {
        OCTOS_EXIT_CRITICAL();
        return false;
    }

    if (mutex->BlockedList.Length > 0) {
        mutex->Owner = list_tail(&(mutex->BlockedList))->Owner;
        higher_priority_task_woken = unblock_one_task(&(mutex->BlockedList)) >
                                     current_tcb->RootPriority;
    } else {
        mutex->Owner = NULL;
    }

    OCTOS_EXIT_CRITICAL();

    if (higher_priority_task_woken) task_yield();

    return true;
}

/**
  * @brief Attempt to acquire a mutex from an ISR context
  * @param mutex Pointer to the mutex to be acquired
  * @retval true if mutex was successfully acquired
  * @retval false if mutex is already owned by another task
  */
OCTOS_INLINE static inline bool mutex_acquire_from_isr(Mutex_t *mutex) {
    OCTOS_ASSERT_IF_INTERRUPT_PRIORITY_INVALID();

    uint32_t saved_intr_status = OCTOS_ENTER_CRITICAL_FROM_ISR();

    if (mutex->Owner != NULL) {
        OCTOS_EXIT_CRITICAL_FROM_ISR(saved_intr_status);
        return false;
    }
    mutex->Owner = current_tcb;

    OCTOS_EXIT_CRITICAL_FROM_ISR(saved_intr_status);

    return true;
}

/**
  * @brief Release a mutex owned by current task from an ISR context
  * @param mutex Pointer to the mutex to be released
  * @retval true if mutex was successfully released
  * @retval false if mutex is not owned by current task
  */
OCTOS_INLINE static inline bool mutex_release_from_isr(Mutex_t *mutex) {
    OCTOS_ASSERT_IF_INTERRUPT_PRIORITY_INVALID();

    bool switch_required = false;

    uint32_t saved_intr_status = OCTOS_ENTER_CRITICAL_FROM_ISR();

    if (mutex->Owner != current_tcb) {
        OCTOS_EXIT_CRITICAL_FROM_ISR(saved_intr_status);
        return false;
    }

    if (mutex->BlockedList.Length > 0) {
        mutex->Owner = list_tail(&(mutex->BlockedList))->Owner;
        switch_required = unblock_one_task(&(mutex->BlockedList)) >
                          current_tcb->RootPriority;
    } else {
        mutex->Owner = NULL;
    }

    OCTOS_EXIT_CRITICAL_FROM_ISR(saved_intr_status);

    task_yield_from_isr(switch_required);

    return true;
}

#endif
