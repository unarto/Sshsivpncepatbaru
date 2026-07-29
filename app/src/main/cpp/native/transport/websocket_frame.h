#ifndef NATIVE_WEBSOCKET_FRAME_H
#define NATIVE_WEBSOCKET_FRAME_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef enum {
    WS_OPCODE_CONT   = 0x0,
    WS_OPCODE_TEXT   = 0x1,
    WS_OPCODE_BINARY = 0x2,
    WS_OPCODE_CLOSE  = 0x8,
    WS_OPCODE_PING   = 0x9,
    WS_OPCODE_PONG   = 0xA
} WsOpcode;

// Encodes a payload into a websocket frame.
// If mask is true, it generates a random masking key and masks the payload.
// Returns the number of bytes written to `out_buf`, or -1 if `out_buf_len` is too small.
ssize_t ws_encode_frame(uint8_t *out_buf, size_t out_buf_len, 
                        const uint8_t *payload, size_t payload_len, 
                        WsOpcode opcode, bool mask, bool fin);

// Decodes a websocket frame from `in_buf`.
// If a complete frame is found, sets `*payload_out` to point to the start of the payload inside `in_buf`,
// sets `*payload_len_out` to the length of the payload, and `*opcode_out` to the opcode.
// Returns the total length of the frame consumed.
// Returns 0 if more data is needed.
// Returns -1 on error (e.g. invalid frame).
// The payload is unmasked IN PLACE inside `in_buf` if it was masked.
ssize_t ws_decode_frame(uint8_t *in_buf, size_t in_buf_len, 
                        uint8_t **payload_out, size_t *payload_len_out, 
                        WsOpcode *opcode_out, bool *fin_out);

#endif
