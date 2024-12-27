#ifndef __LIST_H__
#define __LIST_H__

#include <stdbool.h>
#include <stdint.h>

typedef struct ListItem {
    uint32_t Value;       // Value for sorting
    struct ListItem *Next;// Next item
    struct ListItem *Prev;// Previous item
    void *Owner;          // Owner of this item
    struct List *Parent;  // List containing this item
} ListItem_t;

// List structure
typedef struct List {
    uint32_t Length;    // Number of items in list
    ListItem_t *Current;// Current index
    ListItem_t End;     // End marker
} List_t;

void list_item_init(ListItem_t *item);

void list_init(List_t *list);
bool list_valid(List_t *list);
void list_insert(List_t *list, ListItem_t *new_item);
void list_remove(ListItem_t *item_to_remove);
ListItem_t *list_head(List_t *list);

#endif
