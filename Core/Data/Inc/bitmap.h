#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "attr.h"

/**
  * @brief Bitmap data structure definition
  */
typedef struct {
    uint32_t *data; /*!< Pointer to bitmap data array */
    size_t size;    /*!< Size of the bitmap data array */
} Bitmap_t;

void bitmap_init(Bitmap_t *bm, uint32_t *data, size_t size);

/** 
  * @brief Check if a bitmap is valid
  * @param bm Pointer to the bitmap structure to be checked
  * @retval true if the bitmap is valid (data is not NULL), false otherwise
  */
OCTOS_INLINE static inline bool bitmap_valid(Bitmap_t *bm) {
    return bm->data != NULL;
}

/**
  * @brief Sets a bit at the specified position in the bitmap
  * @param bm Pointer to the bitmap structure
  * @param pos Position of the bit to set (0-based index)
  * @retval None
  */
OCTOS_INLINE static inline void bitmap_set(Bitmap_t *bm, uint32_t pos) {
    uint32_t index = pos / 32;
    uint32_t bit = 31 - (pos % 32);
    bm->data[index] |= (1U << bit);
}

/**
  * @brief Clears a bit at the specified position in the bitmap
  * @param bm Pointer to the bitmap structure 
  * @param pos Position of the bit to clear (0-based index)
  * @retval None
  */
OCTOS_INLINE static inline void bitmap_reset(Bitmap_t *bm, uint32_t pos) {
    uint32_t index = pos / 32;
    uint32_t bit = 31 - (pos % 32);
    bm->data[index] &= ~(1U << bit);
}

/**
  * @brief Finds the position of first zero bit in the bitmap
  * @param bm Pointer to the bitmap structure
  * @return Position of first zero bit if found, -1 if no zero bit exists
  * @note Uses compiler builtin function __builtin_clz for leading zero count
  */
OCTOS_INLINE static inline int32_t bitmap_first_zero(Bitmap_t *bm) {
    for (size_t i = 0; i < (bm->size + 31) / 32; i++)
        if (bm->data[i] != ~0U) return i * 32 + __builtin_clz(~bm->data[i]);

    return -1;
}

/**
  * @brief Finds the position of first set bit (1) in the bitmap
  * @param bm Pointer to the bitmap structure
  * @return Position of first set bit if found, -1 if no set bit exists
  * @note Uses compiler builtin function __builtin_clz for leading zero count
  */
OCTOS_INLINE static inline int32_t bitmap_first_one(Bitmap_t *bm) {
    for (size_t i = 0; i < (bm->size + 31) / 32; i++)
        if (bm->data[i] != 0U) return i * 32 + __builtin_clz(bm->data[i]);

    return -1;
}

#endif
