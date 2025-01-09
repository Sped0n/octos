#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include "Arch/stm32f4xx/Inc/api.h"
#include "list.h"
#include "tcb.h"

extern TCB_t *current_tcb;
extern List_t *ready_list;
extern List_t *terminated_list;

static inline void scheduler_trigger(void) {
    OCTOS_CTX_SWITCH();
}

void scheduler_rr(void);

#endif
