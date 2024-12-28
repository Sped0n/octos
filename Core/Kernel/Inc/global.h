#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <stddef.h>

#define MAX_TASKS 16

typedef enum {
    MILISECONDS = 1000,
    MICROSECONDS = 1000000
} QuantaUnit_t;

typedef struct Quanta {
    QuantaUnit_t Unit;
    size_t Value;
} Quanta_t;


#endif
