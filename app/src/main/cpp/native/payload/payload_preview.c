#include "payload_preview.h"
#include "payload_parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

char* payload_preview_generate(const char* raw_payload, const PayloadPlaceholderContext* ctx) {
    if (!raw_payload || !ctx) return NULL;
    
    PayloadAST* ast = payload_parser_parse(raw_payload);
    if (!ast) return NULL;
    
    size_t total_len = 0;
    size_t capacity = 256;
    char* result = (char*)malloc(capacity);
    if (!result) {
        payload_parser_free(ast);
        return NULL;
    }
    result[0] = '\0';
    
    for (size_t i = 0; i < ast->count; i++) {
        PayloadToken* token = &ast->tokens[i];
        
        char* token_str = NULL;
        
        if (token->type == PAYLOAD_TOKEN_TEXT) {
            token_str = strdup(token->value);
        } else if (token->type == PAYLOAD_TOKEN_SPLIT || token->type == PAYLOAD_TOKEN_DELAY) {
            // Keep split/delay tags as they are for preview visibility (e.g. to see where it splits)
            size_t slen = strlen(token->value) + 3;
            token_str = (char*)malloc(slen);
            if (token_str) {
                snprintf(token_str, slen, "[%s]", token->value);
            }
        } else {
            // Replace placeholder, method, crlf
            token_str = payload_placeholder_replace(token, ctx);
            if (!token_str) {
                // If unknown or unsupported, just leave it raw for preview
                size_t slen = strlen(token->value) + 3;
                token_str = (char*)malloc(slen);
                if (token_str) {
                    snprintf(token_str, slen, "[%s]", token->value);
                }
            }
        }
        
        if (token_str) {
            size_t token_len = strlen(token_str);
            if (total_len + token_len + 1 > capacity) {
                capacity = (total_len + token_len) * 2;
                char* new_res = (char*)realloc(result, capacity);
                if (!new_res) {
                    free(token_str);
                    free(result);
                    payload_parser_free(ast);
                    return NULL;
                }
                result = new_res;
            }
            strcat(result, token_str);
            total_len += token_len;
            free(token_str);
        }
    }
    
    payload_parser_free(ast);
    return result;
}
