#include "list.h"
#include "tcb.h"

TCB_t *current_tcb;
List_t ready_list;
List_t terminated_list;
