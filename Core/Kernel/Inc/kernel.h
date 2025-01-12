#ifndef __KERNEL_H__
#define __KERNEL_H__

#include <stdint.h>

#include "Arch/stm32f4xx/Inc/api.h"
#include "Kernel/Inc/utils.h"

extern volatile uint32_t current_tick;

void kernel_launch(Quanta_t *quanta);
void kernel_tick_increment(void);

/**
 * @brief Safely retrieves the current system tick value
 * @note Uses critical section to prevent race conditions
 * @retval uint32_t Current system tick value
 */
OCTOS_INLINE static inline uint32_t kernel_get_tick(void) {
    OCTOS_ENTER_CRITICAL();
    uint32_t return_tick = current_tick;
    OCTOS_EXIT_CRITICAL();
    return return_tick;
}

/**
 * @brief Safely retrieves the current system tick value from an Interrupt Service Routine (ISR)
 * @note Uses ISR-specific critical section handling
 * @retval uint32_t Current system tick value
 */
OCTOS_INLINE static inline uint32_t kernel_get_tick_from_isr(void) {
    uint32_t saved_intr_status = OCTOS_ENTER_CRITICAL_FROM_ISR();
    uint32_t return_tick = current_tick;
    OCTOS_EXIT_CRITICAL_FROM_ISR(saved_intr_status);
    return return_tick;
}

#endif
