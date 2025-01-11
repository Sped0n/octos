#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "Arch/stm32f4xx/Inc/api.h"
#include "kernel.h"
#include "list.h"
#include "page.h"
#include "task.h"
#include "tcb.h"

extern TCB_t *current_tcb;
extern List_t *ready_list;
extern List_t *pending_ready_list;
extern List_t *delayed_list;
extern List_t *delayed_list_overflow;
extern List_t *suspended_list;
extern List_t *terminated_list;


/**
  * @brief Creates a new task with specified parameters
  * @param func Function pointer to the task entry
  * @param args Arguments passed to the task function
  * @param priority Task priority level
  * @param page_policy Memory page allocation policy
  * @param page_size Size of memory page to allocate
  * @retval true if task created successfully
  * @retval false if task creation failed
  */
bool task_create(TaskFunc_t func, void *args, uint8_t priority, PagePolicy_t page_policy,
                 size_t page_size) {
    OCTOS_ENTER_CRITICAL();

    Page_t page;
    page_alloc(&page, page_policy, page_size);
    if (!page.raw) {
        OCTOS_EXIT_CRITICAL();
        return false;
    }

    TCB_t *tcb = tcb_build(&page, func, args, priority);

    if (tcb->TCBNumber == 0) {
        current_tcb = tcb;
        list_init(ready_list);
    } else {
        list_insert(ready_list, &(tcb->StateListItem));
    }

    OCTOS_EXIT_CRITICAL();
    return true;
}

/**
  * @brief Deletes specified task and moves it to terminated state
  * @note A context switch may occur
  * @param tcb Pointer to task control block to delete
  * @retval None
  */
void task_delete(TCB_t *tcb) {
    bool need_yield = false;

    OCTOS_ENTER_CRITICAL();

    if (!tcb || tcb_status(tcb) == TERMINATED) {
        OCTOS_EXIT_CRITICAL();
        return;
    }

    if (tcb_status(tcb) != RUNNING) {
        list_remove(&(tcb->StateListItem));
    }
    list_insert_end(terminated_list, &(tcb->StateListItem));
    need_yield = tcb == current_tcb;

    OCTOS_EXIT_CRITICAL();

    if (need_yield) task_yield();
}

/*
  * @brief Delays current task for specified number of ticks
  * @param ticks_to_delay Number of ticks to delay task
  * @retval None
  */
void task_delay(uint32_t ticks_to_delay) {
    OCTOS_ENTER_CRITICAL();

    const uint32_t current_tick = kernel_get_tick();
    const uint32_t tick_to_wake = current_tick + ticks_to_delay;

    list_item_set_value(&(current_tcb->StateListItem), tick_to_wake);

    if (tick_to_wake < current_tick) {
        // Overflow will occur, add to overflow list
        list_insert(delayed_list_overflow, &(current_tcb->StateListItem));
    } else {
        // Add to normal delayed list
        list_insert(delayed_list, &(current_tcb->StateListItem));
    }

    OCTOS_EXIT_CRITICAL();

    task_yield();
}

/**
  * @brief Suspend a task from executing
  * @note The task will be moved to the suspended list and a context switch may occur
  * @param tcb Pointer to the TCB of the task to be suspended
  * @retval None
  */
void task_suspend(TCB_t *tcb) {
    bool need_yield = false;

    OCTOS_ENTER_CRITICAL();

    list_insert(suspended_list, &(tcb->StateListItem));
    need_yield = tcb == current_tcb;

    OCTOS_EXIT_CRITICAL();

    if (need_yield) task_yield();
}
