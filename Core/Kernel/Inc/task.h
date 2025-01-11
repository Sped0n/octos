#ifndef __TASK_H__
#define __TASK_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "Kernel/Inc/utils.h"
#include "page.h"
#include "tcb.h"

extern TCB_t *current_tcb;

bool task_create(TaskFunc_t func, void *args, uint8_t priority, PagePolicy_t page_policy,
                 size_t page_size);
void task_delete(TCB_t *tcb);
void task_delay(uint32_t ticks_to_delay);
void task_suspend(TCB_t *tcb);

/**
  * @brief Get the current running task's TCB
  * @retval Pointer to the current task's TCB structure
  */
OCTOS_INLINE static inline TCB_t *task_get_current_tcb(void) {
    return current_tcb;
}

/**
  * @brief Forces a context switch and ensures memory barriers
  * @note Executes context switch followed by data sync and instruction sync barriers
  * @retval None
  */
OCTOS_INLINE static inline void task_yield(void) {
    OCTOS_CTX_SWITCH();
    OCTOS_DSB();
    OCTOS_ISB();
}

/**
  * @brief Performs a task yield from ISR context if the flag is set
  * @param flag Boolean flag indicating whether to perform context switch
  * @retval None
  */
OCTOS_INLINE static inline void task_yield_from_isr(bool flag) {
    if (flag)
        OCTOS_CTX_SWITCH();
}

#endif
