#ifndef NATIVE_HTTP_HEADER_H
#define NATIVE_HTTP_HEADER_H

#include <stddef.h>
#include <stdbool.h>

// Represents a single HTTP header
typedef struct {
    char* key;
    char* value;
} HttpHeader;

// Represents a collection of HTTP headers
typedef struct {
    HttpHeader* headers;
    size_t count;
    size_t capacity;
} HttpHeaderList;

// Initialize a header list
void http_header_init(HttpHeaderList* list);

// Add a header. If the key exists, it will overwrite the existing one.
void http_header_add(HttpHeaderList* list, const char* key, const char* value);

// Remove a header by key
void http_header_remove(HttpHeaderList* list, const char* key);

// Get a header value by key
const char* http_header_get(const HttpHeaderList* list, const char* key);

// Build the headers into a formatted string (allocates memory, caller must free)
char* http_header_build_string(const HttpHeaderList* list);

// Free the header list
void http_header_free(HttpHeaderList* list);

// Parse raw header string (e.g., from custom payload config) and add to list
void http_header_parse_and_add(HttpHeaderList* list, const char* raw_headers_str);

#endif // NATIVE_HTTP_HEADER_H
