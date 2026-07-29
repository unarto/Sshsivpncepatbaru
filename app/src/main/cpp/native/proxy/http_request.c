#include "../payload/payload_manager.h"
#include "../payload/payload_split.h"
#include "http_request.h"
#include "../transport/tcp_transport.h"
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <android/log.h>

#define LOG_TAG "HttpTransport"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

int http_transport_send_payload(int sock, bool use_tls, TlsContext* tls_ctx, const TunnelConfig* cfg) {
    if (cfg->num_payload_chunks <= 0) {
        return 0; // No payload to send
    }

    LOGI("Sending %d Payload Chunks", cfg->num_payload_chunks);
    for (int i = 0; i < cfg->num_payload_chunks; i++) {
        if (cfg->payload_chunks[i].content[0]) {
            if (use_tls && tls_ctx) {
                ssl_client_write(tls_ctx, (const unsigned char*)cfg->payload_chunks[i].content, strlen(cfg->payload_chunks[i].content));
            } else {
                tcp_transport_write(sock, cfg->payload_chunks[i].content, strlen(cfg->payload_chunks[i].content));
            }
        }
        if (cfg->payload_chunks[i].split_type == SPLIT_DELAY) {
            usleep(1000000);
        } else if (cfg->payload_chunks[i].split_type == SPLIT_NORMAL) {
            usleep(200000);
        }
    }
    
    // Suport Dropbear mode (SSH + PAYLOAD directly without reading HTTP proxy response)
    if (cfg->modo_dropbear) {
        LOGI("Modo Dropbear enabled: Skipping HTTP response check and returning socket for direct SSH banner exchange.");
        return 0;
    }

    // Read response (blocking simple read)
    char resp[4096];
    int r = 0;
    if (use_tls && tls_ctx) {
        r = ssl_client_read(tls_ctx, (unsigned char*)resp, sizeof(resp)-1);
    } else {
        r = tcp_transport_read(sock, resp, sizeof(resp)-1);
    }
    
    if (r > 0) {
        resp[r] = '\0';
        LOGI("Payload Response: %s", resp);
        if (strstr(resp, "HTTP/") != NULL && strstr(resp, "200") == NULL && strstr(resp, "101") == NULL) {
            LOGE("Payload response failed: not 200/101");
            return -1;
        }
    } else {
        LOGE("Payload Response empty or failed");
        return -1;
    }

    return 0;
}
