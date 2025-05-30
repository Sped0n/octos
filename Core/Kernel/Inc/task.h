#ifndef __TASK_H__
#define __TASK_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "attr.h"
#include "list.h"
#include "page.h"

/* For zero struct padding */
#define TCB_NAME_MAX_LENGTH 12

/**
  * @brief Function pointer type for tasks that can be executed by the scheduler
  * @param args Pointer to the task's arguments
  */
typedef void (*TaskFunc_t)(void *args);

/**
  * @brief Thread states enumeration
  */
typedef enum OCTOS_PACKED TaskState {
    READY,      /*!< Thread is ready to run */
    RUNNING,    /*!< Thread is currently running */
    BLOCKED,    /*!< Thread is blocked (sleeping or waiting for resource) */
    SUSPENDED,  /*!< Thread is suspended */
    TERMINATED, /*!< Thread has terminated */
    INVALID     /*!< Invalid */
} TaskState_t;

/**
 * @brief Task notification state enumeration
 */
typedef enum OCTOS_PACKED TaskNotifyState {
    IDLE,    /*!< Task is idle and not waiting for a notification */
    PENDING, /*!< Task is pending and waiting for a notification */
    RECEIVED /*!< Task has received a notification */
} TaskNotifyState_t;

/**
 * @brief Task notification action enumeration
 */
typedef enum OCTOS_PACKED TaskNotifyAction {
    NoAction,  /*!< No action is taken on the notification value */
    BitwiseOr, /*!< Perform a bitwise OR operation on the notification value */
    Increment, /*!< Increment the notification value */
    OverwriteSet, /*!< Overwrite the notification value */
    TrySet /*!< Attempt to set the notification value if not already received */
} TaskNotifyAction_t;

/**
 * @brief Task Control Block structure definition
 */
typedef struct TCB {
    uint32_t *StackTop;             /*!< Pointer to the top of task's stack */
    Page_t Page;                    /*!< Memory page allocated for this task */
    ListItem_t StateListItem;       /*!< List item for thread state lists */
    ListItem_t EventListItem;       /*!< List item for event waiting lists */
    uint8_t RootPriority;           /*!< Original priority of the thread */
    uint8_t Priority;               /*!< Current priority of the thread */
    uint8_t MutexHeld;              /*!< Current number of mutexes held */
    char Name[TCB_NAME_MAX_LENGTH]; /*!< Task name */
    TaskNotifyState_t
            NotifyState;    /*!< Current notification state of the task */
    uint32_t NotifiedValue; /*!< Value associated with the notification */
    uint32_t TCBNumber;     /*!< Unique identifier for the thread */
} TCB_t;

/**
 * @brief Task Handle
 */
typedef TCB_t *TaskHandle_t;

/**
 * @brief Task Info
 */
typedef struct {
    char name[TCB_NAME_MAX_LENGTH]; /*!< Task name */
    uint8_t priority;               /*!< Task priority */
    TaskState_t status;             /*!< Task status */
} TaskInfo_t;

/* Misc ----------------------------------------------------------------------*/
TaskState_t task_status(TaskHandle_t handle);
void task_set_timeout(Timeout_t *timeout);
bool task_check_timeout(Timeout_t *timeout, uint32_t ticks_to_delay);
uint8_t task_get_number_of_tasks(void);
bool task_get_info(TaskHandle_t handle, TaskInfo_t *info);
void task_info_list(char *buffer);
/* Task List -----------------------------------------------------------------*/
void task_lists_init(void);
void task_add_to_ready_list(TaskHandle_t handle);
void task_remove_and_add_current_to_delayed_list(uint32_t ticks_to_delay);
bool task_remove_from_delayed_list(TaskHandle_t handle);
void task_add_current_to_event_list(List_t *list, uint32_t ticks_to_wait);
bool task_remove_highest_priority_from_event_list(List_t *list);
/* Task Create and Delete ----------------------------------------------------*/
bool task_create(TaskFunc_t func, void *const args, const char *name,
                 uint8_t priority, size_t page_size_in_words,
                 TaskHandle_t *handle);
bool task_create_static(TaskFunc_t func, void *args, const char *name,
                        uint8_t priority, uint32_t *buffer,
                        size_t page_size_in_words, TaskHandle_t *handle);
void task_delete(TaskHandle_t handle);
void task_release(TaskHandle_t handle);
/* Task Core Operation -------------------------------------------------------*/
bool task_tick_increment(void);
void task_context_switch(void);
uint32_t task_get_tick(void);
uint32_t task_get_tick_from_isr(void);
TCB_t *task_mutex_held_increment(void);
bool task_mutex_held_decrement(TaskHandle_t mutex_owner);
bool task_inherit_priority(TaskHandle_t mutex_owner);
bool task_deinherit_priority(TaskHandle_t mutex_owner);
void task_deinherit_priority_after_timeout(
        TaskHandle_t mutex_owner, uint8_t highest_priority_of_waiting_tasks);
/* Task Basic Operation ------------------------------------------------------*/
void task_yield(void);
void task_yield_from_isr(bool flag);
void task_suspend_all(void);
bool task_resume_all(void);
void task_delay(uint32_t ticks_to_delay);
void task_abort_delay(TaskHandle_t handle);
void task_suspend(TaskHandle_t handle);
void task_resume(TaskHandle_t handle);
void task_resume_from_isr(TaskHandle_t handle);
bool task_notify(TaskHandle_t handle, uint32_t value,
                 TaskNotifyAction_t action);
bool task_notify_from_isr(TaskHandle_t handle, uint32_t value,
                          TaskNotifyAction_t action,
                          bool *const switch_required);
bool task_notify_wait(uint32_t bits_to_clear_on_entry,
                      uint32_t bits_to_clear_on_exit, uint32_t *buffer,
                      uint32_t timeout_ticks);

#endif
