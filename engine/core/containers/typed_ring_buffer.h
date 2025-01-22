#ifndef TYPED_RING_BUFFER_H
#define TYPED_RING_BUFFER_H

#include <stdbool.h>

#define RING_BUFF_MAX_ELEMENTS 32

// This is strongly type DS

//#define TYPED_RING_BUFFER(Type, MaxSize) \

typedef struct ring_buffer
{
    void* buffer[RING_BUFF_MAX_ELEMENTS];
    int   head;
    int   tail;
    int   size;
    int   element_size;
} ring_buffer;

void ring_buffer_init(ring_buffer* rb, int element_size);
void ring_buffer_destroy(ring_buffer* rb);

bool ring_buffer_is_full(const ring_buffer* rb);
bool ring_buffer_is_empty(const ring_buffer* rb);

void* ring_buffer_get_write_buffer(ring_buffer* rb);
void* ring_buffer_get_read_buffer(ring_buffer* rb);

#endif TYPED_RING_BUFFER_H