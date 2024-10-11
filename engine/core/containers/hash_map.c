#include "hash_map.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h> // strcmp

#include "uuid/uuid.h"

// [source]: Hash hash_map Hash Function: FNV-1a
// https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function#FNV-1a_hash
// https://benhoyt.com/writings/hash-hash_map-in-c/

////////////////////////////////////////////////////////////
// Private API

#define FNV_offset_basis    14695981039346656037UL
#define FNV_prime           1099511628211UL

static uint64_t fnv1a_hash(uint8_t* key)
{
    uint64_t hash = FNV_offset_basis;
    while (*key) {
        hash ^= (uint64_t)(unsigned char*)(key); // bitwise XOR
        hash *= FNV_prime;                       // multiply
        key++;
    }
    return hash;
}

static void hash_map_set_entry(hash_map_pair_t* hash_map_entries, size_t capacity, const char* key, void* value, size_t* plength)
{
    // first compute the hash for the key
    uint64_t hash = fnv1a_hash((uint8_t*)key);
    // wrap it around capacity
    size_t index = (size_t)(hash & (uint64_t)(capacity - 1));

    while (hash_map_entries[index].key != NULL)
    {
        if (strcmp(key, hash_map_entries[index].key) == 0) {
            // entry found update it (empty/existing)
            hash_map_entries[index].value = value;
            return;
        }

        // linear probing
        index++;
        if (index >= capacity)
            index = 0;
    }


    if (plength != NULL) {
        key = strdup(key);
        if (key == NULL) {
            return;
        }
        (*plength)++;
    }
    hash_map_entries[index].key = (char*)key;
    hash_map_entries[index].value = value;
}

static bool hash_map_expand(hash_map_t* hash_map)
{
    // Allocate new entries array
    size_t new_capacity = hash_map->capacity * 2;
    if (new_capacity < hash_map->capacity) {
        return false;  // overflow (capacity would be too big)
    }

    hash_map_pair_t* new_entries = calloc(new_capacity, sizeof(hash_map_pair_t));
    if (new_entries == NULL) {
        return false;
    }

    // Iterate entries, move all non-empty ones to new hash_map's entries.
    for (size_t i = 0; i < hash_map->capacity; i++) {
        hash_map_pair_t entry = hash_map->entries[i];
        if (entry.key != NULL) {
            hash_map_set_entry(new_entries, new_capacity, entry.key, entry.value, NULL);
        }
    }

    // Free old entries array and update this hash_map's details.
    free(hash_map->entries);
    hash_map->entries = new_entries;
    hash_map->capacity = new_capacity;
    return true;
}

////////////////////////////////////////////////////////////
// Public API

hash_map_t* hash_map_create(size_t initial_capacity)
{
    // key_value_pair will be allocated/freed on demand
    hash_map_t* hash_map = malloc(sizeof(hash_map_t));
    hash_map->capacity = initial_capacity;
    hash_map->length = 0;
    // create memory for all entries and init them to 0 
    hash_map->entries = (hash_map_pair_t*)calloc(hash_map->capacity, sizeof(hash_map_pair_t));

    return hash_map;
}

void hash_map_destroy(hash_map_t* hash_map)
{
    for (size_t i = 0; i < hash_map->capacity; i++) {
        if (hash_map->entries[i].key)
            free((void*)hash_map->entries[i].key);
        if (hash_map->entries[i].value)
            free((void*)hash_map->entries[i].value);

    }
    free(hash_map->entries);
    free(hash_map);
}

void* hash_map_get_value(const hash_map_t* hash_map, const char* key)
{
    // first comupte the hash for the key
    uint64_t hash = fnv1a_hash((uint8_t*)key);
    // wrap it around capacity
    size_t index = (size_t)(hash & (uint64_t)(hash_map->capacity - 1));

    // TODO: move this to a function bool hash_map_linear_probe(hash_map, key);
    // we use linear probing, if the index is unavailabel we linearly move forward
    // and wrapping the hash_map from start index until we find one
    while (hash_map->entries[index].key != NULL)
    {
        // if empty key found
        if (strcmp(key, hash_map->entries[index].key) == 0) {
            return hash_map->entries[index].value;
        }

        // linear probing
        index++;
        if (index >= hash_map->capacity)
            index = 0;
    }
    return NULL;
}

void hash_map_set_key_value(hash_map_t* hash_map, const char* key, void* value)
{
    // If length will exceed half of current capacity, expand it.
    if (hash_map->length >= hash_map->capacity / 2) {
        if (!hash_map_expand(hash_map)) {
            return;
        }
    }

    hash_map_set_entry(hash_map->entries, hash_map->capacity, key, value, &hash_map->length);
}

void hash_map_set_key_value_pair(hash_map_t* hash_map, hash_map_pair_t* pair)
{
    hash_map_set_key_value(hash_map, pair->key, pair->value);
}

void hash_map_remove_entry(hash_map_t* hash_map, const char* key)
{
    hash_map_iterator_t it = hash_map_iterator(hash_map, key);
    if (it.current_pair.key)
        free(it.current_pair.key);
    if (it.current_pair.value)
        free(it.current_pair.value);
}

hash_map_iterator_t hash_map_iterator_begin(hash_map_t* hash_map)
{
    hash_map_iterator_t it;
    it.hash_map_ref = hash_map;
    it.index = 0;
    return it;
}

hash_map_iterator_t hash_map_iterator(hash_map_t* hash_map, const char* key)
{
    hash_map_iterator_t it;
    it.hash_map_ref = hash_map;

    // first compute the hash for the key
    uint64_t hash = fnv1a_hash((uint8_t*)key);
    // wrap it around capacity
    it.index = (size_t)(hash & (uint64_t)(hash_map->capacity - 1));

    while (hash_map->entries[it.index].key != NULL)
    {
        // if empty key found
        if (strcmp(key, hash_map->entries[it.index].key) == 0) {
            it.current_pair = hash_map->entries[it.index];
        }

        // linear probing
        it.index++;
        if (it.index >= hash_map->capacity)
            it.index = 0;
    }

    return it;
}

bool hash_map_parse_next(hash_map_iterator_t* iterator)
{
    hash_map_t* hash_map = iterator->hash_map_ref;

    while (iterator->index < hash_map->capacity) {
        size_t i = iterator->index;
        iterator->index++;
        if (hash_map->entries[i].key != NULL) {
            // Found next non-empty item, update iterator key and value.
            hash_map_pair_t entry = hash_map->entries[i];
            iterator->current_pair.key = entry.key;
            iterator->current_pair.value = entry.value;
            return true;
        }
    }
    return false;
}

size_t hash_map_get_length(const hash_map_t* hash_map)
{
    return hash_map->length;
}

size_t hash_map_get_capacity(const hash_map_t* hash_map)
{
    return hash_map->length;
}