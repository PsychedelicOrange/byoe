#ifndef TYPED_RING_BUFFER_H
#define TYPED_RING_BUFFER_H

#include <stdbool.h>

#define RING_BUFF_MAX_ELEMENTS 32

typedef struct ring_buffer
{
    void* buffer[RING_BUFF_MAX_ELEMENTS];
    int   head;
    int   tail;
    int   size;
    int   max_size;
} ring_buffer;

void ring_buffer_init(ring_buffer* rb, int max_size);

bool ring_buffer_is_full(const ring_buffer* rb);
bool ring_buffer_is_empty(const ring_buffer* rb);

void* ring_buffer_get_write_buffer(ring_buffer* rb);
void* ring_buffer_get_read_buffer(ring_buffer* rb);

// ring_buffer can be strongly typed with safety checks using macros magic
// we can wrap functions over this API

#define TYPED_RING_BUFFER(Type, MaxSize)                                 \
    typedef struct                                                       \
    {                                                                    \
        Type buffer[MaxSize];                                            \
        int  head;                                                       \
        int  tail;                                                       \
        int  size;                                                       \
    } ring_buffer_##Type;                                                \
                                                                         \
    void ring_buffer_init_##Type(ring_buffer_##Type* rb)                 \
    {                                                                    \
        rb->head     = 0;                                                \
        rb->tail     = 0;                                                \
        rb->size     = 0;                                                \
        rb->max_size = MaxSize;                                          \
    }                                                                    \
                                                                         \
    bool ring_buffer_is_full_##Type(const ring_buffer_##Type* rb)        \
    {                                                                    \
        return rb->size == MaxSize;                                      \
    }                                                                    \
                                                                         \
    bool ring_buffer_is_empty_##Type(const ring_buffer_##Type* rb)       \
    {                                                                    \
        return rb->size == 0;                                            \
    }                                                                    \
                                                                         \
    bool ring_buffer_enqueue_##Type(ring_buffer_##Type* rb, Type value)  \
    {                                                                    \
        if (ring_buffer_is_full_##Type(rb)) {                            \
            return false;                                                \
        }                                                                \
        rb->buffer[rb->head] = value;                                    \
        rb->head             = (rb->head + 1) % MaxSize;                 \
        rb->size++;                                                      \
        return true;                                                     \
    }                                                                    \
                                                                         \
    bool ring_buffer_dequeue_##Type(ring_buffer_##Type* rb, Type* value) \
    {                                                                    \
        if (ring_buffer_is_empty_##Type(rb)) {                           \
            return false;                                                \
        }                                                                \
        *value   = rb->buffer[rb->tail];                                 \
        rb->tail = (rb->tail + 1) % MaxSize;                             \
        rb->size--;                                                      \
        return true;                                                     \
    }

#endif TYPED_RING_BUFFER_H