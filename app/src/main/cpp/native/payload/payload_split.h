#ifndef NATIVE_PAYLOAD_SPLIT_H
#define NATIVE_PAYLOAD_SPLIT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "payload_delay.h"

// Define a chunk of payload to be sent.
// Supports Normal Split, Delay Split, etc.
typedef struct {
    char* data;
    size_t length;
    int delay_after_ms; // Delay applied AFTER this chunk is sent.
} PayloadChunkData;

typedef struct {
    PayloadChunkData* chunks;
    size_t count;
    size_t capacity;
} PayloadChunkList;

// Manage the ChunkList
void payload_split_list_init(PayloadChunkList* list);
void payload_split_list_add(PayloadChunkList* list, const char* data, size_t length, int delay_ms);
void payload_split_list_free(PayloadChunkList* list);
void payload_split_list_print(const PayloadChunkList* list);

// Split Engine State
typedef enum {
    PAYLOAD_SPLIT_STATE_IDLE,
    PAYLOAD_SPLIT_STATE_SENDING,
    PAYLOAD_SPLIT_STATE_WAIT_DELAY,
    PAYLOAD_SPLIT_STATE_DONE,
    PAYLOAD_SPLIT_STATE_ERROR
} PayloadSplitState;

// Asynchronous Split Engine Context
typedef struct {
    PayloadChunkList* list;
    size_t current_chunk_idx;
    size_t current_offset;
    PayloadSplitState state;
    uint64_t delay_target_ms;
    
    // Function pointer to perform non-blocking send on the socket
    // returns >0 (bytes sent), -1 (EAGAIN/EWOULDBLOCK), -2 (ERROR)
    int (*send_func)(void* user_data, const char* data, size_t length);
    void* user_data;
} PayloadSplitEngine;

void payload_split_engine_init(PayloadSplitEngine* engine, PayloadChunkList* list, 
                               int (*send_func)(void*, const char*, size_t), void* user_data);

// Drive the state machine forward (should be called when socket is writable or on timer tick)
// Returns 1 if done, 0 if pending (needs writable event or timer), -1 on error.
int payload_split_engine_process(PayloadSplitEngine* engine);

#endif // NATIVE_PAYLOAD_SPLIT_H
