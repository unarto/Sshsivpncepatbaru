#ifndef PAYLOAD_TRANSPORT_H
#define PAYLOAD_TRANSPORT_H

#include <stddef.h>
#include <stdbool.h>
#include "../ssh/channel.h"

// Note: PayloadChunk is defined in engine/engine.h

// We define a list type for the builder
typedef struct {
    PayloadChunk* chunks;
    size_t chunk_count;
    size_t chunk_capacity;
} PayloadChunkList;

typedef struct {
    char* raw_payload;
    char* host;
    int port;
    
    // Auth info if any
    char* proxy_user;
    char* proxy_pass;

    // Generated chunks
    PayloadChunkList chunk_list;
} PayloadContext;

void payload_transport_init(PayloadContext* ctx, const char* raw, const char* host, int port);
void payload_transport_set_auth(PayloadContext* ctx, const char* user, const char* pass);
void payload_transport_build(PayloadContext* ctx);
void payload_transport_free(PayloadContext* ctx);

#endif // PAYLOAD_TRANSPORT_H
