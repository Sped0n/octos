#ifndef __PAGE_H__
#define __PAGE_H__

#include <stddef.h>
#include <stdint.h>

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

void page_alloc(Page_t *page, PagePolicy_t policy, size_t size);
void page_free(Page_t *page);

#endif
