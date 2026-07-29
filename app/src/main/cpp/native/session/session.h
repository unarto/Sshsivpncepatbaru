#ifndef SESSION_H
#define SESSION_H

#include <stdint.h>
#include <libssh2.h>
#include "../common/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CONN_STATE_DISCONNECTED = 0,
    CONN_STATE_CONNECTING,
    CONN_STATE_PAYLOAD_HANDSHAKE,
    CONN_STATE_PROXY_HANDSHAKE,
    CONN_STATE_TLS_HANDSHAKE,
    CONN_STATE_SSH_HANDSHAKE,
    CONN_STATE_SSH_AUTH,
    CONN_STATE_CONNECTED,
    CONN_STATE_RECONNECTING,
    CONN_STATE_ERROR
} ConnectionState;

#define PROTO_DIRECT      0
#define PROTO_HTTP_PROXY  1
#define PROTO_SOCKS4      2
#define PROTO_SOCKS5      3
#define PROTO_WEBSOCKET   4
#define PROTO_SSL         5

typedef struct ChannelContext {
    /* Socket & Layer FDs */
    int socket_fd;
    void *ssl_ctx;            /* MbedTLS Context Pointer */
    void *ws_ctx;             /* WebSocket Context Pointer */
    LIBSSH2_SESSION *ssh_session;
    LIBSSH2_CHANNEL *ssh_channel;

    /* Host & Routing Config */
    char hostname[256];
    int port;
    char proxy_host[256];
    int proxy_port;
    char sni[256];

    /* Protocols & Modes */
    int protocol;             /* DIRECT, HTTP_PROXY, SOCKS4, SOCKS5, WEBSOCKET, SSL */
    int keepalive_type;       /* NONE, TCP, SSH_IGNORE, WS_PING, HTTP_HEAD */
    
    /* State & Status */
    ConnectionState state;
    int last_error;           /* enum EngineError */

    /* Timing & KeepAlive (CLOCK_MONOTONIC) */
    uint64_t timeout_ms;
    uint64_t last_read_time;
    uint64_t last_write_time;
    uint64_t last_ping_time;
    int reconnect_counter;

    /* Monitoring & Statistics */
    uint64_t bytes_sent;
    uint64_t bytes_received;
    uint32_t latency_ms;

    /* Buffers & Data Stream */
    void *rx_buffer;          /* DynamicBuffer* */
    void *tx_buffer;          /* DynamicBuffer* */
} ChannelContext;

#ifdef __cplusplus
}
#endif

#endif // SESSION_H
