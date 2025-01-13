#include <stdbool.h>

#include "Arch/stm32f4xx/Inc/api.h"
#include "Kernel/Inc/utils.h"
#include "kernel.h"
#include "task.h"

static Quanta_t kernel_quanta_internal;

Quanta_t *kernel_quanta = &kernel_quanta_internal;

/**
  * @brief Initialize and launch the kernel
  * @param quanta Pointer to kernel time quanta configuration structure
  * @retval None
  */
void kernel_launch(Quanta_t *quanta) {
    task_lists_init();

    OCTOS_SETUP_INTPRI();

    kernel_quanta_internal.Value = quanta->Value;
    kernel_quanta_internal.Unit = quanta->Unit;
    OCTOS_SETUP_SYSTICK(quanta);

    OCTOS_ENABLE_SYSTICK();

    OCTOS_SCHED_LAUNCH();
}
