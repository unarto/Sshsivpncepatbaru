#ifndef NATIVE_PAYLOAD_PARSER_H
#define NATIVE_PAYLOAD_PARSER_H

#include <stddef.h>
#include <stdbool.h>

// Token categories for the parsed payload
typedef enum {
    PAYLOAD_TOKEN_TEXT,         // Plain text
    PAYLOAD_TOKEN_CRLF,         // [crlf], [cr], [lf], \r, \n (newline representations)
    PAYLOAD_TOKEN_PLACEHOLDER,  // [host], [port], [host_port], [path], [query], [protocol], [ua], [random]
    PAYLOAD_TOKEN_METHOD,       // [method]
    PAYLOAD_TOKEN_SPLIT,        // [split], [instant_split], [header_split], [byte_split]
    PAYLOAD_TOKEN_DELAY         // [delay_split] or [delay=N]
} PayloadTokenType;

typedef struct {
    PayloadTokenType type;
    char* value;      // Null-terminated string holding the token content
    size_t length;    // Length of the value
    int delay_ms;     // Used only if type == PAYLOAD_TOKEN_DELAY
} PayloadToken;

typedef struct {
    PayloadToken* tokens;
    size_t count;
    size_t capacity;
} PayloadAST;

// Parse the raw payload string into an array of tokens (AST)
PayloadAST* payload_parser_parse(const char* raw_payload);

// Free the AST memory
void payload_parser_free(PayloadAST* ast);

// Debug: Print the parsed AST
void payload_parser_print(const PayloadAST* ast);

#endif // NATIVE_PAYLOAD_PARSER_H
