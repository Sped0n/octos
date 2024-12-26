#include "bitmap.h"

void bitmap_init(bitmap_t *bm, uint32_t *data, size_t size) {
  bm->data = data;
  bm->size = size;
  for (size_t i = 0; i < (size + 31) / 32; i++) {
    bm->data[i] = 0;
  }
}

void bitmap_set(bitmap_t *bm, uint32_t pos) {
  uint32_t index = pos / 32;
  uint32_t bit = 31 - (pos % 32);
  bm->data[index] |= (1U << bit);
}

void bitmap_reset(bitmap_t *bm, uint32_t pos) {
  uint32_t index = pos / 32;
  uint32_t bit = 31 - (pos % 32);
  bm->data[index] &= ~(1U << bit);
}

int bitmap_test(bitmap_t *bm, uint32_t pos) {
  uint32_t index = pos / 32;
  uint32_t bit = 31 - (pos % 32);
  return (bm->data[index] & (1U << bit)) != 0;
}

int32_t bitmap_first_zero(bitmap_t *bm) {
  for (size_t i = 0; i < (bm->size + 31) / 32; i++) {
    if (bm->data[i] != ~0U) {
      return i * 32 + __builtin_clz(~bm->data[i]);
    }
  }
  return -1;
}
