#ifndef MEM_ALLOC_H
#define MEM_ALLOC_H

#include <stdint.h>  // For uintptr_t


size_t align_memory_size(size_t size, size_t alignment)
{
    return (size + (alignment - 1)) & ~(alignment - 1);
}

void* align_memory(void* ptr, size_t alignment) {
    // Convert the pointer to an integer representation (uintptr_t)
    uintptr_t addr = (uintptr_t)ptr;

    // Calculate the aligned address by rounding up to the nearest multiple of 'alignment'
    uintptr_t aligned_addr = (addr + alignment - 1) & ~(alignment - 1);

    // Convert the aligned address back to a pointer and return
    return (void*)aligned_addr;
}

#endif