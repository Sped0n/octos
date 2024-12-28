#ifndef __KERNEL_H__
#define __KERNEL_H__

#include <stddef.h>
#include <stdint.h>

typedef enum {
    MILISECONDS = 1000,
    MICROSECONDS = 1000000
} QuantaUnit_t;

typedef struct Quanta {
    QuantaUnit_t Unit;
    size_t Value;
} Quanta_t;

void kernel_launch(Quanta_t *quanta);
uint32_t kernel_get_current_tick(void);

#endif
