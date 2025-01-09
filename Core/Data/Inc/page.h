#ifndef __PAGE_H__
#define __PAGE_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "bitmap.h"

typedef enum {
    PAGE_POLICY_STATIC,
    PAGE_POLICY_POOL,
    PAGE_POLICY_DYNAMIC,
    PAGE_POLICY_ERROR
} PagePolicy_t;

typedef struct {
    uint32_t *raw;
    size_t size;
    PagePolicy_t policy;
} Page_t;

#define PAGE_POOL_SIZE 8
#define PAGE_SIZE 512// Size in words (uint32_t)

extern uint32_t page_pool[PAGE_POOL_SIZE][PAGE_SIZE];
extern bool page_inited;
extern uint32_t bitmap_data[(PAGE_POOL_SIZE + 31) / 32];
extern Bitmap_t page_bitmap;

void page_alloc(Page_t *page, PagePolicy_t policy, size_t size);
void page_free(Page_t *page);

OCTOS_INLINE inline void page_pool_init(void) {
    page_inited = true;
    bitmap_init(&page_bitmap, bitmap_data, PAGE_POOL_SIZE);
}


#endif
