#include "typed_ring_buffer.h"

#include <stdlib.h>

void ring_buffer_init(ring_buffer* rb, int element_size)
{
    rb->element_size = element_size;
    rb->head         = 0;
    rb->tail         = 0;
    rb->size         = 0;
}

void ring_buffer_destroy(ring_buffer* rb)
{
    rb->element_size = 0;
    rb->head         = 0;
    rb->tail         = 0;
    rb->size         = 0;
}

bool ring_buffer_is_full(const ring_buffer* rb)
{
    return rb->size == RING_BUFF_MAX_ELEMENTS;
}

bool ring_buffer_is_empty(const ring_buffer* rb)
{
    return rb->size == 0;
}

void* ring_buffer_get_write_buffer(ring_buffer* rb)
{
    void* write_buff = NULL;
    if (!ring_buffer_is_full(rb)) {
        write_buff = rb->buffer[rb->head];
        rb->head   = (rb->head + 1) % RING_BUFF_MAX_ELEMENTS;
        rb->size++;
        return write_buff;
    } else
        return write_buff;
}

void* ring_buffer_get_read_buffer(ring_buffer* rb)
{
    void* read_buff = NULL;
    if (!ring_buffer_is_empty(rb)) {
        read_buff = rb->buffer[rb->tail];
        rb->tail  = (rb->tail + 1) % RING_BUFF_MAX_ELEMENTS;
        rb->size++;
        return read_buff;
    } else
        return read_buff;
}
