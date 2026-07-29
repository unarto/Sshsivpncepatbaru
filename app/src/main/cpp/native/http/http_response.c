#include "http_response.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

int http_response_parse_status_code(const char* response) {
    if (!response) return -1;
    
    // Look for "HTTP/"
    const char* ptr = strstr(response, "HTTP/");
    if (!ptr) return -1;
    
    // Skip "HTTP/1.x "
    ptr = strchr(ptr, ' ');
    if (!ptr) return -1;
    
    while (*ptr == ' ') ptr++;
    
    if (isdigit(*ptr)) {
        return atoi(ptr);
    }
    
    return -1;
}

bool http_response_is_success(int status_code) {
    // 200 OK, 101 Switching Protocols, 204 No Content, etc.
    return (status_code >= 200 && status_code < 300) || status_code == 101;
}

char* http_response_extract_header(const char* response, const char* header_key) {
    if (!response || !header_key) return NULL;
    
    // Find the header key case-insensitively
    // We should look for "Key:" at the start of a line to avoid partial matches
    char search_key[256];
    snprintf(search_key, sizeof(search_key), "\n%s:", header_key);
    
    const char* ptr = strcasestr(response, search_key);
    if (!ptr) {
        // Also check if it's the very first line (rare but possible)
        snprintf(search_key, sizeof(search_key), "%s:", header_key);
        if (strncasecmp(response, search_key, strlen(search_key)) == 0) {
            ptr = response;
        } else {
            return NULL;
        }
    } else {
        ptr++; // Skip the newline
    }
    
    // Skip to the colon and then the value
    ptr = strchr(ptr, ':');
    if (!ptr) return NULL;
    ptr++;
    
    while (*ptr == ' ' || *ptr == '\t') ptr++;
    
    const char* end_ptr = strpbrk(ptr, "\r\n");
    size_t len = end_ptr ? (size_t)(end_ptr - ptr) : strlen(ptr);
    
    char* value = (char*)malloc(len + 1);
    if (value) {
        strncpy(value, ptr, len);
        value[len] = '\0';
        return value;
    }
    
    return NULL;
}
