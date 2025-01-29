#include "main.h"
#include "Arch/stm32f4xx/Inc/api.h"
#include "kernel.h"
#include "led.h"
#include "mqueue.h"
#include "shell.h"
#include "sync.h"
#include "task.h"
#include "usart3_dma.h"
#include <stdint.h>
#include <string.h>

#define QUEUE_SIZE 10
#define ITEM_SIZE sizeof(char)

static int help_func(int argc, char **argv);
static int ping_func(int argc, char **argv);
static int list_func(int argc, char **argv);

TaskHandle_t usart_dma_rx_thread_handle;
TaskHandle_t pong1_thread_handle;
Shell_t shell;
ShellCommand_t commands[] = {{.name = "help", .handler = &help_func},
                             {.name = "ping", .handler = &ping_func},
                             {.name = "list", .handler = &list_func}};
MsgQueue_t usart3_rx_queue;
uint8_t usart3_rx_queue_storage[QUEUE_SIZE];
Mutex_t shell_print_mutex;
Barrier_t pong_barrier;
MsgQueue_t pong_queue;
uint8_t pong_queue_storage[QUEUE_SIZE];


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

/* Pong Threads --------------------------------------------------------------*/

void pong1_thread(void) {
    const char msg[] = "pong111111111111111111111111111111\r\n";
    while (1) {
        task_notify_wait(0, 0, NULL, UINT32_MAX);
        barrier_wait(&pong_barrier, UINT32_MAX);
        mutex_acquire(&shell_print_mutex, UINT32_MAX);
        for (size_t i = 0; msg[i] != '\0'; i++) {
            mqueue_send(&pong_queue, &msg[i], UINT32_MAX);
        }
        mutex_release(&shell_print_mutex);
    }
}

void pong2_thread(void) {
    const char msg[] = "pong222222222222222222222222222222\r\n";
    while (1) {
        barrier_wait(&pong_barrier, UINT32_MAX);
        mutex_acquire(&shell_print_mutex, UINT32_MAX);
        for (size_t i = 0; msg[i] != '\0'; i++) {
            mqueue_send(&pong_queue, &msg[i], UINT32_MAX);
        }
        mutex_release(&shell_print_mutex);
    }
}

void pong3_thread(void) {
    const char msg[] = "pong333333333333333333333333333333333\r\n";
    while (1) {
        barrier_wait(&pong_barrier, UINT32_MAX);
        mutex_acquire(&shell_print_mutex, UINT32_MAX);
        for (size_t i = 0; msg[i] != '\0'; i++) {
            mqueue_send(&pong_queue, &msg[i], UINT32_MAX);
        }
        mutex_release(&shell_print_mutex);
    }
}

/* Shell Threads -------------------------------------------------------------*/

int help_func(OCTOS_UNUSED int argc, OCTOS_UNUSED char **argv) {
    mutex_acquire(&shell_print_mutex, UINT32_MAX);
    shell.print("help func\r\n");
    mutex_release(&shell_print_mutex);
    return 0;
}

int ping_func(int argc, char **argv) {
    char buffer[50];
    buffer[1] = '\0';
    if (argc > 1) {
        if (strcmp(argv[1], "--help") == 0) {
            mutex_acquire(&shell_print_mutex, UINT32_MAX);
            shell.print("ping help\r\n");
            mutex_release(&shell_print_mutex);
        } else {
            return 1;
        }
    } else {
        task_notify(pong1_thread_handle, 0, NoAction);
        size_t i = 0;
        size_t pong_cnt = 0;
        while (1) {
            mqueue_recv(&pong_queue, &buffer[i], UINT32_MAX);
            if (buffer[i] == '\n') {
                buffer[i + 1] = '\0';
                i = 0;
                shell.print(buffer);
                pong_cnt++;
                if (pong_cnt == 3) { break; }
                continue;
            }
            i++;
        }
    }
    return 0;
}

int list_func(OCTOS_UNUSED int argc, OCTOS_UNUSED char **argv) {
    char *buffer = OCTOS_MALLOC(512 * sizeof(char));
    task_info_list(buffer);

    mutex_acquire(&shell_print_mutex, UINT32_MAX);
    shell.print(buffer);
    mutex_release(&shell_print_mutex);

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
    mqueue_init(&pong_queue, pong_queue_storage, 1, QUEUE_SIZE);
    mutex_init(&shell_print_mutex);
    barrier_init(&pong_barrier, 3);
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
    task_create((TaskFunc_t) &pong1_thread, NULL, "PONG 1", 1, 256,
                &pong1_thread_handle);
    task_create((TaskFunc_t) &pong2_thread, NULL, "PONG 2", 1, 256, NULL);
    task_create((TaskFunc_t) &pong3_thread, NULL, "PONG 3", 1, 256, NULL);

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
