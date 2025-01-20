#ifndef __TCB_H__
#define __TCB_H__

#include <stddef.h>
#include <stdint.h>

#include "list.h"
#include "page.h"

/**
 * @brief Task Control Block structure definition
 */
typedef struct TCB {
    uint32_t *StackTop;       /*!< Pointer to the top of task's stack */
    Page_t Page;              /*!< Memory page allocated for this task */
    ListItem_t StateListItem; /*!< List item for thread state lists */
    ListItem_t EventListItem; /*!< List item for event waiting lists */
    uint8_t RootPriority;     /*!< Original priority of the thread */
    uint8_t Priority;         /*!< Current priority of the thread */
    char Name[10];            /*< Task name, max length 10 */
    uint32_t TCBNumber;       /*!< Unique identifier for the thread */
} TCB_t;

#endif
