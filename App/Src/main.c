#include "main.h"
#include "Arch/stm32f4xx/Inc/api.h"
#include "kernel.h"
#include "led.h"
#include "mqueue.h"
#include "shell.h"
#include "task.h"
#include "usart3_dma.h"
#include <stdint.h>
#include <string.h>

#define QUEUE_SIZE 10
#define ITEM_SIZE sizeof(char)

static int help_func(int argc, char **argv);
static int ping_func(int argc, char **argv);
static int list_func(int argc, char **argv);

TCB_t *usart_dma_rx_thread_handle;
Shell_t shell;
ShellCommand_t commands[] = {{.name = "help", .handler = &help_func},
                             {.name = "ping", .handler = &ping_func},
                             {.name = "list", .handler = &list_func}};
MsgQueue_t usart3_rx_queue;
uint8_t usart3_rx_queue_storage[QUEUE_SIZE];


/* Simple LED Threads --------------------------------------------------------*/

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
    uint32_t cnt = 0;
    BSP_LED_Toggle(LED3);
    while (1) {
        if (cnt != 0 && cnt % 100000 == 0) BSP_LED_Toggle(LED3);
        cnt++;
    }
}

/* RX Threads ----------------------------------------------------------------*/

void shell_process_char_wrapper(const void *data, size_t len) {
    const uint8_t *tmp = data;

    for (; len > 0; --len, ++tmp) {
        mqueue_send(&usart3_rx_queue, tmp, UINT32_MAX);
    }
}

void usart_dma_rx_thread(void) {
    while (1) {
        task_notify_wait(0, 0, NULL, UINT32_MAX);
        usart3_dma_rx_check(); /* <-- Will call shell_process_char_wrapper */
    }
}

/* Shell Threads -------------------------------------------------------------*/

int help_func(OCTOS_UNUSED int argc, OCTOS_UNUSED char **argv) {
    shell.print("help func\r\n");
    return 0;
}

int ping_func(int argc, char **argv) {
    if (argc > 1) {
        if (strcmp(argv[1], "--help") == 0) {
            shell.print("ping help\r\n");
        } else {
            return 1;
        }
    } else {
        shell.print("pong\r\n");
    }
    return 0;
}

int list_func(OCTOS_UNUSED int argc, OCTOS_UNUSED char **argv) {
    char *buffer = OCTOS_MALLOC(256 * sizeof(char));
    task_info_list(buffer);
    shell.print(buffer);
    OCTOS_FREE(buffer);
    return 0;
}

void shell_thread(void) {
    shell_init(&shell, commands, sizeof(commands) / sizeof(commands[0]),
               &usart3_send_string);
    char buffer;
    while (1) {
        if (mqueue_recv(&usart3_rx_queue, &buffer, UINT32_MAX)) {
            shell_process_char(&shell, buffer);
        }
    }
}

/* Main Functions ------------------------------------------------------------*/

int main(void) {
    mqueue_init(&usart3_rx_queue, usart3_rx_queue_storage, 1, QUEUE_SIZE);
    usart3_dma_init(&shell_process_char_wrapper);
    BSP_LED_Init(LED1);
    BSP_LED_Init(LED2);
    BSP_LED_Init(LED3);

    task_create((TaskFunc_t) &usart_dma_rx_thread, NULL, "UART DMA RX", 3, 512,
                &usart_dma_rx_thread_handle);
    task_create((TaskFunc_t) &shell_thread, NULL, "SHELL", 2, 512, NULL);
    task_create((TaskFunc_t) &led1_thread, NULL, "LED 1", 1, 256, NULL);
    task_create((TaskFunc_t) &led2_thread, NULL, "LED 2", 1, 256, NULL);
    task_create((TaskFunc_t) &led3_thread, NULL, "LED 3", 0, 256, NULL);

    Quanta_t quanta = {.Unit = MILISECONDS, .Value = 5};
    kernel_launch(&quanta);
}

/* IRQHandler ----------------------------------------------------------------*/

void DMA1_Stream1_IRQHandler(void) {
    bool switch_required = false;

    if (usart3_dma_rx_check_ht()) {
        task_notify_from_isr(usart_dma_rx_thread_handle, 0, NoAction,
                             &switch_required);
    }
    if (usart3_dma_rx_check_tc()) {
        task_notify_from_isr(usart_dma_rx_thread_handle, 0, NoAction,
                             &switch_required);
    }

    task_yield_from_isr(switch_required);
}

void USART3_IRQHandler(void) {
    bool switch_required = false;

    if (usart3_dma_rx_check_idle()) {
        task_notify_from_isr(usart_dma_rx_thread_handle, 0, NoAction,
                             &switch_required);
    }

    task_yield_from_isr(switch_required);
}
