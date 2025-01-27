#include <string.h>

#include "usart3_dma.h"

#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_dma.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_usart.h"

#define UART_DMA_RX_BUFFER_SIZE 64

static uint8_t usart_rx_dma_buffer[UART_DMA_RX_BUFFER_SIZE];
static void (*usart3_dma_recv_func)(const void *data, size_t len) = NULL;

/** 
 * @brief Initialize USART3 with DMA for receiving data
 * @note This function also sets up the necessary interrupts and assigns a
 *       callback function for data reception
 * @param recv_func 
 *      Pointer to the callback function that will be called when
 *      data is received
 * @return None
 */
void usart3_dma_init(void (*recv_func)(const void *data, size_t len));
void usart3_dma_init(void (*recv_func)(const void *data, size_t len)) {
    /* Enable clocks */
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART3);
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOD);
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);

    /* Configure USART3 GPIO */
    LL_GPIO_InitTypeDef gpio_init = {0};
    gpio_init.Pin = LL_GPIO_PIN_8 | LL_GPIO_PIN_9;
    gpio_init.Mode = LL_GPIO_MODE_ALTERNATE;
    gpio_init.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    gpio_init.Pull = LL_GPIO_PULL_UP;
    gpio_init.Alternate = LL_GPIO_AF_7;
    LL_GPIO_Init(GPIOD, &gpio_init);

    /* Configure USART3 DMA */
    LL_DMA_SetChannelSelection(DMA1, LL_DMA_STREAM_1, LL_DMA_CHANNEL_4);
    LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_STREAM_1,
                                    LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
    LL_DMA_SetStreamPriorityLevel(DMA1, LL_DMA_STREAM_1, LL_DMA_PRIORITY_LOW);
    LL_DMA_SetMode(DMA1, LL_DMA_STREAM_1, LL_DMA_MODE_CIRCULAR);
    LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_STREAM_1, LL_DMA_PERIPH_NOINCREMENT);
    LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_STREAM_1, LL_DMA_MEMORY_INCREMENT);
    LL_DMA_SetPeriphSize(DMA1, LL_DMA_STREAM_1, LL_DMA_PDATAALIGN_BYTE);
    LL_DMA_SetMemorySize(DMA1, LL_DMA_STREAM_1, LL_DMA_MDATAALIGN_BYTE);
    LL_DMA_DisableFifoMode(DMA1, LL_DMA_STREAM_1);
    LL_DMA_SetPeriphAddress(DMA1, LL_DMA_STREAM_1,
                            LL_USART_DMA_GetRegAddr(USART3));
    LL_DMA_SetMemoryAddress(DMA1, LL_DMA_STREAM_1,
                            (uint32_t) usart_rx_dma_buffer);
    LL_DMA_SetDataLength(DMA1, LL_DMA_STREAM_1, UART_DMA_RX_BUFFER_SIZE);

    /* Enable HT & TC interrupts */
    LL_DMA_EnableIT_HT(DMA1, LL_DMA_STREAM_1);
    LL_DMA_EnableIT_TC(DMA1, LL_DMA_STREAM_1);
    NVIC_SetPriority(DMA1_Stream1_IRQn, 13);
    NVIC_EnableIRQ(DMA1_Stream1_IRQn);

    /* USART configuration */
    LL_USART_InitTypeDef usart_init = {0};
    usart_init.BaudRate = 115200;
    usart_init.DataWidth = LL_USART_DATAWIDTH_8B;
    usart_init.StopBits = LL_USART_STOPBITS_1;
    usart_init.Parity = LL_USART_PARITY_NONE;
    usart_init.TransferDirection = LL_USART_DIRECTION_TX_RX;
    usart_init.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
    usart_init.OverSampling = LL_USART_OVERSAMPLING_16;
    LL_USART_Init(USART3, &usart_init);
    LL_USART_ConfigAsyncMode(USART3);
    LL_USART_EnableDMAReq_RX(USART3);
    LL_USART_EnableIT_IDLE(USART3);

    /* USART interrupt */
    NVIC_SetPriority(USART3_IRQn, 13);
    NVIC_EnableIRQ(USART3_IRQn);

    /* Enable USART and DMA */
    LL_DMA_EnableStream(DMA1, LL_DMA_STREAM_1);
    LL_USART_Enable(USART3);

    /* Receive function */
    if (recv_func != NULL) usart3_dma_recv_func = recv_func;
}

/** 
 * @brief Check for new data received via DMA and process it
 * @note This function calculates the current position in the DMA buffer and
 *       processes new data
 * @return None
 */
void usart3_dma_rx_check(void) {
    static size_t old_pos;
    size_t pos;

    /* Calculate current position in buffer and check for new data available */
    pos = UART_DMA_RX_BUFFER_SIZE - LL_DMA_GetDataLength(DMA1, LL_DMA_STREAM_1);
    if (pos != old_pos) { /* Check change in received data */
        if (usart3_dma_recv_func != NULL) {
            if (pos > old_pos) { /* Current position is over previous one */
                /*
                 * Processing is done in "linear" mode.
                 *
                 * Application processing is fast with single data block,
                 * length is simply calculated by subtracting pointers
                 *
                 * [   0   ]
                 * [   1   ] <- old_pos |------------------------------------|
                 * [   2   ]            |                                    |
                 * [   3   ]            | Single block (len = pos - old_pos) |
                 * [   4   ]            |                                    |
                 * [   5   ]            |------------------------------------|
                 * [   6   ] <- pos
                 * [   7   ]
                 * [ N - 1 ]
                 */
                usart3_dma_recv_func(&usart_rx_dma_buffer[old_pos],
                                     pos - old_pos);
            } else {
                /*
                 * Processing is done in "overflow" mode..
                 *
                 * Application must process data twice,
                 * since there are 2 linear memory blocks to handle
                 *
                 * [   0   ]            |---------------------------------|
                 * [   1   ]            | Second block (len = pos)        |
                 * [   2   ]            |---------------------------------|
                 * [   3   ] <- pos
                 * [   4   ] <- old_pos |---------------------------------|
                 * [   5   ]            |                                 |
                 * [   6   ]            | First block (len = N - old_pos) |
                 * [   7   ]            |                                 |
                 * [ N - 1 ]            |---------------------------------|
                 */
                usart3_dma_recv_func(&usart_rx_dma_buffer[old_pos],
                                     UART_DMA_RX_BUFFER_SIZE - old_pos);
                if (pos > 0) {
                    usart3_dma_recv_func(&usart_rx_dma_buffer[0], pos);
                }
            }
        }
        old_pos = pos; /* Save current position as old for next transfers */
    }
}

/** 
 * @brief Send a string via USART3
 * @param str Pointer to the string to be sent
 * @return None
 */
void usart3_send_string(const char *str) {
    usart3_dma_process_data(str, strlen(str));
}
