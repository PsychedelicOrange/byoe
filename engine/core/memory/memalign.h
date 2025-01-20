#ifndef MEM_ALLOC_H
#define MEM_ALLOC_H

#include <stdint.h>    // For uintptr_t
#include <stddef.h>

size_t align_memory_size(size_t size, size_t alignment);

void* align_memory(void* ptr, size_t alignment);

#endif