#ifndef __TASK_H__
#define __TASK_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "Kernel/Inc/utils.h"
#include "page.h"
#include "tcb.h"

bool task_create(TaskFunc_t func, void *args, uint8_t priority, PagePolicy_t page_policy,
                 size_t page_size);
void task_delete(TCB_t *tcb);
void task_terminate(void);
void task_delay(uint32_t ticks_to_delay);
void task_suspend(void);

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
