#ifndef __USART_H__
#define __USART_H__

#include "stm32f429xx.h"
#include "stm32f4xx.h" // IWYU pragma: keep
#include <stdint.h>

typedef struct {
  GPIO_TypeDef *Port;
  uint32_t Pin;
  uint32_t Alternate;
} EXT_GPIO_TypeDef;

// Structure to hold USART configuration
typedef struct {
  USART_TypeDef *USARTx;
  EXT_GPIO_TypeDef *TX;
  EXT_GPIO_TypeDef *RX;
  uint32_t BaudRate;
} USART_Config_t;

void BSP_USART_Init(USART_Config_t *config);
void BSP_USART_DeInit(USART_Config_t *config);
void BSP_USART_Enable(USART_TypeDef *USARTx);
void BSP_USART_Disable(USART_TypeDef *USARTx);
void BSP_USART_SendByte(USART_TypeDef *USARTx, uint8_t data);
void BSP_USART_SendString(USART_TypeDef *USARTx, const char *str);
void BSP_USART_SendBuffer(USART_TypeDef *USARTx, const uint8_t *buffer,
                          uint32_t length);
uint8_t BSP_USART_ReceiveByte(USART_TypeDef *USARTx);
void BSP_USART_EnableIT_RXNE(USART_TypeDef *USARTx);
void BSP_USART_DisableIT_RXNE(USART_TypeDef *USARTx);

#endif
