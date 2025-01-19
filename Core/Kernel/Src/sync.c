#include <stdint.h>

#include "Arch/stm32f4xx/Inc/api.h"
#include "list.h"
#include "sync.h"
#include "task.h"
#include "tcb.h"

/**
 * @brief Initializes a synchronization core
 * @param core Pointer to the synchronization core to be initialized
 * @return None
 */
OCTOS_INLINE static inline void sync_core_init(SyncCore_t *core) {
    list_init(&(core->BlockedList));
    core->Lock = syncUNLOCKED;
}

/**
 * @brief Locks a synchronization core
 * @note Must call within scheduler suspension
 * @param core Pointer to the synchronization core to be locked
 * @return None
 */
OCTOS_INLINE static inline void sync_lock(SyncCore_t *core) {
    OCTOS_ENTER_CRITICAL();

    if (core->Lock == syncUNLOCKED) core->Lock = syncLOCKED_UNMODIFIED;

    OCTOS_EXIT_CRITICAL();
}

/**
 * @brief Increments the lock counter of a synchronization core
 * @param core Pointer to the synchronization core
 * @param lock Lock value to be incremented
 * @return None
 */
OCTOS_INLINE static inline void sync_lock_increment(SyncCore_t *core,
                                                    int8_t lock) {
    const uint8_t current_number_of_tasks = task_get_number_of_tasks();
    /* Cap lock counter at current_number_of_tasks
     *
     * But why?
     * Beacause lock counter is actually used to keep track of task need to 
     * unblock in msg_queue_unlock, since 
     * numof(blocked task) < numof(current number of tasks), it is not 
     * necessary to let lock counter goes beyond current_number_of_tasks
     * */
    if ((uint8_t) lock < current_number_of_tasks) {
        OCTOS_ASSERT(lock < INT8_MAX);
        core->Lock = lock + 1;
    }
}

/**
 * @brief Unlocks a synchronization core
 * @note Must call within scheduler suspension
 * @param core Pointer to the synchronization core to be unlocked
 * @return None
 */
static void sync_unlock(SyncCore_t *core) {
    OCTOS_ENTER_CRITICAL();

    int8_t lock = core->Lock;
    List_t *const blocked_list = &(core->BlockedList);
    while (lock > syncLOCKED_UNMODIFIED) {
        if (blocked_list->Length > 0) {
            task_remove_highest_priority_from_event_list(blocked_list);
        } else {
            break;
        }
        lock--;
    }
    core->Lock = syncUNLOCKED;

    OCTOS_EXIT_CRITICAL();
}

/* Semaphore -----------------------------------------------------------------*/

/**
 * @brief Initialize a semaphore with specified initial count
 * @param sema Pointer to semaphore structure
 * @param initial_count Initial value for semaphore counter
 * @retval None
 */
void sema_init(Sema_t *sema, int32_t initial_count) {
    sema->Count = initial_count;
    sync_core_init(&(sema->Core));
}

/**
 * @brief Acquires a semaphore
 * @param sema Pointer to the semaphore to be acquired
 * @param timeout_ticks Maximum time to wait for the semaphore
 * @return True if the semaphore is acquired, false otherwise
 */
bool sema_acquire(Sema_t *sema, uint32_t timeout_ticks) {
    Timeout_t timeout;
    bool timeout_set = false;

    OCTOS_ENTER_CRITICAL();
    sema->Count--;
    OCTOS_EXIT_CRITICAL();

    while (true) {
        OCTOS_ENTER_CRITICAL();

        if (sema->Count >= 0) {
            OCTOS_EXIT_CRITICAL();
            return true;
        }

        if (timeout_ticks == 0) {
            OCTOS_EXIT_CRITICAL();
            return false;
        } else if (!timeout_set) {
            /* timeout_ticks == UINT32_MAX means to wait indefinitely */
            if (timeout_ticks != UINT32_MAX) task_set_timeout(&timeout);
            timeout_set = true;
        }

        OCTOS_EXIT_CRITICAL();

        task_suspend_all();
        SyncCore_t *const core = &(sema->Core);
        /* Lock the queue so ISR cannot modify EventListItem */
        sync_lock(core);
        /* Timeout has expired */
        if (timeout_ticks != UINT32_MAX &&
            task_check_timeout(&timeout, timeout_ticks)) {
            sync_unlock(core);
            task_resume_all();
            return false;
        }

        /* Timeout has not expired */
        if (sema->Count < 0) {
            task_add_current_to_event_list(&(core->BlockedList), timeout_ticks);
            sync_unlock(core);
            if (!task_resume_all()) OCTOS_YIELD();
        } else {
            sync_unlock(core);
            task_resume_all();
        }
    }
}

/**
 * @brief Releases a semaphore
 * @param sema Pointer to the semaphore to be released
 * @return None
 */
void sema_release(Sema_t *sema) {
    bool switch_required = false;

    OCTOS_ENTER_CRITICAL();

    sema->Count++;

    if (sema->Count > 0) {
        OCTOS_EXIT_CRITICAL();
        return;
    }

    List_t *const blocked_list = &(sema->Core.BlockedList);
    if (blocked_list->Length > 0) {
        switch_required =
                task_remove_highest_priority_from_event_list(blocked_list);
    }

    OCTOS_EXIT_CRITICAL();

    if (switch_required) OCTOS_YIELD();
}

/**
 * @brief Acquires a semaphore from an ISR
 * @param sema Pointer to the semaphore to be acquired
 * @return True if the semaphore is acquired, false otherwise
 */
bool sema_acquire_from_isr(Sema_t *sema) {
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
 * @brief Releases a semaphore from an ISR
 * @param sema Pointer to the semaphore to be released
 * @param switch_required Pointer to a boolean indicating if a context switch is required
 * @return None
 */
void sema_release_from_isr(Sema_t *sema, bool *const switch_required) {
    OCTOS_ASSERT_IF_INTERRUPT_PRIORITY_INVALID();

    uint32_t saved_intr_status = OCTOS_ENTER_CRITICAL_FROM_ISR();

    sema->Count++;

    if (sema->Count > 0) {
        OCTOS_EXIT_CRITICAL_FROM_ISR(saved_intr_status);
        return;
    }

    SyncCore_t *const core = &(sema->Core);
    const int8_t lock = core->Lock;
    if (core->Lock == syncUNLOCKED) {
        const bool higher_priority_woken =
                task_remove_highest_priority_from_event_list(
                        &(core->BlockedList));
        if (switch_required != NULL) *switch_required = higher_priority_woken;
    } else {
        sync_lock_increment(core, lock);
    }

    OCTOS_EXIT_CRITICAL_FROM_ISR(saved_intr_status);
}

/* Mutex ---------------------------------------------------------------------*/

/**
  * @brief Initialize a mutex
  * @param mutex Pointer to mutex structure
  * @return None
  */
void mutex_init(Mutex_t *mutex) {
    mutex->Owner = NULL;
    sync_core_init(&(mutex->Core));
}

/**
 * @brief Acquires a mutex
 * @param mutex Pointer to the mutex to be acquired
 * @param timeout_ticks Maximum time to wait for the mutex
 * @return True if the mutex is acquired, false otherwise
 */
bool mutex_acquire(Mutex_t *mutex, uint32_t timeout_ticks) {
    Timeout_t timeout;
    bool timeout_set = false;

    OCTOS_ENTER_CRITICAL();

    TCB_t *const tcb = task_get_current();

    OCTOS_EXIT_CRITICAL();

    while (true) {
        OCTOS_ENTER_CRITICAL();

        if (mutex->Owner == tcb) {
            OCTOS_EXIT_CRITICAL();
            return true;
        } else if (mutex->Owner == NULL) {
            mutex->Owner = tcb;
            OCTOS_EXIT_CRITICAL();
            return true;
        }

        if (timeout_ticks == 0) {
            OCTOS_EXIT_CRITICAL();
            return false;
        } else if (!timeout_set) {
            /* timeout_ticks == UINT32_MAX means to wait indefinitely */
            if (timeout_ticks != UINT32_MAX) task_set_timeout(&timeout);
            timeout_set = true;
        }

        OCTOS_EXIT_CRITICAL();

        task_suspend_all();
        SyncCore_t *const core = &(mutex->Core);
        /* Lock the queue so ISR cannot modify EventListItem */
        sync_lock(core);

        /* Timeout has expired */
        if (timeout_ticks != UINT32_MAX &&
            task_check_timeout(&timeout, timeout_ticks)) {
            sync_unlock(core);
            task_resume_all();
            return false;
        }

        /* Timeout has expired */
        if (timeout_ticks != UINT32_MAX &&
            task_check_timeout(&timeout, timeout_ticks)) {
            sync_unlock(core);
            task_resume_all();
            return false;
        }

        /* Timeout has not expired */
        if (mutex->Owner != NULL) {
            task_add_current_to_event_list(&(core->BlockedList), timeout_ticks);
            sync_unlock(core);
            if (!task_resume_all()) OCTOS_YIELD();
        } else {
            sync_unlock(core);
            task_resume_all();
        }
    }
}

/**
 * @brief Releases a mutex
 * @param mutex Pointer to the mutex to be released
 * @return True if the mutex is released, false otherwise
 */
bool mutex_release(Mutex_t *mutex) {
    bool switch_required = false;

    OCTOS_ENTER_CRITICAL();

    TCB_t *const tcb = task_get_current();

    if (mutex->Owner != tcb) {
        OCTOS_EXIT_CRITICAL();
        return false;
    }

    mutex->Owner = NULL;

    List_t *const blocked_list = &(mutex->Core.BlockedList);
    if (blocked_list->Length > 0) {
        switch_required =
                task_remove_highest_priority_from_event_list(blocked_list);
    }

    OCTOS_EXIT_CRITICAL();

    if (switch_required) OCTOS_YIELD();

    return true;
}

/**
 * @brief Acquires a mutex from an ISR
 * @param mutex Pointer to the mutex to be acquired
 * @return True if the mutex is acquired, false otherwise
 */
bool mutex_acquire_from_isr(Mutex_t *mutex) {
    OCTOS_ASSERT_IF_INTERRUPT_PRIORITY_INVALID();

    uint32_t saved_intr_status = OCTOS_ENTER_CRITICAL_FROM_ISR();

    if (mutex->Owner != NULL) {
        OCTOS_EXIT_CRITICAL_FROM_ISR(saved_intr_status);
        return false;
    }
    mutex->Owner = task_get_current();

    OCTOS_EXIT_CRITICAL_FROM_ISR(saved_intr_status);

    return true;
}

/**
 * @brief Releases a mutex from an ISR
 * @param mutex Pointer to the mutex to be released
 * @param switch_required Pointer to a boolean indicating if a context switch is required
 * @return True if the mutex is released, false otherwise
 */
bool mutex_release_from_isr(Mutex_t *mutex, bool *const switch_required) {
    OCTOS_ASSERT_IF_INTERRUPT_PRIORITY_INVALID();

    uint32_t saved_intr_status = OCTOS_ENTER_CRITICAL_FROM_ISR();

    if (mutex->Owner != current_tcb) {
        OCTOS_EXIT_CRITICAL_FROM_ISR(saved_intr_status);
        return false;
    }

    mutex->Owner = NULL;

    SyncCore_t *const core = &(mutex->Core);
    const int8_t lock = core->Lock;
    if (core->Lock == syncUNLOCKED) {
        const bool higher_priority_woken =
                task_remove_highest_priority_from_event_list(
                        &(core->BlockedList));
        if (switch_required != NULL) *switch_required = higher_priority_woken;
    } else {
        sync_lock_increment(core, lock);
    }

    OCTOS_EXIT_CRITICAL_FROM_ISR(saved_intr_status);

    return true;
}
