#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "bitmap.h"
#include "page.h"

#include "stm32f4xx.h"// IWYU pragma: keep

#define PAGE_POOL_SIZE 8
#define PAGE_SIZE 512// Size in words (uint32_t)

static uint32_t page_pool[PAGE_POOL_SIZE][PAGE_SIZE];
static bool page_inited = false;
static uint32_t bitmap_data[(PAGE_POOL_SIZE + 31) / 32];
static bitmap_t page_bitmap;

static void page_pool_init(void) {
    page_inited = true;
    bitmap_init(&page_bitmap, bitmap_data, PAGE_POOL_SIZE);
    memset(page_pool, 0, sizeof(page_pool));
}

void page_alloc(page_t *page, page_policy_t policy, size_t size) {
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
                page->raw = page_pool[index];
                page->size = PAGE_SIZE;
            }
            break;
        }
        case PAGE_POLICY_DYNAMIC:
            __disable_irq();
            page->raw = malloc(size * sizeof(uint32_t));
            __enable_irq();
            if (page->raw) {
                memset(page->raw, 0, size * sizeof(uint32_t));
            }
            break;
        default:
            page->policy = PAGE_POLICY_ERROR;
    }
}

void page_free(page_t *page) {
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
            __disable_irq();
            free(page->raw);
            __enable_irq();
            break;
        default:
            break;
    }
    page->raw = NULL;
}
