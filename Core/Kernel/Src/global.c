#include "list.h"
#include "tcb.h"

TCB_t *current_tcb;
List_t ready_list;
List_t pending_ready_list;
List_t delayed_list;
List_t delayed_list_overflow;
List_t suspended_list;
List_t terminated_list;
