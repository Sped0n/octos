#ifndef __DRIVER_UTILS_H__
#define __DRIVER_UTILS_H__

#include "stm32f4xx.h"// IWYU pragma: keep
#include <stdint.h>

uint32_t GPIO_Get_Clock_Enable_Flag(GPIO_TypeDef *GPIOx);

#endif
