#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "Arch/stm32f4xx/Inc/api.h"
#include "bitmap.h"
#include "config.h"
#include "list.h"
#include "page.h"
#include "task.h"
#include "tcb.h"
#include "utils.h"

TCB_t *volatile current_tcb = NULL;

static volatile uint32_t current_tick = 0;
static volatile uint32_t pended_ticks = 0;
static volatile uint32_t tick_overflows = 0;
static volatile uint32_t next_task_unblock_tick = UINT32_MAX;
static volatile uint32_t scheduler_suspended = 0;
static volatile bool yield_pending = false;
static volatile uint32_t
        top_ready_priority_data[(OCTOS_MAX_PRIORITIES + 31) / 32];
static volatile Bitmap_t top_ready_priority;
static volatile uint8_t current_number_of_tasks = 0;
/* zero is reserved for idle task */
static uint32_t tcb_id = 1;

static List_t ready_list[OCTOS_MAX_PRIORITIES];
static List_t pending_ready_list;
static List_t delayed_list_1;
static List_t delayed_list_2;
static List_t *volatile delayed_list;
static List_t *volatile delayed_list_overflow;
static List_t suspended_list;
static List_t terminated_list;

/* Private Helper ------------------------------------------------------------*/

/**
 * @brief Create and initialize a new Thread Control Block
 * @param page Memory page to use for the thread
 * @param func Function pointer to the thread's task
 * @param args Arguments to pass to the thread's task
 * @param priority Initial priority for the thread
 * @return TCB_t* Pointer to the created Thread Control Block
 */
static TCB_t *tcb_build(Page_t *page, TaskFunc_t func, void *args,
                        uint8_t priority) {
    TCB_t *tcb = (TCB_t *) page->raw;

    tcb->Page.raw = page->raw;
    tcb->Page.size = page->size;
    tcb->Page.policy = page->policy;

    uint32_t *stack_top = &(page->raw[page->size - 16]);
    stack_top[15] = (uint32_t) (1U << 24);// PSR
    stack_top[14] = (uint32_t) func;      // PC
    stack_top[13] = 0;                    // LR
    stack_top[12] = 0;                    // R12
    stack_top[11] = 0;                    // R3
    stack_top[10] = 0;                    // R2
    stack_top[9] = 0;                     // R1
    stack_top[8] = (uint32_t) args;       // R0
    tcb->StackTop = stack_top;

    tcb->TCBNumber = tcb_id++;
    tcb->RootPriority = priority;
    tcb->Priority = priority;
    list_item_init(&(tcb->StateListItem));
    list_item_init(&(tcb->EventListItem));
    list_item_set_value(&(tcb->StateListItem), priority);
    list_item_set_value(&(tcb->StateListItem), priority);
    tcb->StateListItem.Owner = tcb;
    tcb->EventListItem.Owner = tcb;

    return tcb;
}

/** 
 * @brief Perform post-processing after task creation
 * @param tcb Pointer to the TCB of the newly created task
 * @return None
 */
static void task_create_postprocess(TCB_t *tcb) {
    if (current_tcb == NULL) {
        for (size_t i = 0; i < OCTOS_MAX_PRIORITIES; i++)
            list_init(&ready_list[i]);
        bitmap_init((Bitmap_t *) &top_ready_priority,
                    (uint32_t *) top_ready_priority_data, OCTOS_MAX_PRIORITIES);

        current_tcb = tcb;
    }

    task_add_to_ready_list(tcb);
    current_number_of_tasks++;
}

/** 
 * @brief Set the ready priority for a task
 * @param priority The priority to set
 * @return None
 */
static void task_set_ready_priority(uint8_t priority) {
    OCTOS_ASSERT(priority < OCTOS_MAX_PRIORITIES);
    bitmap_set((Bitmap_t *) &top_ready_priority,
               OCTOS_MAX_PRIORITIES - priority - 1);
}

/** 
 * @brief Reset the ready priority for a task
 * @param priority The priority to reset
 * @return None
 */
static void task_reset_ready_priority(uint8_t priority) {
    OCTOS_ASSERT(priority < OCTOS_MAX_PRIORITIES);
    if (ready_list[priority].Length == 0)
        bitmap_reset((Bitmap_t *) &top_ready_priority,
                     OCTOS_MAX_PRIORITIES - priority - 1);
}

/** 
 * @brief Select the highest priority task to execute
 * @return None
 */
static void task_select_highest_priority(void) {
    const int32_t highest_priority =
            OCTOS_MAX_PRIORITIES -
            bitmap_first_one((Bitmap_t *) &top_ready_priority) - 1;
    OCTOS_ASSERT(highest_priority >= 0);
    OCTOS_ASSERT(ready_list[highest_priority].Length > 0);
    current_tcb = list_get_owner_of_next_entry(&ready_list[highest_priority]);
}

/**
 * @brief Update the next task unblock tick
 * @note This function updates the next_task_unblock_tick variable
 *       based on the first element in the delayed list
 * @return None
 */
static void reset_next_task_unblock_tick(void) {
    if (delayed_list->Length > 0)
        next_task_unblock_tick = list_head(delayed_list)->Value;
    else
        next_task_unblock_tick = UINT32_MAX;
}

/* Misc ----------------------------------------------------------------------*/

/**
 * @brief Get the current state of a thread
 * @param tcb Pointer to the Thread Control Block
 * @return ThreadState_t Current state of the thread
 */
TaskState_t task_status(TCB_t *tcb) {
    List_t *const parent = tcb->StateListItem.Parent;
    if (tcb == current_tcb) return RUNNING;
    else if (parent == &terminated_list)
        return TERMINATED;
    else if (parent == delayed_list || parent == delayed_list_overflow)
        return BLOCKED;
    else if (parent == &suspended_list) {
        if (tcb->EventListItem.Parent) return BLOCKED;
        else
            return SUSPENDED;
    } else if (parent != NULL)
        return READY;
    else
        return INVALID;
}

/** 
 * @brief Set a timeout for a task
 * @param timeout Pointer to the timeout structure
 * @return None
 */
void task_set_timeout(Timeout_t *timeout) {
    OCTOS_ENTER_CRITICAL();

    timeout->entering_tick = current_tick;
    timeout->overflows = tick_overflows;

    OCTOS_EXIT_CRITICAL();
}

/** 
 * @brief Check if a timeout has occurred
 * @param timeout Pointer to the timeout structure
 * @param ticks_to_delay The number of ticks to delay
 * @return True if timeout has occurred, false otherwise
 */
bool task_check_timeout(Timeout_t *timeout, uint32_t ticks_to_delay) {
    bool is_timeout = false;

    OCTOS_ENTER_CRITICAL();

    const uint32_t expected_tick = timeout->entering_tick + ticks_to_delay;
    const uint32_t expected_overflow = timeout->entering_tick > expected_tick
                                               ? (timeout->overflows + 1)
                                               : timeout->overflows;

    is_timeout = !((tick_overflows == expected_overflow) &&
                   (current_tick < expected_tick));

    OCTOS_EXIT_CRITICAL();

    return is_timeout;
}

/**
 * @brief Get the number of tasks currently running
 * @return The number of tasks
 */
uint8_t task_get_number_of_tasks(void) { return current_number_of_tasks; }

/* Task List -----------------------------------------------------------------*/

/** 
 * @brief Initialize task lists
 * @return None
 */
void task_lists_init(void) {
    list_init(&pending_ready_list);
    list_init(&suspended_list);
    list_init(&terminated_list);

    list_init(&delayed_list_1);
    list_init(&delayed_list_2);
    delayed_list = &delayed_list_1;
    delayed_list_overflow = &delayed_list_2;
}

/** 
 * @brief Add a task to the ready list
 * @note This function will automatically set top priority, when you need to
 *       insert some task to ready list, do use this function
 * @note This function is not protected by any critical section or scheduler
 *       suspension
 * @param tcb Pointer to the TCB of the task to add
 * @return None
 */
void task_add_to_ready_list(TCB_t *tcb) {
    task_set_ready_priority(tcb->Priority);
    list_insert_end(&ready_list[tcb->Priority], &(tcb->StateListItem));
}

/**
 * @brief Remove the current task from its current state and add it to the delayed list
 * @param ticks_to_delay The number of ticks to delay before the task is ready to run again
 * @note If ticks_to_delay is UINT32_MAX, the task is moved to the suspended list
 * @return None
 */
void task_remove_and_add_current_to_delayed_list(uint32_t ticks_to_delay) {
    ListItem_t *const item = &(current_tcb->StateListItem);
    if (list_remove(item)) task_reset_ready_priority(current_tcb->Priority);

    if (ticks_to_delay == UINT32_MAX) {
        list_insert_end(&suspended_list, item);
    } else {
        const uint32_t tick_to_wake = current_tick + ticks_to_delay;
        list_item_set_value(item, tick_to_wake);
        if (tick_to_wake < current_tick) {
            // Overflow will occur, add to overflow list
            list_insert(delayed_list_overflow, item);
        } else {
            // Add to normal delayed list
            list_insert(delayed_list, item);
            reset_next_task_unblock_tick();
        }
    }
}

/**
 * @brief Remove a task from the delayed list
 * @param tcb Pointer to the TCB of the task to be removed
 * @return True if the removed task has a higher priority than the current task, false otherwise
 */
bool task_remove_from_delayed_list(TCB_t *tcb) {
    ListItem_t *const item = &(tcb->StateListItem);
    List_t *const parent = item->Parent;
    bool item_need_to_reset_value =
            parent == delayed_list || parent == delayed_list_overflow;

    if (!item_need_to_reset_value && parent != &suspended_list) return false;

    /* Remove tcb's StateListItem from its parent */
    list_remove(item);

    /* Reset tcb's StateListItem value from wake tick to priority */
    TCB_t *const owner = item->Owner;
    if (item_need_to_reset_value) list_item_set_value(item, owner->Priority);

    return owner->Priority > current_tcb->Priority;
}

/**
 * @brief Add the current task to an event list and delay it for a specified number of ticks
 * @param list The event list to add the task to
 * @param ticks_to_wait The number of ticks to wait before the task is ready to run again
 * @return None
 */
void task_add_current_to_event_list(List_t *list, uint32_t ticks_to_wait) {
    OCTOS_ASSERT(ticks_to_wait > 0 && ticks_to_wait <= UINT32_MAX);
    OCTOS_ASSERT(current_tcb->EventListItem.Parent == NULL);

    list_insert(list, &(current_tcb->EventListItem));
    task_remove_and_add_current_to_delayed_list(ticks_to_wait);
}

/**
 * @brief Remove the highest priority task from an event list
 * @param list The event list to remove the task from
 * @return True if the removed task has a higher priority than the current task, false otherwise
 */
bool task_remove_highest_priority_from_event_list(List_t *list) {
    if (list->Length == 0) return false;

    bool switch_required = false;

    ListItem_t *item = list_tail(list);
    list_remove(item);
    TCB_t *const owner = item->Owner;

    switch_required = task_remove_from_delayed_list(owner);
    yield_pending |= switch_required;

    if (scheduler_suspended > 0) {
        list_insert_end(&pending_ready_list, &(owner->EventListItem));
    } else {
        task_add_to_ready_list(owner);
    }

    return switch_required;
}


/* Task Create and Delete ----------------------------------------------------*/

/**
 * @brief Creates a new task with dynamically allocated memory
 * @param func Function pointer to the task entry
 * @param args Arguments passed to the task function
 * @param priority Task priority level
 * @param page_size_in_words Size of memory needed in 32-bit words
 * @return True if task created successfully, false otherwise
 */
bool task_create(TaskFunc_t func, void *args, uint8_t priority,
                 size_t page_size_in_words) {
    OCTOS_ASSERT(priority < OCTOS_MAX_PRIORITIES);


    Page_t page = {0};
    page.policy = PAGE_POLICY_DYNAMIC;
    page.size = page_size_in_words;
    page.raw = OCTOS_MALLOC(page_size_in_words * sizeof(uint32_t));

    if (!page.raw) return false;
    memset(page.raw, 0, page_size_in_words * sizeof(uint32_t));

    TCB_t *tcb = tcb_build(&page, func, args, priority);

    task_suspend_all();
    task_create_postprocess(tcb);
    task_resume_all();

    return true;
}

/**
 * @brief Creates a new task using provided static memory
 * @param func Function pointer to the task entry
 * @param args Arguments passed to the task function
 * @param priority Task priority level
 * @param buffer Pointer to memory buffer to use
 * @param page_size_in_words Size of provided buffer in 32-bit words
 * @return True if task created successfully, false otherwise
 */
bool task_create_static(TaskFunc_t func, void *args, uint8_t priority,
                        uint32_t *buffer, size_t page_size_in_words) {
    OCTOS_ASSERT(priority < OCTOS_MAX_PRIORITIES);

    task_suspend_all();

    if (!buffer) {
        task_resume_all();
        return false;
    }

    Page_t page = {0};
    page.policy = PAGE_POLICY_STATIC;
    page.size = page_size_in_words;
    page.raw = buffer;
    memset(page.raw, 0, page_size_in_words * sizeof(uint32_t));

    TCB_t *tcb = tcb_build(&page, func, args, priority);
    task_create_postprocess(tcb);

    task_resume_all();
    return true;
}

/**
 * @brief Deletes task and moves it to terminated state
 * @param tcb Pointer to task control block to delete
 * @return None
 */
void task_delete(TCB_t *tcb) {
    bool switch_required = false;

    OCTOS_ENTER_CRITICAL();

    if (!tcb || task_status(tcb) == TERMINATED) {
        OCTOS_EXIT_CRITICAL();
        return;
    }

    if (list_remove(&(tcb->StateListItem)))
        task_reset_ready_priority(tcb->Priority);

    switch_required = tcb == current_tcb;

    list_insert_end(&terminated_list, &(tcb->StateListItem));
    current_number_of_tasks--;

    OCTOS_EXIT_CRITICAL();

    if (switch_required) {
        OCTOS_ASSERT(scheduler_suspended == 0);
        OCTOS_YIELD();
    } else
        task_release(tcb);
}

/**
 * @brief Release a task and free its resources if necessary
 * @note The task is removed from the terminated list and its dynamic memory is freed if applicable
 * @param tcb Pointer to the TCB of the task to be released
 * @return None
 */
void task_release(TCB_t *tcb) {
    OCTOS_ASSERT(tcb->StateListItem.Parent == &terminated_list);
    list_remove(&(tcb->StateListItem));
    if (tcb->Page.policy == PAGE_POLICY_DYNAMIC) OCTOS_FREE(tcb->Page.raw);
}

/* Task Core Operation -------------------------------------------------------*/

/** 
 * @brief Increment the task tick and handle delayed tasks
 * @return True if a context switch is required, false otherwise
 */
bool task_tick_increment(void) {
    bool switch_required = false;

    if (scheduler_suspended > 0) {
        pended_ticks++;
    } else {
        current_tick++;
        const uint32_t const_tick = current_tick;

        /* Tick overflow, switch delayed list */
        if (const_tick == 0) {
            tick_overflows++;
            List_t *temp;
            temp = delayed_list;
            delayed_list = delayed_list_overflow;
            delayed_list_overflow = temp;
            reset_next_task_unblock_tick();
        }

        /* Wake delayed task if needed */
        if (const_tick >= next_task_unblock_tick) {
            while (true) {
                if (delayed_list->Length == 0) {
                    next_task_unblock_tick = UINT32_MAX;
                    break;
                }

                ListItem_t *head = list_head(delayed_list);
                uint32_t head_item_value = head->Value;

                if (const_tick < head_item_value) {
                    next_task_unblock_tick = head_item_value;
                    break;
                }

                TCB_t *head_owner = head->Owner;
                switch_required |= task_remove_from_delayed_list(head_owner);
                list_remove(&(head_owner->EventListItem));
                task_add_to_ready_list(head_owner);
            }
        }

        /* Round robin within same priority */
        switch_required |= current_tcb->StateListItem.Parent->Length > 1;

        /* Yield pending */
        switch_required |= yield_pending;
    }

    return switch_required;
}

/** 
 * @brief Perform a context switch to the highest priority task
 * @return None
 */
void task_context_switch(void) {
    if (scheduler_suspended > 0) {
        yield_pending = true;
    } else {
        yield_pending = false;
        task_select_highest_priority();
    }
}

/**
 * @brief Safely retrieves the current system tick value
 * @note Uses critical section to prevent race conditions
 * @return Current system tick value
 */
uint32_t task_get_tick(void) {
    OCTOS_ENTER_CRITICAL();
    const uint32_t return_tick = current_tick;
    OCTOS_EXIT_CRITICAL();
    return return_tick;
}

/**
 * @brief Safely retrieves the current system tick value from an Interrupt Service Routine (ISR)
 * @note Uses ISR-specific critical section handling
 * @return Current system tick value
 */
uint32_t task_get_tick_from_isr(void) {
    uint32_t saved_intr_status = OCTOS_ENTER_CRITICAL_FROM_ISR();
    uint32_t return_tick = current_tick;
    OCTOS_EXIT_CRITICAL_FROM_ISR(saved_intr_status);
    return return_tick;
}

/* Task Basic Operation ------------------------------------------------------*/

/** 
 * @brief Yield the current task to allow other tasks to run
 * @note If the scheduler is suspended, the yield will be pending until the scheduler is resumed
 * @return None
 */
void task_yield(void) {
    if (yield_pending) return;

    if (scheduler_suspended > 0) {
        yield_pending = true;
    } else {
        OCTOS_YIELD();
    }
}

/** 
 * @brief Yield the current task from an ISR context
 * @note If the scheduler is suspended, the yield will be pending until the scheduler is resumed
 * @param flag Indicates whether a yield is required
 * @return None
 */
void task_yield_from_isr(bool flag) {
    if (yield_pending || !flag) return;

    if (scheduler_suspended > 0) {
        yield_pending = flag;
    } else {
        OCTOS_YIELD_FROM_ISR(flag);
    }
}

/** 
 * @brief Suspend the scheduler to prevent task switching
 * @note This function increments the scheduler suspension counter
 * @return None
 */
void task_suspend_all(void) {
    OCTOS_ENTER_CRITICAL();
    scheduler_suspended++;
    OCTOS_EXIT_CRITICAL();
}

/** 
 * @brief Resume the scheduler and process any pending ticks
 * @note This function decrements the scheduler suspension counter and processes any pending ticks
 * @return None
 */
bool task_resume_all(void) {
    bool already_yielded = false;

    OCTOS_ENTER_CRITICAL();

    OCTOS_ASSERT(scheduler_suspended > 0);
    scheduler_suspended--;
    if (scheduler_suspended > 0) {
        OCTOS_EXIT_CRITICAL();
        return false;
    }

    while (pending_ready_list.Length > 0) {
        /* EventListItem related operations */
        ListItem_t *const item = list_head(&pending_ready_list);
        list_remove(item);

        TCB_t *const owner = item->Owner;

        /* StateListItem related operations */
        list_remove(&(owner->StateListItem));
        task_add_to_ready_list(owner);

        yield_pending |= owner->Priority > current_tcb->Priority;
    }

    while (pended_ticks > 0) {
        yield_pending |= task_tick_increment();
        pended_ticks--;
    }

    already_yielded = yield_pending;

    OCTOS_EXIT_CRITICAL();

    if (already_yielded) OCTOS_YIELD();

    return already_yielded;
}

/**
 * @brief Delay the current task for a specified number of ticks
 * @param ticks_to_delay Number of ticks to delay the task
 * @return None
 */
void task_delay(uint32_t ticks_to_delay) {
    OCTOS_ASSERT(ticks_to_delay < UINT32_MAX);

    bool already_yielded = false;

    /* ISR cannot modify delayed_list, so we use scheduler suspension here */
    task_suspend_all();

    OCTOS_ASSERT(scheduler_suspended == 1);
    /* When scheduler is suspended, ISR cannot modify StateListItem, so
     * it is safe to proceed without critical section */
    task_remove_and_add_current_to_delayed_list(ticks_to_delay);

    already_yielded = task_resume_all();

    if (!already_yielded) OCTOS_YIELD();
}

/**
 * @brief Abort the delay of a task
 * @param tcb Pointer to the TCB of the task to abort the delay
 * @return None
 */
void task_abort_delay(TCB_t *tcb) {
    /* ISR cannot modify delayed_list, so we use scheduler suspension here */
    task_suspend_all();

    List_t *const parent = tcb->StateListItem.Parent;
    if (parent == delayed_list || parent == delayed_list_overflow) {
        /* When scheduler is suspended, ISR cannot modify StateListItem, so
         * it is safe to proceed without critical section */
        yield_pending |= task_remove_from_delayed_list(tcb);

        /* If possible, remove task from its event list, critical section
         * is needed beacause ISR might modify EventListItem */
        OCTOS_ENTER_CRITICAL();
        list_remove(&(tcb->EventListItem));
        OCTOS_EXIT_CRITICAL();

        task_add_to_ready_list(tcb);
    }

    task_resume_all();
}

/**
 * @brief Suspend a task from executing
 * @note The task will be moved to the suspended list and a context switch may occur
 * @param tcb Pointer to the TCB of the task to be suspended
 * @return None
 */
void task_suspend(TCB_t *tcb) {
    bool switch_required = false;

    /* ISR can modify suspended_list, so we use critical section 
     * instead of scheduler suspension */
    OCTOS_ENTER_CRITICAL();

    const TaskState_t status = task_status(tcb);
    if (status != READY && status != RUNNING) {
        OCTOS_EXIT_CRITICAL();
        return;
    }

    /* Remove task from ready/delayed list and insert it into
     * suspended list */
    if (list_remove(&(tcb->StateListItem))) {
        task_reset_ready_priority(tcb->Priority);
        reset_next_task_unblock_tick();
    }
    list_insert_end(&suspended_list, &(tcb->StateListItem));

    /* Remove task from event list (if possible) */
    list_remove(&(tcb->EventListItem));

    switch_required = tcb == current_tcb;
    if (switch_required && scheduler_suspended > 0) {
        yield_pending = true;
        switch_required = false;
    }

    OCTOS_EXIT_CRITICAL();

    if (switch_required) OCTOS_YIELD();
}

/** 
 * @brief Resume a suspended task
 * @note If the resumed task has a higher priority than the current task, a yield will occur
 * @param tcb Pointer to the TCB of the task to be resumed
 * @return None
 */
void task_resume(TCB_t *tcb) {
    bool switch_required = false;

    /* ISR can modify suspended_list, so we use critical section 
     * instead of scheduler suspension */
    OCTOS_ENTER_CRITICAL();

    if (task_status(tcb) != SUSPENDED) {
        OCTOS_EXIT_CRITICAL();
        return;
    }

    /* No need to consider scheduler suspend, because ISR cannot 
     * modify StateListItem, and we are in a critical section */
    switch_required = task_remove_from_delayed_list(tcb);
    task_add_to_ready_list(tcb);

    if (switch_required && scheduler_suspended > 0) {
        yield_pending = true;
        switch_required = false;
    }

    OCTOS_EXIT_CRITICAL();

    if (switch_required) OCTOS_YIELD();
}

/** 
 * @brief Resume a suspended task from an ISR context
 * @note If the resumed task has a higher priority than the current task, a yield will occur
 * @param tcb Pointer to the TCB of the task to be resumed
 * @return None
 */
void task_resume_from_isr(TCB_t *tcb) {
    OCTOS_ASSERT_IF_INTERRUPT_PRIORITY_INVALID();

    bool switch_required = false;

    /* ISR can modify suspended_list, so we use critical section 
     * instead of scheduler suspension */
    uint32_t saved_intr_status = OCTOS_ENTER_CRITICAL_FROM_ISR();

    if (task_status(tcb) != SUSPENDED) {
        OCTOS_EXIT_CRITICAL_FROM_ISR(saved_intr_status);
        return;
    }

    if (scheduler_suspended == 0) {
        switch_required = task_remove_from_delayed_list(tcb);
        task_add_to_ready_list(tcb);
    } else {
        /* No need to set yield_pending here, task_resume_all will do it
         * for us while resuming the scheduler */
        list_insert_end(&pending_ready_list, &(tcb->EventListItem));
    }

    OCTOS_EXIT_CRITICAL_FROM_ISR(saved_intr_status);

    OCTOS_YIELD_FROM_ISR(switch_required);
}
