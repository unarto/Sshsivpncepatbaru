#include "payload_manager.h"
#include "payload_parser.h"
#include "payload_validator.h"
#include "payload_placeholder.h"
#include "payload_template.h"
#include "payload_ua.h"
#include "../http/http_method.h"
#include "../proxy/http_header.h"
#include "../common/logger.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int payload_manager_build(TunnelConfig* cfg) {
    if (!cfg) return -1;
    
    cfg->num_payload_chunks = 0;
    
    // 1. Get raw payload (or template if empty)
    const char* raw_payload = cfg->raw_payload;
    if (strlen(raw_payload) == 0) {
        raw_payload = payload_template_get(cfg->payload_template_type);
    }
    
    // 2. Validate
    if (!payload_validator_check(raw_payload)) {
        LOGE("Payload Manager: Syntax error or unmatched brackets in payload");
        return -1;
    }
    
    // 3. Setup Context
    PayloadPlaceholderContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.host = cfg->host;
    ctx.port = cfg->port;
    ctx.user_agent = payload_ua_get(cfg->user_agent_type, cfg->custom_user_agent);
    
    ctx.method = "GET";
    ctx.path = "/";
    ctx.protocol = "HTTP/1.1";
    
    // 4. Parse
    PayloadAST* ast = payload_parser_parse(raw_payload);
    if (!ast) {
        LOGE("Payload Manager: Failed to parse AST");
        return -1;
    }
    
    // 5. Build chunks
    char buffer[2048];
    buffer[0] = '\0';
    size_t current_len = 0;
    
    for (size_t i = 0; i < ast->count; i++) {
        PayloadToken* token = &ast->tokens[i];
        
        if (token->type == PAYLOAD_TOKEN_SPLIT || token->type == PAYLOAD_TOKEN_DELAY) {
            if (current_len > 0) {
                if (cfg->num_payload_chunks < 16) {
                    int chunk_idx = cfg->num_payload_chunks;
                    strncpy(cfg->payload_chunks[chunk_idx].content, buffer, sizeof(cfg->payload_chunks[chunk_idx].content) - 1);
                    cfg->payload_chunks[chunk_idx].split_type = (token->type == PAYLOAD_TOKEN_DELAY) ? SPLIT_DELAY : SPLIT_NORMAL;
                    cfg->num_payload_chunks++;
                }
                buffer[0] = '\0';
                current_len = 0;
            }
            continue;
        }
        
        char* token_str = NULL;
        if (token->type == PAYLOAD_TOKEN_TEXT) {
            token_str = strdup(token->value);
        } else {
            token_str = payload_placeholder_replace(token, &ctx);
        }
        
        if (token_str) {
            size_t token_len = strlen(token_str);
            if (current_len + token_len < sizeof(buffer) - 1) {
                strcat(buffer, token_str);
                current_len += token_len;
            } else {
                LOGE("Payload Manager: Buffer overflow building chunk");
            }
            free(token_str);
        }
    }
    
    if (current_len > 0 && cfg->num_payload_chunks < 16) {
        int chunk_idx = cfg->num_payload_chunks;
        strncpy(cfg->payload_chunks[chunk_idx].content, buffer, sizeof(cfg->payload_chunks[chunk_idx].content) - 1);
        cfg->payload_chunks[chunk_idx].split_type = SPLIT_NORMAL;
        cfg->num_payload_chunks++;
    }
    
    payload_parser_free(ast);
    LOGI("Payload Manager: Successfully built %d chunks", cfg->num_payload_chunks);
    return 0;
}
