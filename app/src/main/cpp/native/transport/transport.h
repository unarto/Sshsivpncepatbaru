#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <stddef.h>
#include <sys/types.h>
#include "../session/session.h"
#include "../common/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize the transport layer (e.g. MbedTLS global init) */
void transport_init(void);

/* Connect to the destination asynchronously. 
 * Returns ENGINE_OK on success, ENGINE_INPROGRESS if non-blocking, or error. 
 */
int transport_connect(ChannelContext *ctx, const char *host, int port);

/* Base read from socket or TLS without WebSocket decoding */
ssize_t transport_read_base(ChannelContext *ctx, void *buf, size_t len);

/* Base write to socket or TLS without WebSocket encoding */
ssize_t transport_write_base(ChannelContext *ctx, const void *buf, size_t len);

/* Read up to 'len' bytes into 'buf'.
 * Returns bytes read, or negative EngineError on failure.
 */
ssize_t transport_read(ChannelContext *ctx, void *buf, size_t len);

/* Read exactly 'len' bytes into 'buf'.
 * Returns bytes read, or negative EngineError on failure.
 */
ssize_t transport_read_exact(ChannelContext *ctx, void *buf, size_t len);

/* Write up to 'len' bytes from 'buf'.
 * Returns bytes written, or negative EngineError on failure.
 */
ssize_t transport_write(ChannelContext *ctx, const void *buf, size_t len);

/* Write exactly 'len' bytes from 'buf'.
 * Returns bytes written, or negative EngineError on failure.
 */
ssize_t transport_write_all(ChannelContext *ctx, const void *buf, size_t len);

/* Close the transport connection */
void transport_close(ChannelContext *ctx);

#ifdef __cplusplus
}
#endif

#endif // TRANSPORT_H
