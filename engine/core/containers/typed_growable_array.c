#include "typed_growable_array.h"

#include <stdlib.h>
#include <string.h>

TypedGrowableArray typed_growable_array_create(uint32_t element_size, uint32_t initial_capacity)
{
    TypedGrowableArray array;
    array.data         = malloc(element_size * initial_capacity);
    array.size         = 0;
    array.capacity     = initial_capacity;
    array.element_size = element_size;
    return array;
}

void typed_growable_array_free(TypedGrowableArray* array)
{
    if (array->data) {
        free(array->data);
        array->data = NULL;
    }
    array->size         = 0;
    array->capacity     = 0;
    array->element_size = 0;
}

void* typed_growable_array_get_element(TypedGrowableArray* array, uint32_t index)
{
    return (char*) array->data + (index * array->element_size);
}

int typed_growable_array_append(TypedGrowableArray* array, const void* element)
{
    if (array->size >= array->capacity) {
        uint32_t new_capacity = array->capacity * 2;
        void*    new_data     = realloc(array->data, new_capacity * array->element_size);
        if (!new_data) return -1;
        array->data     = new_data;
        array->capacity = new_capacity;
    }
    memcpy((char*) array->data + (array->size * array->element_size), element, array->element_size);
    array->size++;
    return 0;
}

int typed_growable_array_remove(TypedGrowableArray* array, uint32_t index)
{
    if (index >= array->size) return -1;
    void*    dest          = (char*) array->data + (index * array->element_size);
    void*    src           = (char*) array->data + ((index + 1) * array->element_size);
    uint32_t bytes_to_move = (array->size - index - 1) * array->element_size;
    memmove(dest, src, bytes_to_move);
    array->size--;
    return 0;
}

void typed_growable_array_iterate(const TypedGrowableArray* array, void (*callback)(void* element, uint32_t index))
{
    for (uint32_t i = 0; i < array->size; i++) {
        void* element = (char*) array->data + (i * array->element_size);
        callback(element, i);
    }
}