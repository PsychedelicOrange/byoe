#ifndef HASH_MAP_H
#define HASH_MAP_H

#include <stddef.h>
#include <stdint.h>

// [source]: Hash Table Hash Function: FNV-1a
// https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function#FNV-1a_hash

/*******************************/
// HashMap
/*******************************/

typedef struct hash_map_pair_t
{
    const char* key;
    void* value;
} hash_map_pair_t;

typedef struct hash_map_t
{
    hash_map_pair_t* entries;
    size_t capacity;
    size_t lenght;
} hash_map_t;


#endif