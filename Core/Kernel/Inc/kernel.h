#ifndef __KERNEL_H__
#define __KERNEL_H__

#include "Kernel/Inc/utils.h"
#include <stdint.h>

extern uint32_t current_tick;

void kernel_launch(Quanta_t *quanta);
void kernel_tick_increment(void);
OCTOS_INLINE static inline uint32_t kernel_get_tick(void) {
    return current_tick;
}

#endif
