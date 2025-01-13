#ifndef __ARCH_STM32F4xx_API_H__
#define __ARCH_STM32F4xx_API_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "Kernel/Inc/config.h"
#include "Kernel/Inc/utils.h"
#include "attr.h"

#include "stm32f4xx.h"// IWYU pragma: keep

#define OCTOS_DSB() __DSB()
#define OCTOS_ISB() __ISB()
#define OCTOS_ASSERT(x)                                                        \
    if ((x) == 0) OCTOS_ASSERT_CALLED(__FILE__, __LINE__)

extern volatile uint32_t critical_nesting;

OCTOS_NAKED void OCTOS_SCHED_LAUNCH(void);
void OCTOS_SETUP_INTPRI(void);
void OCTOS_SETUP_SYSTICK(Quanta_t *quanta);
void OCTOS_ENABLE_SYSTICK(void);
void OCTOS_ASSERT_CALLED(const char *file, uint64_t line);
void *OCTOS_MALLOC(size_t wanted_size);
void OCTOS_FREE(void *ptr_to_free);

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
    __set_BASEPRI(OCTOS_MAX_SYSCALL_INTERRUPT_PRIORITY
                  << (8 - __NVIC_PRIO_BITS));
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
    if (critical_nesting == 0) { __set_BASEPRI(0); }
}

/**
  * @brief Set interrupt mask from ISR context by configuring BASEPRI register
  * @note Uses BASEPRI register to disable interrupts with priority less than or equal to
  *       OCTOS_MAX_SYSCALL_INTERRUPT_PRIORITY
  * @note Priority levels are mapped to bits [7:4] according to Cortex-M4 architecture  
  * @retval Original BASEPRI value before masking
  */
OCTOS_INLINE static inline uint32_t OCTOS_ENTER_CRITICAL_FROM_ISR(void) {
    uint32_t original_base_priority = __get_BASEPRI();
    // according to
    // * Cortex-M4 Devices Generic User Guide (Page 2-9), bits [31:8] are reserved
    // * RM0090 Rev 21 (Page 374), 4 bits of interrupt priority (16 levels) are used
    // the priority levels are map to bit [7:4]
    __set_BASEPRI(OCTOS_MAX_SYSCALL_INTERRUPT_PRIORITY
                  << (8 - __NVIC_PRIO_BITS));
    __DSB();
    __ISB();
    return original_base_priority;
}

/**
  * @brief Clear interrupt mask from ISR context
  * @param new_mask_value New BASEPRI value to restore
  * @retval None
  */
OCTOS_INLINE static inline void
OCTOS_EXIT_CRITICAL_FROM_ISR(uint32_t new_mask_value) {
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

OCTOS_INLINE static inline void
OCTOS_ASSERT_IF_INTERRUPT_PRIORITY_INVALID(void) {
    uint32_t isr_number = __get_IPSR();
    uint32_t current_priority = NVIC_GetPriority((IRQn_Type) (isr_number - 16));
    OCTOS_ASSERT(current_priority >= OCTOS_MAX_SYSCALL_INTERRUPT_PRIORITY);
}

OCTOS_INLINE static inline void OCTOS_YIELD(void) {
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
    OCTOS_DSB();
    OCTOS_ISB();
}

OCTOS_INLINE static inline void OCTOS_YIELD_FROM_ISR(bool flag) {
    OCTOS_ASSERT_IF_INTERRUPT_PRIORITY_INVALID();
    if (flag) SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}


#endif
