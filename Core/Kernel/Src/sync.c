#include <stdint.h>

#include "Arch/stm32f4xx/Inc/api.h"
#include "list.h"
#include "sync.h"
#include "task.h"

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
OCTOS_INLINE static inline void sync_unlock(SyncCore_t *core) {
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

/** 
 * @brief Notify a single task waiting on a synchronization core
 * @note If a higher priority task is woken, the switch_required flag will be set
 * @param core Pointer to the synchronization core
 * @param switch_required Pointer to a boolean flag indicating if a context switch is required
 * @return None
 */
OCTOS_INLINE static inline void sync_notify(SyncCore_t *core,
                                            bool *const switch_required) {
    List_t *const blocked_list = &(core->BlockedList);

    if (blocked_list->Length > 0) {
        const bool higher_priority_woken =
                task_remove_highest_priority_from_event_list(blocked_list);

        if (switch_required != NULL) *switch_required |= higher_priority_woken;
    }
}

/** 
 * @brief Notify all tasks waiting on a synchronization core
 * @note If a higher priority task is woken, the switch_required flag will be set
 * @param core Pointer to the synchronization core
 * @param switch_required Pointer to a boolean flag indicating if a context switch is required
 * @return None
 */
OCTOS_INLINE static inline void sync_notify_all(SyncCore_t *core,
                                                bool *const switch_required) {
    bool higher_priority_woken = false;
    List_t *const blocked_list = &(core->BlockedList);

    while (blocked_list->Length > 0) {
        higher_priority_woken |=
                task_remove_highest_priority_from_event_list(blocked_list);
    }

    if (switch_required != NULL) *switch_required |= higher_priority_woken;
}

/** 
 * @brief Notify a single task waiting on a synchronization core from an ISR
 * @note If the core is unlocked, a task will be notified. If locked, the lock count is incremented
 * @param core Pointer to the synchronization core
 * @param switch_required Pointer to a boolean flag indicating if a context switch is required
 * @return None
 */
OCTOS_INLINE static inline void
sync_notify_from_isr(SyncCore_t *core, bool *const switch_required) {
    const int8_t lock = core->Lock;

    if (core->Lock == syncUNLOCKED) {
        const bool higher_priority_woken =
                task_remove_highest_priority_from_event_list(
                        &(core->BlockedList));

        if (switch_required != NULL) *switch_required |= higher_priority_woken;
    } else {
        sync_lock_increment(core, lock);
    }
}

/** 
 * @brief Notify all tasks waiting on a synchronization core from an ISR
 * @note If the core is unlocked, all tasks will be notified. If locked, the lock count is incremented for each task
 * @param core Pointer to the synchronization core
 * @param switch_required Pointer to a boolean flag indicating if a context switch is required
 * @return None
 */
OCTOS_INLINE static inline void
sync_notify_all_from_isr(SyncCore_t *core, bool *const switch_required) {
    const int8_t lock = core->Lock;
    List_t *const blocked_list = &(core->BlockedList);

    if (core->Lock == syncUNLOCKED) {
        bool higher_priority_woken = false;

        while (blocked_list->Length > 0) {
            higher_priority_woken |=
                    task_remove_highest_priority_from_event_list(blocked_list);
        }

        if (switch_required != NULL) *switch_required = higher_priority_woken;
    } else {
        for (size_t i = blocked_list->Length; i > 0; i--) {
            sync_lock_increment(core, lock);
        }
    }
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

    while (true) {
        OCTOS_ENTER_CRITICAL();

        if (!timeout_set) sema->Count--;

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
    sync_notify(&(sema->Core), &switch_required);

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

    sync_notify_from_isr(&(sema->Core), switch_required);

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
 * @brief Acquire a mutex with a specified timeout
 * @note This function blocks the current task if the mutex is already 
 *       owned by another task
 * @note Will perform priority inheritance if needed
 * @param mutex Pointer to the mutex to be acquired
 * @param timeout_ticks Timeout value in ticks, 0 for no wait, UINT32_MAX
 *        for indefinite wait
 * @return True if the mutex was acquired, false otherwise
 */
bool mutex_acquire(Mutex_t *mutex, uint32_t timeout_ticks) {
    Timeout_t timeout;
    bool timeout_set = false;
    bool inheritance_occured = false;

    while (true) {
        OCTOS_ENTER_CRITICAL();
        if (mutex->Owner == NULL) {
            mutex->Owner = task_mutex_held_increment();
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

            if (inheritance_occured) {
                OCTOS_ENTER_CRITICAL();
                const uint8_t highest_priority_of_waiting_tasks =
                        list_item_get_value(list_tail(&(core->BlockedList)));
                task_deinherit_priority_after_timeout(
                        mutex->Owner, highest_priority_of_waiting_tasks);
                OCTOS_EXIT_CRITICAL();
            }

            return false;
        }

        /* Timeout has not expired */
        TCB_t *const owner = mutex->Owner;
        if (owner != NULL) {
            OCTOS_ENTER_CRITICAL();
            inheritance_occured = task_inherit_priority(owner);
            OCTOS_EXIT_CRITICAL();

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
 * @brief Release a mutex
 * @note This function should be called when a task is done with the mutex
 * @param mutex Pointer to the mutex to be released
 * @return True if a context switch is required, false otherwise
 */
bool mutex_release(Mutex_t *mutex) {
    bool switch_required = false;

    OCTOS_ENTER_CRITICAL();

    if (!task_mutex_held_decrement(mutex->Owner)) {
        OCTOS_EXIT_CRITICAL();
        return false;
    }

    switch_required |= task_deinherit_priority(mutex->Owner);
    mutex->Owner = NULL;
    sync_notify(&(mutex->Core), &switch_required);

    OCTOS_EXIT_CRITICAL();

    if (switch_required) OCTOS_YIELD();

    return true;
}

/* Condtion ------------------------------------------------------------------*/

/**
 * @brief Initialize a condition variable
 * @param cond Pointer to the condition variable to initialize
 * @return None
 */
void cond_init(Cond_t *cond) { sync_core_init(&(cond->Core)); }

/**
 * @brief Wait on a condition variable
 * @note If the timeout expires, the function returns false
 * @param cond Pointer to the condition variable
 * @param timeout_ticks Timeout in ticks (UINT32_MAX for indefinite wait)
 * @retval true Condition was signaled
 * @retval false Timeout expired
 */
bool cond_wait(Cond_t *cond, uint32_t timeout_ticks) {
    Timeout_t timeout;
    bool timeout_set = false;

    while (true) {
        OCTOS_ENTER_CRITICAL();

        if (timeout_ticks == 0) {
            OCTOS_EXIT_CRITICAL();
            return false;
        } else if (!timeout_set) {
            /* timeout_ticks == UINT32_MAX means to wait indefinitely */
            if (timeout_ticks != UINT32_MAX) task_set_timeout(&timeout);
            timeout_set = true;
        } else {
            OCTOS_EXIT_CRITICAL();
            return true;
        }

        OCTOS_EXIT_CRITICAL();

        task_suspend_all();
        SyncCore_t *const core = &(cond->Core);
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
        task_add_current_to_event_list(&(core->BlockedList), timeout_ticks);
        sync_unlock(core);
        if (!task_resume_all()) OCTOS_YIELD();
    }
}

/**
 * @brief Notify one task waiting on the condition variable
 * @param cond Pointer to the condition variable
 * @return None
 */
void cond_notify(Cond_t *cond) {
    bool switch_required = false;

    OCTOS_ENTER_CRITICAL();

    sync_notify(&(cond->Core), &switch_required);

    OCTOS_EXIT_CRITICAL();

    if (switch_required) OCTOS_YIELD();
}

/**
 * @brief Notify all tasks waiting on the condition variable
 * @param cond Pointer to the condition variable
 * @return None
 */
void cond_notify_all(Cond_t *cond) {
    bool switch_required = false;

    OCTOS_ENTER_CRITICAL();

    sync_notify_all(&(cond->Core), &switch_required);

    OCTOS_EXIT_CRITICAL();

    if (switch_required) OCTOS_YIELD();
}

/**
 * @brief Notify one task waiting on the condition variable from an ISR
 * @param cond Pointer to the condition variable
 * @param switch_required Pointer to a boolean indicating if a context switch is required
 * @return None
 */
void cond_notify_from_isr(Cond_t *cond, bool *const switch_required) {
    OCTOS_ASSERT_IF_INTERRUPT_PRIORITY_INVALID();

    uint32_t saved_intr_status = OCTOS_ENTER_CRITICAL_FROM_ISR();

    SyncCore_t *const core = &(cond->Core);
    List_t *const blocked_list = &(core->BlockedList);
    if (blocked_list->Length == 0) {
        OCTOS_EXIT_CRITICAL_FROM_ISR(saved_intr_status);
        return;
    }

    sync_notify_from_isr(&(cond->Core), switch_required);

    OCTOS_EXIT_CRITICAL_FROM_ISR(saved_intr_status);
}

/**
 * @brief Notify all tasks waiting on the condition variable from an ISR
 * @param cond Pointer to the condition variable
 * @param switch_required 
 *      Pointer to a boolean indicating if a context switch is required
 * @return None
 */
void cond_notify_all_from_isr(Cond_t *cond, bool *const switch_required) {
    OCTOS_ASSERT_IF_INTERRUPT_PRIORITY_INVALID();

    uint32_t saved_intr_status = OCTOS_ENTER_CRITICAL_FROM_ISR();

    SyncCore_t *const core = &(cond->Core);
    List_t *const blocked_list = &(core->BlockedList);
    if (blocked_list->Length == 0) {
        OCTOS_EXIT_CRITICAL_FROM_ISR(saved_intr_status);
        return;
    }

    sync_notify_all_from_isr(&(cond->Core), switch_required);

    OCTOS_EXIT_CRITICAL_FROM_ISR(saved_intr_status);
}


/* Barrier -------------------------------------------------------------------*/

/**
 * @brief Initialize a barrier
 * @param barrier Pointer to the barrier to initialize
 * @param parties Number of parties required to release the barrier
 * @return None
 */
void barrier_init(Barrier_t *barrier, uint32_t parties) {
    barrier->Parties = parties;
    barrier->Count = 0;
    sync_core_init(&(barrier->Core));
}

/**
 * @brief Wait at the barrier
 * @note If the timeout expires, the function returns false
 * @param barrier Pointer to the barrier
 * @param timeout_ticks Timeout in ticks (UINT32_MAX for indefinite wait)
 * @retval true Barrier was released
 * @retval false Timeout expired
 */
bool barrier_wait(Barrier_t *barrier, uint32_t timeout_ticks) {
    OCTOS_ENTER_CRITICAL();

    barrier->Count++;
    if (barrier->Count == barrier->Parties) {
        bool switch_required = false;
        List_t *const blocked_list = &(barrier->Core.BlockedList);
        while (blocked_list->Length > 0) {
            switch_required |=
                    task_remove_highest_priority_from_event_list(blocked_list);
        }
        barrier->Count = 0;
        OCTOS_EXIT_CRITICAL();

        if (switch_required) OCTOS_YIELD();
        return true;
    }

    OCTOS_EXIT_CRITICAL();

    Timeout_t timeout;
    bool timeout_set = false;

    while (true) {
        OCTOS_ENTER_CRITICAL();

        if (timeout_ticks == 0) {
            OCTOS_EXIT_CRITICAL();
            return false;
        } else if (!timeout_set) {
            /* timeout_ticks == UINT32_MAX means to wait indefinitely */
            if (timeout_ticks != UINT32_MAX) task_set_timeout(&timeout);
            timeout_set = true;
        } else {
            OCTOS_EXIT_CRITICAL();
            return true;
        }

        OCTOS_EXIT_CRITICAL();

        task_suspend_all();
        SyncCore_t *const core = &(barrier->Core);
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
        if (barrier->Count != barrier->Parties) {
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
 * @brief Reset the barrier
 * @param barrier Pointer to the barrier
 * @return None
 */
void barrier_reset(Barrier_t *barrier) {
    bool switch_required = false;

    OCTOS_ENTER_CRITICAL();

    sync_notify_all(&(barrier->Core), &switch_required);
    barrier->Count = 0;

    OCTOS_EXIT_CRITICAL();

    if (switch_required) OCTOS_YIELD();
}

/** 
 * @brief Reset a barrier from an ISR context
 * @note This function is used to reset a barrier and unblock all tasks
 *       waiting on it. 
 *       If the barrier is unlocked, tasks are moved to the ready list.
 *       If the barrier is locked, the lock count is incremented for
 *       each blocked task.
 * @param barrier Pointer to the barrier to be reset
 * @param switch_required 
 *      Pointer to a boolean that indicates if a context switch is required
 * @return None
 */
void barrier_reset_from_isr(Barrier_t *barrier, bool *const switch_required) {
    OCTOS_ASSERT_IF_INTERRUPT_PRIORITY_INVALID();

    uint32_t saved_intr_status = OCTOS_ENTER_CRITICAL_FROM_ISR();

    sync_notify_all_from_isr(&(barrier->Core), switch_required);
    barrier->Count = 0;

    OCTOS_EXIT_CRITICAL_FROM_ISR(saved_intr_status);
}

/* Event ---------------------------------------------------------------------*/

/**
 * @brief Initialize an event
 * @param event Pointer to the Event_t structure to be initialized
 * @return None
 */
void event_init(Event_t *event) {
    event->Flag = false;
    sync_core_init(&(event->Core));
}

/**
 * @brief Check if an event is set
 * @param event Pointer to the Event_t structure to check
 * @return True if the event is set, false otherwise
 */
bool event_is_set(Event_t *event) {
    bool result;

    OCTOS_ENTER_CRITICAL();
    result = event->Flag;
    OCTOS_EXIT_CRITICAL();

    return result;
}

/**
 * @brief Set an event
 * @note If the event is set and tasks are blocked on it, they will be unblocked
 * @param event Pointer to the Event_t structure to set
 * @return None
 */
void event_set(Event_t *event) {
    bool switch_required = false;

    OCTOS_ENTER_CRITICAL();

    /* Micro optimization */
    if (event->Flag == true) {
        OCTOS_EXIT_CRITICAL();
        return;
    }

    event->Flag = true;
    sync_notify_all(&(event->Core), &switch_required);

    OCTOS_EXIT_CRITICAL();

    if (switch_required) OCTOS_YIELD();
}

/**
 * @brief Clear an event
 * @param event Pointer to the Event_t structure to clear
 * @return None
 */
void event_clear(Event_t *event) {
    OCTOS_ENTER_CRITICAL();

    event->Flag = false;

    OCTOS_EXIT_CRITICAL();
}

/**
 * @brief Wait for an event to be set
 * @note If the event is not set, the calling task will be blocked until
 *       the event is set or the timeout expires
 * @param event Pointer to the Event_t structure to wait on
 * @param timeout_ticks Timeout in ticks (UINT32_MAX for indefinite wait)
 * @retval True if the event was set
 * @retval False if the timeout expired
 */
bool event_wait(Event_t *event, uint32_t timeout_ticks) {
    Timeout_t timeout;
    bool timeout_set = false;

    while (true) {
        OCTOS_ENTER_CRITICAL();

        if (event->Flag == true) {
            OCTOS_EXIT_CRITICAL();
            return true;
        } else if (timeout_ticks == 0) {
            OCTOS_EXIT_CRITICAL();
            return false;
        } else if (!timeout_set) {
            /* timeout_ticks == UINT32_MAX means to wait indefinitely */
            if (timeout_ticks != UINT32_MAX) task_set_timeout(&timeout);
            timeout_set = true;
        }

        OCTOS_EXIT_CRITICAL();

        task_suspend_all();
        SyncCore_t *const core = &(event->Core);
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
        if (event->Flag != true) {
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
 * @brief Check if an event is set from an ISR
 * @param event Pointer to the Event_t structure to check
 * @return True if the event is set, false otherwise
 */
bool event_is_set_from_isr(Event_t *event) {
    OCTOS_ASSERT_IF_INTERRUPT_PRIORITY_INVALID();

    bool result;

    uint32_t saved_intr_status = OCTOS_ENTER_CRITICAL_FROM_ISR();
    result = event->Flag;
    OCTOS_EXIT_CRITICAL_FROM_ISR(saved_intr_status);

    return result;
}

/**
 * @brief Set an event from an ISR
 * @note If the event is set and tasks are blocked on it, they will
 *       be unblocked
 * @param event Pointer to the Event_t structure to set
 * @param switch_required 
 *      Pointer to a boolean indicating if a context switch is required
 * @return None
 */
void event_set_from_isr(Event_t *event, bool *const switch_required) {
    OCTOS_ASSERT_IF_INTERRUPT_PRIORITY_INVALID();

    uint32_t saved_intr_status = OCTOS_ENTER_CRITICAL_FROM_ISR();

    if (event->Flag == true) {
        if (switch_required != NULL) *switch_required = false;
        OCTOS_EXIT_CRITICAL_FROM_ISR(saved_intr_status);
        return;
    }

    event->Flag = true;
    sync_notify_all_from_isr(&(event->Core), switch_required);

    OCTOS_EXIT_CRITICAL_FROM_ISR(saved_intr_status);
}

/**
 * @brief Clear an event from an ISR
 * @param event Pointer to the Event_t structure to clear
 * @return None
 */
void event_clear_from_isr(Event_t *event) {
    OCTOS_ASSERT_IF_INTERRUPT_PRIORITY_INVALID();

    uint32_t saved_intr_status = OCTOS_ENTER_CRITICAL_FROM_ISR();

    event->Flag = false;

    OCTOS_EXIT_CRITICAL_FROM_ISR(saved_intr_status);
}
