#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <stddef.h>
#include <stdint.h>

#include "Kernel/Inc/utils.h"

typedef struct {
    uint32_t *data;
    size_t size;
} Bitmap_t;

OCTOS_INLINE inline void bitmap_init(Bitmap_t *bm, uint32_t *data, size_t size) {
    bm->data = data;
    bm->size = size;
    for (size_t i = 0; i < (size + 31) / 32; i++) {
        bm->data[i] = 0;
    }
}

OCTOS_INLINE inline void bitmap_set(Bitmap_t *bm, uint32_t pos) {
    uint32_t index = pos / 32;
    uint32_t bit = 31 - (pos % 32);
    bm->data[index] |= (1U << bit);
}

OCTOS_INLINE inline void bitmap_reset(Bitmap_t *bm, uint32_t pos) {
    uint32_t index = pos / 32;
    uint32_t bit = 31 - (pos % 32);
    bm->data[index] &= ~(1U << bit);
}

OCTOS_INLINE inline int32_t bitmap_first_zero(Bitmap_t *bm) {
    for (size_t i = 0; i < (bm->size + 31) / 32; i++) {
        if (bm->data[i] != ~0U) {
            return i * 32 + __builtin_clz(~bm->data[i]);
        }
    }
    return -1;
}

#endif
