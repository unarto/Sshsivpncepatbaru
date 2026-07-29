#include "keepalive.h"
#include "../transport/tcp_transport.h"
#include "../transport/websocket_frame.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <android/log.h>

#define LOG_TAG "KeepAlive"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static time_t g_last_ping_time = 0;

int keepalive_set_tcp_options(int fd, int idle_sec, int interval_sec, int count) {
    if (fd < 0) return -1;
    
    int optval = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) < 0) {
        LOGE("Failed to set SO_KEEPALIVE");
        return -1;
    }
    
#if defined(TCP_KEEPIDLE)
    if (idle_sec > 0) {
        setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &idle_sec, sizeof(idle_sec));
    }
#endif

#if defined(TCP_KEEPINTVL)
    if (interval_sec > 0) {
        setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &interval_sec, sizeof(interval_sec));
    }
#endif

#if defined(TCP_KEEPCNT)
    if (count > 0) {
        setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &count, sizeof(count));
    }
#endif

    LOGI("TCP KeepAlive configured on fd %d (idle=%ds, intvl=%ds, cnt=%d)", fd, idle_sec, interval_sec, count);
    return 0;
}

int keepalive_send_ssh(LIBSSH2_SESSION *session) {
    if (!session) return -1;
    
    int seconds_to_next = 0;
    int rc = libssh2_keepalive_send(session, &seconds_to_next);
    if (rc == 0) {
        LOGI("SSH KeepAlive sent successfully");
        return 0;
    } else if (rc == LIBSSH2_ERROR_EAGAIN) {
        return 0; // Non-blocking in progress
    }
    
    LOGE("SSH KeepAlive send failed: %d", rc);
    return -1;
}

int keepalive_send_ws_ping(int fd, bool use_tls, TlsContext *tls_ctx) {
    if (fd < 0) return -1;
    
    uint8_t frame[32];
    uint8_t payload[] = "ping";
    
    ssize_t frame_len = ws_encode_frame(frame, sizeof(frame), payload, 4, WS_OPCODE_PING, true, true);
    if (frame_len <= 0) {
        LOGE("Failed to encode WebSocket Ping frame");
        return -1;
    }
    
    int ret = 0;
    if (use_tls && tls_ctx && tls_ctx->initialized) {
        ret = ssl_client_write(tls_ctx, frame, (size_t)frame_len);
    } else {
        ret = (int)tcp_transport_write(fd, frame, (size_t)frame_len);
    }
    
    if (ret > 0) {
        LOGI("WebSocket Ping frame sent (%zd bytes)", frame_len);
        return 0;
    }
    
    LOGE("Failed to send WebSocket Ping frame");
    return -1;
}

int keepalive_send_http_head(int fd, const char *host, bool use_tls, TlsContext *tls_ctx) {
    if (fd < 0) return -1;
    
    char req[512];
    const char *target_host = (host && host[0] != '\0') ? host : "127.0.0.1";
    int req_len = snprintf(req, sizeof(req), 
        "HEAD / HTTP/1.1\r\n"
        "Host: %s\r\n"
        "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64)\r\n"
        "Connection: keep-alive\r\n\r\n", 
        target_host);
        
    if (req_len <= 0 || req_len >= (int)sizeof(req)) return -1;
    
    int ret = 0;
    if (use_tls && tls_ctx && tls_ctx->initialized) {
        ret = ssl_client_write(tls_ctx, (const unsigned char *)req, req_len);
    } else {
        ret = (int)tcp_transport_write(fd, req, req_len);
    }
    
    if (ret > 0) {
        LOGI("HTTP HEAD keep-alive request sent (%d bytes)", req_len);
        return 0;
    }
    
    LOGE("Failed to send HTTP HEAD keep-alive request");
    return -1;
}

void keepalive_periodic(int fd, LIBSSH2_SESSION *session, const char *host, bool use_tls, TlsContext *tls_ctx, KeepaliveType type) {
    time_t now = time(NULL);
    if (g_last_ping_time == 0) {
        g_last_ping_time = now;
        return;
    }
    
    if (now - g_last_ping_time < 30) {
        return;
    }
    
    g_last_ping_time = now;
    
    switch (type) {
        case KEEPALIVE_TYPE_TCP:
            if (fd >= 0) keepalive_set_tcp_options(fd, 30, 5, 3);
            break;
        case KEEPALIVE_TYPE_SSH_IGNORE:
            if (session) keepalive_send_ssh(session);
            break;
        case KEEPALIVE_TYPE_WS_PING:
            keepalive_send_ws_ping(fd, use_tls, tls_ctx);
            break;
        case KEEPALIVE_TYPE_HTTP_HEAD:
            keepalive_send_http_head(fd, host, use_tls, tls_ctx);
            break;
        default:
            break;
    }
}

void keepalive_start(void) {
    g_last_ping_time = time(NULL);
    LOGI("KeepAlive service initialized");
}
