#include "kernel.h"
#include "global.h"
#include "scheduler.h"
#include "stm32f429xx.h"
#include "stm32f4xx.h" // IWYU pragma: keep
#include <stdint.h>

void kernel_launch(uint32_t quanta_in_ms) {
  /* Reset systick */
  SysTick->CTRL = 0;
  /* Clear systick current value register */
  SysTick->VAL = 0;
  /* Load quanta */
  SysTick->LOAD = (quanta_in_ms * (BUS_FREQ / 1000)) - 1;
  /* Set systick to low priority */
  NVIC_SetPriority(SysTick_IRQn, 15);
  /* Enable systick, select internal clock */
  SysTick->CTRL |= SYST_CLCKSRC_INTERNAL | SYST_ENABLE | SYST_TICKINT;
  /* Launch scheduler */
  scheduler_launch();
}
