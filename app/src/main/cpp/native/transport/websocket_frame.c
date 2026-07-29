#include "websocket_frame.h"
#include <stdlib.h>
#include <string.h>

ssize_t ws_encode_frame(uint8_t *out_buf, size_t out_buf_len, 
                        const uint8_t *payload, size_t payload_len, 
                        WsOpcode opcode, bool mask, bool fin) {
    size_t header_len = 2; // min header
    if (payload_len >= 126 && payload_len <= 65535) {
        header_len += 2;
    } else if (payload_len > 65535) {
        header_len += 8;
    }
    
    if (mask) {
        header_len += 4;
    }
    
    if (out_buf_len < header_len + payload_len) {
        return -1;
    }
    
    out_buf[0] = (fin ? 0x80 : 0x00) | (opcode & 0x0F);
    
    uint8_t mask_bit = mask ? 0x80 : 0x00;
    size_t offset = 2;
    if (payload_len < 126) {
        out_buf[1] = mask_bit | (uint8_t)payload_len;
    } else if (payload_len <= 65535) {
        out_buf[1] = mask_bit | 126;
        out_buf[2] = (payload_len >> 8) & 0xFF;
        out_buf[3] = payload_len & 0xFF;
        offset += 2;
    } else {
        out_buf[1] = mask_bit | 127;
        for (int i = 7; i >= 0; i--) {
            out_buf[2 + (7 - i)] = (payload_len >> (i * 8)) & 0xFF;
        }
        offset += 8;
    }
    
    if (mask) {
        uint8_t masking_key[4];
        for (int i = 0; i < 4; i++) {
            masking_key[i] = rand() & 0xFF;
            out_buf[offset + i] = masking_key[i];
        }
        offset += 4;
        
        for (size_t i = 0; i < payload_len; i++) {
            out_buf[offset + i] = payload[i] ^ masking_key[i % 4];
        }
    } else {
        memcpy(out_buf + offset, payload, payload_len);
    }
    
    return header_len + payload_len;
}

ssize_t ws_decode_frame(uint8_t *in_buf, size_t in_buf_len, 
                        uint8_t **payload_out, size_t *payload_len_out, 
                        WsOpcode *opcode_out, bool *fin_out) {
    if (in_buf_len < 2) return 0; // need more data
    
    uint8_t b0 = in_buf[0];
    uint8_t b1 = in_buf[1];
    
    bool fin = (b0 & 0x80) != 0;
    WsOpcode opcode = (WsOpcode)(b0 & 0x0F);
    bool masked = (b1 & 0x80) != 0;
    uint64_t payload_len = b1 & 0x7F;
    
    size_t header_len = 2;
    
    if (payload_len == 126) {
        if (in_buf_len < 4) return 0;
        payload_len = (in_buf[2] << 8) | in_buf[3];
        header_len += 2;
    } else if (payload_len == 127) {
        if (in_buf_len < 10) return 0;
        payload_len = 0;
        for (int i = 0; i < 8; i++) {
            payload_len = (payload_len << 8) | in_buf[2 + i];
        }
        header_len += 8;
    }
    
    if (masked) {
        if (in_buf_len < header_len + 4) return 0;
        header_len += 4;
    }
    
    if (in_buf_len < header_len + payload_len) return 0; // wait for full payload
    
    uint8_t *payload = in_buf + header_len;
    
    if (masked) {
        uint8_t *masking_key = in_buf + header_len - 4;
        for (size_t i = 0; i < payload_len; i++) {
            payload[i] ^= masking_key[i % 4];
        }
    }
    
    if (payload_out) *payload_out = payload;
    if (payload_len_out) *payload_len_out = (size_t)payload_len;
    if (opcode_out) *opcode_out = opcode;
    if (fin_out) *fin_out = fin;
    
    return header_len + payload_len;
}
