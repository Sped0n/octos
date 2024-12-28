#include "kernel.h"
#include "Arch/stm32f4xx/Inc/api.h"
#include "list.h"
#include "tcb.h"

TCB_t *current_tcb;
uint32_t current_tick;

List_t ready_list;
List_t pending_ready_list;
List_t delayed_list;
List_t delayed_list_overflow;
List_t suspended_list;
List_t terminated_list;

Quanta_t kernel_quanta;

void kernel_launch(Quanta_t *quanta) {
    setup_interrupt_priority();
    kernel_quanta.Value = quanta->Value;
    kernel_quanta.Unit = quanta->Unit;
    setup_systick(&kernel_quanta);
    enable_systick();
    scheduler_launch();
}
