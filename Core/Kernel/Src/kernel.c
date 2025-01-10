#include "kernel.h"
#include "Arch/stm32f4xx/Inc/api.h"
#include "Kernel/Inc/utils.h"
#include "list.h"
#include "tcb.h"

TCB_t *current_tcb;
volatile uint32_t current_tick;

static List_t ready_list_internal;
static List_t pending_ready_list_internal;
static List_t delayed_list_internal;
static List_t delayed_list_overflow_internal;
static List_t suspended_list_internal;
static List_t terminated_list_internal;

static Quanta_t kernel_quanta_internal;

List_t *ready_list = &ready_list_internal;
List_t *pending_ready_list = &pending_ready_list_internal;
List_t *delayed_list = &delayed_list_internal;
List_t *delayed_list_overflow = &delayed_list_overflow_internal;
List_t *suspended_list = &suspended_list_internal;
List_t *terminated_list = &terminated_list_internal;

Quanta_t *kernel_quanta = &kernel_quanta_internal;


/**
  * @brief Initialize and launch the kernel
  * @param quanta Pointer to kernel time quanta configuration structure
  * @retval None
  */
void kernel_launch(Quanta_t *quanta) {
    list_init(pending_ready_list);
    list_init(delayed_list);
    list_init(delayed_list_overflow);
    list_init(suspended_list);
    list_init(terminated_list);

    OCTOS_SETUP_INTPRI();
    kernel_quanta_internal.Value = quanta->Value;
    kernel_quanta_internal.Unit = quanta->Unit;
    OCTOS_SETUP_SYSTICK(quanta);

    OCTOS_ENABLE_SYSTICK();

    OCTOS_SCHED_LAUNCH();
}

/**
  * @brief Increment system tick and handle delayed tasks
  * @note Called by SysTick interrupt handler
  * @note Moves tasks from delayed list to ready list when their delay expires
  * @note Handles tick counter overflow by swapping delayed lists
  * @retval None
  */
void kernel_tick_increment(void) {
    current_tick++;

    if (current_tick == 0) {
        List_t *temp;
        temp = delayed_list;
        delayed_list = delayed_list_overflow;
        delayed_list_overflow = temp;
    }

    while (delayed_list->Length > 0) {
        ListItem_t *head = list_head(delayed_list);
        if (list_item_get_value(head) <= current_tick) {
            list_remove(head);
            list_item_set_value(head, ((TCB_t *) head->Owner)->Priority);
            list_insert_end(pending_ready_list, head);
        } else {
            break;
        }
    }
}
