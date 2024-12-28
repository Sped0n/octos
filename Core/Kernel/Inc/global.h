#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <stddef.h>

#define MAX_TASKS 16

#define MICROS_FLATTEN __attribute__((flatten))
#define MICROS_INLINE __attribute__((always_inline))
#define MICROS_NO_INLINE __attribute__((noinline))
#define MICROS_NAKED __attribute__((naked))
#define MICROS_USED __attribute__((used))
#define MICROS_PACKED __attribute__((packed))

#endif
