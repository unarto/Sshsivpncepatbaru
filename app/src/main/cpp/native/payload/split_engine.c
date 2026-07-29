#include "split_engine.h"
#include "../transport/transport.h"
#include "../common/error.h"
#include <string.h>
#include <unistd.h>
#include <errno.h>

static uint64_t get_current_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

void se_init(SplitEngineContext *se, ChannelContext *ctx, const TunnelConfig *cfg) {
    memset(se, 0, sizeof(SplitEngineContext));
    se->ctx = ctx;
    memcpy(&se->cfg, cfg, sizeof(TunnelConfig));
    se->current_chunk = 0;
    se->chunk_offset = 0;
    
    if (se->cfg.num_payload_chunks <= 0) {
        se->state = SE_STATE_DONE;
    } else {
        se->state = SE_STATE_SENDING_CHUNK;
    }
}

int se_process(SplitEngineContext *se) {
    if (se->state == SE_STATE_DONE) return 1;
    if (se->state == SE_STATE_ERROR) return -1;

    if (se->state == SE_STATE_WAIT_DELAY) {
        if (get_current_time_ms() >= se->delay_until_ms) {
            se->current_chunk++;
            if (se->current_chunk >= se->cfg.num_payload_chunks) {
                if (se->cfg.modo_dropbear) {
                    LOGI("Payload: Modo Dropbear enabled, bypassing proxy response");
                    se->state = SE_STATE_DONE;
                    return 1;
                }
                se->state = SE_STATE_READ_RESPONSE;
            } else {
                se->chunk_offset = 0;
                se->state = SE_STATE_SENDING_CHUNK;
            }
        }
        return 0; // pending
    }

    if (se->state == SE_STATE_SENDING_CHUNK) {
        PayloadChunk *chunk = &se->cfg.payload_chunks[se->current_chunk];
        size_t len = strlen(chunk->content);
        if (len == 0) {
            se->state = SE_STATE_WAIT_DELAY;
            se->delay_until_ms = get_current_time_ms();
            return 0;
        }

        int r = transport_write(se->ctx, chunk->content + se->chunk_offset, len - se->chunk_offset);

        if (r > 0) {
            se->chunk_offset += r;
            if (se->chunk_offset >= len) {
                se->state = SE_STATE_WAIT_DELAY;
                uint64_t delay = (chunk->split_type == SPLIT_DELAY) ? 1000 : ((chunk->split_type == SPLIT_NORMAL) ? 200 : 0);
                se->delay_until_ms = get_current_time_ms() + delay;
            }
        } else if (r != ENGINE_AGAIN && r != ENGINE_INTR) {
            LOGE("Payload chunk write error: %d", r);
            se->state = SE_STATE_ERROR;
            return -1;
        }
        return 0;
    }

    if (se->state == SE_STATE_READ_RESPONSE) {
        int r = transport_read(se->ctx, se->resp_buf + se->resp_len, sizeof(se->resp_buf) - 1 - se->resp_len);

        if (r > 0) {
            se->resp_len += r;
            se->resp_buf[se->resp_len] = '\0';
            
            if (strstr(se->resp_buf, "\r\n\r\n") != NULL || strstr(se->resp_buf, "\n\n") != NULL) {
                LOGI("Payload Response: %s", se->resp_buf);
                if (strstr(se->resp_buf, "HTTP/") != NULL && strstr(se->resp_buf, "200") == NULL && strstr(se->resp_buf, "101") == NULL) {
                    LOGE("Payload response failed: not 200/101");
                    se->state = SE_STATE_ERROR;
                    return -1;
                }
                se->state = SE_STATE_DONE;
                return 1;
            }
        } else if (r == 0) {
            LOGE("Payload response failed: connection closed");
            se->state = SE_STATE_ERROR;
            return -1;
        } else if (r != ENGINE_AGAIN && r != ENGINE_INTR) {
            LOGE("Payload response read error: %d", r);
            se->state = SE_STATE_ERROR;
            return -1;
        }
    }

    return 0;
}

uint32_t se_get_events(SplitEngineContext *se) {
    if (se->state == SE_STATE_SENDING_CHUNK) return EVENT_WRITE;
    if (se->state == SE_STATE_READ_RESPONSE) return EVENT_READ;
    return 0; // wait delay or idle requires no fd events, handled by timer
}
