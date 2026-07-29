#include "http_header.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../common/logger.h"

#define INITIAL_HEADER_CAPACITY 8

void http_header_init(HttpHeaderList* list) {
    if (!list) return;
    list->headers = NULL;
    list->count = 0;
    list->capacity = 0;
}

void http_header_add(HttpHeaderList* list, const char* key, const char* value) {
    if (!list || !key || !value) return;
    
    // Check if key already exists, update it if so
    for (size_t i = 0; i < list->count; i++) {
        if (strcasecmp(list->headers[i].key, key) == 0) {
            free(list->headers[i].value);
            list->headers[i].value = strdup(value);
            return;
        }
    }
    
    if (list->count >= list->capacity) {
        list->capacity = (list->capacity == 0) ? INITIAL_HEADER_CAPACITY : list->capacity * 2;
        HttpHeader* new_headers = (HttpHeader*)realloc(list->headers, sizeof(HttpHeader) * list->capacity);
        if (!new_headers) {
            LOGE("HTTP Header: Memory allocation failed");
            return;
        }
        list->headers = new_headers;
    }
    
    list->headers[list->count].key = strdup(key);
    list->headers[list->count].value = strdup(value);
    list->count++;
}

void http_header_remove(HttpHeaderList* list, const char* key) {
    if (!list || !key) return;
    for (size_t i = 0; i < list->count; i++) {
        if (strcasecmp(list->headers[i].key, key) == 0) {
            free(list->headers[i].key);
            free(list->headers[i].value);
            // Shift remaining
            for (size_t j = i; j < list->count - 1; j++) {
                list->headers[j] = list->headers[j + 1];
            }
            list->count--;
            return;
        }
    }
}

const char* http_header_get(const HttpHeaderList* list, const char* key) {
    if (!list || !key) return NULL;
    for (size_t i = 0; i < list->count; i++) {
        if (strcasecmp(list->headers[i].key, key) == 0) {
            return list->headers[i].value;
        }
    }
    return NULL;
}

char* http_header_build_string(const HttpHeaderList* list) {
    if (!list || list->count == 0) return strdup("");
    
    size_t total_len = 0;
    for (size_t i = 0; i < list->count; i++) {
        total_len += strlen(list->headers[i].key) + 2 + strlen(list->headers[i].value) + 2; // "Key: Value\r\n"
    }
    
    char* buffer = (char*)malloc(total_len + 1);
    if (!buffer) return NULL;
    buffer[0] = '\0';
    
    for (size_t i = 0; i < list->count; i++) {
        strcat(buffer, list->headers[i].key);
        strcat(buffer, ": ");
        strcat(buffer, list->headers[i].value);
        strcat(buffer, "\r\n");
    }
    
    return buffer;
}

void http_header_free(HttpHeaderList* list) {
    if (!list) return;
    for (size_t i = 0; i < list->count; i++) {
        if (list->headers[i].key) free(list->headers[i].key);
        if (list->headers[i].value) free(list->headers[i].value);
    }
    if (list->headers) {
        free(list->headers);
        list->headers = NULL;
    }
    list->count = 0;
    list->capacity = 0;
}

void http_header_parse_and_add(HttpHeaderList* list, const char* raw_headers_str) {
    if (!list || !raw_headers_str) return;
    
    char* copy = strdup(raw_headers_str);
    if (!copy) return;
    
    char* saveptr;
    char* line = strtok_r(copy, "\r\n", &saveptr);
    while (line) {
        char* colon = strchr(line, ':');
        if (colon) {
            *colon = '\0';
            char* key = line;
            char* value = colon + 1;
            
            // Trim leading spaces from value
            while (*value == ' ' || *value == '\t') value++;
            // Trim leading spaces from key
            while (*key == ' ' || *key == '\t') key++;
            
            if (strlen(key) > 0 && strlen(value) > 0) {
                http_header_add(list, key, value);
            }
        }
        line = strtok_r(NULL, "\r\n", &saveptr);
    }
    free(copy);
}
