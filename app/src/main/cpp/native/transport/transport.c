#include "transport.h"
#include "tcp_transport.h"
#include "tls_transport.h"
#include "websocket_transport.h"
#include "../common/error.h"
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

void transport_init(void) {
    tls_transport_global_init();
}

int transport_connect(ChannelContext *ctx, const char *host, int port) {
    if (!ctx) return ENGINE_INVALID_ARG;
    
    int fd = tcp_transport_connect_nonblocking(host, port);
    if (fd < 0) {
        return fd; // Error code (ENGINE_IO_ERROR)
    }
    
    ctx->socket_fd = fd;
    ctx->state = CONN_STATE_CONNECTING;
    return ENGINE_OK; // Indicates non-blocking socket is created and connect() initiated
}

/* Base transport reads from either TLS or raw TCP */
ssize_t transport_read_base(ChannelContext *ctx, void *buf, size_t len) {
    if (ctx->ssl_ctx) {
        return tls_transport_read(ctx, buf, len);
    }
    return tcp_transport_read(ctx->socket_fd, buf, len);
}

/* Base transport writes to either TLS or raw TCP */
ssize_t transport_write_base(ChannelContext *ctx, const void *buf, size_t len) {
    if (ctx->ssl_ctx) {
        return tls_transport_write(ctx, buf, len);
    }
    return tcp_transport_write(ctx->socket_fd, buf, len);
}

ssize_t transport_read(ChannelContext *ctx, void *buf, size_t len) {
    if (!ctx || !buf) return ENGINE_INVALID_ARG;
    
    if (ctx->protocol == PROTO_WEBSOCKET && ctx->ws_ctx) {
        return websocket_transport_read(ctx, buf, len);
    }
    
    return transport_read_base(ctx, buf, len);
}

ssize_t transport_write(ChannelContext *ctx, const void *buf, size_t len) {
    if (!ctx || !buf) return ENGINE_INVALID_ARG;
    
    if (ctx->protocol == PROTO_WEBSOCKET && ctx->ws_ctx) {
        return websocket_transport_write(ctx, buf, len);
    }
    
    return transport_write_base(ctx, buf, len);
}

ssize_t transport_write_all(ChannelContext *ctx, const void *buf, size_t len) {
    if (!ctx || !buf) return ENGINE_INVALID_ARG;
    
    size_t total_written = 0;
    const char *ptr = (const char *)buf;
    
    while (total_written < len) {
        ssize_t ret = transport_write(ctx, ptr + total_written, len - total_written);
        if (ret > 0) {
            total_written += ret;
        } else if (ret == ENGINE_AGAIN || ret == ENGINE_INTR) {
            // Because this is non-blocking, transport_write_all shouldn't actually block!
            // Wait, write_all in non-blocking mode?
            // If it can't write all immediately, it should return what it wrote so far,
            // or an error if nothing was written.
            if (total_written > 0) return total_written;
            return ret;
        } else {
            return ret; // Error
        }
    }
    return total_written;
}

ssize_t transport_read_exact(ChannelContext *ctx, void *buf, size_t len) {
    if (!ctx || !buf) return ENGINE_INVALID_ARG;
    
    size_t total_read = 0;
    char *ptr = (char *)buf;
    
    while (total_read < len) {
        ssize_t ret = transport_read(ctx, ptr + total_read, len - total_read);
        if (ret > 0) {
            total_read += ret;
        } else if (ret == 0) { // EOF
            return total_read;
        } else if (ret == ENGINE_AGAIN || ret == ENGINE_INTR) {
            if (total_read > 0) return total_read; // Return partial
            return ret;
        } else {
            return ret; // Error
        }
    }
    return total_read;
}

void transport_close(ChannelContext *ctx) {
    if (!ctx) return;
    
    if (ctx->ssl_ctx) {
        tls_transport_close(ctx);
        free(ctx->ssl_ctx);
        ctx->ssl_ctx = NULL;
    }
    
    if (ctx->socket_fd >= 0) {
        tcp_transport_close(ctx->socket_fd);
        ctx->socket_fd = -1;
    }
    
    ctx->state = CONN_STATE_DISCONNECTED;
}
