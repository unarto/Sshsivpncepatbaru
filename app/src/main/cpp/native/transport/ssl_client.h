#ifndef NATIVE_SSL_CLIENT_H
#define NATIVE_SSL_CLIENT_H

#include <mbedtls/net_sockets.h>
#include <mbedtls/ssl.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>

typedef struct {
    mbedtls_net_context server_fd;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;
    int initialized;
} TlsContext;

int ssl_client_init(TlsContext *ctx);
int ssl_client_handshake(TlsContext *ctx, int fd, const char *sni);
int ssl_client_read(TlsContext *ctx, unsigned char *buf, size_t len);
int ssl_client_write(TlsContext *ctx, const unsigned char *buf, size_t len);
void ssl_client_close(TlsContext *ctx);

#endif
