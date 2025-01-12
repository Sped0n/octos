#include <stddef.h>
#include <stdint.h>

#include "Arch/stm32f4xx/Inc/api.h"
#include "bitmap.h"
#include "config.h"
#include "page.h"

static uint32_t page_pool[PAGE_POOL_SIZE][PAGE_SIZE];
static uint32_t page_bitmap_data[(PAGE_POOL_SIZE + 31) / 32];
static Bitmap_t page_bitmap;

/**
  * @brief Allocates memory for a page based on specified policy
  * @param page Pointer to page structure to initialize
  * @param policy Memory allocation policy to use
  * @param size_in_words Requested page size in 32-bit words
  * @retval None
  */
void page_alloc(Page_t *page, PagePolicy_t policy, size_t size_in_words) {
    page->raw = NULL;
    page->size = size_in_words;
    page->policy = policy;

    switch (policy) {
        case PAGE_POLICY_POOL: {
            if (page_bitmap.size != PAGE_POOL_SIZE) page_pool_init();
            int32_t index = bitmap_first_zero(&page_bitmap);
            if (index >= 0 && index < PAGE_POOL_SIZE) {
                bitmap_set(&page_bitmap, index);
                memset(page->raw, 0, size_in_words * sizeof(uint32_t));
                page->raw = page_pool[index];
                page->size = PAGE_SIZE;
            }
            break;
        }
        case PAGE_POLICY_DYNAMIC:
            page->raw = OCTOS_MALLOC(size_in_words * sizeof(uint32_t));
            if (page->raw)
                memset(page->raw, 0, size_in_words * sizeof(uint32_t));
            break;
        default:
            page->policy = PAGE_POLICY_ERROR;
    }
}

/**
  * @brief Frees allocated page memory
  * @param page Pointer to page structure to free
  * @retval None
  */
void page_free(Page_t *page) {
    if (!page->raw) return;

    switch (page->policy) {
        case PAGE_POLICY_POOL:
            for (int i = 0; i < PAGE_POOL_SIZE; i++)
                if (page->raw == page_pool[i]) {
                    bitmap_reset(&page_bitmap, i);
                    break;
                }
            break;
        case PAGE_POLICY_DYNAMIC:
            OCTOS_FREE(page->raw);
            break;
        default:
            break;
    }
    page->raw = NULL;
}

/**
  * @brief Initializes the page pool system
  * @note Must be called before using PAGE_POLICY_POOL allocation
  * @retval None
  */
void page_pool_init(void) {
    bitmap_init(&page_bitmap, page_bitmap_data, PAGE_POOL_SIZE);
}
