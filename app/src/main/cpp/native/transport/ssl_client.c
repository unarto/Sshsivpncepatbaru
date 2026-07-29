#include "ssl_client.h"
#include <android/log.h>
#include <string.h>

#define LOG_TAG "TlsTransport"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

int ssl_client_init(TlsContext *ctx) {
    if (!ctx) return -1;
    memset(ctx, 0, sizeof(TlsContext));
    ctx->initialized = 1;
    return 0;
}

int ssl_client_handshake(TlsContext *ctx, int fd, const char *sni) {
    if (!ctx || !ctx->initialized) return -1;
    
    mbedtls_net_init(&ctx->server_fd);
    mbedtls_ssl_init(&ctx->ssl);
    mbedtls_ssl_config_init(&ctx->conf);
    mbedtls_ctr_drbg_init(&ctx->ctr_drbg);
    mbedtls_entropy_init(&ctx->entropy);
    ctx->server_fd.fd = fd;
    
    if (mbedtls_ctr_drbg_seed(&ctx->ctr_drbg, mbedtls_entropy_func, &ctx->entropy, (const unsigned char *)"sivpn", 5) != 0) {
        LOGE("mbedtls_ctr_drbg_seed failed");
        return -1;
    }
    
    if (mbedtls_ssl_config_defaults(&ctx->conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT) != 0) {
        LOGE("mbedtls_ssl_config_defaults failed");
        return -1;
    }
    
    mbedtls_ssl_conf_authmode(&ctx->conf, MBEDTLS_SSL_VERIFY_NONE); // We don't verify cert for custom SNI proxy
    mbedtls_ssl_conf_rng(&ctx->conf, mbedtls_ctr_drbg_random, &ctx->ctr_drbg);
    
    if (mbedtls_ssl_setup(&ctx->ssl, &ctx->conf) != 0) {
        LOGE("mbedtls_ssl_setup failed");
        return -1;
    }
    
    if (sni && sni[0] != '\0') {
        mbedtls_ssl_set_hostname(&ctx->ssl, sni);
    }
    
    mbedtls_ssl_set_bio(&ctx->ssl, &ctx->server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);
    
    int ret;
    while ((ret = mbedtls_ssl_handshake(&ctx->ssl)) != 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            LOGE("mbedtls_ssl_handshake failed: -0x%04x", -ret);
            return -1;
        }
    }
    
    LOGI("TLS Handshake OK");
    return 0;
}

int ssl_client_read(TlsContext *ctx, unsigned char *buf, size_t len) {
    if (!ctx || !ctx->initialized) return -1;
    return mbedtls_ssl_read(&ctx->ssl, buf, len);
}

int ssl_client_write(TlsContext *ctx, const unsigned char *buf, size_t len) {
    if (!ctx || !ctx->initialized) return -1;
    return mbedtls_ssl_write(&ctx->ssl, buf, len);
}

void ssl_client_close(TlsContext *ctx) {
    if (!ctx || !ctx->initialized) return;
    
    mbedtls_ssl_close_notify(&ctx->ssl);
    mbedtls_ssl_free(&ctx->ssl);
    mbedtls_ssl_config_free(&ctx->conf);
    mbedtls_ctr_drbg_free(&ctx->ctr_drbg);
    mbedtls_entropy_free(&ctx->entropy);
    mbedtls_net_free(&ctx->server_fd);
    ctx->initialized = 0;
}
