#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

typedef struct random_uuid_t {
    uint32_t data[4]; // 32 bits * 4 = 128 bits
} random_uuid_t;

void uuid_generate(random_uuid_t* out);

void uuid_destroy(random_uuid_t* out);

void uuid_copy(const random_uuid_t* src, random_uuid_t* dest);

void uuid_print(const random_uuid_t* uuid);

char* uuid_to_string(const random_uuid_t* uuid);

// compare two UUIDs (returns 1 if equal, 0 if not)
int uuid_compare(const random_uuid_t* uuid1, const random_uuid_t* uuid2);

// check if a UUID is null (all bits are zero)
bool uuid_is_null(const random_uuid_t* uuid);
