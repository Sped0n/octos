#ifndef __TASK_H__
#define __TASK_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "attr.h"
#include "tcb.h"

/**
  * @brief Function pointer type for tasks that can be executed by the scheduler
  * @param args Pointer to the task's arguments
  */
typedef void (*TaskFunc_t)(void *args);

/**
  * @brief Thread states enumeration
  */
typedef enum TaskState {
    READY,      /*!< Thread is ready to run */
    RUNNING,    /*!< Thread is currently running */
    BLOCKED,    /*!< Thread is blocked (sleeping or waiting for resource) */
    SUSPENDED,  /*!< Thread is suspended */
    TERMINATED, /*!< Thread has terminated */
    INVALID     /*!< Invalid */
} TaskState_t;

extern TCB_t *volatile current_tcb;

/* Misc ----------------------------------------------------------------------*/
TaskState_t task_status(TCB_t *tcb);
void task_set_timeout(Timeout_t *timeout);
bool task_check_timeout(Timeout_t *timeout, uint32_t ticks_to_delay);
uint8_t task_get_number_of_tasks(void);
/* Task List -----------------------------------------------------------------*/
void task_lists_init(void);
void task_add_to_ready_list(TCB_t *tcb);
void task_remove_and_add_current_to_delayed_list(uint32_t ticks_to_delay);
bool task_remove_from_delayed_list(TCB_t *tcb);
void task_add_current_to_event_list(List_t *list, uint32_t ticks_to_wait);
bool task_remove_highest_priority_from_event_list(List_t *list);
/* Task Create and Delete ----------------------------------------------------*/
bool task_create(TaskFunc_t func, void *args, uint8_t priority,
                 size_t page_size_in_words);
bool task_create_static(TaskFunc_t func, void *args, uint8_t priority,
                        uint32_t *buffer, size_t page_size_in_words);
void task_delete(TCB_t *tcb);
void task_release(TCB_t *tcb);
/* Task Core Operation -------------------------------------------------------*/
bool task_tick_increment(void);
void task_context_switch(void);
uint32_t task_get_tick(void);
uint32_t task_get_tick_from_isr(void);
/* Task Basic Operation ------------------------------------------------------*/
void task_yield(void);
void task_yield_from_isr(bool flag);
void task_suspend_all(void);
bool task_resume_all(void);
void task_delay(uint32_t ticks_to_delay);
void task_abort_delay(TCB_t *tcb);
void task_suspend(TCB_t *tcb);
void task_resume(TCB_t *tcb);
void task_resume_from_isr(TCB_t *tcb);


/**
  * @brief Get the current running task's TCB
  * @return Pointer to the current task's TCB structure
  */
OCTOS_INLINE static inline TCB_t *task_get_current(void) { return current_tcb; }

#endif
