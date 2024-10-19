#ifndef HASH_MAP_H
#define HASH_MAP_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/*******************************/
// HashMap
/*******************************/

typedef struct hash_map_pair_t
{
    char* key;
    void* value;
    uint64_t hash; 
} hash_map_pair_t;

typedef struct hash_map_t
{
    hash_map_pair_t* entries;
    size_t capacity;
    size_t length;
} hash_map_t;

typedef struct hash_map_iterator_t
{
    hash_map_t* hash_map_ref;
    hash_map_pair_t current_pair;
    size_t index; //current index of the iterator in the hash_map
} hash_map_iterator_t;

hash_map_t* hash_map_create(size_t initial_capacity);
void hash_map_destroy(hash_map_t* hash_map);

void* hash_map_get_value(const hash_map_t* hash_map, const char* key);
void hash_map_set_key_value(hash_map_t* hash_map, const char* key, void* value);
void hash_map_set_key_value_pair(hash_map_t* hash_map, hash_map_pair_t* pair);

void hash_map_remove_entry(hash_map_t* hash_map, const char* key);

hash_map_iterator_t hash_map_iterator_begin(hash_map_t* hash_map);
hash_map_iterator_t hash_map_iterator(hash_map_t* hash_map, const char* key);
// This is used to find the next free available key in the hash_map
// don't use it for iterating over items, Only use it while accessing single element
// Use array like accessing while iterating over the entire list. 
bool hash_map_parse_next(hash_map_iterator_t* iterator);

// utils
inline size_t hash_map_get_length(const hash_map_t* hash_map) { return hash_map->length; }

inline size_t hash_map_get_capacity(const hash_map_t* hash_map) { return hash_map->length; }

#endif