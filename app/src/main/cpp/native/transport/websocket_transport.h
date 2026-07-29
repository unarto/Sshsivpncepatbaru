#ifndef WEBSOCKET_TRANSPORT_H
#define WEBSOCKET_TRANSPORT_H

#include <stddef.h>
#include <sys/types.h>
#include "../session/session.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WS_STATE_INIT = 0,
    WS_STATE_HANDSHAKE_SENDING,
    WS_STATE_HANDSHAKE_READING,
    WS_STATE_CONNECTED
} WsTransportState;

typedef struct {
    WsTransportState state;
    char *path;
    char *origin;
    char *cookie;
    char *custom_headers;
    
    uint8_t *rx_buf;
    size_t rx_buf_size;
    size_t rx_buf_len;
} WsTransportContext;

/* Initialize WebSocket context for the channel */
int websocket_transport_init_ctx(ChannelContext *ctx, const char *path, const char *origin, const char *cookie, const char *custom_headers);

/* Setup WebSocket handshake non-blocking step.
 * Returns ENGINE_OK on completion, ENGINE_AGAIN if needs more IO, or error. 
 */
int websocket_transport_handshake_step(ChannelContext *ctx);

ssize_t websocket_transport_read(ChannelContext *ctx, void *buf, size_t len);
ssize_t websocket_transport_write(ChannelContext *ctx, const void *buf, size_t len);
void websocket_transport_close(ChannelContext *ctx);

#ifdef __cplusplus
}
#endif

#endif // WEBSOCKET_TRANSPORT_H
