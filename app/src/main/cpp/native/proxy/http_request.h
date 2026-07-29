#ifndef HTTP_TRANSPORT_H
#define HTTP_TRANSPORT_H

#include <stdbool.h>
#include "../ssh/channel.h"
#include "../transport/ssl_client.h"

int http_transport_send_payload(int sock, bool use_tls, TlsContext* tls_ctx, const TunnelConfig* cfg);

#endif // HTTP_TRANSPORT_H
