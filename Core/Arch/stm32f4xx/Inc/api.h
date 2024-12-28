#ifndef __ARCH_STM32F4xx_API_H__
#define __ARCH_STM32F4xx_API_H__

#include "global.h"
#include "kernel.h"
#include "stm32f4xx.h"// IWYU pragma: keep

#define MAX_SYSCALL_INTERRUPT_PRIORITY 14
#define MICROS_DSB() __DSB()
#define MICROS_ISB() __ISB()

static uint32_t interrupt_state;

MICROS_INLINE static inline void ENTER_CRITICAL(void) {
    interrupt_state = __get_BASEPRI();
    __set_BASEPRI(MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - __NVIC_PRIO_BITS));
    __DSB();
    __ISB();
}

MICROS_INLINE static inline void EXIT_CRITICAL(void) {
    __set_BASEPRI(interrupt_state);
    __DSB();
    __ISB();
}

MICROS_INLINE static inline void CTX_SWITCH(void) {
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

MICROS_NAKED void scheduler_launch(void);
void setup_interrupt_priority(void);
void setup_systick(Quanta_t *quanta);
void enable_systick(void);

#endif
