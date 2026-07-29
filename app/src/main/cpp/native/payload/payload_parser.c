#include "payload_parser.h"
#include "../common/logger.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#define INITIAL_AST_CAPACITY 16

static void ast_add_token(PayloadAST* ast, PayloadTokenType type, const char* val, size_t len) {
    if (ast->count >= ast->capacity) {
        ast->capacity = (ast->capacity == 0) ? INITIAL_AST_CAPACITY : ast->capacity * 2;
        PayloadToken* new_tokens = (PayloadToken*)realloc(ast->tokens, sizeof(PayloadToken) * ast->capacity);
        if (!new_tokens) {
            LOGE("Payload Parser: Memory allocation failed");
            return;
        }
        ast->tokens = new_tokens;
    }

    PayloadToken* token = &ast->tokens[ast->count];
    token->type = type;
    token->length = len;
    token->delay_ms = 0;
    
    token->value = (char*)malloc(len + 1);
    if (token->value) {
        memcpy(token->value, val, len);
        token->value[len] = '\0';
    } else {
        token->value = NULL;
    }
    
    ast->count++;
}

static void ast_add_delay_token(PayloadAST* ast, const char* val, size_t len, int delay_ms) {
    ast_add_token(ast, PAYLOAD_TOKEN_DELAY, val, len);
    if (ast->count > 0) {
        ast->tokens[ast->count - 1].delay_ms = delay_ms;
    }
}

// Helper to determine the token type of a tag like [host]
static void parse_tag_and_add(PayloadAST* ast, const char* tag, size_t len) {
    if (len == 0) return;
    
    // Check CRLF
    if (strncasecmp(tag, "crlf", len) == 0 || strncasecmp(tag, "cr", len) == 0 || strncasecmp(tag, "lf", len) == 0) {
        ast_add_token(ast, PAYLOAD_TOKEN_CRLF, tag, len);
        return;
    }
    
    // Check Method
    if (strncasecmp(tag, "method", len) == 0) {
        ast_add_token(ast, PAYLOAD_TOKEN_METHOD, tag, len);
        return;
    }
    
    // Check Split
    if (strncasecmp(tag, "split", len) == 0 || 
        strncasecmp(tag, "instant_split", len) == 0 || 
        strncasecmp(tag, "header_split", len) == 0 || 
        strncasecmp(tag, "byte_split", len) == 0) {
        ast_add_token(ast, PAYLOAD_TOKEN_SPLIT, tag, len);
        return;
    }
    
    // Check Delay
    if (strncasecmp(tag, "delay_split", 11) == 0 || strncasecmp(tag, "delay", 5) == 0) {
        int delay = 0;
        const char* eq = memchr(tag, '=', len);
        if (eq) {
            delay = atoi(eq + 1);
        } else {
            delay = 100; // default delay if not specified
        }
        ast_add_delay_token(ast, tag, len, delay);
        return;
    }
    
    // Default to Placeholder (host, port, ua, etc.)
    ast_add_token(ast, PAYLOAD_TOKEN_PLACEHOLDER, tag, len);
}

PayloadAST* payload_parser_parse(const char* raw_payload) {
    if (!raw_payload) return NULL;

    PayloadAST* ast = (PayloadAST*)malloc(sizeof(PayloadAST));
    if (!ast) return NULL;
    
    ast->tokens = NULL;
    ast->count = 0;
    ast->capacity = 0;

    const char* p = raw_payload;
    const char* text_start = p;

    while (*p != '\0') {
        // Handle explicit newline characters \r or \n
        if (*p == '\\' && (*(p+1) == 'r' || *(p+1) == 'n')) {
            if (p > text_start) {
                ast_add_token(ast, PAYLOAD_TOKEN_TEXT, text_start, p - text_start);
            }
            if (*(p+1) == 'r') {
                ast_add_token(ast, PAYLOAD_TOKEN_CRLF, "cr", 2);
            } else {
                ast_add_token(ast, PAYLOAD_TOKEN_CRLF, "lf", 2);
            }
            p += 2;
            text_start = p;
            continue;
        }

        // Handle tags in brackets
        if (*p == '[') {
            if (p > text_start) {
                ast_add_token(ast, PAYLOAD_TOKEN_TEXT, text_start, p - text_start);
            }
            
            const char* tag_start = p + 1;
            const char* tag_end = strchr(tag_start, ']');
            
            if (tag_end) {
                size_t tag_len = tag_end - tag_start;
                parse_tag_and_add(ast, tag_start, tag_len);
                p = tag_end + 1;
                text_start = p;
            } else {
                // No closing bracket found, treat as text
                p++;
            }
            continue;
        }
        
        p++;
    }

    if (p > text_start) {
        ast_add_token(ast, PAYLOAD_TOKEN_TEXT, text_start, p - text_start);
    }

    return ast;
}

void payload_parser_free(PayloadAST* ast) {
    if (!ast) return;
    for (size_t i = 0; i < ast->count; i++) {
        if (ast->tokens[i].value) {
            free(ast->tokens[i].value);
        }
    }
    if (ast->tokens) {
        free(ast->tokens);
    }
    free(ast);
}

void payload_parser_print(const PayloadAST* ast) {
    if (!ast) return;
    LOGI("=== Payload AST ===");
    for (size_t i = 0; i < ast->count; i++) {
        PayloadToken* t = &ast->tokens[i];
        switch (t->type) {
            case PAYLOAD_TOKEN_TEXT:
                LOGI("TEXT: '%s'", t->value); break;
            case PAYLOAD_TOKEN_CRLF:
                LOGI("CRLF: [%s]", t->value); break;
            case PAYLOAD_TOKEN_PLACEHOLDER:
                LOGI("PLACEHOLDER: [%s]", t->value); break;
            case PAYLOAD_TOKEN_METHOD:
                LOGI("METHOD: [%s]", t->value); break;
            case PAYLOAD_TOKEN_SPLIT:
                LOGI("SPLIT: [%s]", t->value); break;
            case PAYLOAD_TOKEN_DELAY:
                LOGI("DELAY: [%s] (ms: %d)", t->value, t->delay_ms); break;
        }
    }
    LOGI("===================");
}
