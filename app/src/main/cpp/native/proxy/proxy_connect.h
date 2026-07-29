#ifndef NATIVE_PROXY_CONNECT_H
#define NATIVE_PROXY_CONNECT_H

#include <stdbool.h>
#include "../transport/transport.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PROXY_TYPE_DIRECT = 0,
    PROXY_TYPE_HTTP = 1,
    PROXY_TYPE_SOCKS4 = 2,
    PROXY_TYPE_SOCKS5 = 3
} ProxyType;

// Connects to the destination through a proxy.
// - ctx: ChannelContext with connected socket and initialized transport
// - dest_host, dest_port: the target SSH server
// - proxy_user, proxy_pass: credentials (if any, can be NULL)
// Returns 0 on success, -1 on error.
// Note: This blocks until the proxy handshake is complete. For a fully non-blocking engine,
// this should be state-machine driven, but for simplicity we can use transport_read/write with timeout.
int proxy_connect_socks5(ChannelContext *ctx, const char* dest_host, int dest_port, const char* proxy_user, const char* proxy_pass);

int proxy_connect_socks4(ChannelContext *ctx, const char* dest_host, int dest_port, const char* proxy_user);

// Direct just returns success if we are already connected to dest.
int proxy_connect_direct(const char* dest_host, int dest_port);

#ifdef __cplusplus
}
#endif

#endif

// Connects to the destination through an HTTP Proxy.
// Returns 0 on success, -1 on error.
int proxy_connect_http(ChannelContext *ctx, const char* dest_host, int dest_port, const char* proxy_payload);

