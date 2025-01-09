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

OCTOS_INLINE inline void page_pool_init(void) {
    page_inited = true;
    bitmap_init(&page_bitmap, bitmap_data, PAGE_POOL_SIZE);
}

OCTOS_INLINE inline void page_alloc(Page_t *page, PagePolicy_t policy, size_t size) {
    page->raw = NULL;
    page->size = size;
    page->policy = policy;

    switch (policy) {
        case PAGE_POLICY_POOL: {
            if (!page_inited)
                page_pool_init();
            int32_t index = bitmap_first_zero(&page_bitmap);
            if (index >= 0 && index < PAGE_POOL_SIZE) {
                bitmap_set(&page_bitmap, index);
                memset(page->raw, 0, size * sizeof(uint32_t));
                page->raw = page_pool[index];
                page->size = PAGE_SIZE;
            }
            break;
        }
        case PAGE_POLICY_DYNAMIC:
            page->raw = malloc(size * sizeof(uint32_t));
            if (page->raw) {
                memset(page->raw, 0, size * sizeof(uint32_t));
            }
            break;
        default:
            page->policy = PAGE_POLICY_ERROR;
    }
}

OCTOS_INLINE inline void page_free(Page_t *page) {
    if (!page->raw)
        return;

    switch (page->policy) {
        case PAGE_POLICY_POOL:
            for (int i = 0; i < PAGE_POOL_SIZE; i++) {
                if (page->raw == page_pool[i]) {
                    bitmap_reset(&page_bitmap, i);
                    break;
                }
            }
            break;
        case PAGE_POLICY_DYNAMIC:
            free(page->raw);
            break;
        default:
            break;
    }
    page->raw = NULL;
}

#endif
