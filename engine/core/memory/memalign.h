#ifndef MEM_ALLOC_H
#define MEM_ALLOC_H

#include <stddef.h>
#include <stdint.h>    // For uintptr_t

size_t align_memory_size(size_t size, size_t alignment);

void* align_memory(void* ptr, size_t alignment);

#endif