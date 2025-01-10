#ifndef __PAGE_H__
#define __PAGE_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/**
  * @brief Page allocation policy enumeration
  */
typedef enum {
    PAGE_POLICY_STATIC,  /*!< Static allocation policy */
    PAGE_POLICY_POOL,    /*!< Pool-based allocation policy */
    PAGE_POLICY_DYNAMIC, /*!< Dynamic allocation policy */
    PAGE_POLICY_ERROR    /*!< Error state policy */
} PagePolicy_t;

/**
  * @brief Page structure definition
  */
typedef struct {
    uint32_t *raw;       /*!< Pointer to raw page data */
    size_t size;         /*!< Size of page in words */
    PagePolicy_t policy; /*!< Page allocation policy */
} Page_t;

void page_alloc(Page_t *page, PagePolicy_t policy, size_t size_in_words);
void page_free(Page_t *page);
void page_pool_init(void);

#endif
