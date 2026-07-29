#include "tls_transport.h"
#include "../common/error.h"
#include <string.h>

void tls_transport_global_init(void) {
    // No-op for now, mbedtls doesn't require strict global init in most cases
}

int tls_transport_handshake_step(ChannelContext *ctx) {
    if (!ctx || !ctx->ssl_ctx) return ENGINE_INVALID_ARG;
    TlsContext *tls = (TlsContext *)ctx->ssl_ctx;
    
    if (!tls->initialized) {
        mbedtls_net_init(&tls->server_fd);
        mbedtls_ssl_init(&tls->ssl);
        mbedtls_ssl_config_init(&tls->conf);
        mbedtls_ctr_drbg_init(&tls->ctr_drbg);
        mbedtls_entropy_init(&tls->entropy);
        tls->server_fd.fd = ctx->socket_fd; // Attach existing non-blocking socket
        
        if (mbedtls_ctr_drbg_seed(&tls->ctr_drbg, mbedtls_entropy_func, &tls->entropy, (const unsigned char *)"sivpn", 5) != 0) {
            return ENGINE_TLS_ERROR;
        }
        
        if (mbedtls_ssl_config_defaults(&tls->conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT) != 0) {
            return ENGINE_TLS_ERROR;
        }
        
        mbedtls_ssl_conf_authmode(&tls->conf, MBEDTLS_SSL_VERIFY_NONE); 
        mbedtls_ssl_conf_rng(&tls->conf, mbedtls_ctr_drbg_random, &tls->ctr_drbg);
        
        if (mbedtls_ssl_setup(&tls->ssl, &tls->conf) != 0) {
            return ENGINE_TLS_ERROR;
        }
        
        if (ctx->sni[0] != '\0') {
            mbedtls_ssl_set_hostname(&tls->ssl, ctx->sni);
        }
        
        mbedtls_ssl_set_bio(&tls->ssl, &tls->server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);
        tls->initialized = 1;
    }
    
    int ret = mbedtls_ssl_handshake(&tls->ssl);
    if (ret == 0) {
        return ENGINE_OK; // Handshake complete
    }
    
    if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
        return ENGINE_AGAIN;
    }
    
    return ENGINE_TLS_ERROR;
}

ssize_t tls_transport_read(ChannelContext *ctx, void *buf, size_t len) {
    if (!ctx || !ctx->ssl_ctx || !buf) return ENGINE_INVALID_ARG;
    TlsContext *tls = (TlsContext *)ctx->ssl_ctx;
    
    int ret = mbedtls_ssl_read(&tls->ssl, (unsigned char *)buf, len);
    if (ret > 0) {
        return ret;
    }
    if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
        return ENGINE_AGAIN;
    }
    if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
        return 0; // EOF
    }
    return ENGINE_IO_ERROR;
}

ssize_t tls_transport_write(ChannelContext *ctx, const void *buf, size_t len) {
    if (!ctx || !ctx->ssl_ctx || !buf) return ENGINE_INVALID_ARG;
    TlsContext *tls = (TlsContext *)ctx->ssl_ctx;
    
    int ret = mbedtls_ssl_write(&tls->ssl, (const unsigned char *)buf, len);
    if (ret > 0) {
        return ret;
    }
    if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
        return ENGINE_AGAIN;
    }
    return ENGINE_IO_ERROR;
}

void tls_transport_close(ChannelContext *ctx) {
    if (!ctx || !ctx->ssl_ctx) return;
    TlsContext *tls = (TlsContext *)ctx->ssl_ctx;
    if (tls->initialized) {
        mbedtls_ssl_close_notify(&tls->ssl);
        mbedtls_ssl_free(&tls->ssl);
        mbedtls_ssl_config_free(&tls->conf);
        mbedtls_ctr_drbg_free(&tls->ctr_drbg);
        mbedtls_entropy_free(&tls->entropy);
        // Do NOT free mbedtls_net_free(&tls->server_fd); as we manage the socket FD elsewhere
        tls->initialized = 0;
    }
}
