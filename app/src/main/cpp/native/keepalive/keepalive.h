#ifndef NATIVE_KEEPALIVE_H
#define NATIVE_KEEPALIVE_H

#include <stdbool.h>
#include <stdint.h>
#include <libssh2.h>
#include "../transport/ssl_client.h"

typedef enum {
    KEEPALIVE_TYPE_NONE = 0,
    KEEPALIVE_TYPE_TCP = 1,
    KEEPALIVE_TYPE_SSH_IGNORE = 2,
    KEEPALIVE_TYPE_WS_PING = 3,
    KEEPALIVE_TYPE_HTTP_HEAD = 4
} KeepaliveType;

// Set TCP KeepAlive socket options (SO_KEEPALIVE, TCP_KEEPIDLE, TCP_KEEPINTVL, TCP_KEEPCNT)
int keepalive_set_tcp_options(int fd, int idle_sec, int interval_sec, int count);

// Send SSH Ignore / KeepAlive message using libssh2
int keepalive_send_ssh(LIBSSH2_SESSION *session);

// Send WebSocket Ping frame
int keepalive_send_ws_ping(int fd, bool use_tls, TlsContext *tls_ctx);

// Send HTTP HEAD request to maintain HTTP proxy/tunnel session
int keepalive_send_http_head(int fd, const char *host, bool use_tls, TlsContext *tls_ctx);

// Periodically send ping based on type
void keepalive_periodic(int fd, LIBSSH2_SESSION *session, const char *host, bool use_tls, TlsContext *tls_ctx, KeepaliveType type);

void keepalive_start(void);

#endif
