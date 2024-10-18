#include "uuid.h"

#include "../rng/rng.h"

void uuid_generate(uuid_t* out)
{
    // Generate random 32-bit values for each part of the UUID
    for (int i = 0; i < 4; i++) {
        out->data[i] = rng_generate(); // Random 32-bit value
    }
}

void uuid_copy(const uuid_t* src, uuid_t* dest) 
{
    memcpy(dest, src, sizeof(uuid_t));
}

void uuid_print(const uuid_t* uuid) 
{
    printf("%08x-%08x-%08x-%08x\n", uuid->data[0], uuid->data[1], uuid->data[2], uuid->data[3]);
}

char* uuid_to_string(const uuid_t* uuid) 
{
    // Allocate memory for the UUID string (8 characters * 4 segments + 3 dashes + null terminator)
    char* uuid_str = (char*)malloc(37); // 36 characters + 1 for null terminator
    if (uuid_str == NULL) {
        return NULL; // Handle memory allocation failure
    }

    // Format the UUID into the allocated string
    snprintf(uuid_str, 37, "%08x-%08x-%08x-%08x",
             uuid->data[0], uuid->data[1], uuid->data[2], uuid->data[3]);

    return uuid_str;
}

// compare two UUIDs (returns 1 if equal, 0 if not)
int uuid_compare(const uuid_t* uuid1, const uuid_t* uuid2) 
{
    return memcmp(uuid1, uuid2, sizeof(uuid_t)) == 0;
}

// check if a UUID is null (all bits are zero)
bool uuid_is_null(const uuid_t* uuid) 
{
    for (int i = 0; i < 4; i++) {
        if (uuid->data[i] != 0) {
            return false; // Found a non-zero part, so it's not null
        }
    }
    return true; // All parts are zero, so it is null
}