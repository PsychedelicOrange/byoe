#ifndef TYPED_GROWABLE_ARRAY_H
#define TYPED_GROWABLE_ARRAY_H

#include "../logging/log.h"

#include <stdint.h>

typedef struct TypedGrowableArray
{
    void*       data;
    uint32_t      size;
    uint32_t      capacity;
    uint32_t      element_size;
} TypedGrowableArray;

TypedGrowableArray typed_growable_array_create(uint32_t element_size, uint32_t initial_capacity);
void               typed_growable_array_free(TypedGrowableArray* array);

void* typed_growable_array_get_element(TypedGrowableArray* array, uint32_t index);
int   typed_growable_array_append(TypedGrowableArray* array, const void* element);
int   typed_growable_array_remove(TypedGrowableArray* array, uint32_t index);
void  typed_growable_array_iterate(const TypedGrowableArray* array, void (*callback)(void* element, uint32_t index));

#endif