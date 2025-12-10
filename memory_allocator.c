#include "memory_allocator.h"

#define POOL_SIZE (1024 * 1024 * 10)  /* 10MB pool */
#define MIN_BLOCK_SIZE sizeof(mem_block_t)

static char memory_pool[POOL_SIZE];
static mem_block_t *free_list = NULL;
static int allocator_initialized = 0;
static size_t total_allocated = 0;
static size_t total_free = POOL_SIZE;

/* Initialize memory allocator */
void init_allocator(void) {
    if (allocator_initialized) return;
    
    /* Initialize the entire pool as one free block */
    mem_block_t *initial_block = (mem_block_t*)memory_pool;
    initial_block->size = POOL_SIZE - sizeof(mem_block_t);
    initial_block->in_use = 0;
    initial_block->next = NULL;
    
    free_list = initial_block;
    allocator_initialized = 1;
    total_allocated = 0;
    total_free = POOL_SIZE - sizeof(mem_block_t);
    
    log_message("Memory allocator initialized\n");
}

/* Allocate memory */
void* alloc_mem(size_t size) {
    if (!allocator_initialized) {
        init_allocator();
    }
    
    if (size == 0) return NULL;
    
    /* Align size to 8 bytes */
    size = (size + 7) & ~7;
    size += sizeof(mem_block_t);
    
    mem_block_t *current = free_list;
    mem_block_t *prev = NULL;
    
    /* First fit algorithm */
    while (current != NULL) {
        if (!current->in_use && current->size >= size) {
            /* Found suitable block */
            if (current->size >= size + sizeof(mem_block_t) + MIN_BLOCK_SIZE) {
                /* Split the block */
                mem_block_t *new_block = (mem_block_t*)((char*)current + size);
                new_block->size = current->size - size;
                new_block->in_use = 0;
                new_block->next = current->next;
                
                current->size = size - sizeof(mem_block_t);
                current->next = new_block;
            }
            
            current->in_use = 1;
            total_allocated += current->size + sizeof(mem_block_t);
            total_free -= (current->size + sizeof(mem_block_t));
            
            /* Remove from free list */
            if (prev == NULL) {
                free_list = current->next;
            } else {
                prev->next = current->next;
            }
            
            return (void*)((char*)current + sizeof(mem_block_t));
        }
        prev = current;
        current = current->next;
    }
    
    /* No suitable block found */
    return NULL;
}

/* Free memory */
void free_mem(void *ptr) {
    if (ptr == NULL || !allocator_initialized) return;
    
    mem_block_t *block = (mem_block_t*)((char*)ptr - sizeof(mem_block_t));
    
    if (!block->in_use) {
        /* Double free detection */
        return;
    }
    
    block->in_use = 0;
    total_allocated -= (block->size + sizeof(mem_block_t));
    total_free += (block->size + sizeof(mem_block_t));
    
    /* Coalesce with adjacent free blocks */
    mem_block_t *current = (mem_block_t*)memory_pool;
    mem_block_t *last_free = NULL;
    
    /* Simple coalescing: add to free list and merge if adjacent */
    block->next = free_list;
    free_list = block;
    
    /* Try to merge with next block */
    mem_block_t *next_block = (mem_block_t*)((char*)block + block->size + sizeof(mem_block_t));
    if ((char*)next_block < (char*)memory_pool + POOL_SIZE && !next_block->in_use) {
        block->size += next_block->size + sizeof(mem_block_t);
        block->next = next_block->next;
    }
}

/* Get total allocated memory */
size_t get_total_allocated(void) {
    return total_allocated;
}

/* Get total free memory */
size_t get_total_free(void) {
    return total_free;
}

/* Cleanup allocator */
void cleanup_allocator(void) {
    if (allocator_initialized) {
        init_allocator();  /* Reset to initial state */
        log_message("Memory allocator cleaned up\n");
    }
}

