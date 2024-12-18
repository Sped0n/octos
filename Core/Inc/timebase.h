#ifndef __TIMEBASE_H__
#define __TIMEBASE_H__

#include "stm32f4xx.h" // IWYU pragma: keep
#include <stdint.h>

void timebase_init(void);
void tick_increment(void);
uint32_t get_tick(void);
void delay(uint32_t delay);

#endif
