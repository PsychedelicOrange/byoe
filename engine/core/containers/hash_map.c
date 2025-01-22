#include "hash_map.h"

#include <stdio.h>
#include <stdlib.h>

#include "common.h"

#include "simd/intrinsics.h"

#include "logging/log.h"
#include "uuid/uuid.h"

// [source]: Hash hash_map Hash Function: FNV-1a
// https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function#FNV-1a_hash

////////////////////////////////////////////////////////////
// Private API

// Hash function
static uint64_t murmur_hash_uuid(const random_uuid_t* uuid)
{
    const uint64_t seed = 0xc70f6907UL;            // Seed for hashing
    const uint64_t m    = 0xc6a4a7935bd1e995UL;    // Multiplier constant
    const int      r    = 47;                      // Right shift for mixing

    uint64_t hash = seed ^ (16 * m);    // UUID is 16 bytes (128 bits)

    const uint64_t* data = (const uint64_t*) uuid;

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

//-----------------------------------------------------------------
// Internal methods
static size_t hash_map_quadratic_probe(size_t index, size_t i, size_t capacity)
{
    return (index + i * i) % capacity;
}

static void hash_map_set_entry(hash_map_pair_t* hash_map_entries, size_t capacity, random_uuid_t key, void* value, size_t* plength)
{
    // first compute the hash for the key
    hash_map_entries->hash = murmur_hash_uuid((const random_uuid_t*) &key);
    // wrap it around capacity
    size_t index = (size_t) (hash_map_entries->hash & (uint64_t) (capacity - 1));
    size_t i     = 0;

    while (!uuid_is_null(&hash_map_entries[index].key)) {
        if (memcmp(key.data, &hash_map_entries[index].key.data, sizeof(random_uuid_t)) == 0) {
            // entry found update it (empty/existing)
            hash_map_entries[index].value = value;
            return;
        }

        // Quadratic probing
        i++;
        index = hash_map_quadratic_probe(index, i, capacity);
    }

    if (plength != NULL) {
        (*plength)++;
    }

    uuid_copy(&key, &hash_map_entries[index].key);
    hash_map_entries[index].value = value;
}

static bool hash_map_expand(hash_map_t* hash_map)
{
    //LOG_INFO("[Hash Map] Expanding hash map... previous size: %zu, new size: %zu\n", hash_map->capacity, hash_map->capacity * 2);
    // Allocate new entries array
    size_t new_capacity = hash_map->capacity * 2;
    if (new_capacity < hash_map->capacity) {
        LOG_ERROR("[Hash Map] Failed to expand, overflow!\n");
        return false;    // overflow (capacity would be too big)
    }

    hash_map_pair_t* new_entries = calloc(new_capacity, sizeof(hash_map_pair_t));
    if (new_entries == NULL) {
        LOG_ERROR("[Hash Map] Failed to allocate new entries using calloc!\n");
        return false;
    }

    // Iterate entries, move all non-empty ones to new hash_map's entries.
    for (size_t i = 0; i < hash_map->capacity; i++) {
        hash_map_pair_t entry = hash_map->entries[i];
        prefetch(&entry);
        if (!uuid_is_null(&entry.key)) {
            hash_map_set_entry(new_entries, new_capacity, entry.key, entry.value, NULL);
        }
    }

    // Free old entries array and update this hash_map's details.
    free(hash_map->entries);
    hash_map->entries  = new_entries;
    hash_map->capacity = new_capacity;

    //print_map(hash_map);

    return true;
}

////////////////////////////////////////////////////////////
// Public API

hash_map_t* hash_map_create(size_t initial_capacity)
{
    // key_value_pair will be allocated/freed on demand
    hash_map_t* hash_map = calloc(1, sizeof(hash_map_t));
    hash_map->capacity   = initial_capacity;
    hash_map->length     = 0;
    // create memory for all entries and init them to 0
    hash_map->entries = (hash_map_pair_t*) calloc(hash_map->capacity, sizeof(hash_map_pair_t));

    return hash_map;
}

void hash_map_destroy(hash_map_t* hash_map)
{
    for (size_t i = 0; i < hash_map->capacity; i++) {
        if (!uuid_is_null(&hash_map->entries[i].key)) {
            uuid_destroy(&hash_map->entries[i].key);
        }
    }
    free(hash_map->entries);
    hash_map->entries = NULL;
    free(hash_map);
    hash_map = NULL;
}

void hash_map_print(hash_map_t* map)
{
    printf(COLOR_BLUE "Hash Map Contents:\n" COLOR_RESET);
    if (!map)
        return;
    for (size_t i = 0; i < map->capacity; i++) {
        if (map->entries && !uuid_is_null(&map->entries[i].key) && map->entries[i].value)
            printf("[%zu] Key: %s, Value: %s\n", i, uuid_to_string(&map->entries[i].key), (char*) map->entries[i].value);
        else
            printf("[%zu] Key: NULL, Value: NULL\n", i);
    }
    printf("\n");
}

void* hash_map_get_value(const hash_map_t* hash_map, random_uuid_t key)
{
    // Compute the hash for the key
    uint64_t hash  = murmur_hash_uuid((const random_uuid_t*) &key);
    size_t   index = (size_t) (hash & (uint64_t) (hash_map->capacity - 1));
    size_t   i     = 0;

    // Prefetch memory ahead of the lookup
    prefetch(&hash_map->entries[index]);

    while (i < hash_map->capacity)    // Optimized loop to reduce condition checks
    {
        // If key is not NULL, compare it
        if (!uuid_is_null(&hash_map->entries[index].key)) {
            // Compare the key using memcmp for faster comparison
            if (memcmp(key.data, hash_map->entries[index].key.data, sizeof(random_uuid_t)) == 0) {
                return hash_map->entries[index].value;
            }
        }

        i++;
        index = hash_map_quadratic_probe(index, i, hash_map->capacity);

        // Prefetch the next entry
        prefetch(&hash_map->entries[index]);
    }

    LOG_ERROR("-----------------------------\n");
    LOG_ERROR("hash_map value not found! INDEX : %zu KEY: %s\n", index, uuid_to_string(&key));
    LOG_ERROR("-----------------------------\n");
    return NULL;
}

void hash_map_set_key_value(hash_map_t* hash_map, random_uuid_t key, void* value)
{
    // If length will exceed half of current capacity, expand it.
    if (hash_map->length > hash_map->capacity / 2) {
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

void hash_map_remove_entry(hash_map_t* hash_map, random_uuid_t key)
{
    hash_map_iterator_t it = hash_map_iterator(hash_map, key);
    if (!uuid_is_null(&it.current_pair.key)) {
        uuid_destroy(&it.current_pair.key);
        uuid_destroy(&hash_map->entries[it.index].key);
    }
    if (it.current_pair.value != NULL) {
        it.current_pair.value             = NULL;
        hash_map->entries[it.index].value = NULL;
    }
}

hash_map_iterator_t hash_map_iterator_begin(hash_map_t* hash_map)
{
    hash_map_iterator_t it;
    it.hash_map_ref = hash_map;
    it.index        = 0;
    return it;
}

hash_map_iterator_t hash_map_iterator(hash_map_t* hash_map, random_uuid_t key)
{
    hash_map_iterator_t it;
    it.hash_map_ref = hash_map;

    // First compute the hash for the key
    uint64_t hash = murmur_hash_uuid((const random_uuid_t*) &key);
    // Wrap it around capacity
    it.index = (size_t) (hash & (uint64_t) (hash_map->capacity - 1));
    size_t i = 0;

    prefetch(&hash_map->entries[it.index]);

    // Start probing for the key
    while (it.index < hash_map->capacity) {
        // If we find an empty entry (NULL key), the key is not present
        if (!uuid_is_null(&hash_map->entries[it.index].key)) {
            // If the key matches, set the iterator's current pair and return
            if (memcmp(key.data, hash_map->entries[it.index].key.data, sizeof(random_uuid_t)) == 0) {
                it.current_pair.key   = hash_map->entries[it.index].key;
                it.current_pair.value = hash_map->entries[it.index].value;
                hash_map->length--;
                return it;
            }
        }

        i++;
        it.index = hash_map_quadratic_probe(it.index, i, hash_map->capacity);

        prefetch(&hash_map->entries[it.index]);
    }

    // If not found, return an invalid iterator
    it.current_pair.value = NULL;
    return it;
}

bool hash_map_parse_next(hash_map_iterator_t* iterator)
{
    hash_map_t* hash_map = iterator->hash_map_ref;

    while (iterator->index < hash_map->capacity) {
        size_t i = iterator->index;
        iterator->index++;
        if (!uuid_is_null(&hash_map->entries[i].key)) {
            // Found next non-empty item, update iterator key and value.
            hash_map_pair_t entry        = hash_map->entries[i];
            iterator->current_pair.key   = entry.key;
            iterator->current_pair.value = entry.value;
            return true;
        }
    }
    return false;
}
