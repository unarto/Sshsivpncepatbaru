#include "../common/utils.h"
#include "payload_variable.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

// Helpers
static void add_chunk(PayloadChunkList* list, const char* content, SplitType type) {
    if (!content || strlen(content) == 0) return;
    if (list->chunk_count >= list->chunk_capacity) {
        list->chunk_capacity = list->chunk_capacity == 0 ? 4 : list->chunk_capacity * 2;
        list->chunks = realloc(list->chunks, list->chunk_capacity * sizeof(PayloadChunk));
    }
    strncpy(list->chunks[list->chunk_count].content, content, sizeof(list->chunks[list->chunk_count].content) - 1);
    list->chunks[list->chunk_count].content[sizeof(list->chunks[list->chunk_count].content) - 1] = '\0';
    list->chunks[list->chunk_count].split_type = type;
    list->chunk_count++;
}

static void update_last_chunk_split(PayloadChunkList* list, SplitType type) {
    if (list->chunk_count > 0) {
        list->chunks[list->chunk_count - 1].split_type = type;
    }
}

static char* generate_random(int len) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    char* str = malloc(len + 1);
    for (int i = 0; i < len; i++) {
        str[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    str[len] = '\0';
    return str;
}

void payload_transport_init(PayloadContext* ctx, const char* raw, const char* host, int port) {
    if (!ctx) return;
    memset(ctx, 0, sizeof(PayloadContext));
    if (raw) ctx->raw_payload = strdup(raw);
    if (host) ctx->host = strdup(host);
    ctx->port = port;
    srand(time(NULL));
}

void payload_transport_set_auth(PayloadContext* ctx, const char* user, const char* pass) {
    if (user) ctx->proxy_user = strdup(user);
    if (pass) ctx->proxy_pass = strdup(pass);
}

void payload_transport_build(PayloadContext* ctx) {
    if (!ctx) return;
    
    char host_port[256];
    char host_no_port[256];
    
    // Format host and port (handle IPv6 brackets)
    int is_ipv6 = (strchr(ctx->host, ':') != NULL);
    if (is_ipv6 && ctx->host[0] != '[') {
        snprintf(host_port, sizeof(host_port), "[%s]:%d", ctx->host, ctx->port);
        snprintf(host_no_port, sizeof(host_no_port), "[%s]", ctx->host);
    } else {
        snprintf(host_port, sizeof(host_port), "%s:%d", ctx->host, ctx->port);
        snprintf(host_no_port, sizeof(host_no_port), "%s", ctx->host);
    }
    
    const char* protocol = "HTTP/1.1";
    const char* ua = "Mozilla/5.0 (Windows NT 10.0; Win64; x64)";

    if (!ctx->raw_payload || strlen(ctx->raw_payload) == 0) {
        char default_req[1024];
        snprintf(default_req, sizeof(default_req), 
            "CONNECT %s %s\r\nHost: %s\r\nUser-Agent: %s\r\nProxy-Connection: Keep-Alive\r\n", 
            host_port, protocol, host_port, ua);
        
        if (ctx->proxy_user && ctx->proxy_pass) {
            char auth_str[512];
            snprintf(auth_str, sizeof(auth_str), "%s:%s", ctx->proxy_user, ctx->proxy_pass);
            char* b64_auth = utils_base64_encode_alloc((unsigned char*)auth_str, strlen(auth_str));
            strcat(default_req, "Proxy-Authorization: Basic ");
            strcat(default_req, b64_auth);
            strcat(default_req, "\r\n");
            free(b64_auth);
        }
        strcat(default_req, "\r\n");
        add_chunk(&ctx->chunk_list, default_req, SPLIT_NORMAL);
        return;
    }

    const char* p = ctx->raw_payload;
    size_t len = strlen(p);
    
    char* cur_chunk = malloc(len * 4 + 1); // rough estimate
    size_t cur_pos = 0;
    cur_chunk[0] = '\0';
    
    static int rotate_counter = 0;

    for (size_t i = 0; i < len; ) {
        if (p[i] == '[') {
            const char* closing = strchr(p + i, ']');
            if (closing) {
                size_t tag_len = closing - (p + i) - 1;
                char tag[256];
                if (tag_len < sizeof(tag)) {
                    strncpy(tag, p + i + 1, tag_len);
                    tag[tag_len] = '\0';
                    
                    // lowercase tag for matching
                    for (int j = 0; j < tag_len; j++) tag[j] = tolower(tag[j]);
                    
                    int matched = 1;
                    SplitType stype = SPLIT_NORMAL;
                    int is_split = 0;

                    if (strcmp(tag, "split") == 0) { is_split = 1; stype = SPLIT_NORMAL; }
                    else if (strcmp(tag, "instant_split") == 0) { is_split = 1; stype = SPLIT_INSTANT; }
                    else if (strcmp(tag, "delay_split") == 0) { is_split = 1; stype = SPLIT_DELAY; }
                    
                    if (is_split) {
                        if (cur_pos > 0) {
                            cur_chunk[cur_pos] = '\0';
                            add_chunk(&ctx->chunk_list, cur_chunk, stype);
                            cur_pos = 0;
                            cur_chunk[0] = '\0';
                        } else {
                            update_last_chunk_split(&ctx->chunk_list, stype);
                        }
                    }
                    else if (strcmp(tag, "host_port") == 0 || strcmp(tag, "ssh") == 0) { strcpy(cur_chunk + cur_pos, host_port); cur_pos += strlen(host_port); }
                    else if (strcmp(tag, "host") == 0 || strcmp(tag, "real_host") == 0 || strcmp(tag, "host_no_port") == 0) { 
                        strcpy(cur_chunk + cur_pos, host_no_port); cur_pos += strlen(host_no_port); 
                    }
                    else if (strcmp(tag, "port") == 0) { char port_s[16]; snprintf(port_s, sizeof(port_s), "%d", ctx->port); strcpy(cur_chunk + cur_pos, port_s); cur_pos += strlen(port_s); }
                    else if (strcmp(tag, "protocol") == 0 || strcmp(tag, "http_version") == 0) { strcpy(cur_chunk + cur_pos, protocol); cur_pos += strlen(protocol); }
                    else if (strcmp(tag, "method") == 0) { strcpy(cur_chunk + cur_pos, "CONNECT"); cur_pos += 7; }
                    else if (strcmp(tag, "path") == 0) { strcpy(cur_chunk + cur_pos, "/"); cur_pos += 1; }
                    else if (strcmp(tag, "ua") == 0 || strcmp(tag, "user-agent") == 0) { strcpy(cur_chunk + cur_pos, ua); cur_pos += strlen(ua); }
                    else if (strcmp(tag, "raw") == 0) { 
                        cur_pos += snprintf(cur_chunk + cur_pos, 256, "CONNECT %s %s\r\n\r\n", host_port, protocol); 
                    }
                    else if (strcmp(tag, "netdata") == 0) { 
                        cur_pos += snprintf(cur_chunk + cur_pos, 256, "CONNECT %s %s\r\n", host_port, protocol); 
                    }
                    else if (strcmp(tag, "websocket_key") == 0) { 
                        char* ws_key = utils_generate_ws_key_alloc();
                        strcpy(cur_chunk + cur_pos, ws_key); cur_pos += strlen(ws_key);
                        free(ws_key);
                    }
                    else if (strcmp(tag, "websocket_version") == 0) { strcpy(cur_chunk + cur_pos, "13"); cur_pos += 2; }
                    else if (strcmp(tag, "websocket_extensions") == 0) { strcpy(cur_chunk + cur_pos, "permessage-deflate; client_max_window_bits"); cur_pos += strlen("permessage-deflate; client_max_window_bits"); }
                    else if (strcmp(tag, "crlf") == 0) { strcpy(cur_chunk + cur_pos, "\r\n"); cur_pos += 2; }
                    else if (strcmp(tag, "lfcr") == 0) { strcpy(cur_chunk + cur_pos, "\n\r"); cur_pos += 2; }
                    else if (strcmp(tag, "cr") == 0) { strcpy(cur_chunk + cur_pos, "\r"); cur_pos += 1; }
                    else if (strcmp(tag, "lf") == 0) { strcpy(cur_chunk + cur_pos, "\n"); cur_pos += 1; }
                    else if (strcmp(tag, "random") == 0) { char* rand_s = generate_random(6); strcpy(cur_chunk + cur_pos, rand_s); cur_pos += 6; free(rand_s); }
                    else if (strncmp(tag, "random=", 7) == 0) {
                        // Very simple random choice parser
                        char* ops = strdup(tag + 7);
                        char* p_op = strtok(ops, ",");
                        char* choices[32];
                        int count = 0;
                        while(p_op && count < 32) {
                            while(isspace((unsigned char)*p_op)) p_op++;
                            char* end = p_op + strlen(p_op) - 1;
                            while(end > p_op && isspace((unsigned char)*end)) { *end = '\0'; end--; }
                            if (strlen(p_op) > 0) choices[count++] = p_op;
                            p_op = strtok(NULL, ",");
                        }
                        if (count > 0) {
                            char* picked = choices[rand() % count];
                            strcpy(cur_chunk + cur_pos, picked);
                            cur_pos += strlen(picked);
                        }
                        free(ops);
                    }
                    else if (strcmp(tag, "rotate") == 0) { char* rand_s = generate_random(6); strcpy(cur_chunk + cur_pos, rand_s); cur_pos += 6; free(rand_s); }
                    else if (strncmp(tag, "rotate=", 7) == 0) {
                        char* ops = strdup(tag + 7);
                        char* p_op = strtok(ops, ",");
                        char* choices[32];
                        int count = 0;
                        while(p_op && count < 32) {
                            while(isspace((unsigned char)*p_op)) p_op++;
                            char* end = p_op + strlen(p_op) - 1;
                            while(end > p_op && isspace((unsigned char)*end)) { *end = '\0'; end--; }
                            if (strlen(p_op) > 0) choices[count++] = p_op;
                            p_op = strtok(NULL, ",");
                        }
                        if (count > 0) {
                            char* picked = choices[rotate_counter % count];
                            rotate_counter++;
                            strcpy(cur_chunk + cur_pos, picked);
                            cur_pos += strlen(picked);
                        }
                        free(ops);
                    }
                    else {
                        matched = 0;
                    }

                    if (matched) {
                        i = (closing - p) + 1;
                        continue;
                    }
                }
            }
        }
        
        if (p[i] == '\\' && i + 1 < len) {
            if (p[i+1] == 'r' && i + 3 < len && p[i+2] == '\\' && p[i+3] == 'n') {
                strcpy(cur_chunk + cur_pos, "\r\n"); cur_pos += 2; i += 4; continue;
            }
            if (p[i+1] == 'n' && i + 3 < len && p[i+2] == '\\' && p[i+3] == 'r') {
                strcpy(cur_chunk + cur_pos, "\n\r"); cur_pos += 2; i += 4; continue;
            }
            if (p[i+1] == 'r') { cur_chunk[cur_pos++] = '\r'; i += 2; continue; }
            if (p[i+1] == 'n') { cur_chunk[cur_pos++] = '\n'; i += 2; continue; }
            if (p[i+1] == 't') { cur_chunk[cur_pos++] = '\t'; i += 2; continue; }
            if (p[i+1] == '\\') { cur_chunk[cur_pos++] = '\\'; i += 2; continue; }
        }
        
        cur_chunk[cur_pos++] = p[i++];
    }
    
    if (cur_pos > 0) {
        cur_chunk[cur_pos] = '\0';
        add_chunk(&ctx->chunk_list, cur_chunk, SPLIT_NORMAL);
    }
    free(cur_chunk);

    // Apply Proxy Auth to the last chunk if needed
    if (ctx->proxy_user && ctx->proxy_pass && ctx->chunk_list.chunk_count > 0) {
        PayloadChunk* last_chunk = &ctx->chunk_list.chunks[ctx->chunk_list.chunk_count - 1];
        if (!strstr(last_chunk->content, "Proxy-Authorization:") && !strstr(last_chunk->content, "proxy-authorization:")) {
            char auth_str[512];
            snprintf(auth_str, sizeof(auth_str), "%s:%s", ctx->proxy_user, ctx->proxy_pass);
            char* b64_auth = utils_base64_encode_alloc((unsigned char*)auth_str, strlen(auth_str));
            char auth_header[512];
            snprintf(auth_header, sizeof(auth_header), "Proxy-Authorization: Basic %s\r\n", b64_auth);
            free(b64_auth);

            size_t curr_len = strlen(last_chunk->content);
            char temp_content[2048];
            
            if (curr_len >= 4 && strcmp(last_chunk->content + curr_len - 4, "\r\n\r\n") == 0) {
                strncpy(temp_content, last_chunk->content, curr_len - 2);
                temp_content[curr_len - 2] = '\0';
                strncat(temp_content, auth_header, sizeof(temp_content) - strlen(temp_content) - 1);
                strncat(temp_content, "\r\n", sizeof(temp_content) - strlen(temp_content) - 1);
            } else if (curr_len >= 2 && strcmp(last_chunk->content + curr_len - 2, "\r\n") == 0) {
                strncpy(temp_content, last_chunk->content, sizeof(temp_content) - 1);
                temp_content[sizeof(temp_content) - 1] = '\0';
                strncat(temp_content, auth_header, sizeof(temp_content) - strlen(temp_content) - 1);
            } else {
                strncpy(temp_content, last_chunk->content, sizeof(temp_content) - 1);
                temp_content[sizeof(temp_content) - 1] = '\0';
                strncat(temp_content, "\r\n", sizeof(temp_content) - strlen(temp_content) - 1);
                strncat(temp_content, auth_header, sizeof(temp_content) - strlen(temp_content) - 1);
            }
            strncpy(last_chunk->content, temp_content, sizeof(last_chunk->content) - 1);
            last_chunk->content[sizeof(last_chunk->content) - 1] = '\0';
        }
    }
}

void payload_transport_free(PayloadContext* ctx) {
    if (!ctx) return;
    if (ctx->raw_payload) free(ctx->raw_payload);
    if (ctx->host) free(ctx->host);
    if (ctx->proxy_user) free(ctx->proxy_user);
    if (ctx->proxy_pass) free(ctx->proxy_pass);
    
    if (ctx->chunk_list.chunks) free(ctx->chunk_list.chunks);
    memset(ctx, 0, sizeof(PayloadContext));
}
