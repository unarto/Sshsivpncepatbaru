#ifndef NATIVE_WEBSOCKET_H
#define NATIVE_WEBSOCKET_H

#include <stdbool.h>

// Request parameters for a WebSocket handshake
typedef struct {
    const char *host;
    const char *path;
    const char *origin;
    const char *cookie;
    const char *custom_headers; // e.g. "X-Forwarded-For: 127.0.0.1\r\n"
} WsHandshakeReq;

// Generates an HTTP GET request for a WebSocket handshake and writes it to `out_buf`.
// Returns the length of the request, or -1 on error.
int ws_build_handshake_request(char *out_buf, int out_buf_len, const WsHandshakeReq *req);

// Parses an HTTP response to check if the WebSocket handshake was successful (101 Switching Protocols).
// Returns 1 if successful, 0 if need more data (incomplete response), -1 on error (e.g. 403 Forbidden).
int ws_parse_handshake_response(const char *response_buf, int response_len);

#endif
