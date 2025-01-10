#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include "Arch/stm32f4xx/Inc/api.h"
#include "Kernel/Inc/utils.h"

void scheduler_rr(void);

/**
  * @brief Triggers a context switch in the scheduler
  * @note This function is inline and calls the context switch macro
  * @retval None
  */
OCTOS_INLINE static inline void scheduler_trigger(void) {
    OCTOS_CTX_SWITCH();
}

#endif
