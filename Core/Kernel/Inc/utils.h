#ifndef __KERNEL_UTILS_H__
#define __KERNEL_UTILS_H__

#include <stddef.h>
#include <stdint.h>

#include "attr.h"

/**
  * @brief Time units enumeration for system timing operations
  */
typedef enum {
    SECONDS = 1,           /*!< Time unit in seconds */
    MILISECONDS = 1000,    /*!< Time unit in milliseconds */
    MICROSECONDS = 1000000 /*!< Time unit in microseconds */
} QuantaUnit_t;

/**
  * @brief System time quanta structure definition
  */
typedef struct Quanta {
    QuantaUnit_t Unit; /*!< Time unit for the quanta */
    size_t Value;      /*!< Numerical value of the quanta */
} Quanta_t;

typedef struct Timeout {
    uint32_t entering_tick;
    uint32_t overflows;
} Timeout_t;

extern Quanta_t *kernel_quanta;

/**
  * @brief Converts time value from one unit to system ticks
  * @param time Time value to convert
  * @param unit Unit of the input time value (@ref QuantaUnit_t)
  * @retval uint32_t Number of system ticks
  */
OCTOS_INLINE static inline uint32_t time_to_ticks(uint16_t time,
                                                  QuantaUnit_t unit) {
    return (time * kernel_quanta->Unit) / (kernel_quanta->Value * unit);
}

#endif
