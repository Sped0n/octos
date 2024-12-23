#include "kernel.h"
#include "global.h"
#include "scheduler.h"
#include "stm32f4xx.h" // IWYU pragma: keep
#include "system_stm32f4xx.h"
#include <stdint.h>

void kernel_launch(uint32_t quanta_in_ms) {
  /* Reset systick */
  SysTick->CTRL = 0;
  /* Clear systick current value register */
  SysTick->VAL = 0;
  /* Load quanta */
  SysTick->LOAD = (quanta_in_ms * (SystemCoreClock / 1000)) - 1;
  /* Set priority for interrupts (Max 0, Min 15) */
  NVIC_SetPriority(SysTick_IRQn, 0);
  NVIC_SetPriority(PendSV_IRQn, 15);
  NVIC_SetPriority(SVCall_IRQn, 14);
  /* Enable systick, select internal clock */
  SysTick->CTRL |= SYST_CLCKSRC_INTERNAL | SYST_ENABLE | SYST_TICKINT;
  /* Launch scheduler */
  scheduler_launch();
}
