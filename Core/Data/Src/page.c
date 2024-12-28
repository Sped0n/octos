#include <stdbool.h>

#include "bitmap.h"
#include "page.h"

uint32_t page_pool[PAGE_POOL_SIZE][PAGE_SIZE];
bool page_inited = false;
uint32_t bitmap_data[(PAGE_POOL_SIZE + 31) / 32];
Bitmap_t page_bitmap;
