#ifndef __KERNEL_UTILS_H__
#define __KERNEL_UTILS_H__

#include <stddef.h>
#include <stdint.h>

#define MICROS_FLATTEN __attribute__((flatten))
#define MICROS_INLINE __attribute__((always_inline))
#define MICROS_NO_INLINE __attribute__((noinline))
#define MICROS_NAKED __attribute__((naked))
#define MICROS_USED __attribute__((used))
#define MICROS_PACKED __attribute__((packed))

typedef enum {
    SECONDS = 1,
    MILISECONDS = 1000,
    MICROSECONDS = 1000000
} QuantaUnit_t;

typedef struct Quanta {
    QuantaUnit_t Unit;
    size_t Value;
} Quanta_t;

extern Quanta_t *kernel_quanta;

MICROS_INLINE static inline uint32_t time_to_ticks(uint16_t time, QuantaUnit_t unit) {
    return (time * kernel_quanta->Unit) / (kernel_quanta->Value * unit);
}

#endif
