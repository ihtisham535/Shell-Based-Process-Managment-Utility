#ifndef MEMORY_ALLOCATOR_H
#define MEMORY_ALLOCATOR_H

#include "common.h"

/* Memory Allocator Functions */
void* alloc_mem(size_t size);
void free_mem(void *ptr);
void init_allocator(void);
void cleanup_allocator(void);
size_t get_total_allocated(void);
size_t get_total_free(void);

#endif /* MEMORY_ALLOCATOR_H */

