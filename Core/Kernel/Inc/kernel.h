#ifndef __KERNEL_H__
#define __KERNEL_H__

#include <stdint.h>

#include "Kernel/Inc/utils.h"

extern volatile uint32_t current_tick;

void kernel_launch(Quanta_t *quanta);
void kernel_tick_increment(void);

/**
  * @brief Get the current system tick value
  * @retval Current system tick counter value
  */
OCTOS_INLINE static inline uint32_t kernel_get_tick(void) {
    return current_tick;
}

#endif
