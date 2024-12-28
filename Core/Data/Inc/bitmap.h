#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t *data;
    size_t size;
} Bitmap_t;

void bitmap_init(Bitmap_t *bm, uint32_t *data, size_t size);
void bitmap_set(Bitmap_t *bm, uint32_t pos);
void bitmap_reset(Bitmap_t *bm, uint32_t pos);
int32_t bitmap_first_zero(Bitmap_t *bm);

#endif
