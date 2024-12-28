#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "list.h"

// Initialize a list item
void list_item_init(ListItem_t *item) {
    item->Parent = NULL;
    item->Value = 0;
}

void list_item_set_value(ListItem_t *item, uint32_t value) {
    item->Value = value;
}

// Initialize a list
void list_init(List_t *list) {
    // Set list end marker value to maximum possible
    list->End.Value = 0xFF;

    // Initialize end marker pointers
    list->End.Next = &(list->End);
    list->End.Prev = &(list->End);

    list->Length = 0;
    list->Current = &(list->End);
}

bool list_valid(List_t *list) { return list->Current != NULL; }

// Insert item into list in order (based on xItemValue)
void list_insert(List_t *list, ListItem_t *new_item) {
    ListItem_t *iterator;

    // Find insertion position
    for (iterator = &(list->End); iterator->Next->Value <= new_item->Value;
         iterator = iterator->Next) {
        // Empty loop body
    }

    // Insert the new item
    new_item->Next = iterator->Next;
    new_item->Prev = iterator;
    iterator->Next->Prev = new_item;
    iterator->Next = new_item;

    // Remember which list the item is in
    new_item->Parent = list;

    // Increment counter
    list->Length++;
}

void list_insert_end(List_t *list, ListItem_t *new_item) {
    // Get the last real item (End.Prev points to the last actual item)
    ListItem_t *last = list->End.Prev;

    // Insert the new item before the end marker
    new_item->Next = &(list->End);
    new_item->Prev = last;
    last->Next = new_item;
    list->End.Prev = new_item;

    // Remember which list the item is in
    new_item->Parent = list;

    // Increment counter
    list->Length++;
}

// Remove item from list
void list_remove(ListItem_t *item_to_remove) {
    if (!item_to_remove->Parent)
        return;
    List_t *list = item_to_remove->Parent;

    item_to_remove->Prev->Next = item_to_remove->Next;
    item_to_remove->Next->Prev = item_to_remove->Prev;

    // Reset the index if it points to the item being removed
    if (list->Current == item_to_remove) {
        list->Current = item_to_remove->Prev;
    }

    item_to_remove->Parent = NULL;
    list->Length--;
}

ListItem_t *list_head(List_t *list) {
    if (list->Length == 0)
        return NULL;
    return list->End.Next;
}
