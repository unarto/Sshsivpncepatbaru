#ifndef TLS_TRANSPORT_H
#define TLS_TRANSPORT_H

#include <stddef.h>
#include <sys/types.h>
#include "../session/session.h"
#include <mbedtls/ssl.h>
#include <mbedtls/net_sockets.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    mbedtls_net_context server_fd;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    int initialized;
} TlsContext;

/* Initialize TLS global state if needed */
void tls_transport_global_init(void);

/* Setup TLS handshake non-blocking step.
 * Returns ENGINE_OK on completion, ENGINE_AGAIN if needs more IO, or error. 
 */
int tls_transport_handshake_step(ChannelContext *ctx);

ssize_t tls_transport_read(ChannelContext *ctx, void *buf, size_t len);
ssize_t tls_transport_write(ChannelContext *ctx, const void *buf, size_t len);
void tls_transport_close(ChannelContext *ctx);

#ifdef __cplusplus
}
#endif

#endif // TLS_TRANSPORT_H
