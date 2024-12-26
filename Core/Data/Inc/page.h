#ifndef __PAGE_H__
#define __PAGE_H__

#include <stddef.h>
#include <stdint.h>

typedef enum {
  PAGE_POLICY_STATIC,
  PAGE_POLICY_POOL,
  PAGE_POLICY_DYNAMIC,
  PAGE_POLICY_ERROR
} page_policy_t;

typedef struct {
  uint32_t *raw;
  size_t size;
  page_policy_t policy;
} page_t;

void page_alloc(page_t *page, page_policy_t policy, size_t size);
void page_free(page_t *page);

#endif
