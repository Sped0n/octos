#include "bitmap.h"

void bitmap_init(Bitmap_t *bm, uint32_t *data, size_t size) {
    bm->data = data;
    bm->size = size;
    for (size_t i = 0; i < (size + 31) / 32; i++) {
        bm->data[i] = 0;
    }
}
