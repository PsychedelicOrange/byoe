#include "hash_map.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h> // strcmp

#include "common.h"

#include "uuid/uuid.h"
#include "logging/log.h"

// TODO:
// [ ] use memcmp instead of strcmp
// [ ] use __builtin_prefetch for array elements
// [ ] Improve collision handling: use quadratic probing or linked lists
// [ ] Fix get_key_value bug: add a new test

// void print_map(hash_map_t* map) {
//     printf(COLOR_BLUE "Hash Map Contents:\n" COLOR_RESET);
//     if (!map)
//         return;
//     for (size_t i = 0; i < map->capacity; i++)
//     {
//         if (map->entries && map->entries[i].key && map->entries[i].value)
//             printf("Key: %s, Value: %s\n", map->entries[i].key, (char*)map->entries[i].value);
//     }
//     printf("\n");
// }

// [source]: Hash hash_map Hash Function: FNV-1a
// https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function#FNV-1a_hash
// https://benhoyt.com/writings/hash-hash_map-in-c/

////////////////////////////////////////////////////////////
// Private API

// Hash functions

#define FNV_offset_basis    14695981039346656037UL
#define FNV_prime           1099511628211UL

// static uint64_t fnv1a_hash(uint8_t* key)
// {
//     uint64_t hash = FNV_offset_basis;
//     while (*key) {
//         hash ^= (uint64_t)(unsigned char*)(key); // bitwise XOR
//         hash *= FNV_prime;                       // multiply
//         key++;
//     }
//     return hash;
// }

static uint64_t murmur_hash_uuid(const uuid_t* uuid)
{
    const uint64_t seed = 0xc70f6907UL;         // Seed for hashing
    const uint64_t m = 0xc6a4a7935bd1e995UL;    // Multiplier constant
    const int r = 47;                           // Right shift for mixing

    uint64_t hash = seed ^ (16 * m);            // UUID is 16 bytes (128 bits)

    const uint64_t* data = (const uint64_t*)uuid;

    // Hash the first 64 bits
    uint64_t k1 = data[0];
    k1 *= m;
    k1 ^= k1 >> r;
    k1 *= m;
    hash ^= k1;
    hash *= m;

    // Hash the second 64 bits
    uint64_t k2 = data[1];
    k2 *= m;
    k2 ^= k2 >> r;
    k2 *= m;
    hash ^= k2;
    hash *= m;

    // Final mixing of the hash to reduce entropy
    hash ^= hash >> r;
    hash *= m;
    hash ^= hash >> r;

    return hash;
}

// Internal methods

static size_t quadratic_probe(size_t index, size_t i, size_t capacity)
{
    return (index + i * i) & (capacity - 1);
}

static void hash_map_set_entry(hash_map_pair_t* hash_map_entries, size_t capacity, const char* key, void* value, size_t* plength)
{
    // first compute the hash for the key
    uint64_t hash = murmur_hash_uuid((const uuid_t*)key);
    // wrap it around capacity
    size_t index = (size_t)(hash & (uint64_t)(capacity - 1));

    while (hash_map_entries[index].key != NULL)
    {
        if (memcmp(key, hash_map_entries[index].key, sizeof(uuid_t)) == 0) {
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
		{
			char* tmp = malloc(sizeof(char) * (strlen(key) + 1));
			if(tmp == NULL)return;
			memcpy(tmp,key,sizeof(char) * (strlen(key) + 1));
			key = tmp;
		}
        (*plength)++;
    }
    hash_map_entries[index].key = (char*)key;
    hash_map_entries[index].value = value;
}

static bool hash_map_expand(hash_map_t* hash_map)
{
    LOG_ERROR("[Hash Map] Expanding hash map... previous size: %zu new size: %zu\n", hash_map->capacity, hash_map->capacity * 2);
    // Allocate new entries array
    size_t new_capacity = hash_map->capacity * 2;
    if (new_capacity < hash_map->capacity) {
        LOG_ERROR("[Hash Map] Failed to expand, overflow!\n");
        return false;  // overflow (capacity would be too big)
    }

    hash_map_pair_t* new_entries = calloc(new_capacity, sizeof(hash_map_pair_t));
    if (new_entries == NULL) {
        LOG_ERROR("[Hash Map] Failed to allocate new entries using calloc!\n");
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
        if (hash_map->entries[i].key) {
            free((void*)hash_map->entries[i].key);
            hash_map->entries[i].key = NULL;
        }
    }
    free(hash_map->entries);
    hash_map->entries = NULL;
    free(hash_map);
    hash_map = NULL;
}

void* hash_map_get_value(const hash_map_t* hash_map, const char* key)
{
    // Compute the hash for the key
    uint64_t hash = murmur_hash_uuid((const uuid_t*)key);
    size_t index = (size_t)(hash & (uint64_t)(hash_map->capacity - 1));
    size_t ogIndex = index;  // Original index to detect wrap-around

    // Prefetch memory ahead of the lookup
    __builtin_prefetch(&hash_map->entries[index], 0, 1);

    while (1)  // Optimized loop to reduce condition checks
    {
        // If key is not NULL, compare it
        if (hash_map->entries[index].key != NULL) {
            // Compare the key using memcmp for faster comparison
            if (memcmp(key, hash_map->entries[index].key, strlen(key) + 1) == 0) {
                return hash_map->entries[index].value;
            }
        }

        // Linear probing to the next index
        index = (index + 1) & (hash_map->capacity - 1);

        // Prefetch the next entry
        __builtin_prefetch(&hash_map->entries[index], 0, 1);

        // If we have come full circle and didn't find the key, exit
        if (index == ogIndex) {
            break;
        }
    }

    LOG_SUCCESS("-----------------------------\n");
    LOG_ERROR("hash_map value not found! INDEX : %zu KEY: %s\n", index, uuid_to_string((uuid_t*)key));
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
    if (it.current_pair.key != NULL) {
        free(it.current_pair.key);
        it.current_pair.key = NULL;
        hash_map->entries[it.index].key = NULL;
    }
    if (it.current_pair.value != NULL) {
        it.current_pair.value = NULL;
        hash_map->entries[it.index].value = NULL;
    }
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

    // First compute the hash for the key
    uint64_t hash = murmur_hash_uuid((const uuid_t*)key);
    // Wrap it around capacity
    it.index = (size_t)(hash & (uint64_t)(hash_map->capacity - 1));

    size_t ogIndex = it.index;  // Original index to detect wrap-around
    bool wrapped = false;

    // Start probing for the key
    while (true)
    {
        // If we find an empty entry (NULL key), the key is not present
        if (hash_map->entries[it.index].key != NULL) {
            // If the key matches, set the iterator's current pair and return
            if (strcmp(key, hash_map->entries[it.index].key) == 0) {
                it.current_pair.key = hash_map->entries[it.index].key;
                it.current_pair.value = hash_map->entries[it.index].value;
                hash_map->length--;
                return it;
            }
        }

        // Linear probing to the next index
        it.index++;
        if (it.index >= hash_map->capacity) {
            it.index = 0;  // Wrap around to the beginning
            wrapped = true;
        }

        // If we have come full circle and didn't find the key, exit
        if (it.index == ogIndex && wrapped == true) {
            break;
        }
    }

    // If not found, return an invalid iterator
    it.current_pair.key = NULL;
    it.current_pair.value = NULL;
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