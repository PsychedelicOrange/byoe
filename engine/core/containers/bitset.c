#include "bitset.h"

#include <stdlib.h>

#include "../memory/memalign.h"

Bitset bitset_create(uint32_t num_bits)
{
    Bitset   bitset    = {0};
    uint32_t num_bools = (uint32_t) align_memory_size(num_bits, 8);
    bitset.bits        = (bool*) calloc(num_bools, sizeof(bool));
    bitset.size        = num_bools * sizeof(bool);
    return bitset;
}

void bitset_destroy(Bitset* bitset)
{
    free(bitset->bits);
}

void bitset_set(Bitset* bitset, uint32_t index)
{
    bitset->bits[index / 8] |= 1 << (index % 8);
}

void bitset_clear(Bitset* bitset, uint32_t index)
{
    bitset->bits[index / 8] &= ~(1 << (index % 8));
}

bool bitset_get(Bitset* bitset, uint32_t index)
{
    return (bitset->bits[index / 8] >> (index % 8)) & 1;
}
