#ifndef NATIVE_BUFFER_H
#define NATIVE_BUFFER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Dynamic Buffer (Linear expandable buffer for packet assembling and queuing)
typedef struct {
    uint8_t* data;
    size_t capacity;
    size_t size;    // total unread bytes currently held
    size_t offset;  // read offset in data buffer
} DynamicBuffer;

DynamicBuffer* buffer_create(size_t initial_capacity);
void buffer_destroy(DynamicBuffer* buf);
int buffer_append(DynamicBuffer* buf, const uint8_t* data, size_t len);
size_t buffer_consume(DynamicBuffer* buf, size_t len);
void buffer_clear(DynamicBuffer* buf);
uint8_t* buffer_get_read_ptr(DynamicBuffer* buf);
size_t buffer_get_readable_size(const DynamicBuffer* buf);

// Ring Buffer (Circular fixed-size high-throughput buffer for streaming I/O)
typedef struct {
    uint8_t* buffer;
    size_t capacity;
    size_t head;  // write index
    size_t tail;  // read index
    size_t count; // number of unread bytes
} RingBuffer;

RingBuffer* ring_buffer_create(size_t capacity);
void ring_buffer_destroy(RingBuffer* rb);
size_t ring_buffer_write(RingBuffer* rb, const uint8_t* data, size_t len);
size_t ring_buffer_read(RingBuffer* rb, uint8_t* out_data, size_t max_len);
size_t ring_buffer_peek(const RingBuffer* rb, uint8_t* out_data, size_t max_len);
size_t ring_buffer_advance_read(RingBuffer* rb, size_t len);
size_t ring_buffer_available_data(const RingBuffer* rb);
size_t ring_buffer_available_space(const RingBuffer* rb);
void ring_buffer_clear(RingBuffer* rb);

#endif // NATIVE_BUFFER_H
