#include "../common/utils.h"
#include "websocket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>


int ws_build_handshake_request(char *out_buf, int out_buf_len, const WsHandshakeReq *req) {
    if (!out_buf || out_buf_len <= 0 || !req || !req->host) return -1;
    
    char ws_key[32];
    utils_generate_ws_key(ws_key, sizeof(ws_key));
    
    const char *path = req->path ? req->path : "/";
    
    int len = snprintf(out_buf, out_buf_len, 
        "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "Sec-WebSocket-Key: %s\r\n",
        path, req->host, ws_key);
        
    if (len < 0 || len >= out_buf_len) return -1;
    
    if (req->origin && strlen(req->origin) > 0) {
        int r = snprintf(out_buf + len, out_buf_len - len, "Origin: %s\r\n", req->origin);
        if (r < 0 || r >= out_buf_len - len) return -1;
        len += r;
    }
    
    if (req->cookie && strlen(req->cookie) > 0) {
        int r = snprintf(out_buf + len, out_buf_len - len, "Cookie: %s\r\n", req->cookie);
        if (r < 0 || r >= out_buf_len - len) return -1;
        len += r;
    }
    
    if (req->custom_headers && strlen(req->custom_headers) > 0) {
        int r = snprintf(out_buf + len, out_buf_len - len, "%s", req->custom_headers);
        if (r < 0 || r >= out_buf_len - len) return -1;
        len += r;
    }
    
    int r = snprintf(out_buf + len, out_buf_len - len, "\r\n");
    if (r < 0 || r >= out_buf_len - len) return -1;
    len += r;
    
    return len;
}

int ws_parse_handshake_response(const char *response_buf, int response_len) {
    if (!response_buf || response_len <= 0) return -1;
    
    // Check if we have a full HTTP header
    const char *header_end = strstr(response_buf, "\r\n\r\n");
    if (!header_end) {
        // Need more data to parse HTTP header
        return 0;
    }
    
    // Basic check for HTTP/1.1 101 Switching Protocols
    if (strncmp(response_buf, "HTTP/1.1 101", 12) == 0 || strncmp(response_buf, "HTTP/1.0 101", 12) == 0) {
        // Check for Upgrade: websocket
        // A more robust implementation would make this case-insensitive and allow variable spacing
        return 1;
    }
    
    return -1; // Failed or unsupported status code
}
