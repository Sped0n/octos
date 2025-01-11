#ifndef __SYNC_H__
#define __SYNC_H__

#include <stddef.h>
#include <stdint.h>

#include "Arch/stm32f4xx/Inc/api.h"
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

extern TCB_t *current_tcb;
extern List_t *pending_ready_list;


/* Helper Functions ----------------------------------------------------------*/

/**
  * @brief Blocks the current task by adding it to the specified blocked list
  * @param blocked_list Pointer to the list where task will be blocked
  * @retval None
  */
OCTOS_INLINE static inline void block_current_task(List_t *blocked_list) {
    list_insert(blocked_list, &(current_tcb->StateListItem));
}

/**
  * @brief Unblocks one task from a blocked list and moves it to the pending ready list
  * @param blocked_list Pointer to the list containing blocked tasks
  * @retval Root priority of the unblocked task
  */
OCTOS_INLINE static inline uint8_t unblock_one_task(List_t *blocked_list) {
    ListItem_t *tail = list_tail(blocked_list);
    list_remove(tail);
    list_insert_end(pending_ready_list, tail);
    return ((TCB_t *) tail->Owner)->RootPriority;
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
    if (sema->Count < 0)
        block_current_task(&(sema->BlockedList));

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
        higher_priority_task_woken = unblock_one_task(&(sema->BlockedList)) > current_tcb->RootPriority;
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
    bool higher_priority_task_woken = false;

    uint32_t saved_intr_status = OCTOS_ENTER_CRITICAL_FROM_ISR();

    sema->Count++;

    if (sema->Count > 0) {
        OCTOS_EXIT_CRITICAL_FROM_ISR(saved_intr_status);
        return;
    }

    if (sema->BlockedList.Length > 0) {
        higher_priority_task_woken = unblock_one_task(&(sema->BlockedList)) > current_tcb->RootPriority;
    }

    OCTOS_EXIT_CRITICAL_FROM_ISR(saved_intr_status);

    task_yield_from_isr(higher_priority_task_woken);
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

/**
  * @brief Acquire (lock) a mutex
  * @note This function must be called from task context
  * @note Implements priority inheritance if needed
  * @param mutex Pointer to mutex structure
  * @retval None
  */
OCTOS_INLINE static inline void mutex_acquire(Mutex_t *mutex) {
    bool need_yield = false;

    OCTOS_ENTER_CRITICAL();

    if (mutex->Owner == NULL)
        mutex->Owner = current_tcb;
    else if (mutex->Owner == current_tcb)
        ;
    else {
        need_yield = true;
        block_current_task(&(mutex->BlockedList));
        if (current_tcb->RootPriority > mutex->Owner->RootPriority) {
            /* Priority inheritance */
            mutex->Owner->Priority = current_tcb->RootPriority;
            /* Update current priority value */
            mutex->Owner->StateListItem.Value = current_tcb->RootPriority;
        }
    }

    OCTOS_EXIT_CRITICAL();

    if (need_yield) task_yield();
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
        higher_priority_task_woken = unblock_one_task(&(mutex->BlockedList)) > current_tcb->RootPriority;
    } else {
        mutex->Owner = NULL;
    }

    tcb_recover_priority(current_tcb);

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
    bool higher_priority_task_woken = false;

    uint32_t saved_intr_status = OCTOS_ENTER_CRITICAL_FROM_ISR();

    if (mutex->Owner != current_tcb) {
        OCTOS_EXIT_CRITICAL_FROM_ISR(saved_intr_status);
        return false;
    }

    if (mutex->BlockedList.Length > 0) {
        mutex->Owner = list_tail(&(mutex->BlockedList))->Owner;
        higher_priority_task_woken = unblock_one_task(&(mutex->BlockedList)) > current_tcb->RootPriority;
    } else {
        mutex->Owner = NULL;
    }

    tcb_recover_priority(current_tcb);

    OCTOS_EXIT_CRITICAL_FROM_ISR(saved_intr_status);

    task_yield_from_isr(higher_priority_task_woken);

    return true;
}

#endif
