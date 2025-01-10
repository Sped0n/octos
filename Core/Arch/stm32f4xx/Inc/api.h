#ifndef __ARCH_STM32F4xx_API_H__
#define __ARCH_STM32F4xx_API_H__

#include <stdint.h>

#include "Kernel/Inc/utils.h"
#include "stm32f4xx.h"// IWYU pragma: keep

#define OCTOS_MAX_SYSCALL_INTERRUPT_PRIORITY 14
#define OCTOS_DSB() __DSB()
#define OCTOS_ISB() __ISB()

extern uint32_t critical_nesting;

OCTOS_NAKED void OCTOS_SCHED_LAUNCH(void);
void OCTOS_SETUP_INTPRI(void);
void OCTOS_SETUP_SYSTICK(Quanta_t *quanta);
void OCTOS_ENABLE_SYSTICK(void);

/**
  * @brief Enter critical section by setting BASEPRI register to mask interrupts
  * @note Uses BASEPRI register to disable interrupts with priority less than or equal to 
  *       OCTOS_MAX_SYSCALL_INTERRUPT_PRIORITY
  * @note Priority levels are mapped to bits [7:4] according to Cortex-M4 architecture
  * @retval None
  */
OCTOS_INLINE static inline void OCTOS_ENTER_CRITICAL(void) {
    // according to
    // * Cortex-M4 Devices Generic User Guide (Page 2-9), bits [31:8] are reserved
    // * RM0090 Rev 21 (Page 374), 4 bits of interrupt priority (16 levels) are used
    // the priority levels are map to bit [7:4]
    __set_BASEPRI(OCTOS_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - __NVIC_PRIO_BITS));
    __DSB();
    __ISB();
    critical_nesting++;
}

/**
  * @brief Exit critical section by restoring original BASEPRI value
  * @note Decrements critical nesting counter and clears BASEPRI when counter reaches 0
  * @retval None
  */
OCTOS_INLINE static inline void OCTOS_EXIT_CRITICAL(void) {
    critical_nesting--;
    if (critical_nesting == 0) {
        __set_BASEPRI(0);
    }
}

/**
  * @brief Set interrupt mask from ISR context by configuring BASEPRI register
  * @note Uses BASEPRI register to disable interrupts with priority less than or equal to
  *       OCTOS_MAX_SYSCALL_INTERRUPT_PRIORITY
  * @note Priority levels are mapped to bits [7:4] according to Cortex-M4 architecture  
  * @retval Original BASEPRI value before masking
  */
OCTOS_INLINE static inline uint32_t OCTOS_SET_INT_MASK_FROM_ISR(void) {
    uint32_t original_base_priority = __get_BASEPRI();
    // according to
    // * Cortex-M4 Devices Generic User Guide (Page 2-9), bits [31:8] are reserved
    // * RM0090 Rev 21 (Page 374), 4 bits of interrupt priority (16 levels) are used
    // the priority levels are map to bit [7:4]
    __set_BASEPRI(OCTOS_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - __NVIC_PRIO_BITS));
    __DSB();
    __ISB();
    return original_base_priority;
}

/**
  * @brief Clear interrupt mask from ISR context
  * @param new_mask_value New BASEPRI value to restore
  * @retval None
  */
OCTOS_INLINE static inline void OCTOS_CLEAR_INT_MASK_FROM_ISR(uint32_t new_mask_value) {
    __set_BASEPRI(new_mask_value);
}

/**
  * @brief Trigger PendSV exception to perform context switch
  * @note Sets PENDSVSET bit in ICSR register to trigger PendSV exception
  * @retval None
  */
OCTOS_INLINE static inline void OCTOS_CTX_SWITCH(void) {
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

#endif
