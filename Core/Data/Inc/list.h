#ifndef __LIST_H__
#define __LIST_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

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
    size_t Length;       /*!< Number of items in the list */
    ListItem_t *Current; /*!< Used to walk through the list */
    ListItem_t End;      /*!< End marker of the list */
} List_t;

/* ListItem ------------------------------------------------------------------*/

/**
 * @brief Initializes a list item
 * @param item: Pointer to the ListItem_t structure to initialize
 * @return None
 */
OCTOS_INLINE static inline void list_item_init(ListItem_t *item) {
    item->Parent = NULL;
    item->Value = 0;
}

/**
 * @brief Sets the value of a list item
 * @param item: Pointer to the ListItem_t structure
 * @param value: The value to set
 * @return None
 */
OCTOS_INLINE static inline void list_item_set_value(ListItem_t *item,
                                                    uint32_t value) {
    item->Value = value;
}

/**
 * @brief Gets the value of a list item
 * @param item: Pointer to the ListItem_t structure
 * @return The value stored in the list item
 */
OCTOS_INLINE static inline uint32_t list_item_get_value(ListItem_t *item) {
    return item->Value;
}

/* List ----------------------------------------------------------------------*/

/**
 * @brief Initializes a list
 * @param list: Pointer to the List_t structure to initialize
 * @return None
 */
OCTOS_INLINE inline void list_init(List_t *list) {
    list->End.Value = UINT32_MAX;

    list->End.Next = &(list->End);
    list->End.Prev = &(list->End);

    list->Length = 0;
}

/** 
 * @brief Check if a list is valid
 * @param list: Pointer to the list to be checked
 * @retval true If the list is valid
 * @retval false Otherwise
 */
OCTOS_INLINE static inline bool list_valid(List_t *list) {
    return list->End.Value == UINT32_MAX;
}

/** 
 * @brief Insert an item into a list in sorted order
 * @param list: Pointer to the list where the item will be inserted
 * @param new_item: Pointer to the item to be inserted
 * @retval true If the insertion is successful
 * @retval false Otherwise
 */
OCTOS_INLINE static inline bool list_insert(List_t *list,
                                            ListItem_t *new_item) {
    OCTOS_DSB();
    OCTOS_ISB();

    if (new_item->Parent != NULL) return false;

    ListItem_t *iterator;

    for (iterator = &(list->End); iterator->Next->Value < new_item->Value;
         iterator = iterator->Next);

    new_item->Next = iterator->Next;
    new_item->Prev = iterator;
    iterator->Next->Prev = new_item;
    iterator->Next = new_item;

    new_item->Parent = list;
    if (list->Length == 0) list->Current = new_item;
    list->Length++;

    return true;
}

/** 
 * @brief Insert an item at the end of a list
 * @note This is a unordered insertion
 * @param list: Pointer to the list where the item will be inserted
 * @param new_item: Pointer to the item to be inserted
 * @retval true If the insertion is successful 
 * @retval false Otherwise
 */
OCTOS_INLINE static inline bool list_insert_end(List_t *list,
                                                ListItem_t *new_item) {
    OCTOS_DSB();
    OCTOS_ISB();

    if (new_item->Parent != NULL) return false;

    ListItem_t *last = list->End.Prev;

    new_item->Next = &(list->End);
    new_item->Prev = last;
    last->Next = new_item;
    list->End.Prev = new_item;

    new_item->Parent = list;
    if (list->Length == 0) list->Current = new_item;
    list->Length++;

    return true;
}

/** 
 * @brief Remove an item from a list
 * @note This function removes the specified item from its parent list
 *       and updates the list's current pointer
 * @param item_to_remove: Pointer to the list item to be removed
 * @retval true If the item was successfully removed
 * @retval false If the item has no parent list
 */
OCTOS_INLINE static inline bool list_remove(ListItem_t *item_to_remove) {
    OCTOS_DSB();
    OCTOS_ISB();

    if (item_to_remove->Parent == NULL) return false;

    List_t *list = item_to_remove->Parent;

    list->Current = list->Current->Next == &(list->End)
                            ? list->Current->Next->Next
                            : list->Current->Next;

    item_to_remove->Prev->Next = item_to_remove->Next;
    item_to_remove->Next->Prev = item_to_remove->Prev;

    item_to_remove->Parent = NULL;
    list->Length--;
    return true;
}

/**
 * @brief Gets the first item in the list
 * @param list: Pointer to the List_t structure
 * @return Pointer to the first ListItem_t in the list, or NULL
 *         if the list is empty
 */
OCTOS_INLINE static inline ListItem_t *list_head(List_t *list) {
    if (list->Length == 0) return NULL;
    return list->End.Next;
}

/**
  * @brief Gets the last item in the list
  * @param list: Pointer to the List_t structure
  * @return Pointer to the last ListItem_t in the list, or NULL
  *         if the list is empty
  */
OCTOS_INLINE static inline ListItem_t *list_tail(List_t *list) {
    if (list->Length == 0) return NULL;
    return list->End.Prev;
}

/** 
 * @brief Get the owner of the next entry in the list
 * @note This function advances the list's current pointer to the next
 *       entry and returns the owner of that entry
 * @param list: Pointer to the list
 * @return Pointer to the owner of the next entry in the list
 */
OCTOS_INLINE static inline void *list_get_owner_of_next_entry(List_t *list) {
    list->Current = list->Current->Next == &(list->End)
                            ? list->Current->Next->Next
                            : list->Current->Next;
    return list->Current->Owner;
}

#endif
