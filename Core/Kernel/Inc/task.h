#ifndef __TASK_H__
#define __TASK_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "Arch/stm32f4xx/Inc/api.h"
#include "attr.h"
#include "tcb.h"

extern TCB_t *current_tcb;
extern volatile bool yield_pending;
extern volatile uint32_t scheduler_suspended;

bool task_create(TaskFunc_t func, void *args, uint8_t priority,
                 size_t page_size_in_words);
bool task_create_static(TaskFunc_t func, void *args, uint8_t priority,
                        uint32_t *buffer, size_t page_size_in_words);
void task_delete(TCB_t *tcb);
void task_release(TCB_t *tcb);
void task_delay(uint32_t ticks_to_delay);
void task_suspend(TCB_t *tcb);
void task_resume(TCB_t *tcb);
void task_resume_from_isr(TCB_t *tcb);
void task_resume_all(void);

/**
  * @brief Get the current running task's TCB
  * @retval Pointer to the current task's TCB structure
  */
OCTOS_INLINE static inline TCB_t *task_get_current_tcb(void) {
    return current_tcb;
}

/** 
  * @brief Yield the current task to allow other tasks to run
  * @note If the scheduler is suspended, the yield will be pending until the scheduler is resumed
  * @retval None
  */
OCTOS_INLINE static inline void task_yield(void) {
    OCTOS_ENTER_CRITICAL();

    if (yield_pending) {
        OCTOS_EXIT_CRITICAL();
        return;
    }

    if (scheduler_suspended > 0) {
        yield_pending = true;
    } else {
        OCTOS_YIELD();
    }

    OCTOS_EXIT_CRITICAL();
}

/** 
  * @brief Yield the current task from an ISR context
  * @note If the scheduler is suspended, the yield will be pending until the scheduler is resumed
  * @param flag Indicates whether a yield is required
  * @retval None
  */
OCTOS_INLINE static inline void task_yield_from_isr(bool flag) {
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
  * @retval None
  */
OCTOS_INLINE static inline void task_suspend_all(void) {
    uint32_t saved_intr_status = OCTOS_ENTER_CRITICAL_FROM_ISR();
    scheduler_suspended++;
    OCTOS_EXIT_CRITICAL_FROM_ISR(saved_intr_status);
}

#endif
