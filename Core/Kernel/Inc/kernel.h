#ifndef __KERNEL_H__
#define __KERNEL_H__

#include "Kernel/Inc/utils.h"
#include "mqueue.h"// IWYU pragma: keep
#include "sync.h"  // IWYU pragma: keep
#include "task.h"  // IWYU pragma: keep

void kernel_launch(Quanta_t *quanta);

#endif
