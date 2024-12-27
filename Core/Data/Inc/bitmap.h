#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t *data;
    size_t size;
} bitmap_t;

void bitmap_init(bitmap_t *bm, uint32_t *data, size_t size);
void bitmap_set(bitmap_t *bm, uint32_t pos);
void bitmap_reset(bitmap_t *bm, uint32_t pos);
int bitmap_test(bitmap_t *bm, uint32_t pos);
int32_t bitmap_first_zero(bitmap_t *bm);

#endif
