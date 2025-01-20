#ifndef INTRINSICS_H
#define INTRINSICS_H

void prefetch(const void* ptr);
int  likely(int expr);
int  unlikely(int expr);

// Function to perform prefetch for an array
void prefetch_array(const void* array, size_t size);

#endif