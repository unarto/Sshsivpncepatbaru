#include "../payload/payload_manager.h"
#include "../payload/payload_split.h"
#include "proxy_connect.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <errno.h>

/* Configuration & Timeout */
#ifndef PROXY_CONNECT_TIMEOUT_MS
#define PROXY_CONNECT_TIMEOUT_MS 5000
#endif

/* Logging Macros */
#define LOG_TAG "ProxyConnect"

#if defined(__ANDROID__) && !defined(DISABLE_PROXY_LOG)
#include <android/log.h>
#define PROXY_LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define PROXY_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#else
#define PROXY_LOGI(...) ((void)0)
#define PROXY_LOGE(...) ((void)0)
#endif

/* SOCKS Protocol Definitions */
#define SOCKS5_VERSION              0x05
#define SOCKS5_METHOD_NO_AUTH       0x00
#define SOCKS5_METHOD_USER_PASS     0x02
#define SOCKS5_METHOD_NO_ACCEPTABLE 0xFF

#define SOCKS5_CMD_CONNECT          0x01
#define SOCKS5_RESERVED             0x00

#define SOCKS5_ATYP_IPV4            0x01
#define SOCKS5_ATYP_DOMAIN          0x03
#define SOCKS5_ATYP_IPV6            0x04

#define SOCKS5_REP_SUCCESS          0x00
#define SOCKS5_AUTH_VER_USERPASS    0x01
#define SOCKS5_AUTH_SUCCESS         0x00

#define SOCKS4_VERSION              0x04
#define SOCKS4_CMD_CONNECT          0x01
#define SOCKS4_REP_GRANTED          0x5A

/* Write all buffer content with select timeout */
static int write_all(ChannelContext *ctx, const uint8_t *buf, size_t len, int timeout_ms) {
    size_t written = 0;
    while (written < len) {
        fd_set wfds;
        FD_ZERO(&wfds);
        FD_SET(ctx->socket_fd, &wfds);

        struct timeval tv;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;

        int ret = select(ctx->socket_fd + 1, NULL, &wfds, NULL, &tv);
        if (ret < 0) {
            if (errno == EINTR) continue;
            PROXY_LOGE("write_all select failed: %s", strerror(errno));
            return -1;
        }
        if (ret == 0) {
            PROXY_LOGE("write_all timeout after %d ms", timeout_ms);
            return -1;
        }

        ssize_t w = transport_write(ctx, buf + written, len - written);
        if (w < 0) {
            if (w == ENGINE_AGAIN || w == ENGINE_INTR) continue;
            PROXY_LOGE("write_all write failed with error code %d", (int)w);
            return -1;
        }
        if (w == 0) {
            PROXY_LOGE("write_all socket closed by peer"); // Not strictly possible for write, but defensive
            return -1;
        }
        written += (size_t)w;
    }
    return 0;
}

/* Read exact number of bytes with select timeout */
static int read_exact(ChannelContext *ctx, uint8_t *buf, size_t len, int timeout_ms) {
    size_t read_bytes = 0;
    while (read_bytes < len) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(ctx->socket_fd, &rfds);

        struct timeval tv;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;

        int ret = select(ctx->socket_fd + 1, &rfds, NULL, NULL, &tv);
        if (ret < 0) {
            if (errno == EINTR) continue;
            PROXY_LOGE("read_exact select failed: %s", strerror(errno));
            return -1;
        }
        if (ret == 0) {
            PROXY_LOGE("read_exact timeout after %d ms", timeout_ms);
            return -1;
        }

        ssize_t r = transport_read(ctx, buf + read_bytes, len - read_bytes);
        if (r < 0) {
            if (r == ENGINE_AGAIN || r == ENGINE_INTR) continue;
            PROXY_LOGE("read_exact read failed with error code %d", (int)r);
            return -1;
        }
        if (r == 0) {
            PROXY_LOGE("read_exact EOF from peer");
            return -1;
        }
        read_bytes += (size_t)r;
    }
    return 0;
}

int proxy_connect_socks5(ChannelContext *ctx, const char* dest_host, int dest_port, const char* proxy_user, const char* proxy_pass) {
    /* Input Validation */
    if (!ctx || ctx->socket_fd < 0) {
        PROXY_LOGE("SOCKS5 error: invalid socket descriptor");
        return -1;
    }
    if (!dest_host || dest_host[0] == '\0') {
        PROXY_LOGE("SOCKS5 error: null or empty dest_host");
        return -1;
    }
    if (dest_port <= 0 || dest_port > 65535) {
        PROXY_LOGE("SOCKS5 error: invalid dest_port %d", dest_port);
        return -1;
    }

    size_t host_len = strlen(dest_host);
    if (host_len > 255) {
        PROXY_LOGE("SOCKS5 error: dest_host length exceeds 255 bytes (%zu)", host_len);
        return -1;
    }

    /* 1. Send Greeting Request */
    uint8_t greeting[4];
    size_t greeting_len = 0;
    greeting[0] = SOCKS5_VERSION;

    bool has_auth = (proxy_user && proxy_pass && proxy_user[0] != '\0');
    if (has_auth) {
        greeting[1] = 0x02; // Offer 2 methods
        greeting[2] = SOCKS5_METHOD_NO_AUTH;
        greeting[3] = SOCKS5_METHOD_USER_PASS;
        greeting_len = 4;
    } else {
        greeting[1] = 0x01; // Offer 1 method
        greeting[2] = SOCKS5_METHOD_NO_AUTH;
        greeting_len = 3;
    }

    if (write_all(ctx, greeting, greeting_len, PROXY_CONNECT_TIMEOUT_MS) < 0) {
        PROXY_LOGE("SOCKS5 error: failed to send greeting");
        return -1;
    }

    /* 2. Receive Greeting Response */
    uint8_t resp1[2];
    if (read_exact(ctx, resp1, 2, PROXY_CONNECT_TIMEOUT_MS) < 0) {
        PROXY_LOGE("SOCKS5 error: failed to read greeting response");
        return -1;
    }

    if (resp1[0] != SOCKS5_VERSION) {
        PROXY_LOGE("SOCKS5 error: invalid server version 0x%02X", resp1[0]);
        return -1;
    }

    if (resp1[1] == SOCKS5_METHOD_USER_PASS) {
        if (!has_auth) {
            PROXY_LOGE("SOCKS5 error: server requested auth but no credentials provided");
            return -1;
        }

        size_t ulen = strlen(proxy_user);
        size_t plen = strlen(proxy_pass);
        if (ulen > 255 || plen > 255) {
            PROXY_LOGE("SOCKS5 error: username/password length exceeds 255 bytes");
            return -1;
        }

        uint8_t auth_req[515];
        auth_req[0] = SOCKS5_AUTH_VER_USERPASS;
        auth_req[1] = (uint8_t)ulen;
        memcpy(auth_req + 2, proxy_user, ulen);
        auth_req[2 + ulen] = (uint8_t)plen;
        memcpy(auth_req + 3 + ulen, proxy_pass, plen);

        if (write_all(ctx, auth_req, 3 + ulen + plen, PROXY_CONNECT_TIMEOUT_MS) < 0) {
            PROXY_LOGE("SOCKS5 error: failed to send auth request");
            return -1;
        }

        uint8_t auth_resp[2];
        if (read_exact(ctx, auth_resp, 2, PROXY_CONNECT_TIMEOUT_MS) < 0) {
            PROXY_LOGE("SOCKS5 error: failed to read auth response");
            return -1;
        }

        if (auth_resp[1] != SOCKS5_AUTH_SUCCESS) {
            PROXY_LOGE("SOCKS5 error: authentication failed with code 0x%02X", auth_resp[1]);
            return -1;
        }
    } else if (resp1[1] != SOCKS5_METHOD_NO_AUTH) {
        PROXY_LOGE("SOCKS5 error: no acceptable auth method 0x%02X", resp1[1]);
        return -1;
    }

    /* 3. Send Connection Request */
    uint8_t req[262];
    req[0] = SOCKS5_VERSION;
    req[1] = SOCKS5_CMD_CONNECT;
    req[2] = SOCKS5_RESERVED;
    req[3] = SOCKS5_ATYP_DOMAIN;
    req[4] = (uint8_t)host_len;
    memcpy(req + 5, dest_host, host_len);
    req[5 + host_len] = (uint8_t)((dest_port >> 8) & 0xFF);
    req[6 + host_len] = (uint8_t)(dest_port & 0xFF);

    if (write_all(ctx, req, 7 + host_len, PROXY_CONNECT_TIMEOUT_MS) < 0) {
        PROXY_LOGE("SOCKS5 error: failed to send connect request");
        return -1;
    }

    /* 4. Receive Connection Response */
    uint8_t resp2[4];
    if (read_exact(ctx, resp2, 4, PROXY_CONNECT_TIMEOUT_MS) < 0) {
        PROXY_LOGE("SOCKS5 error: failed to read connect response header");
        return -1;
    }

    if (resp2[0] != SOCKS5_VERSION) {
        PROXY_LOGE("SOCKS5 error: invalid connect response version 0x%02X", resp2[0]);
        return -1;
    }

    if (resp2[1] != SOCKS5_REP_SUCCESS) {
        PROXY_LOGE("SOCKS5 error: connect request failed with reply code 0x%02X", resp2[1]);
        return -1;
    }

    /* Read bound address based on address type */
    if (resp2[3] == SOCKS5_ATYP_IPV4) {
        uint8_t discard_buffer[6]; // 4 bytes IP + 2 bytes port
        if (read_exact(ctx, discard_buffer, 6, PROXY_CONNECT_TIMEOUT_MS) < 0) return -1;
    } else if (resp2[3] == SOCKS5_ATYP_DOMAIN) {
        uint8_t dlen;
        if (read_exact(ctx, &dlen, 1, PROXY_CONNECT_TIMEOUT_MS) < 0) return -1;
        uint8_t discard_buffer[258]; // domain length bytes + 2 bytes port
        if (read_exact(ctx, discard_buffer, (size_t)dlen + 2, PROXY_CONNECT_TIMEOUT_MS) < 0) return -1;
    } else if (resp2[3] == SOCKS5_ATYP_IPV6) {
        uint8_t discard_buffer[18]; // 16 bytes IP + 2 bytes port
        if (read_exact(ctx, discard_buffer, 18, PROXY_CONNECT_TIMEOUT_MS) < 0) return -1;
    } else {
        PROXY_LOGE("SOCKS5 error: unknown address type 0x%02X in response", resp2[3]);
        return -1;
    }

    PROXY_LOGI("SOCKS5 handshake successfully established to %s:%d", dest_host, dest_port);
    return 0;
}

int proxy_connect_socks4(ChannelContext *ctx, const char* dest_host, int dest_port, const char* proxy_user) {
    /* Input Validation */
    if (!ctx || ctx->socket_fd < 0) {
        PROXY_LOGE("SOCKS4 error: invalid socket descriptor");
        return -1;
    }
    if (!dest_host || dest_host[0] == '\0') {
        PROXY_LOGE("SOCKS4 error: null or empty dest_host");
        return -1;
    }
    if (dest_port <= 0 || dest_port > 65535) {
        PROXY_LOGE("SOCKS4 error: invalid dest_port %d", dest_port);
        return -1;
    }

    size_t host_len = strlen(dest_host);
    if (host_len > 255) {
        PROXY_LOGE("SOCKS4 error: dest_host length exceeds 255 bytes (%zu)", host_len);
        return -1;
    }

    /* SOCKS4a packet construction:
     * VN: 1 byte (0x04)
     * CD: 1 byte (0x01 = CONNECT)
     * DSTPORT: 2 bytes (big endian)
     * DSTIP: 4 bytes (0.0.0.x with x != 0 for SOCKS4a extension)
     * USERID: variable (null-terminated)
     * HOSTNAME: variable (null-terminated)
     */
    uint8_t req[528];
    req[0] = SOCKS4_VERSION;
    req[1] = SOCKS4_CMD_CONNECT;
    req[2] = (uint8_t)((dest_port >> 8) & 0xFF);
    req[3] = (uint8_t)(dest_port & 0xFF);
    req[4] = 0x00; // SOCKS4a dummy IP 0.0.0.1
    req[5] = 0x00;
    req[6] = 0x00;
    req[7] = 0x01;

    size_t offset = 8;
    if (proxy_user) {
        size_t ulen = strlen(proxy_user);
        if (ulen > 255) {
            PROXY_LOGE("SOCKS4 error: user length exceeds 255 bytes");
            return -1;
        }
        memcpy(req + offset, proxy_user, ulen);
        offset += ulen;
    }
    req[offset++] = 0x00; // Null terminator for USERID

    memcpy(req + offset, dest_host, host_len);
    offset += host_len;
    req[offset++] = 0x00; // Null terminator for HOSTNAME

    if (write_all(ctx, req, offset, PROXY_CONNECT_TIMEOUT_MS) < 0) {
        PROXY_LOGE("SOCKS4 error: failed to send request");
        return -1;
    }

    /* Read SOCKS4 response (8 bytes) */
    uint8_t resp[8];
    if (read_exact(ctx, resp, 8, PROXY_CONNECT_TIMEOUT_MS) < 0) {
        PROXY_LOGE("SOCKS4 error: failed to read response");
        return -1;
    }

    if (resp[1] != SOCKS4_REP_GRANTED) {
        PROXY_LOGE("SOCKS4 error: request rejected/failed with code 0x%02X", resp[1]);
        return -1;
    }

    PROXY_LOGI("SOCKS4a handshake successfully established to %s:%d", dest_host, dest_port);
    return 0;
}

int proxy_connect_direct(const char* dest_host, int dest_port) {
    if (!dest_host || dest_host[0] == '\0' || dest_port <= 0 || dest_port > 65535) {
        PROXY_LOGE("Direct connection error: invalid parameter");
        return -1;
    }
    PROXY_LOGI("Direct connection mode active for %s:%d", dest_host, dest_port);
    return 0;
}

#include "../http/http_response.h"

int proxy_connect_http(ChannelContext *ctx, const char* dest_host, int dest_port, const char* proxy_payload) {
    if (!ctx || ctx->socket_fd < 0) {
        PROXY_LOGE("HTTP Proxy error: invalid socket descriptor");
        return -1;
    }
    
    if (!proxy_payload || strlen(proxy_payload) == 0) {
        // Default CONNECT payload if none provided
        char default_payload[512];
        snprintf(default_payload, sizeof(default_payload), 
                 "CONNECT %s:%d HTTP/1.1\r\nHost: %s:%d\r\n\r\n", 
                 dest_host, dest_port, dest_host, dest_port);
                 
        if (write_all(ctx, (const uint8_t*)default_payload, strlen(default_payload), PROXY_CONNECT_TIMEOUT_MS) < 0) {
            PROXY_LOGE("HTTP Proxy error: failed to send default CONNECT payload");
            return -1;
        }
    } else {
        // Send custom payload (already generated by PayloadBuilder)
        if (write_all(ctx, (const uint8_t*)proxy_payload, strlen(proxy_payload), PROXY_CONNECT_TIMEOUT_MS) < 0) {
            PROXY_LOGE("HTTP Proxy error: failed to send custom payload");
            return -1;
        }
    }
    
    // Read response
    uint8_t resp_buf[4096];
    int r = transport_read(ctx, resp_buf, sizeof(resp_buf) - 1, PROXY_CONNECT_TIMEOUT_MS);
    if (r <= 0) {
        PROXY_LOGE("HTTP Proxy error: empty or failed response");
        return -1;
    }
    
    resp_buf[r] = '\0';
    PROXY_LOGI("HTTP Proxy Response: \n%s", (char*)resp_buf);
    
    int status = http_response_parse_status_code((const char*)resp_buf);
    if (!http_response_is_success(status)) {
        PROXY_LOGE("HTTP Proxy error: connection rejected with status %d", status);
        return -1;
    }
    
    PROXY_LOGI("HTTP Proxy handshake successfully established to %s:%d (Status: %d)", dest_host, dest_port, status);
    return 0;
}
