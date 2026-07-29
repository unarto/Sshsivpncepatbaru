#include "buffer.h"
#include <stdlib.h>
#include <string.h>

// --- Dynamic Buffer Implementation ---

DynamicBuffer* buffer_create(size_t initial_capacity) {
    if (initial_capacity == 0) initial_capacity = 1024;
    
    DynamicBuffer* buf = (DynamicBuffer*)malloc(sizeof(DynamicBuffer));
    if (!buf) return NULL;
    
    buf->data = (uint8_t*)malloc(initial_capacity);
    if (!buf->data) {
        free(buf);
        return NULL;
    }
    
    buf->capacity = initial_capacity;
    buf->size = 0;
    buf->offset = 0;
    return buf;
}

void buffer_destroy(DynamicBuffer* buf) {
    if (buf) {
        if (buf->data) free(buf->data);
        free(buf);
    }
}

int buffer_append(DynamicBuffer* buf, const uint8_t* data, size_t len) {
    if (!buf || !data || len == 0) return 0;
    
    // Shift unread data back to index 0 if offset is substantial
    if (buf->offset > 1024 || (buf->offset > 0 && buf->size == 0)) {
        if (buf->size > 0) {
            memmove(buf->data, buf->data + buf->offset, buf->size);
        }
        buf->offset = 0;
    }
    
    // Check if capacity expansion is required
    if (buf->offset + buf->size + len > buf->capacity) {
        size_t new_cap = buf->capacity * 2;
        if (new_cap < buf->offset + buf->size + len) {
            new_cap = buf->offset + buf->size + len + 1024;
        }
        
        uint8_t* new_data = (uint8_t*)realloc(buf->data, new_cap);
        if (!new_data) return -1;
        
        buf->data = new_data;
        buf->capacity = new_cap;
    }
    
    memcpy(buf->data + buf->offset + buf->size, data, len);
    buf->size += len;
    return 0;
}

size_t buffer_consume(DynamicBuffer* buf, size_t len) {
    if (!buf || buf->size == 0 || len == 0) return 0;
    
    if (len > buf->size) {
        len = buf->size;
    }
    
    buf->offset += len;
    buf->size -= len;
    
    if (buf->size == 0) {
        buf->offset = 0;
    }
    
    return len;
}

void buffer_clear(DynamicBuffer* buf) {
    if (buf) {
        buf->size = 0;
        buf->offset = 0;
    }
}

uint8_t* buffer_get_read_ptr(DynamicBuffer* buf) {
    if (!buf || !buf->data || buf->size == 0) return NULL;
    return buf->data + buf->offset;
}

size_t buffer_get_readable_size(const DynamicBuffer* buf) {
    return buf ? buf->size : 0;
}

// --- Ring Buffer Implementation ---

RingBuffer* ring_buffer_create(size_t capacity) {
    if (capacity == 0) capacity = 65536; // 64KB default
    
    RingBuffer* rb = (RingBuffer*)malloc(sizeof(RingBuffer));
    if (!rb) return NULL;
    
    rb->buffer = (uint8_t*)malloc(capacity);
    if (!rb->buffer) {
        free(rb);
        return NULL;
    }
    
    rb->capacity = capacity;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
    return rb;
}

void ring_buffer_destroy(RingBuffer* rb) {
    if (rb) {
        if (rb->buffer) free(rb->buffer);
        free(rb);
    }
}

size_t ring_buffer_available_data(const RingBuffer* rb) {
    return rb ? rb->count : 0;
}

size_t ring_buffer_available_space(const RingBuffer* rb) {
    return rb ? (rb->capacity - rb->count) : 0;
}

size_t ring_buffer_write(RingBuffer* rb, const uint8_t* data, size_t len) {
    if (!rb || !data || len == 0) return 0;
    
    size_t space = ring_buffer_available_space(rb);
    if (space == 0) return 0;
    
    size_t to_write = (len < space) ? len : space;
    size_t chunk1 = rb->capacity - rb->head;
    
    if (chunk1 > to_write) chunk1 = to_write;
    size_t chunk2 = to_write - chunk1;
    
    memcpy(rb->buffer + rb->head, data, chunk1);
    if (chunk2 > 0) {
        memcpy(rb->buffer, data + chunk1, chunk2);
    }
    
    rb->head = (rb->head + to_write) % rb->capacity;
    rb->count += to_write;
    return to_write;
}

size_t ring_buffer_read(RingBuffer* rb, uint8_t* out_data, size_t max_len) {
    if (!rb || !out_data || max_len == 0) return 0;
    
    size_t available = ring_buffer_available_data(rb);
    if (available == 0) return 0;
    
    size_t to_read = (max_len < available) ? max_len : available;
    size_t chunk1 = rb->capacity - rb->tail;
    
    if (chunk1 > to_read) chunk1 = to_read;
    size_t chunk2 = to_read - chunk1;
    
    memcpy(out_data, rb->buffer + rb->tail, chunk1);
    if (chunk2 > 0) {
        memcpy(out_data + chunk1, rb->buffer, chunk2);
    }
    
    rb->tail = (rb->tail + to_read) % rb->capacity;
    rb->count -= to_read;
    return to_read;
}

size_t ring_buffer_peek(const RingBuffer* rb, uint8_t* out_data, size_t max_len) {
    if (!rb || !out_data || max_len == 0) return 0;
    
    size_t available = ring_buffer_available_data(rb);
    if (available == 0) return 0;
    
    size_t to_read = (max_len < available) ? max_len : available;
    size_t chunk1 = rb->capacity - rb->tail;
    
    if (chunk1 > to_read) chunk1 = to_read;
    size_t chunk2 = to_read - chunk1;
    
    memcpy(out_data, rb->buffer + rb->tail, chunk1);
    if (chunk2 > 0) {
        memcpy(out_data + chunk1, rb->buffer, chunk2);
    }
    
    return to_read;
}

size_t ring_buffer_advance_read(RingBuffer* rb, size_t len) {
    if (!rb || len == 0) return 0;
    
    size_t available = ring_buffer_available_data(rb);
    size_t to_advance = (len < available) ? len : available;
    
    rb->tail = (rb->tail + to_advance) % rb->capacity;
    rb->count -= to_advance;
    return to_advance;
}

void ring_buffer_clear(RingBuffer* rb) {
    if (rb) {
        rb->head = 0;
        rb->tail = 0;
        rb->count = 0;
    }
}
