#include "timebase.h"

#define ONE_SEC_LOAD 16000000 // Clock tree is not configured yet

#define SYST_CSR_ENABLE (1u << 0)
#define SYST_CSR_TICKINT (1u << 1)
#define SYST_CSR_CLKSRC (1u << 2)
#define SYST_CSR_CNTFLAG (1u << 16)

volatile uint32_t g_curr_tick;
volatile uint32_t g_curr_tick_p;
volatile uint32_t tick_freq = 1;

void tick_increment(void) { g_curr_tick += tick_freq; }

uint32_t get_tick(void) {
  __disable_irq();
  g_curr_tick_p = g_curr_tick;
  __enable_irq();

  return g_curr_tick_p;
}

void delay(uint32_t seconds) {
  uint32_t tickstart = get_tick();
  uint32_t wait = seconds;
  if (wait > 0xFFFFFFFU) {
    wait += (uint32_t)(tick_freq);
  }
  while ((get_tick() - tickstart) < seconds)
    ;
}

void timebase_init(void) {
  /* Reload the timer with number of cycles per second */
  SysTick->LOAD = ONE_SEC_LOAD - 1;
  /* Clear Systick current value register */
  SysTick->VAL = 0;
  /* Systick control flag */
  SysTick->CTRL = SYST_CSR_ENABLE | SYST_CSR_TICKINT | SYST_CSR_CLKSRC;
  /* Enbale global interrupt */
  __enable_irq();
}
