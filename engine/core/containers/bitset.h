#ifndef BITSET_H
#define BITSET_H

#include <stdbool.h>
#include <stdint.h>

typedef struct Bitset
{
    bool*    bits;
    uint32_t size;
} Bitset;

Bitset bitset_create(uint32_t num_bits);
void   bitset_destroy(Bitset* bitset);

void bitset_set(Bitset* bitset, uint32_t index);
void bitset_clear(Bitset* bitset, uint32_t index);
bool bitset_get(Bitset* bitset, uint32_t index);

#endif    // BITSET_H