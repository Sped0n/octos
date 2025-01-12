#ifndef __LIST_H__
#define __LIST_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "Arch/stm32f4xx/Inc/api.h"
#include "attr.h"

/**
  * @brief List item structure definition
  */
typedef struct ListItem {
    uint32_t Value;        /*!< Value used for sorting */
    struct ListItem *Next; /*!< Pointer to next item in list */
    struct ListItem *Prev; /*!< Pointer to previous item in list */
    void *Owner;           /*!< Pointer to the owner of this item */
    struct List *Parent;   /*!< Pointer to the list containing this item */
} ListItem_t;

/**
  * @brief List structure definition
  */
typedef struct List {
    size_t Length;  /*!< Number of items in the list */
    ListItem_t End; /*!< End marker of the list */
} List_t;

/* ListItem ------------------------------------------------------------------*/

/**
  * @brief Initializes a list item
  * @param item Pointer to the ListItem_t structure to initialize
  * @retval None
  */
OCTOS_INLINE inline void list_item_init(ListItem_t *item) {
    item->Parent = NULL;
    item->Value = 0;
}

/**
  * @brief Sets the value of a list item
  * @param item Pointer to the ListItem_t structure
  * @param value The value to set
  * @retval None
  */
OCTOS_INLINE static inline void list_item_set_value(ListItem_t *item, uint32_t value) {
    item->Value = value;
}

/**
  * @brief Gets the value of a list item
  * @param item Pointer to the ListItem_t structure
  * @return The value stored in the list item
  */
OCTOS_INLINE static inline uint32_t list_item_get_value(ListItem_t *item) {
    return item->Value;
}

/* List ----------------------------------------------------------------------*/

/**
  * @brief Initializes a list
  * @param list Pointer to the List_t structure to initialize
  * @retval None
  */
OCTOS_INLINE inline void list_init(List_t *list) {
    list->End.Value = UINT32_MAX;

    list->End.Next = &(list->End);
    list->End.Prev = &(list->End);

    list->Length = 0;
}

/**
  * @brief Inserts a new item into the list in sorted order
  * @param list Pointer to the List_t structure
  * @param new_item Pointer to the new ListItem_t to insert
  * @retval None
  */
OCTOS_INLINE static inline void list_insert(List_t *list, ListItem_t *new_item) {
    OCTOS_DSB();
    OCTOS_ISB();

    ListItem_t *iterator;

    for (iterator = &(list->End); iterator->Next->Value < new_item->Value;
         iterator = iterator->Next);

    new_item->Next = iterator->Next;
    new_item->Prev = iterator;
    iterator->Next->Prev = new_item;
    iterator->Next = new_item;

    new_item->Parent = list;

    list->Length++;
}

/**
  * @brief Inserts a new item at the end of the list
  * @param list Pointer to the List_t structure
  * @param new_item Pointer to the new ListItem_t to insert
  * @retval None
  */
OCTOS_INLINE static inline void list_insert_end(List_t *list, ListItem_t *new_item) {
    OCTOS_DSB();
    OCTOS_ISB();

    ListItem_t *last = list->End.Prev;

    new_item->Next = &(list->End);
    new_item->Prev = last;
    last->Next = new_item;
    list->End.Prev = new_item;

    new_item->Parent = list;

    list->Length++;
}

/**
  * @brief Removes an item from its parent list
  * @param item_to_remove Pointer to the ListItem_t to remove
  * @retval None
  * @note If the item has no parent list, the function returns without doing anything
  */
OCTOS_INLINE static inline void list_remove(ListItem_t *item_to_remove) {
    OCTOS_DSB();
    OCTOS_ISB();

    if (!item_to_remove->Parent)
        return;
    List_t *list = item_to_remove->Parent;

    item_to_remove->Prev->Next = item_to_remove->Next;
    item_to_remove->Next->Prev = item_to_remove->Prev;

    item_to_remove->Parent = NULL;
    list->Length--;
}

/**
  * @brief Gets the first item in the list
  * @param list Pointer to the List_t structure
  * @return Pointer to the first ListItem_t in the list, or NULL if the list is empty
  */
OCTOS_INLINE static inline ListItem_t *list_head(List_t *list) {
    if (list->Length == 0)
        return NULL;
    return list->End.Next;
}

/**
  * @brief Gets the last item in the list
  * @param list Pointer to the List_t structure
  * @return Pointer to the last ListItem_t in the list, or NULL if the list is empty
  */
OCTOS_INLINE static inline ListItem_t *list_tail(List_t *list) {
    if (list->Length == 0)
        return NULL;
    return list->End.Prev;
}

#endif
