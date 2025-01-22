#include "uuid.h"

#include "../rng/rng.h"

void uuid_generate(random_uuid_t* out)
{
    // Generate random 32-bit values for each part of the UUID
    for (int i = 0; i < 4; i++) {
        out->data[i] = rng_generate();    // Random 32-bit value
    }
}

void uuid_destroy(random_uuid_t* out)
{
    for (int i = 0; i < 4; i++) {
        out->data[i] = 0;    // Random 32-bit value
    }
}

void uuid_copy(const random_uuid_t* src, random_uuid_t* dest)
{
    memcpy(dest, src, sizeof(random_uuid_t));
}

void uuid_print(const random_uuid_t* uuid)
{
    printf("%08x-%08x-%08x-%08x\n", uuid->data[0], uuid->data[1], uuid->data[2], uuid->data[3]);
}

char* uuid_to_string(const random_uuid_t* uuid)
{
    // Allocate memory for the UUID string (8 characters * 4 segments + 3 dashes + null terminator)
    char* uuid_str = (char*) malloc(37);    // 36 characters + 1 for null terminator
    if (uuid_str == NULL) {
        return NULL;    // Handle memory allocation failure
    }

    // Format the UUID into the allocated string
    snprintf(uuid_str, 37, "%08x-%08x-%08x-%08x", uuid->data[0], uuid->data[1], uuid->data[2], uuid->data[3]);

    return uuid_str;
}

// compare two UUIDs (returns 1 if equal, 0 if not)
int uuid_compare(const random_uuid_t* uuid1, const random_uuid_t* uuid2)
{
    return memcmp(uuid1, uuid2, sizeof(random_uuid_t)) == 0;
}

/*
bool uuid_is_null(const random_uuid_t* uuid) {
    __m128i uuid_vec = _mm_loadu_si128((const __m128i*)uuid->data);

    __m128i zero_vec = _mm_setzero_si128();
    __m128i cmp_result = _mm_cmpeq_epi32(uuid_vec, zero_vec);

    int mask = _mm_movemask_epi8(cmp_result);

    return (mask == 0xFFFF);
}
*/

bool uuid_is_null(const random_uuid_t* uuid)
{
    return (uuid->data[0] | uuid->data[1] | uuid->data[2] | uuid->data[3]) == 0;
}