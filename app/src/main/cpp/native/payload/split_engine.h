#ifndef NATIVE_SPLIT_ENGINE_H
#define NATIVE_SPLIT_ENGINE_H

#include "../ssh/channel.h"
#include "../session/session.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

typedef enum {
    SE_STATE_IDLE,
    SE_STATE_SENDING_CHUNK,
    SE_STATE_WAIT_DELAY,
    SE_STATE_READ_RESPONSE,
    SE_STATE_DONE,
    SE_STATE_ERROR
} SplitEngineState;

typedef struct {
    ChannelContext *ctx;
    TunnelConfig cfg;
    int current_chunk;
    size_t chunk_offset;
    SplitEngineState state;
    uint64_t delay_until_ms;
    char resp_buf[4096];
    int resp_len;
} SplitEngineContext;

void se_init(SplitEngineContext *se, ChannelContext *ctx, const TunnelConfig *cfg);
int se_process(SplitEngineContext *se);
uint32_t se_get_events(SplitEngineContext *se);

#endif
