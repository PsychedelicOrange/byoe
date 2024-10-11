#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

typedef struct uuid_t {
    uint32_t data[4]; // 32 bits * 4 = 128 bits
} uuid_t;

// TODO: Refactor this to return uuid_t instead
void uuid_generate(uuid_t* out);

void uuid_copy(const uuid_t* src, uuid_t* dest);

void uuid_print(const uuid_t* uuid);

// compare two UUIDs (returns 1 if equal, 0 if not)
int uuid_compare(const uuid_t* uuid1, const uuid_t* uuid2);

// check if a UUID is null (all bits are zero)
bool uuid_is_null(const uuid_t* uuid);
