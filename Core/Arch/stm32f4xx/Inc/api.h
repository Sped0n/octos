#ifndef __ARCH_STM32F4xx_API_H__
#define __ARCH_STM32F4xx_API_H__

#include "global.h"
#include "stm32f4xx.h"// IWYU pragma: keep

#define MICROS_ENABLE_IRQ() __enable_irq()
#define MICROS_DISABLE_IRQ() __disable_irq()
#define MICROS_TRIGGER_PENDSV() SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk

__attribute__((naked)) void scheduler_launch(void);
void setup_interrupt_priority(void);
void setup_systick(Quanta_t *quanta);
void enable_systick(void);

#endif
