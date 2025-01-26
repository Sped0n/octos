#include "main.h"
#include "kernel.h"
#include "led.h"
#include "task.h"
#include "usart3_dma.h"
#include "utils.h"
#include <stdint.h>

TCB_t *usart_dma_rx_thread_handle;

void counter_thread(void) {
    volatile uint32_t cnt = 1;
    while (1) { cnt++; }
}

void led1_thread(void) {
    BSP_LED_Toggle(LED1);
    while (1) {
        task_delay(time_to_ticks(1, SECONDS));
        BSP_LED_Toggle(LED1);
    }
}

void led2_thread(void) {
    BSP_LED_Toggle(LED2);
    while (1) {
        task_delay(time_to_ticks(2, SECONDS));
        BSP_LED_Toggle(LED2);
    }
}

void led3_thread(void) {
    BSP_LED_Toggle(LED3);
    while (1) {
        task_delay(time_to_ticks(500, MILISECONDS));
        BSP_LED_Toggle(LED3);
    }
}

void usart_dma_rx_thread(void) {
    uint32_t buffer;
    usart3_send_string("USART DMA example: DMA HT & TC + USART IDLE LINE IRQ + "
                       "RTOS processing\r\n");
    usart3_send_string("Start sending data to STM32\r\n");

    while (1) {
        task_notify_wait(0, UINT32_MAX, &buffer, UINT32_MAX);
        for (uint32_t i = buffer; i > 0; i--) usart3_dma_rx_check();
    }
}

int main(void) {
    usart3_dma_init();
    BSP_LED_Init(LED1);
    BSP_LED_Init(LED2);
    BSP_LED_Init(LED3);

    task_create((TaskFunc_t) &usart_dma_rx_thread, NULL, "UART DMA RX", 2, 512,
                &usart_dma_rx_thread_handle);
    task_create((TaskFunc_t) &led1_thread, NULL, "LED 1", 1, 256, NULL);
    task_create((TaskFunc_t) &led2_thread, NULL, "LED 2", 1, 256, NULL);
    task_create((TaskFunc_t) &led3_thread, NULL, "LED 3", 1, 256, NULL);
    task_create((TaskFunc_t) &counter_thread, NULL, "CNT", 0, 256, NULL);

    Quanta_t quanta = {.Unit = MILISECONDS, .Value = 5};
    kernel_launch(&quanta);
}

void DMA1_Stream1_IRQHandler(void) {
    bool switch_required = false;

    if (usart3_dma_rx_check_ht()) {
        task_notify_from_isr(usart_dma_rx_thread_handle, 0, Increment,
                             &switch_required);
    }
    if (usart3_dma_rx_check_tc()) {
        task_notify_from_isr(usart_dma_rx_thread_handle, 0, Increment,
                             &switch_required);
    }

    task_yield_from_isr(switch_required);
}

void USART3_IRQHandler(void) {
    bool switch_required = false;

    if (usart3_dma_rx_check_idle()) {
        task_notify_from_isr(usart_dma_rx_thread_handle, 0, Increment,
                             &switch_required);
    }

    task_yield_from_isr(switch_required);
}
