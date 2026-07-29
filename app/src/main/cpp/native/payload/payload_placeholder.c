#include "payload_placeholder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

static void generate_random_string(char* buf, size_t length) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (size_t i = 0; i < length; i++) {
        int key = rand() % (int)(sizeof(charset) - 1);
        buf[i] = charset[key];
    }
    buf[length] = '\0';
}

char* payload_placeholder_replace(const PayloadToken* token, const PayloadPlaceholderContext* ctx) {
    if (!token || !token->value) return NULL;
    
    if (token->type == PAYLOAD_TOKEN_CRLF) {
        if (strcasecmp(token->value, "cr") == 0) return strdup("\r");
        if (strcasecmp(token->value, "lf") == 0) return strdup("\n");
        if (strcasecmp(token->value, "crlf") == 0) return strdup("\r\n");
        return NULL;
    }
    
    if (token->type == PAYLOAD_TOKEN_METHOD) {
        if (ctx->method) return strdup(ctx->method);
        return strdup("GET");
    }
    
    if (token->type == PAYLOAD_TOKEN_PLACEHOLDER) {
        if (strcasecmp(token->value, "host") == 0) {
            return ctx->host ? strdup(ctx->host) : strdup("");
        } else if (strcasecmp(token->value, "port") == 0) {
            char buf[32];
            snprintf(buf, sizeof(buf), "%d", ctx->port);
            return strdup(buf);
        } else if (strcasecmp(token->value, "host_port") == 0) {
            char buf[256];
            snprintf(buf, sizeof(buf), "%s:%d", ctx->host ? ctx->host : "", ctx->port);
            return strdup(buf);
        } else if (strcasecmp(token->value, "path") == 0) {
            return ctx->path ? strdup(ctx->path) : strdup("/");
        } else if (strcasecmp(token->value, "query") == 0) {
            return ctx->query ? strdup(ctx->query) : strdup("");
        } else if (strcasecmp(token->value, "protocol") == 0) {
            return ctx->protocol ? strdup(ctx->protocol) : strdup("HTTP/1.1");
        } else if (strcasecmp(token->value, "ua") == 0) {
            return ctx->user_agent ? strdup(ctx->user_agent) : strdup("SiVPN/1.0");
        } else if (strcasecmp(token->value, "random") == 0) {
            char buf[32];
            generate_random_string(buf, 16);
            return strdup(buf);
        } else if (strcasecmp(token->value, "cr") == 0) {
            return strdup("\r");
        } else if (strcasecmp(token->value, "lf") == 0) {
            return strdup("\n");
        } else if (strcasecmp(token->value, "crlf") == 0) {
            return strdup("\r\n");
        }
    }
    
    return NULL;
}
