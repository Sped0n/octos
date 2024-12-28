#include "kernel.h"
#include "Arch/stm32f4xx/Inc/api.h"
#include "global.h"

void kernel_launch(Quanta_t *quanta) {
    setup_interrupt_priority();
    setup_systick(quanta);
    enable_systick();
    scheduler_launch();
}
