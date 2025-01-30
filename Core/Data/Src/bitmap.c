#include <stddef.h>
#include <stdint.h>

#include "bitmap.h"

/**
 * @brief Initializes a bitmap structure with given data array and size
 * @param bm: Pointer to the Bitmap_t structure to initialize
 * @param data: Pointer to uint32_t array that will store the bitmap data
 * @param size: Size of the bitmap in bits
 * @note The function clears all bits in the bitmap by setting data array to zero
 * @return None
 */
void bitmap_init(Bitmap_t *bm, uint32_t *data, size_t size) {
    bm->data = data;
    bm->size = size;
    for (size_t i = 0; i < (size + 31) / 32; i++) bm->data[i] = 0;
}
