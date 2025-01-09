#ifndef __ARCH_STM32F4xx_API_H__
#define __ARCH_STM32F4xx_API_H__

#include "Kernel/Inc/utils.h"
#include "stm32f4xx.h"// IWYU pragma: keep
#include <stdint.h>

#define OCTOS_MAX_SYSCALL_INTERRUPT_PRIORITY 14
#define OCTOS_DSB() __DSB()
#define OCTOS_ISB() __ISB()

extern uint32_t interrupt_state;

OCTOS_INLINE static inline void OCTOS_ENTER_CRITICAL(void) {
    interrupt_state = __get_BASEPRI();
    __set_BASEPRI(OCTOS_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - __NVIC_PRIO_BITS));
    __DSB();
    __ISB();
}

OCTOS_INLINE static inline void OCTOS_EXIT_CRITICAL(void) {
    __set_BASEPRI(interrupt_state);
    __DSB();
    __ISB();
}

OCTOS_INLINE static inline void CTX_SWITCH(void) {
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

OCTOS_NAKED void scheduler_launch(void);
void setup_interrupt_priority(void);
void setup_systick(Quanta_t *quanta);
void enable_systick(void);


#endif
