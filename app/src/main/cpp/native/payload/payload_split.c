#include "payload_split.h"
#include "../common/logger.h"
#include <stdlib.h>
#include <string.h>

#define INITIAL_CHUNK_CAPACITY 8

void payload_split_list_init(PayloadChunkList* list) {
    if (!list) return;
    list->chunks = NULL;
    list->count = 0;
    list->capacity = 0;
}

void payload_split_list_add(PayloadChunkList* list, const char* data, size_t length, int delay_ms) {
    if (!list || !data || length == 0) return;
    
    if (list->count >= list->capacity) {
        list->capacity = (list->capacity == 0) ? INITIAL_CHUNK_CAPACITY : list->capacity * 2;
        PayloadChunkData* new_chunks = (PayloadChunkData*)realloc(list->chunks, sizeof(PayloadChunkData) * list->capacity);
        if (!new_chunks) {
            LOGE("Payload Split: Memory allocation failed");
            return;
        }
        list->chunks = new_chunks;
    }
    
    PayloadChunkData* chunk = &list->chunks[list->count];
    chunk->data = (char*)malloc(length);
    if (chunk->data) {
        memcpy(chunk->data, data, length);
        chunk->length = length;
        chunk->delay_after_ms = delay_ms;
        list->count++;
    }
}

void payload_split_list_free(PayloadChunkList* list) {
    if (!list) return;
    for (size_t i = 0; i < list->count; i++) {
        if (list->chunks[i].data) {
            free(list->chunks[i].data);
        }
    }
    if (list->chunks) {
        free(list->chunks);
    }
    list->chunks = NULL;
    list->count = 0;
    list->capacity = 0;
}

void payload_split_list_print(const PayloadChunkList* list) {
    if (!list) return;
    LOGI("=== Payload Split Chunks ===");
    for (size_t i = 0; i < list->count; i++) {
        LOGI("Chunk %zu: [%zu bytes] delay=%d ms", i, list->chunks[i].length, list->chunks[i].delay_after_ms);
    }
    LOGI("============================");
}

void payload_split_engine_init(PayloadSplitEngine* engine, PayloadChunkList* list, 
                               int (*send_func)(void*, const char*, size_t), void* user_data) {
    if (!engine) return;
    engine->list = list;
    engine->current_chunk_idx = 0;
    engine->current_offset = 0;
    engine->state = (list && list->count > 0) ? PAYLOAD_SPLIT_STATE_SENDING : PAYLOAD_SPLIT_STATE_DONE;
    engine->delay_target_ms = 0;
    engine->send_func = send_func;
    engine->user_data = user_data;
}

int payload_split_engine_process(PayloadSplitEngine* engine) {
    if (!engine || !engine->list) return -1;
    
    if (engine->state == PAYLOAD_SPLIT_STATE_DONE) return 1;
    if (engine->state == PAYLOAD_SPLIT_STATE_ERROR) return -1;
    
    while (engine->current_chunk_idx < engine->list->count) {
        
        if (engine->state == PAYLOAD_SPLIT_STATE_WAIT_DELAY) {
            if (!payload_delay_is_elapsed(engine->delay_target_ms)) {
                return 0; // Still waiting
            }
            // Delay elapsed, move to next chunk
            engine->state = PAYLOAD_SPLIT_STATE_SENDING;
            engine->current_chunk_idx++;
            engine->current_offset = 0;
            continue; // Re-evaluate in the loop
        }
        
        if (engine->state == PAYLOAD_SPLIT_STATE_SENDING) {
            // Check if we are done with all chunks
            if (engine->current_chunk_idx >= engine->list->count) {
                break;
            }
            
            PayloadChunkData* chunk = &engine->list->chunks[engine->current_chunk_idx];
            size_t remaining = chunk->length - engine->current_offset;
            
            if (remaining > 0 && engine->send_func) {
                int sent = engine->send_func(engine->user_data, 
                                             chunk->data + engine->current_offset, 
                                             remaining);
                if (sent > 0) {
                    engine->current_offset += sent;
                    if (engine->current_offset < chunk->length) {
                        return 0; // Not fully sent, wait for next writable event
                    }
                } else if (sent == -1) {
                    // EAGAIN / EWOULDBLOCK
                    return 0; 
                } else {
                    // Error
                    LOGE("Payload Split: Send error");
                    engine->state = PAYLOAD_SPLIT_STATE_ERROR;
                    return -1;
                }
            }
            
            // Finished sending current chunk, process delay if any
            if (chunk->delay_after_ms > 0) {
                engine->state = PAYLOAD_SPLIT_STATE_WAIT_DELAY;
                engine->delay_target_ms = payload_delay_set_target(chunk->delay_after_ms);
                return 0; // Yield to event loop to wait
            } else {
                // No delay, proceed to next chunk immediately
                engine->current_chunk_idx++;
                engine->current_offset = 0;
            }
        }
    }
    
    engine->state = PAYLOAD_SPLIT_STATE_DONE;
    return 1;
}
